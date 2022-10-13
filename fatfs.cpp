#include "fatfs.h"

std::string ConvertDosTimeToHuman(uint16_t* dosdate, uint16_t* dostime)
{
    std::string humanstring = "";
    humanstring += std::to_string(((*dosdate & 0x1e0) >> 5)); // MONTH
    humanstring += "/"; // DIVIDER
    humanstring += std::to_string(((*dosdate & 0x1f) >> 0)); // MONTH
    humanstring += "/"; // DIVIDER
    humanstring += std::to_string(((*dosdate & 0xfe00) >> 9) + 1980); // YEAR
    humanstring += " ";
    if(dostime == NULL)
	humanstring += "00:00:00";
    else
    {
	humanstring += std::to_string(((*dostime & 0xf800) >> 11)); // HOUR
	humanstring += ":";
	humanstring += std::to_string(((*dostime & 0x7e0) >> 5)); // MINUTE
	humanstring += ":";
	humanstring += std::to_string(((*dostime & 0x1f) >> 0) * 2); // SECOND
    }
    humanstring += " (UTC)";
    
    return humanstring;
}

void GetNextCluster(std::ifstream* rawcontent, uint32_t clusternum, fatinfo* curfat, std::vector<uint32_t>* clusterlist)
{
    uint32_t curcluster = 0;
    int fatbyte1 = 0;
    if(curfat->fattype == 1) // FAT 12
    {
	fatbyte1 = clusternum + (clusternum / 2);
	uint8_t* cc = new uint8_t[2];
	uint16_t ccluster = 0;
	ReadContent(rawcontent, cc, curfat->fatoffset + fatbyte1, 2);
	ReturnUint16(&ccluster, cc);
	delete[] cc;
	if(clusternum & 0x0001) // ODD
	{
	    curcluster = ccluster >> 4;
	}
	else // EVEN
	{
	    curcluster = ccluster & 0x0FFF;
	}
	if(curcluster < 0x0FF7 && curcluster >= 2)
	{
	    clusterlist->push_back(curcluster);
	    GetNextCluster(rawcontent, curcluster, curfat, clusterlist);
	}
    }
    else if(curfat->fattype == 2) // FAT16
    {
        if(clusternum >= 2)
        {
            fatbyte1 = clusternum * 2;
            uint8_t* cc = new uint8_t[2];
            ReadContent(rawcontent, cc, curfat->fatoffset + fatbyte1, 2);
            ReturnUint32(&curcluster, cc);
            delete[] cc;
            if(curcluster < 0xFFF7 && curcluster >= 2)
            {
                clusterlist->push_back(curcluster);
                GetNextCluster(rawcontent, curcluster, curfat, clusterlist);
            }
        }
    }
    else if(curfat->fattype == 3) // FAT32
    {
        if(clusternum >= 2)
        {
            fatbyte1 = clusternum * 4;
            uint8_t* cc = new uint8_t[4];
            uint32_t ccluster = 0;
            ReadContent(rawcontent, cc, curfat->fatoffset + fatbyte1, 4);
            ReturnUint32(&ccluster, cc);
            delete[] cc;
            curcluster = ccluster & 0x0FFFFFFF;
            if(curcluster < 0x0FFFFFF7 && curcluster >= 2)
            {
                clusterlist->push_back(curcluster);
                GetNextCluster(rawcontent, curcluster, curfat, clusterlist);
            }
        }
    }
    else if(curfat->fattype == 4) // EXFAT
    {
        if(clusternum >= 2)
        {
	    curcluster = 0;
            fatbyte1 = clusternum * 4;
            uint8_t* cc = new uint8_t[4];
            ReadContent(rawcontent, cc, curfat->fatoffset + fatbyte1, 4);
            ReturnUint32(&curcluster, cc);
            delete[] cc;
            if(curcluster < 0xFFFFFFF7 && curcluster >= 2)
            {
                clusterlist->push_back(curcluster);
                GetNextCluster(rawcontent, curcluster, curfat, clusterlist);
            }
        }
    }
}

std::string ConvertClustersToExtents(std::vector<uint32_t>* clusterlist, fatinfo* curfat)
{
    uint32_t clustersize = curfat->sectorspercluster * curfat->bytespersector;
    uint64_t rootdiroffset = curfat->clusterareastart * curfat->bytespersector;
    /*
    if(curfat->fattype == 4) // EXFAT
    {
	rootdiroffset = (((curfat->rootdircluster - 2) * curfat->sectorspercluster) + curfat->clusterareastart) * curfat->bytespersector;
    }
    */

    std::string extentstring = "";
    int blkcnt = 1;
    uint32_t startvalue = clusterlist->at(0);
    for(int i=1; i < clusterlist->size(); i++)
    {
	uint32_t oldvalue = clusterlist->at(i-1);
	uint32_t newvalue = clusterlist->at(i);
	if(newvalue - oldvalue == 1)
	    blkcnt++;
	else
	{
	    if(rootdiroffset > 0)
		extentstring += std::to_string(rootdiroffset + ((startvalue - 2) * clustersize)) + "," + std::to_string(blkcnt * clustersize) + ";";
	    else
		extentstring += std::to_string(startvalue * clustersize) + "," + std::to_string(blkcnt * clustersize) + ";";
	    startvalue = clusterlist->at(i);
	    blkcnt = 1;
	}
	if(i == clusterlist->size() - 1)
	{
	    if(rootdiroffset > 0)
		extentstring += std::to_string(rootdiroffset + ((startvalue - 2) * clustersize)) + "," + std::to_string(blkcnt * clustersize) + ";";
	    else
		extentstring += std::to_string(startvalue * clustersize) + "," + std::to_string(blkcnt * clustersize) + ";";
	    startvalue = clusterlist->at(i);
	    blkcnt = 1;
	}
    }
    if(clusterlist->size() == 1)
    {
	if(rootdiroffset > 0)
	    extentstring += std::to_string(rootdiroffset + ((startvalue - 2) * clustersize)) + "," + std::to_string(blkcnt * clustersize) + ";";
	else
	    extentstring += std::to_string(startvalue * clustersize) + "," + std::to_string(blkcnt * clustersize) + ";";
    }

    return extentstring;
}

void ParseFatInfo(std::ifstream* rawcontent, fatinfo* curfat)
{
    // BYTES PER SECTOR
    uint8_t* bps = new uint8_t[2];
    uint16_t bytespersector = 0;
    ReadContent(rawcontent, bps, 11, 2);
    ReturnUint16(&bytespersector, bps);
    delete[] bps;
    curfat->bytespersector = bytespersector;
    //std::cout << "Bytes Per Sector: " << bytespersector << std::endl;
    // FAT COUNT
    uint8_t* fc = new uint8_t[1];
    uint8_t fatcount = 0;
    ReadContent(rawcontent, fc, 16, 1);
    fatcount = (uint8_t)fc[0];
    delete[] fc;
    //std::cout << "Fat Count: " << (unsigned int)fatcount << std::endl;
    // SECTORS PER CLUSTER
    uint8_t* spc = new uint8_t[1];
    uint8_t sectorspercluster = 0;
    ReadContent(rawcontent, spc, 13, 1);
    sectorspercluster = (uint8_t)spc[0];
    delete[] spc;
    curfat->sectorspercluster = sectorspercluster;
    //std::cout << "Sectors per cluster: " << (unsigned int)sectorspercluster << std::endl;
    // RESERVED AREA SIZE
    uint8_t* ras = new uint8_t[2];
    uint16_t reservedareasize = 0;
    ReadContent(rawcontent, ras, 14, 2);
    ReturnUint16(&reservedareasize, ras);
    delete[] ras;
    //std::cout << "Reserved Area Size: " << reservedareasize << std::endl;
    // ROOT DIRECTORY MAX FILES
    uint8_t* rdmf = new uint8_t[2];
    uint16_t rootdirmaxfiles = 0;
    ReadContent(rawcontent, rdmf, 17, 2);
    ReturnUint16(&rootdirmaxfiles, rdmf);
    delete[] rdmf;
    //std::cout << "Root Dir Max Files: " << rootdirmaxfiles << std::endl;

    if(curfat->fattype == 1 || curfat->fattype == 2)
    {
        // FAT SIZE
        uint8_t* fs = new uint8_t[2];
        uint16_t fatsize = 0;
        ReadContent(rawcontent, fs, 22, 2);
        ReturnUint16(&fatsize, fs);
        delete[] fs;
        curfat->fatsize = fatsize;
        //std::cout << "Fat Size: " << fatsize << std::endl;
        // CLUSTER AREA START
	curfat->clusterareastart = reservedareasize + fatcount * fatsize + ((rootdirmaxfiles * 32) + (bytespersector - 1)) / bytespersector;
        //std::cout << "Cluster Area Start: " << reservedareasize + fatcount * fatsize + ((rootdirmaxfiles * 32) + (bytespersector - 1)) / bytespersector << std::endl;
        // ROOT DIRECTORY LAYOUT
        curfat->rootdirlayout = std::to_string((reservedareasize + fatcount * fatsize) * bytespersector) + "," + std::to_string(rootdirmaxfiles * 32 + bytespersector - 1) + ";";
	curfat->curdirlayout = curfat->rootdirlayout;
        //std::cout << "Root Directory Layout: " << curfat->rootdirlayout << std::endl;
    }
    else if(curfat->fattype == 3) // FAT32
    {
        // FAT OFFSET
        curfat->fatoffset = reservedareasize * bytespersector;
        //std::cout << "Fat Offset: " << curfat->fatoffset << std::endl;
	// FAT SIZE
	uint8_t* fs = new uint8_t[4];
	uint32_t fatsize = 0;
	ReadContent(rawcontent, fs, 36, 4);
	ReturnUint32(&fatsize, fs);
	delete[] fs;
	curfat->fatsize = fatsize;
	//std::cout << "Fat Size: " << fatsize << std::endl;
	// CLUSTER AREA START
	curfat->clusterareastart = reservedareasize + (fatcount * fatsize);
	//std::cout << "Cluster Area Start: " << curfat->clusterareastart << std::endl;
	// ROOT DIRECTORY LAYOUT
	uint8_t* rdc = new uint8_t[4];
	uint32_t rootdircluster = 0;
	ReadContent(rawcontent, rdc, 44, 4);
	ReturnUint32(&rootdircluster, rdc);
	delete[] rdc;
	std::vector<uint32_t> clusterlist;
	clusterlist.clear();
	if(rootdircluster >= 2)
	{
	    clusterlist.push_back(rootdircluster);
	    GetNextCluster(rawcontent, rootdircluster, curfat, &clusterlist);
	}
	if(clusterlist.size() > 0)
	    curfat->rootdirlayout = ConvertClustersToExtents(&clusterlist, curfat);
	clusterlist.clear();
	curfat->curdirlayout = curfat->rootdirlayout;
	//std::cout << "Root Directory Layout: " << curfat->rootdirlayout << std::endl;
    }
    else if(curfat->fattype == 4) // EXFAT
    {
	// FAT SIZE
	uint8_t* fs = new uint8_t[4];
	uint32_t fatsize = 0;
	ReadContent(rawcontent, fs, 84, 4);
	ReturnUint32(&fatsize, fs);
	delete[] fs;
	curfat->fatsize = fatsize;
	//std::cout << "FAT Size: " << fatsize << std::endl;
	// BYTES PER SECTOR
	uint8_t* bps = new uint8_t[1];
	ReadContent(rawcontent, bps, 108, 1);
	uint8_t bpspow = (uint8_t)bps[0];
	delete[] bps;
	uint16_t bytespersector = pow(2, bpspow);
	//std::cout << "Bytes Per Sector: " << bytespersector << std::endl;
	curfat->bytespersector = bytespersector;
	// SECTORS PER CLUSTER
	uint8_t* spc = new uint8_t[1];
	ReadContent(rawcontent, spc, 109, 1);
	uint8_t spcpow = (uint8_t)spc[0];
	delete[] spc;
	uint16_t sectorspercluster = pow(2, spcpow);
	//std::cout << "Sectors Per Cluster: " << sectorspercluster << std::endl;
	curfat->sectorspercluster = sectorspercluster;
	// FAT OFFSET
	uint8_t* fo = new uint8_t[4];
	uint32_t foff = 0;
	ReadContent(rawcontent, fo, 80, 4);
	ReturnUint32(&foff, fo);
	delete[] fo;
	curfat->fatoffset = foff * curfat->bytespersector;
	//std::cout << "FAT Offset: " << curfat->fatoffset << std::endl;
	// CLUSTER AREA START
	uint8_t* cas = new uint8_t[4];
	uint32_t clusterareastart = 0;
	ReadContent(rawcontent, cas, 88, 4);
	ReturnUint32(&clusterareastart, cas);
	delete[] cas;
	curfat->clusterareastart = clusterareastart;
	//std::cout << "Cluster Area Start: " << clusterareastart << std::endl;
	// ROOT DIRECTORY LAYOUT
	uint8_t* rdc = new uint8_t[4];
	uint32_t rootdircluster = 0;
	ReadContent(rawcontent, rdc, 96, 4);
	ReturnUint32(&rootdircluster, rdc);
	delete[] rdc;
	curfat->rootdircluster = rootdircluster;
	std::vector<uint32_t> clusterlist;
	clusterlist.clear();
	if(rootdircluster >= 2)
	{
	    clusterlist.push_back(rootdircluster);
	    GetNextCluster(rawcontent, rootdircluster, curfat, &clusterlist);
	}
	if(clusterlist.size() > 0)
	    curfat->rootdirlayout = ConvertClustersToExtents(&clusterlist, curfat);
	clusterlist.clear();
	curfat->curdirlayout = curfat->rootdirlayout;
	//std::cout << "Root Directory Layout: " << curfat->rootdirlayout << std::endl;
    }
}

void ParseFatForensics(std::string filename, std::string mntptstr, std::string devicestr, uint8_t ftype)
{
    std::ifstream devicebuffer(devicestr.c_str(), std::ios::in|std::ios::binary);
    std::cout << filename << " || " << mntptstr << " || " << devicestr  << " || " << (int)ftype << std::endl;
    // FTYPE = 1 (FAT12) = 2 (FAT16) = 3 (FAT32) = 4 (EXFAT)
    
    fatinfo curfat;
    curfat.fattype = ftype;
    // GET FAT FILESYSTEM INFO
    ParseFatInfo(&devicebuffer, &curfat);
    std::string pathstring = "";
    if(mntptstr.compare("/") == 0)
        pathstring = filename;
    else
    {
        std::size_t initpos = filename.find(mntptstr);
        pathstring = filename.substr(initpos + mntptstr.size());
    }
    std::cout << "pathstring: " << pathstring << std::endl;
    // SPLIT CURRENT FILE PATH INTO DIRECTORY STEPS
    std::vector<std::string> pathvector;
    std::istringstream iss(pathstring);
    std::string pp;
    while(getline(iss, pp, '/'))
        pathvector.push_back(pp);

    if(pathvector.size() > 2)
    {
        std::string nextdirlayout = "";
	nextdirlayout = ParseFatPath(&devicebuffer, &curfat, pathvector.at(1));
        curfat.curdirlayout = nextdirlayout;
	std::cout << "child path: " << pathvector.at(1) << "'s layout: " << nextdirlayout << std::endl;

        std::cout << "path vector size: " << pathvector.size() << std::endl;
	for(int i=1; i < pathvector.size() - 2; i++)
        {
	    nextdirlayout = ParseFatPath(&devicebuffer, &curfat, pathvector.at(i+1));
	    curfat.curdirlayout = nextdirlayout;
	    std::cout << "child path: " << pathvector.at(i+1) << "'s layout: " << nextdirlayout << std::endl;
	}
    }
    std::cout << "now to parse fat file: " << pathvector.at(pathvector.size() - 1) << std::endl;
    std::string forensicsinfo = ParseFatFile(&devicebuffer, &curfat, pathvector.at(pathvector.size() - 1));
    std::cout << forensicsinfo << std::endl;

}

std::string ParseFatPath(std::ifstream* rawcontent, fatinfo* curfat, std::string childpath)
{
    uint8_t isrootdir = 0;
    // NEED TO DETERMINE IF IT'S ROOT DIRECTORY OR NOT AND HANDLE ACCORDINGLY
    if(curfat->curdirlayout.compare(curfat->rootdirlayout) == 0)
    {
	isrootdir = 1;
	std::cout << "root dir layout matches curdirlayout" << std::endl;
    }
    else
    {
	isrootdir = 0;
	std::cout << "curdirlayout is not rootdirlayout" << std::endl;
    }

    std::cout << "child path to find: " << childpath << std::endl;
    // GET THE DIRECTORY CONTENT OFFSETS/LENGTHS AND THEN LOOP OVER THEM
    std::vector<std::string> dirlayoutlist;
    dirlayoutlist.clear();
    std::istringstream rdll(curfat->curdirlayout);
    std::string rdls;
    while(getline(rdll, rdls, ';'))
	dirlayoutlist.push_back(rdls);
    for(int i=0; i < dirlayoutlist.size(); i++)
    {
	uint64_t diroffset = 0;
	uint64_t dirlength = 0;
	std::size_t layoutsplit = dirlayoutlist.at(i).find(",");
	if(i == 0)
	{
	    diroffset = std::stoull(dirlayoutlist.at(i).substr(0, layoutsplit));
	    dirlength = std::stoull(dirlayoutlist.at(i).substr(layoutsplit+1));
	    if(isrootdir == 0) // sub directory
	    {
		diroffset = diroffset + 64; // skip . and .. directories
		dirlength = dirlength - 64; // adjust read size for the 64 byte skip
	    }
	}
	std::cout << "dir offset: " << diroffset << " dir length: " << dirlength << std::endl;
	unsigned int direntrycount = dirlength / 32;
	std::cout << "dir entry count: " << direntrycount << std::endl;
	// PARSE DIRECTORY ENTRIES
	std::string longnamestring = "";
	for(unsigned int j=0; j < direntrycount; j++)
	{
	    uint8_t* fc = new uint8_t[1];
	    uint8_t firstchar = 0;
	    ReadContent(rawcontent, fc, diroffset + j*32, 1);
	    firstchar = (uint8_t)fc[0];
	    delete[] fc;
	    std::cout << "first char: " << (int)firstchar << std::endl;
            if(firstchar == 0xe5) // deleted entry, skip
            {
            }
            else if(firstchar == 0x00) // entry is free and all remaining are free
                break;
            else
            {
                uint8_t* fa = new uint8_t[1];
                uint8_t fileattr = 0;
                ReadContent(rawcontent, fa, diroffset + j*32 + 11, 1);
                fileattr = (uint8_t)fa[0];
                delete[] fa;
                if(fileattr == 0x0f || fileattr == 0x3f) // Long Directory Name
                {
                    unsigned int lsn = ((int)firstchar & 0x0f);
                    if(lsn <= 20)
                    {
                        // process long file name part here... then add to the long file name...
                        std::string longname = "";
                        int arr[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
                        for(int k=0; k < 13; k++)
                        {
                            uint16_t longletter = 0;
                            uint8_t* ll = new uint8_t[3];
                            ReadContent(rawcontent, ll, diroffset + j*32 + arr[k], 2);
                            ReturnUint16(&longletter, ll);
                            delete[] ll;
                            if(longletter < 0xFFFF)
                            {
                                longname += (char)longletter;
                            }
                        }
                        longnamestring.insert(0, longname);
                    }
                }
		else
		{
		    if(fileattr == 0x10)
		    {
			//std::cout << "long name: " << longnamestring << "|" << std::endl;
			if(longnamestring.find(childpath) != std::string::npos)
			{
			    uint8_t* hcn = new uint8_t[2];
			    uint16_t hiclusternum = 0;
			    ReadContent(rawcontent, hcn, diroffset + j*32 + 20, 2); // always zero for fat12/16
			    ReturnUint16(&hiclusternum, hcn);
			    delete[] hcn;
			    uint8_t* lcn = new uint8_t[2];
			    uint16_t loclusternum = 0;
			    ReadContent(rawcontent, lcn, diroffset + j*32 + 26, 2);
			    ReturnUint16(&loclusternum, lcn);
			    delete[] lcn;
			    uint32_t clusternum = ((uint32_t)hiclusternum >> 16) + loclusternum;
			    std::vector<uint32_t> clusterlist;
			    clusterlist.clear();
			    //std::cout << "first cluster: " << clusternum << std::endl;
			    if(clusternum >= 2)
			    {
				clusterlist.push_back(clusternum);
				GetNextCluster(rawcontent, clusternum, curfat, &clusterlist);
			    }
			    std::string layout = "";
			    if(clusterlist.size() > 0)
			    {
				layout = ConvertClustersToExtents(&clusterlist, curfat);
			    }
			    clusterlist.clear();
			    //std::cout << "layout: " << layout << std::endl;
			    return layout;
			}
		    }
		    longnamestring = "";
		}
            }
	}
    }

    return "";
}

std::string ParseFatFile(std::ifstream* rawcontent, fatinfo* curfat, std::string childfile)
{
    // NEED TO SPIT OUT ORIGINAL INFORMATION FOR FILE WHICH IS IN || || PORTIONS
    std::string fileforensics = "";
    //std::cout << "child file to find: " << childfile << std::endl;
    //std::cout << "parent directory layout: " << curfat->curdirlayout << std::endl;

    uint8_t isrootdir = 0;
    if(curfat->curdirlayout.compare(curfat->rootdirlayout) == 0)
	isrootdir = 1;
    else
	isrootdir = 0;

    std::vector<std::string> dirlayoutlist;
    dirlayoutlist.clear();
    std::istringstream dll(curfat->curdirlayout);
    std::string dls;
    while(getline(dll, dls, ';'))
	dirlayoutlist.push_back(dls);
    for(int i=0; i < dirlayoutlist.size(); i++)
    {
	uint64_t diroffset = 0;
	uint64_t dirlength = 0;
	std::size_t layoutsplit = dirlayoutlist.at(i).find(",");
	if(i == 0)
	{
	    diroffset = std::stoull(dirlayoutlist.at(i).substr(0, layoutsplit));
	    dirlength = std::stoull(dirlayoutlist.at(i).substr(layoutsplit+1));
	    if(isrootdir == 0) // sub directory
	    {
		diroffset = diroffset + 64; // skip . and .. directories
		dirlength = dirlength - 64; // adjust read size for the 64 byte skip
	    }
	}
	//std::cout << "dir offset: " << diroffset << " dir length: " << dirlength << std::endl;
	unsigned int direntrycount = dirlength / 32;
	//std::cout << "dir entry count: " << direntrycount << std::endl;
	// PARSE DIRECTORY ENTRIES
	std::string longnamestring = "";
	for(unsigned int j=0; j < direntrycount; j++)
	{
	    uint8_t* fc = new uint8_t[1];
	    uint8_t firstchar = 0;
	    ReadContent(rawcontent, fc, diroffset + j*32, 1);
	    firstchar = (uint8_t)fc[0];
	    delete[] fc;
	    if(firstchar == 0xe5) // deleted entry, skip
	    {
	    }
	    else if(firstchar == 0x00) // entry is free and all remaining are free
		break;
	    else
	    {
                uint8_t* fa = new uint8_t[1];
                uint8_t fileattr = 0;
                ReadContent(rawcontent, fa, diroffset + j*32 + 11, 1);
                fileattr = (uint8_t)fa[0];
                delete[] fa;
                if(fileattr == 0x0f || fileattr == 0x3f) // Long Directory Name
                {
                    unsigned int lsn = ((int)firstchar & 0x0f);
                    if(lsn <= 20)
                    {
                        // process long file name part here... then add to the long file name...
                        std::string longname = "";
                        int arr[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
                        for(int k=0; k < 13; k++)
                        {
                            uint16_t longletter = 0;
                            uint8_t* ll = new uint8_t[3];
                            ReadContent(rawcontent, ll, diroffset + j*32 + arr[k], 2);
                            ReturnUint16(&longletter, ll);
                            delete[] ll;
                            if(longletter < 0xFFFF)
                            {
                                longname += (char)longletter;
                            }
                        }
                        longnamestring.insert(0, longname);
                    }
                }
		else // parse file contents here...
		{
		    //std::cout << "long name: " << longnamestring << "|" << std::endl;
		    if(longnamestring.find(childfile) != std::string::npos)
		    {
			// PRINT OUT LONG NAME
			fileforensics += "Long Name|" + longnamestring + "\n";
			//std::cout << "child file found, parse here..." << std::endl;
			// GET ALIAS NAME AND PRINT OUT
			uint8_t* rname = new uint8_t[8];
			ReadContent(rawcontent, rname, diroffset + j*32 + 1, 7);
			rname[7] = '\0';
			std::string restname((char*)rname);
			delete[] rname;
			//std::cout << "file name: " << (char)firstchar << restname << std::endl;
			uint8_t* ename = new uint8_t[4];
			ReadContent(rawcontent, ename, diroffset + j*32 + 8, 3);
			ename[3] = '\0';
			std::string extname((char*)ename);
			delete[] ename;
			std::size_t findempty = 0;
			while(extname.size() > 0)
			{
			    findempty = extname.find(" ", 0);
			    if(findempty != std::string::npos)
				extname.erase(findempty, 1);
			    else
				break;
			}
			//std::cout << "extname after find/erase loop: " << extname << "|" << std::endl;
			fileforensics += "Alias Name|" + std::string(1, (char)firstchar) + restname;
			if(extname.size() > 0)
			    fileforensics += "." + extname + "\n";
			else
			    fileforensics += "\n";
			// GET FILE PROPERTIES
			fileforensics += "Attributes|";
			if(fileattr & 0x01)
			    fileforensics += "Read Only,";
			if(fileattr & 0x02)
			    fileforensics += "Hidden File,";
			if(fileattr & 0x04)
			    fileforensics += "System File,";
			if(fileattr & 0x08)
			    fileforensics += "Volume ID,";
			if(fileattr & 0x10)
			    fileforensics += "Sub Directory,";
			if(fileattr & 0x20)
			    fileforensics += "Archive File,";
			fileforensics += "\n";
			// GET LOGICAL FILE SIZE
			uint8_t* ls = new uint8_t[4];
			uint32_t logicalsize = 0;
			ReadContent(rawcontent, ls, diroffset + j*32 + 28, 4);
			ReturnUint32(&logicalsize, ls);
			delete[] ls;
			fileforensics += "Logical Size|" + std::to_string(logicalsize) + " bytes\n";
			// GET PHYSICAL FILE SIZE (SHOULD BE LAYOUT)
			uint8_t* hcn = new uint8_t[2];
			uint16_t hiclusternum = 0;
			ReadContent(rawcontent, hcn, diroffset + j*32 + 20, 2); // always zero for fat12/16
			ReturnUint16(&hiclusternum, hcn);
			delete[] hcn;
			uint8_t* lcn = new uint8_t[2];
			uint16_t loclusternum = 0;
			ReadContent(rawcontent, lcn, diroffset + j*32 + 26, 2);
			ReturnUint16(&loclusternum, lcn);
			delete[] lcn;
			uint32_t clusternum = ((uint32_t)hiclusternum >> 16) + loclusternum;
			std::vector<uint32_t> clusterlist;
			clusterlist.clear();
			//std::cout << "first cluster: " << clusternum << std::endl;
			if(clusternum >= 2)
			{
			    clusterlist.push_back(clusternum);
			    GetNextCluster(rawcontent, clusternum, curfat, &clusterlist);
			}
			std::string layout = "";
			if(clusterlist.size() > 0)
			{
			    layout = ConvertClustersToExtents(&clusterlist, curfat);
			}
			clusterlist.clear();
			fileforensics += "Physical Layout|" + layout + " (offset,size;) [bytes]\n";
			//std::cout << "layout: " << layout << std::endl;
			// GET DATE/TIME's
			uint8_t* cd = new uint8_t[2];
			uint16_t createdate = 0;
			ReadContent(rawcontent, cd, diroffset + j*32 + 16, 2);
			ReturnUint16(&createdate, cd);
			delete[] cd;
			uint8_t* ct = new uint8_t[2];
			uint16_t createtime = 0;
			ReadContent(rawcontent, ct, diroffset + j*32 + 14, 2);
			ReturnUint16(&createtime, ct);
			fileforensics += "Create Date|" + ConvertDosTimeToHuman(&createdate, &createtime) + "\n";
			uint8_t* ad = new uint8_t[2];
			uint16_t accessdate = 0;
			ReadContent(rawcontent, ad, diroffset + j*32 + 18, 2);
			ReturnUint16(&accessdate, ad);
			delete[] ad;
			fileforensics += "Access Date|" + ConvertDosTimeToHuman(&accessdate, NULL) + "\n";
			uint8_t* md = new uint8_t[2];
			uint16_t modifydate = 0;
			ReadContent(rawcontent, md, diroffset + j*32 + 24, 2);
			ReturnUint16(&modifydate, md);
			delete[] md;
			uint8_t* mt = new uint8_t[2];
			uint16_t modifytime = 0;
			ReadContent(rawcontent, mt, diroffset + j*32 + 22, 2);
			ReturnUint16(&modifytime, mt);
			delete[] mt;
			fileforensics += "Modify Date|" + ConvertDosTimeToHuman(&modifydate, &modifytime) + "\n";
		    }
		    longnamestring = "";
		}
	    }
	}
    }

    return fileforensics;
}
