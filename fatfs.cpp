#include "fatfs.h"

/*
void GetNextCluster(ForImg* curimg, uint32_t clusternum, uint8_t fstype, qulonglong fatoffset, QList<uint>* clusterlist)
{
    uint32_t curcluster = 0;
    int fatbyte1 = 0;
    if(fstype == 1) // FAT12
    {
        fatbyte1 = clusternum + (clusternum / 2);
        if(clusternum & 0x0001) // ODD
            curcluster = (qFromLittleEndian<uint16_t>(curimg->ReadContent(fatoffset + fatbyte1, 2)) >> 4);
        else // EVEN
            curcluster = (qFromLittleEndian<uint16_t>(curimg->ReadContent(fatoffset + fatbyte1, 2)) & 0x0FFF);
        //if(curcluster != 0x0FF7)
        //    clusterlist->append(curcluster);
        //if(curcluster < 0x0FF8 && curcluster >= 2)
        if(curcluster < 0x0FF7 && curcluster >= 2)
        {
            clusterlist->append(curcluster);
            GetNextCluster(curimg, curcluster, fstype, fatoffset, clusterlist);
        }
    }
    else if(fstype == 2) // FAT16
    {
        if(clusternum >= 2)
        {
            fatbyte1 = clusternum * 2;
            curcluster = qFromLittleEndian<uint16_t>(curimg->ReadContent(fatoffset + fatbyte1, 2));
            //if(curcluster != 0xFFF7)
            //    clusterlist->append(curcluster);
            //if(curcluster < 0xFFF8 && curcluster >= 2)
            if(curcluster < 0xFFF7 && curcluster >= 2)
            {
                clusterlist->append(curcluster);
                GetNextCluster(curimg, curcluster, fstype, fatoffset, clusterlist);
            }
        }
    }
    else if(fstype == 3) // FAT32
    {
        if(clusternum >= 2)
        {
            fatbyte1 = clusternum * 4;
            curcluster = (qFromLittleEndian<uint32_t>(curimg->ReadContent(fatoffset + fatbyte1, 4)) & 0x0FFFFFFF);
	    //if(curcluster != 0x0FFFFFF7)
		//clusterlist->append(curcluster);
	    if(curcluster < 0x0FFFFFF7 && curcluster >= 2)
            {
                clusterlist->append(curcluster);
		GetNextCluster(curimg, curcluster, fstype, fatoffset, clusterlist);
            }
        }
    }
    else if(fstype == 4) // EXFAT
    {
        if(clusternum >= 2)
        {
            fatbyte1 = clusternum * 4;
            curcluster = qFromLittleEndian<uint32_t>(curimg->ReadContent(fatoffset + fatbyte1, 4));
	    //if(curcluster != 0xFFFFFFF7)
		//clusterlist->append(curcluster);
	    if(curcluster < 0xFFFFFFF7 && curcluster >= 2)
            {
                clusterlist->append(curcluster);
		GetNextCluster(curimg, curcluster, fstype, fatoffset, clusterlist);
            }
        }
    }
}
*/

/*
 uint16_t bytespersector = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 11, 2));
 uint8_t fatcount = qFromLittleEndian<uint8_t>(curimg->ReadContent(curstartsector*512 + 16, 1));
uint8_t sectorspercluster = qFromLittleEndian<uint8_t>(curimg->ReadContent(curstartsector*512 + 13, 1));
//uint16_t bytesperclutser = bytespersector * sectorspercluster;
uint16_t reservedareasize = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 14, 2));
uint16_t rootdirmaxfiles = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 17, 2));
out << "Reserved Area Size|" << QString::number(reservedareasize) << "|Size of the reserved area at the beginning of the file system." << Qt::endl;
out << "Root Directory Max Files|" << QString::number(rootdirmaxfiles) << "|Maximum number of root directory entries." << Qt::endl;
if(fatstr == "FAT12" || fatstr == "FAT16" || fat32str == "FAT32")
{
    out << "File System Sector Count|";
    if(qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 19, 2)) == 0)
        out << QString::number(qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector*512 + 32, 4)));
    else
        out << QString::number(qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 19, 2)));
    out << "|Total sectors in the volume." << Qt::endl;
}
//qulonglong rootdiroffset = 0;
if(fatstr == "FAT12" || fatstr == "FAT16")
{
    out << "Root Directory Offset|" << QString::number((qulonglong)(curstartsector*512 + (reservedareasize + fatcount * fatsize) * bytespersector)) << "|Byte offset for the root directory" << Qt::endl;
    out << "Root Directory Sectors|" << QString::number(((rootdirmaxfiles * 32) + (bytespersector - 1)) / bytespersector) << "|Number of sectors for the root directory." << Qt::endl;
    out << "Root Directory Size|" << QString::number((rootdirmaxfiles * 32) + (bytespersector - 1)) << "|Size in bytes for the root directory." << Qt::endl;
    out << "Cluster Area Start|" << QString::number((qulonglong)(curstartsector + reservedareasize + fatcount * fatsize + ((rootdirmaxfiles * 32) + (bytespersector - 1)) / bytespersector)) << "|Byte offset to the start of the cluster area." << Qt::endl;
    out << "Root Directory Layout|" << QString(QString::number((qulonglong)(curstartsector*512 + (reservedareasize + fatcount * fatsize) * bytespersector)) + "," + QString::number((rootdirmaxfiles * 32) + (bytespersector - 1)) + ";") << "|Layout for the root directory." << Qt::endl;
}
else if(fat32str == "FAT32")
{
    uint32_t rootdircluster = qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector*512 + 44, 4));
    qulonglong rootdiroffset = (qulonglong)(curstartsector*512 + (reservedareasize + fatcount * fatsize) * bytespersector);
    out << "Root Directory Cluster|" << QString::number(rootdircluster) << "|Clutser offset to the start of the root directory." << Qt::endl;
    out << "Root Directory Offset|" << QString::number(rootdiroffset) << "|Byte offset to the start of the root directory." << Qt::endl;
    qulonglong clusterareastart = (qulonglong)(curstartsector + reservedareasize + (fatcount * fatsize));
    out << "Cluster Area Start|" << QString::number(clusterareastart) << "|Byte offset to the start of the cluster area." << Qt::endl;
    QList<uint> clusterlist;
    clusterlist.clear();
    // FirstSectorOfCluster = ((N-2) * sectorspercluster) + firstdatasector [rootdirstart];
    if(rootdircluster >= 2)
    {
        clusterlist.append(rootdircluster);
        GetNextCluster(curimg, rootdircluster, 3, fatoffset, &clusterlist);
    }
    out << "Root Directory Layout|" << ConvertBlocksToExtents(clusterlist, sectorspercluster * bytespersector, rootdiroffset) << "|Layout for the root directory";
    //qDebug() << "Extent String:" << ConvertBlocksToExtents(clusterlist, sectorspercluster * bytespersector, rootdiroffset);
    clusterlist.clear();
}
else if(exfatstr == "EXFAT")
{
    uint32_t fatsize = qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector*512 + 84, 4));
    out << "FAT Size|" << QString::number(fatsize) << "|Size of the FAT." << Qt::endl;
    bytespersector = pow(2, qFromLittleEndian<uint8_t>(curimg->ReadContent(curstartsector*512 + 108, 1))); // power of 2 so 2^(bytespersector)
    sectorspercluster = pow(2, qFromLittleEndian<uint8_t>(curimg->ReadContent(curstartsector*512 + 109, 1))); // power of 2 so 2^(sectorspercluster)
    out << "Bytes Per Sector|" << QString::number(bytespersector) << "|Number of bytes per sector, usually 512." << Qt::endl;
    out << "Sectors Per Cluster|" << QString::number(sectorspercluster) << "|Number of sectors per cluster." << Qt::endl;
    fatcount = qFromLittleEndian<uint8_t>(curimg->ReadContent(curstartsector*512 + 110, 1)); // 1 or 2, 2 if TexFAT is in use
    out << "FAT Count|" << QString::number(fatcount) << "| Number of FAT's in the file system." << Qt::endl;
    qulonglong fatoffset = (qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector*512 + 80, 4)) * bytespersector) + (curstartsector * 512);
    out << "FAT Offset|" << QString::number(fatoffset) << "|Byte offset to the start of the first FAT." << Qt::endl;
    uint32_t rootdircluster = qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector*512 + 96, 4));
    uint32_t clusterstart = qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector*512 + 88, 4));
    out << "Cluster Area Start|" << QString::number(clusterstart) << "|Sector offset to the start of the cluster area." << Qt::endl;
    qulonglong rootdiroffset = (qulonglong)((curstartsector * 512) + (((rootdircluster - 2) * sectorspercluster) + clusterstart) * bytespersector);
    out << "Root Directory Offset|" << QString::number(rootdiroffset) << "|Byte offset to the start of the root directory." << Qt::endl;
    out << "Root Directory Size|" << QString::number(((rootdirmaxfiles * 32) + (bytespersector - 1))) << "|Size in bytes of the root directory." << Qt::endl;
    //fsinfo.insert("rootdirsectors", QVariant(((fsinfo.value("rootdirmaxfiles").toUInt() * 32) + (fsinfo.value("bytespersector").toUInt() - 1)) / fsinfo.value("bytespersector").toUInt()));
    //fsinfo.insert("rootdirsize", QVariant(fsinfo.value("rootdirsectors").toUInt() * fsinfo.value("bytespersector").toUInt()));
    QList<uint> clusterlist;
    clusterlist.clear();
    if(rootdircluster >= 2)
    {
        clusterlist.append(rootdircluster);
        GetNextCluster(curimg, rootdircluster, 4, fatoffset, &clusterlist);
    }
    QString rootdirlayout = ConvertBlocksToExtents(clusterlist, sectorspercluster * bytespersector, clusterstart * bytespersector);
    out << "Root Directory Layout|" << rootdirlayout << "|Layout for the root directory";
    //qDebug() << "Extent String:" << rootdirlayout;
    clusterlist.clear();
    for(int i=0; i < rootdirlayout.split(";", Qt::SkipEmptyParts).count(); i++)
    {
        uint curoffset = 0;
        qDebug() << "i:" << i << "rootdiroffset:" << rootdirlayout.split(";").at(i).split(",").at(0) << "rootdirlength:" << rootdirlayout.split(";").at(i).split(",").at(1);
        while(curoffset < rootdirlayout.split(";").at(i).split(",").at(1).toUInt())
        {
            if(qFromLittleEndian<uint8_t>(curimg->ReadContent(rootdirlayout.split(";").at(i).split(",").at(0).toULongLong() + curoffset, 1)) == 0x83)
                break;
            curoffset += 32;
        }
        qDebug() << "curoffset:" << curoffset;
        if(curoffset < rootdirlayout.split(";").at(i).split(",").at(1))
        {
            if(qFromLittleEndian<uint8_t>(curimg->ReadContent(rootdirlayout.split(";").at(i).split(",").at(0).toULongLong() + curoffset + 1, 1)) > 0)
            {
                for(int j=0; j < qFromLittleEndian<uint8_t>(curimg->ReadContent(rootdirlayout.split(";").at(i).split(",").at(0).toULongLong() + curoffset + 1, 1)); j++)
                    partitionname += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdirlayout.split(";").at(i).split(",").at(0).toULongLong() + curoffset + 2 + j*2, 2))));
                qDebug() << "partitionname:" << partitionname;
                out << "Volume Label|" << partitionname << "|Label for the file system volume." << Qt::endl;
                partitionname += " [EXFAT]";
            }
        }
    }
}

 */

void ParseFatInfo(std::ifstream* rawcontent, fatinfo* curfat, uint8_t ftype)
{
    if(ftype == 1 || ftype == 2)
    {
        uint8_t* bps = new uint8_t[2];
        uint16_t bytespersector = 0;
        ReadContent(rawcontent, bps, 11, 2);
        ReturnUint16(&bytespersector, bps);
        delete[] bps;
        curfat->bytespersector = bytespersector;
        std::cout << "Bytes Per Sector: " << bytespersector << std::endl;
        uint8_t* fc = new uint8_t[1];
        uint8_t fatcount = 0;
        ReadContent(rawcontent, fc, 16, 1);
        fatcount = (uint8_t)fc[0];
        delete[] fc;
        std::cout << "Fat Count: " << (unsigned int)fatcount << std::endl;
        uint8_t* spc = new uint8_t[1];
        uint8_t sectorspercluster = 0;
        ReadContent(rawcontent, spc, 13, 1);
        sectorspercluster = (uint8_t)spc[0];
        delete[] spc;
        curfat->sectorspercluster = sectorspercluster;
        std::cout << "Sectors per cluster: " << (unsigned int)sectorspercluster << std::endl;
        uint8_t* ras = new uint8_t[2];
        uint16_t reservedareasize = 0;
        ReadContent(rawcontent, ras, 14, 2);
        ReturnUint16(&reservedareasize, ras);
        delete[] ras;
        std::cout << "Reserved Area Size: " << reservedareasize << std::endl;
        curfat->fatoffset = reservedareasize * bytespersector;
        std::cout << "Fat Offset: " << curfat->fatoffset << std::endl;
        uint8_t* fs = new uint8_t[2];
        uint16_t fatsize = 0;
        ReadContent(rawcontent, fs, 22, 2);
        ReturnUint16(&fatsize, fs);
        delete[] fs;
        curfat->fatsize = fatsize;
        std::cout << "Fat Size: " << fatsize << std::endl;
        uint8_t* rdmf = new uint8_t[2];
        uint16_t rootdirmaxfiles = 0;
        ReadContent(rawcontent, rdmf, 17, 2);
        ReturnUint16(&rootdirmaxfiles, rdmf);
        delete[] rdmf;
        std::cout << "Root Dir Max Files: " << rootdirmaxfiles << std::endl;
        std::cout << "Cluster Area Start: " << reservedareasize + fatcount * fatsize + ((rootdirmaxfiles * 32) + (bytespersector - 1)) / bytespersector << std::endl;
        curfat->rootdirlayout = std::to_string((reservedareasize + fatcount * fatsize) * bytespersector) + "," + std::to_string(rootdirmaxfiles * 32 + bytespersector - 1) + ";";
        std::cout << "Root Directory Layout: " << curfat->rootdirlayout << std::endl;
    }

    /*
            uint16_t rootdirmaxfiles = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 17, 2));
            out << "Cluster Area Start|" << curstartsector + reservedareasize + fatcount * fatsize + ((rootdirmaxfiles * 32) + (bytespersector - 1)) / bytespersector << "|Byte offset to the start of the cluster area." << Qt::endl;
            out << "Root Directory Layout|" << QString(QString::number((qulonglong)(curstartsector*512 + (reservedareasize + fatcount * fatsize) * bytespersector)) + "," + QString::number((rootdirmaxfiles * 32) + (bytespersector - 1)) + ";") << "|Layout for the root directory." << Qt::endl;
    */
}

void ParseFatForensics(std::string filename, std::string mntptstr, std::string devicestr, uint8_t ftype)
{
    std::ifstream devicebuffer(devicestr.c_str(), std::ios::in|std::ios::binary);
    std::cout << filename << " || " << mntptstr << " || " << devicestr  << " || " << ftype << std::endl;
    // FTYPE = 1 (FAT12) = 2 (FAT16) = 3 (FAT32) = 4 (EXFAT)
    
    fatinfo curfat;
    // GET FAT FILESYSTEM INFO
    ParseFatInfo(&devicebuffer, &curfat, ftype);
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

    std::cout << "path vector size: " << pathvector.size() << std::endl;
    // PARSE ROOT DIRECTORY OF THE MOUNTED FILE SYSTEM
    uint32_t returninode = 0;
    returninode = ParseFatPath(&devicebuffer, &curfat, pathvector.at(1));
    std::cout << "next inode to traverse: " << returninode << std::endl;


    // PARSE ROOT DIRECTORY, WHICH IS ZERO I GUESS
    // curinode = ParseFatDirectory(curimg, curstartsector, ptreecnt, 0, "", "");
    /*
    // MAY WANT THIS IN HERE TO LOOP AND PARSE THE RESPECTIVE DIRECTORIES TO GET TO THE FILE...
    std::cout << "path vector size: " << pathvector.size() << std::endl;
    // PARSE ROOT DIRECTORY AND GET INODE FOR THE NEXT DIRECTORY IN PATH VECTOR

    std::cout << "Inode for " << pathvector.at(1) << ": " << returninode << std::endl;
    std::cout << "Loop Over the Remaining Paths\n";

    for(int i=1; i < pathvector.size() - 2; i++)
    {
        returninode = ParseExtPath(&devicebuffer, &cursb, returninode, pathvector.at(i));
        std::cout << "Inode for " << pathvector.at(i) << ": " << returninode << std::endl;
        //std::cout << "i: " << i << " Next Directory to Parse: " << pathvector.at(i+1) << std::endl;
    }
    
    ParseExtFile(&devicebuffer, &cursb, returninode, pathvector.at(pathvector.size() - 1));
    */
}

uint32_t ParseFatPath(std::ifstream* rawcontent, fatinfo* curfat, std::string childpath)
{
    // NEED TO DETERMINE IF IT'S ROOT DIRECTORY OR NOT AND HANDLE ACCORDINGLY
    std::cout << "child path to find: " << childpath << std::endl;
    // GET THE DIRECTORY CONTENT OFFSETS/LENGTHS AND THEN LOOP OVER THEM
    std::vector<std::string> rootdirlayoutlist;
    rootdirlayoutlist.clear();
    std::istringstream rdll(curfat->rootdirlayout);
    std::string rdls;
    while(getline(rdll, rdls, ';'))
	rootdirlayoutlist.push_back(rdls);
    for(int i=0; i < rootdirlayoutlist.size(); i++)
    {
	std::size_t layoutsplit = rootdirlayoutlist.at(i).find(",");
	uint64_t rootdiroffset = std::stoull(rootdirlayoutlist.at(i).substr(0, layoutsplit));
	uint64_t rootdirlength = std::stoull(rootdirlayoutlist.at(i).substr(layoutsplit+1));
	std::cout << "root dir offset: " << rootdiroffset << " root dir length: " << rootdirlength << std::endl;
	unsigned int rootdirentrycount = rootdirlength / 32;
	std::cout << "root dir entry count: " << rootdirentrycount << std::endl;
	// PARSE DIRECTORY ENTRIES
	std::string longnamestring = "";
	for(unsigned int j=0; j < rootdirentrycount; j++)
	{
	    uint8_t* fc = new uint8_t[1];
	    uint8_t firstchar = 0;
	    ReadContent(rawcontent, fc, rootdiroffset + j*32, 1);
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
                ReadContent(rawcontent, fa, rootdiroffset + j*32 + 11, 1);
                fileattr = (uint8_t)fa[0];
                delete[] fa;
                if(fileattr == 0x0f || fileattr == 0x3f)
                {
                    // long name is 260 / 13, so no more than 20 names
                    unsigned int lsn = ((int)firstchar & 0x4f);
                    if(lsn <= 20)
                        std::cout << "lfn sequence number: " << ((int)firstchar & 0x4f) << std::endl;
                }
            }
            /*
            if(fileattr == 0x0f || fileattr == 0x3f) // long directory entry for succeeding short entry...
	    {
		//std::cout << "get long directory name here, which should come after short entry..." << std::endl;
		std::string l3 = "";
		std::string l2 = "";
		std::string l1 = "";
		//for(int k=0; k < int array.size(); k++) int array [28, 30, 14, 16, 18, 20, 22, 24, 1, 3, 5, 7, 9]
		//int arr[13] =  {28, 30, 14, 16, 18, 20, 22, 24, 1, 3, 5, 7, 9};
		int arr2[13] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
		for(int k=0; k < 13; k++)
		{
		    uint16_t longletter = 0;
		    uint8_t* ll = new uint8_t[3];
		    ReadContent(rawcontent, ll, rootdiroffset + j*32 + arr2[k], 2);
		    ReturnUint16(&longletter, ll);
		    delete[] ll;
		    if(longletter < 0xFFFF)
		    {
			l1 += (char)longletter;
			//std::cout << "long letter "<< arr[k] << " has value: " << (char)longletter << std::endl;
		    }
		}
		//std::cout << "long name: " << l1 << "|" << std::endl;
		longnamestring.insert(0, l1);

	    }
            if(fileattr & 0x10)
	    {
		if(!longnamestring.empty())
		{
		    std::cout << "long name: " << longnamestring << "|" << std::endl;
		    longnamestring = "";
		}
                //if(!longnamestring.isEmpty())
		std::cout << "first char: " << std::hex << (int)firstchar << std::endl;
		std::cout << "File Attribute: " << std::hex << (int)fileattr << std::endl;
                std::cout << " Current Directory Entry is a Sub Directory, get name and see if it matches childpath\n";
		if(firstchar == 0xe5 || firstchar == 0x05) // was allocated but now free
		{
		    std::cout << "deleted, so skip..\n";
		}
		uint8_t* rname = new uint8_t[8];
		ReadContent(rawcontent, rname, rootdiroffset + j*32 + 1, 7);
		rname[7] = '\0';
		std::string restname((char*)rname);
		delete[] rname;
		std::cout << "file name: " << (char)firstchar << restname << std::endl;
		uint8_t* ename = new uint8_t[4];
		ReadContent(rawcontent, ename, rootdiroffset + j*32 + 8, 3);
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
		std::cout << "extname after find/erase loop: " << extname << "|" << std::endl;

		if(extname.size() > 0)
		    std::cout << "alias name: " << (char)firstchar << restname + "." + extname << std::endl;
		else
		    std::cout << "alias name: " << (char)firstchar << restname << std::endl;
	    }
            */
	}
    }

    return 2;
}

/*
qulonglong ParseFatDirectory(ForImg* curimg, uint32_t curstartsector, uint8_t ptreecnt, qulonglong parinode, QString parfilename, QString dirlayout)
{
    if(!dirlayout.isEmpty())
	rootdirlayout = dirlayout;
    for(int i=0; i < rootdirlayout.split(";", Qt::SkipEmptyParts).count(); i++)
    {
	if(i == 0)
	    if(dirlayout.isEmpty()) // root directory
	        rootdiroffset = rootdirlayout.split(";", Qt::SkipEmptyParts).at(i).split(",").at(0).toULongLong();
		rootdirsize = rootdirlayout.split(";", Qt::SkipEmptyParts).at(i).split(",").at(1).toULongLong();
	    else // sub directory
		rootdiroffset = rootdirlayout.split(";", Qt::SkipEmptyParts).at(i).split(",").at(0).toULongLong() + 64; // skip . and .. directories
		rootdirsize = rootdirlayout.split(";", Qt::SkipEmptyParts).at(i).split(",").at(1).toULongLong() - 64; // adjust read size for the 64 byte skip
        uint rootdirentrycount = rootdirsize / 32;
	if(parinode == 0)
	    inodecnt = 0;
	else
	    inodecnt = parinode + 1;
        for(uint j=0; j < rootdirentrycount; j++)
        {
	    QString longname = "";
            //qDebug() << "firstchar:" << QString::number(firstchar, 16) << "fileattr:" << QString::number(fileattr, 16);
            if(fileattr != 0x0f && fileattr != 0x00 && fileattr != 0x3f) // need to process differently // 0x3f is ATTR_LONG_NAME_MASK which is a long name entry sub component
            {
                if(!longnamestring.isEmpty())
                {
                    out << "Long Name|" << longnamestring << "|Long name for the file." << Qt::endl;
		    longname = longnamestring;
                    longnamestring = "";
                }
                else
                    out << "Long Name| |Long name for the file." << Qt::endl;

		//uint8_t createtenth = rootdirbuf.at(i*32 + 13); // NOT GOING TO USE RIGHT NOW...
                qint64 createdate = ConvertDosTimeToUnixTime(qFromLittleEndian<uint8_t>(curimg->ReadContent(rootdiroffset + j*32 + 15, 1)), qFromLittleEndian<uint8_t>(curimg->ReadContent(rootdiroffset + j*32 + 14, 1)), qFromLittleEndian<uint8_t>(curimg->ReadContent(rootdiroffset + j*32 + 17, 1)), qFromLittleEndian<uint8_t>(curimg->ReadContent(rootdiroffset + j*32 + 16, 1)));
                qint64 accessdate = ConvertDosTimeToUnixTime(0x00, 0x00, qFromLittleEndian<uint8_t>(curimg->ReadContent(rootdiroffset + j*32 + 19, 1)), qFromLittleEndian<uint8_t>(curimg->ReadContent(rootdiroffset + j*32 + 18, 1)));
                qint64 modifydate = ConvertDosTimeToUnixTime(qFromLittleEndian<uint8_t>(curimg->ReadContent(rootdiroffset + j*32 + 23, 1)), qFromLittleEndian<uint8_t>(curimg->ReadContent(rootdiroffset + j*32 + 22, 1)), qFromLittleEndian<uint8_t>(curimg->ReadContent(rootdiroffset + j*32 + 25, 1)), qFromLittleEndian<uint8_t>(curimg->ReadContent(rootdiroffset + j*32 + 24, 1)));
                uint16_t hiclusternum = qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 20, 2)); // always zero for fat12/16
                uint16_t loclusternum = qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 26, 2));
                uint32_t clusternum = ((uint32_t)hiclusternum >> 16) + loclusternum;
                QList<uint> clusterlist;
                clusterlist.clear();
                if(clusternum >= 2)
                {
                    clusterlist.append(clusternum);
                    GetNextCluster(curimg, clusternum, fstype, fatoffset, &clusterlist);
                }
                QString layout = "";
                if(clusterlist.count() > 0)
		    layout = ConvertBlocksToExtents(clusterlist, sectorspercluster * bytespersector, clusterareastart * bytespersector);
		out << "Layout|" << layout << "|File offset,size; layout in bytes." << Qt::endl;
                qulonglong physicalsize = clusterlist.count() * sectorspercluster * bytespersector;
		out << "Physical Size|" << QString::number(physicalsize) << "|Sector Size in Bytes for the file." << Qt::endl;
                clusterlist.clear();
		uint32_t logicalsize = 0;
		uint8_t itemtype = 0;
		if(fileattr & 0x10) // sub directory attribute
		{
		    if(firstchar == 0xe5 || firstchar == 0x05) // deleted directory
			itemtype = 2;
		    else // directory
			itemtype = 3;
                    qulonglong curdirsize = 0;
                    int lastdirentry = 0;
                    int curdircnt = 0;
                    for(int k = 0; k < layout.split(";", Qt::SkipEmptyParts).count(); k++)
                    {
                        curdirsize += layout.split(";").at(k).split(",").at(1).toULongLong();
                    }
                    for(int k=0; k < layout.split(";", Qt::SkipEmptyParts).count(); k++)
                    {
                        for(uint m=0; m < curdirsize / 32; m++)
                        {
                            uint8_t curfirstchar = qFromLittleEndian<uint8_t>(curimg->ReadContent(layout.split(";").at(k).split(",").at(0).toULongLong() + m*32, 1));
                            if(curfirstchar == 0x00) // entry is free and all remaining are free
                            {
                                lastdirentry = curdircnt;
                                break;
                            }
                            curdircnt++;
                        }
                    }
                    logicalsize = lastdirentry * 32;
		}
		else
		{
		    if(firstchar == 0xe5 || firstchar == 0x05) // deleted file
			itemtype = 4;
		    else
			itemtype = 5;
		    logicalsize = qFromLittleEndian<uint32_t>(curimg->ReadContent(rootdiroffset + j*32 + 28, 4));
		}
		out << "Logical Size|" << QString::number(logicalsize) << "|Size in Bytes for the file." << Qt::endl;
		//qDebug() << "logicalsize:" << logicalsize;
		// ADD FILE INFO TO THE NODE TREE...
		QString filepath = "";
		QString filename = "";
		if(longname.isEmpty())
		    filename = aliasname;
		else
		    filename = longname;
		QHash<QString, QVariant> nodedata;
		nodedata.clear();
		nodedata.insert("name", QByteArray(filename.toStdString().c_str()).toBase64());
		if(parfilename.isEmpty())
		    filepath = "/";
		else
		    filepath = parfilename;
		nodedata.insert("path", QByteArray(filepath.toUtf8()).toBase64());
                nodedata.insert("size", logicalsize);
                nodedata.insert("create", createdate);
                nodedata.insert("access", accessdate);
                nodedata.insert("modify", modifydate);
                //nodedata.insert("status", "0");
                //nodedata.insert("hash", "0");
		if(logicalsize > 0) // Get Category/Signature
		{
		    if(itemtype == 3 && isdeleted == 0)
                    {
			nodedata.insert("cat", "Directory");
                        nodedata.insert("sig", "Directory"); // category << signature
                    }
		    else
		    {
			//QString catsig = GenerateCategorySignature(curimg, filename, layout); might not need this method, could just read more and see if it has word/ in hex.
			QString catsig = GenerateCategorySignature(curimg, filename, layout.split(";").at(0).split(",").at(0).toULongLong());
			//qDebug() << filename.left(20) << catsig;
                        nodedata.insert("cat", catsig.split("/").first());
                        nodedata.insert("sig", catsig.split("/").last());
		    }
		}
                else
                {
                    nodedata.insert("cat", "Empty");
                    nodedata.insert("sig", "Zero File");
                }
                //nodedata.insert("tag", "0");
                nodedata.insert("id", QString("e" + curimg->MountPath().split("/").last().split("-e").last() + "-p" + QString::number(ptreecnt) + "-f" + QString::number(inodecnt)));
		//nodedata.insert("match", 0);
                QString parentstr = "";
                if(parinode == 0)
                    parentstr = QString("e" + curimg->MountPath().split("/").last().split("-e").last() + "-p" + QString::number(ptreecnt));
                else
                    parentstr = QString("e" + curimg->MountPath().split("/").last().split("-e").last() + "-p" + QString::number(ptreecnt) + "-f" + QString::number(parinode));
                mutex.lock();
                treenodemodel->AddNode(nodedata, parentstr, itemtype, isdeleted);
                mutex.unlock();
                if(nodedata.value("id").toString().split("-").count() == 3)
                {
                    listeditems.append(nodedata.value("id").toString());
                    filesfound++;
                    isignals->ProgUpd();
                }
                inodecnt++;
                out.flush();
                fileprop.close();
                nodedata.clear();
		//qDebug() << "inodecnt:" << inodecnt << filename << layout;
                if(fileattr & 0x10 && logicalsize > 0) // sub directory
                {
                    if(firstchar != 0xe5 && firstchar != 0x05) // not deleted
			inodecnt = ParseFatDirectory(curimg, curstartsector, ptreecnt, inodecnt-1, QString(filepath + filename + "/"), layout);
                }
            }
            else if(fileattr == 0x0f || 0x3f) // long directory entry for succeeding short entry...
            {
                if(firstchar & 0x40)
                {
                    if(!longnamestring.isEmpty()) // orphan long entry
                    {
			QHash<QString, QVariant> nodedata;
			nodedata.clear();
			nodedata.insert("name", QByteArray(longnamestring.toUtf8()).toBase64());
                        nodedata.insert("path", QByteArray("/orphans/").toBase64());
                        nodedata.insert("size", 0);
                        nodedata.insert("create", "0");
                        nodedata.insert("access", "0");
                        nodedata.insert("modify", "0");
                        nodedata.insert("status", "0");
                        nodedata.insert("hash", "0");
                        nodedata.insert("cat", "Empty");
                        nodedata.insert("sig", "Empty File");
                        nodedata.insert("tag", "0");
                        nodedata.insert("id", QString("e" + curimg->MountPath().split("/").last().split("-e").last() + "-p" + QString::number(ptreecnt) + "-f" + QString::number(inodecnt)));
			nodedata.insert("match", 0);
			mutex.lock();
			treenodemodel->AddNode(nodedata, QString("e" + curimg->MountPath().split("/").last().split("-e").last() + "-p" + QString::number(ptreecnt) + "-o"), 4, 1);
			mutex.unlock();
			nodedata.clear();
			inodecnt++;
                        //qDebug() << "orphan:" << longnamestring;
                        //orphaninfo.clear();
                        //orphaninfo.insert("filename", QVariant(longnamestring));
                        //orphanlist->append(orphaninfo);
                        //orphanlist->append(longnamestring);
                        longnamestring = "";
                    }
                }
                QString l3 = "";
                QString l2 = "";
                QString l1 = "";
                if(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 28, 2)) < 0xFFFF)
                    l3 += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 28, 2))));
                if(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 30, 2)) < 0xFFFF)
                    l3 += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 30, 2))));
                if(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 14, 2)) < 0xFFFF)
                    l2 += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 14, 2))));
                if(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 16, 2)) < 0xFFFF)
                    l2 += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 16, 2))));
                if(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 18, 2)) < 0xFFFF)
                    l2 += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 18, 2))));
                if(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 20, 2)) < 0xFFFF)
                    l2 += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 20, 2))));
                if(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 22, 2)) < 0xFFFF)
                    l2 += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 22, 2))));
                if(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 24, 2)) < 0xFFFF)
                    l2 += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 24, 2))));
                if(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 1, 2)) < 0xFFFF)
                    l1 += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 1, 2))));
                if(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 3, 2)) < 0xFFFF)
                    l1 += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 3, 2))));
                if(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 5, 2)) < 0xFFFF)
                    l1 += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 5, 2))));
                if(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 7, 2)) < 0xFFFF)
                    l1 += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 7, 2))));
                if(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 9, 2)) < 0xFFFF)
                    l1 += QString(QChar(qFromLittleEndian<uint16_t>(curimg->ReadContent(rootdiroffset + j*32 + 9, 2))));
                longnamestring.prepend(QString(l1 + l2 + l3).toUtf8());
            }
        }
    }
    return inodecnt;
}
*/
