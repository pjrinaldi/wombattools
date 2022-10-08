#include "extfs.h"

void GetContentBlocks(std::ifstream* devicebuffer, uint32_t blocksize, uint64_t curoffset, uint32_t* incompatflags, std::vector<uint32_t>* blocklist)
{
    uint8_t* iflags = new uint8_t[4];
    devicebuffer->seekg(curoffset + 32);
    devicebuffer->read((char*)iflags, 4);
    uint32_t inodeflags = (uint32_t)iflags[0] | (uint32_t)iflags[1] << 8 | (uint32_t)iflags[2] << 16 | (uint32_t)iflags[3] << 24;
    delete[] iflags;
    if(*incompatflags & 0x40 && inodeflags & 0x80000) // FS USES EXTENTS AND INODE USES EXTENTS
    {
        //std::cout << "uses extents\n";
        uint8_t* eentry = new uint8_t[2];
        devicebuffer->seekg(curoffset + 42);
        devicebuffer->read((char*)eentry, 2);
        uint16_t extententries = (uint16_t)eentry[0] | (uint16_t)eentry[1] << 8;
        delete[] eentry;
        uint8_t* edepth = new uint8_t[2];
        devicebuffer->seekg(curoffset + 46);
        devicebuffer->read((char*)edepth, 2);
        uint16_t extentdepth = (uint16_t)edepth[0] | (uint16_t)edepth[1] << 8;
        delete[] edepth;
        if(extentdepth == 0) // use ext4_extent
        {
            for(uint16_t i=0; i < extententries; i++)
            {
                uint8_t* strtblk = new uint8_t[6];
                devicebuffer->seekg(curoffset + 58 + i*12);
                devicebuffer->read((char*)strtblk, 6);
                uint16_t starthi = (uint16_t)strtblk[0] | (uint16_t)strtblk[1] << 8;
                uint32_t startlo = (uint32_t)strtblk[2] | (uint32_t)strtblk[3] << 8 | (uint32_t)strtblk[4] << 16 | (uint32_t)strtblk[5] << 24;
                delete[] strtblk;
                std::cout << "start block: " << ((uint64_t)starthi >> 32) + startlo << std::endl;
                blocklist->push_back(((uint64_t)starthi >> 32) + startlo); // block #, not bytes
            }
        }
        else // use ext4_extent_idx
        {
        }
    }
    else // USES DIRECT AND INDIRECT BLOCKS
    {
        std::cout << "uses direct and indirect blocks\n";
    }
}
/*
{
    if(incompatflags->contains("Files use Extents,") && inodeflags & 0x80 000) // FS USES EXTENTS AND INODE USES EXTENTS
    {
        //qDebug() << "extententries:" << extententries << "extentdepth:" << extentdepth;
        else // use ext4_extent_idx
        {
	    QList<uint32_t> leafnodes;
	    leafnodes.clear();
	    for(uint16_t i=0; i < extententries; i++)
		leafnodes.append(qFromLittleEndian<uint32_t>(curimg->ReadContent(curoffset + 56 + i*12, 4)));
	    for(int i=0; i < leafnodes.count(); i++)
	    {
		uint16_t lextententries = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector * 512 + leafnodes.at(i) * blocksize + 2, 2));
		uint16_t lextentdepth = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector * 512 + leafnodes.at(i) * blocksize + 6, 2));
		if(extentdepth == 0) // use ext4_extent
		{
		    for(uint16_t j=0; j < lextententries; j++)
		    {
			uint16_t blocklength = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector * 512 + leafnodes.at(i) * blocksize + 16 + j*12, 2));
			uint16_t starthi = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector * 512 + leafnodes.at(i) * blocksize + 18 + j*12, 2));
			uint32_t startlo = qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector * 512 + leafnodes.at(i) * blocksize + 20 + j*12, 4));
			quint64 startblock = (((uint64_t)starthi >> 32) + startlo); // block #, not bytes
			blocklist->append(startblock);
		    }
		}
		else // use ext4_extent_idx
		{
		    qDebug() << "repeat leafnode exercise here...";
		}
		//qDebug() << "leaf header:" << QString::number(qFromLittleEndian<uint16_t>(leafnode.mid(0, 2)), 16);
		//qDebug() << "extent entries:" << qFromLittleEndian<uint16_t>(leafnode.mid(2, 2));
		//qDebug() << "max extent entries:" << qFromLittleEndian<uint16_t>(leafnode.mid(4, 2));
		//qDebug() << "extent depth:" << qFromLittleEndian<uint16_t>(leafnode.mid(6, 2));
	    }
	    //qDebug() << "extent header:" << QString::number(qFromLittleEndian<uint16_t>(curinodebuf.mid(40, 2)), 16);
	    //qDebug() << "extent entries:" << qFromLittleEndian<uint16_t>(curinodebuf.mid(42, 2));
	    //qDebug() << "max extent entries:" << qFromLittleEndian<uint16_t>(curinodebuf.mid(44, 2));
	    //qDebug() << "extent depth:" << qFromLittleEndian<uint16_t>(curinodebuf.mid(46, 2));
	    
	    //qDebug() << "ei_block:" << qFromLittleEndian<uint32_t>(curinodebuf.mid(52, 4));
	    //qDebug() << "ei_leaf_lo:" << qFromLittleEndian<uint32_t>(curinodebuf.mid(56, 4));
	    //qDebug() << "ei_leaf_hi:" << qFromLittleEndian<uint16_t>(curinodebuf.mid(60, 2));
	    //qDebug() << "use extent idx";
        }
    }
    else // direct and indirect blocks
    {
	for(int i=0; i < 12; i++)
	{
	    uint32_t curdirectblock = qFromLittleEndian<uint32_t>(curimg->ReadContent(curoffset + 40 + i*4, 4));
	    if(curdirectblock > 0)
		blocklist->append(curdirectblock);
	}
	//qDebug() << "blocklist before indirects:" << *blocklist;
	uint32_t singleindirect = qFromLittleEndian<uint32_t>(curimg->ReadContent(curoffset + 88, 4));
	uint32_t doubleindirect = qFromLittleEndian<uint32_t>(curimg->ReadContent(curoffset + 92, 4));
	uint32_t tripleindirect = qFromLittleEndian<uint32_t>(curimg->ReadContent(curoffset + 96, 4));
        if(singleindirect > 0)
        {
            for(uint i=0; i < blocksize / 4; i++)
            {
                uint32_t cursingledirect = qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector * 512 + singleindirect * blocksize + i*4, 4));
                if(cursingledirect > 0)
                    blocklist->append(cursingledirect);
            }
        }
        if(doubleindirect > 0)
        {
            QList<uint32_t> sinlist;
            sinlist.clear();
            for(uint i=0; i < blocksize / 4; i++)
            {
                uint32_t sindirect = qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector * 512 + doubleindirect * blocksize + i*4, 4));
                if(sindirect > 0)
                    sinlist.append(sindirect);
            }
            for(int i=0; i < sinlist.count(); i++)
            {
                for(uint j=0; j < blocksize / 4; j++)
                {
                    uint32_t sdirect = qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector * 512 + sinlist.at(i) * blocksize + j*4, 4));
                    if(sdirect > 0)
                        blocklist->append(sdirect);
                }
            }
            sinlist.clear();
        }
        if(tripleindirect > 0)
        {
            QList<uint32_t> dinlist;
            QList<uint32_t> sinlist;
            dinlist.clear();
            sinlist.clear();
            for(uint i=0; i < blocksize / 4; i++)
            {
                uint32_t dindirect = qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector * 512 + tripleindirect * blocksize + i*4, 4));
                if(dindirect > 0)
                    dinlist.append(dindirect);
            }
            for(int i=0; i < dinlist.count(); i++)
            {
                for(uint j=0; j < blocksize / 4; j++)
                {
                    uint32_t sindirect = qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector * 512 + dinlist.at(i) * blocksize + j*4, 4));
                    if(sindirect > 0)
                        sinlist.append(sindirect);
                }
                for(int j=0; j < sinlist.count(); j++)
                {
                    for(uint k=0; k < blocksize / 4; k++)
                    {
                        uint32_t sdirect = qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector * 512 + sinlist.at(j) * blocksize + k*4, 4));
                        if(sdirect > 0)
                            blocklist->append(sdirect);
                    }
                }
            }
        }
    }
}
*/

void ParseExtForensics(std::string filename, std::string mntptstr, std::string devicestr, uint64_t curextinode)
{
    std::ifstream devicebuffer(devicestr.c_str(), std::ios::in|std::ios::binary);
    std::cout << filename << " " << mntptstr << " " << devicestr << std::endl;
    std::vector<std::string> pathvector;
    std::istringstream iss(filename);
    std::string s;
    while(getline(iss, s, '/'))
        pathvector.push_back(s);
    uint32_t blocksize = 0;
    uint32_t bsizepow = 0;
    uint8_t* bsize = new uint8_t[4];
    devicebuffer.seekg(1048);
    devicebuffer.read((char*)bsize, 4);
    bsizepow = (uint32_t)bsize[0] | (uint32_t)bsize[1] << 8 | (uint32_t)bsize[2] << 16 | (uint32_t)bsize[3] << 24;
    delete[] bsize;
    blocksize = 1024 * pow(2, bsizepow);
    //std::cout << "block size: " << bsizepow << std::endl;
    std::cout << "blocksize: " << blocksize << std::endl;
    uint8_t* isize = new uint8_t[2];
    uint16_t inodesize = 0;
    devicebuffer.seekg(1112);
    devicebuffer.read((char*)isize, 2);
    inodesize = (uint16_t)isize[0] | (uint16_t)isize[1] << 8;
    delete[] isize;
    std::cout << "inode size: " << inodesize << std::endl;
    uint32_t blkgrpinodecnt = 0;
    uint8_t* bgicnt = new uint8_t[4];
    devicebuffer.seekg(1064);
    devicebuffer.read((char*)bgicnt, 4);
    blkgrpinodecnt = (uint32_t)bgicnt[0] | (uint32_t)bgicnt[1] << 8 | (uint32_t)bgicnt[2] << 16 | (uint32_t)bgicnt[3] << 24;
    delete[] bgicnt;
    std::cout << "block group inode count: " << blkgrpinodecnt << std::endl;
    uint32_t rootinodetableaddress = 0;
    uint8_t* ritaddr = new uint8_t[4];
    if(blkgrpinodecnt > 2)
        devicebuffer.seekg(2056);
    else
        devicebuffer.seekg(2088);
    devicebuffer.read((char*)ritaddr, 4);
    rootinodetableaddress = (uint32_t)ritaddr[0] | (uint32_t)ritaddr[1] << 8 | (uint32_t)ritaddr[2] << 16 | (uint32_t)ritaddr[3] << 24;
    delete[] ritaddr;
    std::cout << "root inode table address: " << rootinodetableaddress << std::endl;
    uint8_t exttype = 2;
    uint8_t* extflags = new uint8_t[12];
    devicebuffer.seekg(1116);
    devicebuffer.read((char*)extflags, 12);
    uint32_t compatflags = (uint32_t)extflags[0] | (uint32_t)extflags[1] << 8 | (uint32_t)extflags[2] << 16 | (uint32_t)extflags[3] << 24;
    uint32_t incompatflags = (uint32_t)extflags[4] | (uint32_t)extflags[5] << 8 | (uint32_t)extflags[6] << 16 | (uint32_t)extflags[7] << 24;
    uint32_t readonlyflags = (uint32_t)extflags[8] | (uint32_t)extflags[9] << 8 | (uint32_t)extflags[10] << 16 | (uint32_t)extflags[11] << 24;
    delete[] extflags;
    if((compatflags & 0x00000200UL != 0) || (incompatflags & 0x0001f7c0UL != 0) || (readonlyflags & 0x00000378UL != 0))
        exttype = 4;
    else if((compatflags & 0x00000004UL != 0) || (incompatflags & 0x0000000cUL != 0))
        exttype = 3;
    std::cout << "exttype: EXT" << (unsigned int)exttype << std::endl;
    std::string revstr;
    uint8_t* revbig = new uint8_t[4];
    devicebuffer.seekg(1100);
    devicebuffer.read((char*)revbig, 4);
    revstr.append(std::to_string((uint32_t)revbig[0] | (uint32_t)revbig[1] << 8 | (uint32_t)revbig[2] << 16 | (uint32_t)revbig[3] << 24));
    delete[] revbig;
    revstr.append(".");
    uint8_t* revsml = new uint8_t[2];
    devicebuffer.seekg(1086);
    devicebuffer.read((char*)revsml, 2);
    revstr.append(std::to_string((uint16_t)revsml[0] | (uint16_t)revsml[1] << 8));
    delete[] revsml;
    float revision = std::stof(revstr);
    std::cout << "revstr: " << revstr << std::endl;
    std::cout << "rev float: " << revision << std::endl;
    uint16_t grpdescsize = 32;
    if(incompatflags & 0x80)
    {
        uint8_t* grpdesc = new uint8_t[2];
        devicebuffer.seekg(1278);
        devicebuffer.read((char*)grpdesc, 2);
        grpdescsize = (uint16_t)grpdesc[0] | (uint16_t)grpdesc[1] << 8;
        delete[] grpdesc;
    }
    std::cout << "group descriptor size: " << grpdescsize << std::endl;
    uint8_t* fsblk = new uint8_t[4];
    devicebuffer.seekg(1028);
    devicebuffer.read((char*)fsblk, 4);
    uint32_t fsblkcnt = (uint32_t)fsblk[0] | (uint32_t)fsblk[1] << 8 | (uint32_t)fsblk[2] << 16 | (uint32_t)fsblk[3] << 24;
    delete[] fsblk;
    std::cout << "fs block cnt:" << fsblkcnt << std::endl;
    uint8_t* blkgrp = new uint8_t[4];
    devicebuffer.seekg(1056);
    devicebuffer.read((char*)blkgrp, 4);
    uint32_t blkgrpblkcnt = (uint32_t)blkgrp[0] | (uint32_t)blkgrp[1] << 8 | (uint32_t)blkgrp[2] << 16 | (uint32_t)blkgrp[4] << 24;
    delete[] blkgrp;
    std::cout << "block group block count: " << blkgrpblkcnt << std::endl;
    uint32_t blockgroupcount = fsblkcnt / blkgrpblkcnt;
    unsigned int blkgrpcntrem = fsblkcnt % blkgrpblkcnt;
    if(blkgrpcntrem > 0)
        blockgroupcount++;
    if(blockgroupcount == 0)
        blockgroupcount = 1;
    std::cout << "block group count: " << blockgroupcount << std::endl;
    std::vector<uint32_t> inodeaddrtables;
    inodeaddrtables.clear();
    for(unsigned int i=0; i < blockgroupcount; i++)
    {
        uint8_t* iaddr = new uint8_t[4];
        if(blocksize == 1024)
            devicebuffer.seekg(2*blocksize + i * grpdescsize + 8);
        else
            devicebuffer.seekg(blocksize + i * grpdescsize + 8);
        devicebuffer.read((char*)iaddr, 4);
        inodeaddrtables.push_back((uint32_t)iaddr[0] | (uint32_t)iaddr[1] << 8 | (uint32_t)iaddr[2] << 16 | (uint32_t)iaddr[3] << 24);
    }
    uint8_t bgnumber = 0;
    uint32_t inodestartingblock = 0;
    for(int i=1; i <= inodeaddrtables.size(); i++)
    {
        if(curextinode < i*blkgrpinodecnt) // if i generalize function, then 2 would be replaced with curextinode as a passed variable
        {
            inodestartingblock = inodeaddrtables.at(i-1);
            bgnumber = i-1;
            break;
        }
    }
    std::cout << "inode starting block: " << inodestartingblock << std::endl;
    std::cout << "block group number: " << (unsigned int)bgnumber << std::endl;
    uint64_t relcurinode = curextinode - 1  - (bgnumber * blkgrpinodecnt);
    uint64_t curoffset = inodestartingblock * blocksize + inodesize * relcurinode;
    std::cout << "relcurinode: " << relcurinode << std::endl;
    std::cout << "curoffset: " << curoffset << std::endl;
    std::vector<uint32_t> blocklist;
    blocklist.clear();
    GetContentBlocks(&devicebuffer, blocksize, curoffset, &incompatflags, &blocklist);


    devicebuffer.close();
    //for(int i=0; i < pathvector.size(); i++)
    //    std::cout << "path part: " << pathvector.at(i) << "\n";
    // Parse Root Directory to find the file
    // root directory inode is 2

    /*
        uint32_t blkgrp0startblk = qFromLittleEndian<uint32_t>(curimg->ReadContent(curstartsector * 512 + 1044, 4));
        out << "Block Group 0 Start Block|" << QString::number(blkgrp0startblk) << "|Starting block number for block group 0." << Qt::endl;

    if(!parfilename.isEmpty())
	inodecnt = parinode + 1;
    QString dirlayout = "";
        QList<uint32_t> blocklist;
        blocklist.clear();
        GetContentBlocks(curimg, curstartsector, blocksize, curoffset, &incompatflags, &blocklist);
        dirlayout = ConvertBlocksToExtents(blocklist, blocksize);
        blocklist.clear();
     */ 
}

