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
            std::vector<uint32_t> leafnodes;
            leafnodes.clear();
            for(uint16_t i=0; i < extententries; i++)
            {
                uint8_t* lnode = new uint8_t[4];
                devicebuffer->seekg(curoffset + 56 + i*12);
                devicebuffer->read((char*)lnode, 4);
                leafnodes.push_back((uint32_t)lnode[0] | (uint32_t)lnode[1] << 8 | (uint32_t)lnode[2] << 16 | (uint32_t)lnode[3] << 24);
                delete[] lnode;
            }
            for(int i=0; i < leafnodes.size(); i++)
            {
                uint8_t* lentry = new uint8_t[2];
                devicebuffer->seekg(leafnodes.at(i) * blocksize + 2);
                devicebuffer->read((char*)lentry, 2);
                uint16_t lextententries = (uint16_t)lentry[0] | (uint16_t)lentry[1] << 8;
                delete[] lentry;
                uint8_t* ldepth = new uint8_t[2];
                devicebuffer->seekg(leafnodes.at(i) * blocksize + 6);
                devicebuffer->read((char*)ldepth, 2);
                uint16_t lextentdepth = (uint16_t)ldepth[0] | (uint16_t)ldepth[1] << 8;
                delete[] ldepth;
                if(extentdepth == 0) // use ext4_extent
                {
                    for(uint16_t j=0; j < lextententries; j++)
                    {
                        uint8_t* strtblk = new uint8_t[6];
                        devicebuffer->seekg(leafnodes.at(i) * blocksize + 18 + j*12);
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
                    std::cout << "repeat leafnode execise here...";
                }
            }
        }
    }
    else // USES DIRECT AND INDIRECT BLOCKS
    {
        //std::cout << "uses direct and indirect blocks\n";
        for(int i=0; i < 12; i++)
        {
            uint8_t* dirblk = new uint8_t[4];
            devicebuffer->seekg(curoffset + 40 + i*4);
            devicebuffer->read((char*)dirblk, 4);
            uint32_t curdirblk = (uint32_t)dirblk[0] | (uint32_t)dirblk[1] << 8 | (uint32_t)dirblk[2] << 16 | (uint32_t)dirblk[3] << 24;
            delete[] dirblk;
            if(curdirblk > 0)
                blocklist->push_back(curdirblk);
        }
        uint8_t* indirect = new uint8_t[12];
        devicebuffer->seekg(curoffset + 88);
        devicebuffer->read((char*)indirect, 12);
        uint32_t sindirect = (uint32_t)indirect[0] | (uint32_t)indirect[1] << 8 | (uint32_t)indirect[2] << 16 | (uint32_t)indirect[3] << 24;
        uint32_t dindirect = (uint32_t)indirect[4] | (uint32_t)indirect[5] << 8 | (uint32_t)indirect[6] << 16 | (uint32_t)indirect[7] << 24;
        uint32_t tindirect = (uint32_t)indirect[8] | (uint32_t)indirect[9] << 8 | (uint32_t)indirect[10] << 16 | (uint32_t)indirect[11] << 24;
        delete[] indirect;
        if(sindirect > 0)
        {
            for(unsigned int i=0; i < blocksize / 4; i++)
            {
                uint8_t* csd = new uint8_t[4];
                devicebuffer->seekg(sindirect * blocksize + i*4);
                devicebuffer->read((char*)csd, 4);
                uint32_t cursdirect = (uint32_t)csd[0] | (uint32_t)csd[1] << 8 | (uint32_t)csd[2] << 16 | (uint32_t)csd[3] << 24;
                delete[] csd;
                if(cursdirect > 0)
                    blocklist->push_back(cursdirect);
            }
        }
        if(dindirect > 0)
        {
            std::vector<uint32_t> sinlist;
            sinlist.clear();
            for(unsigned int i=0; i < blocksize / 4; i++)
            {
                uint8_t* sind = new uint8_t[4];
                devicebuffer->seekg(dindirect * blocksize + i*4);
                devicebuffer->read((char*)sind, 4);
                uint32_t sindirect = (uint32_t)sind[0] | (uint32_t)sind[1] << 8 | (uint32_t)sind[2] << 16 | (uint32_t)sind[3] << 24;
                delete[] sind;
                if(sindirect > 0)
                    sinlist.push_back(sindirect);
            }
            for(int i=0; i < sinlist.size(); i++)
            {
                for(unsigned int j=0; j < blocksize / 4; j++)
                {
                    uint8_t* sd = new uint8_t[4];
                    devicebuffer->seekg(sinlist.at(i) * blocksize + j*4);
                    devicebuffer->read((char*)sd, 4);
                    uint32_t sdirect = (uint32_t)sd[0] | (uint32_t)sd[1] << 8 | (uint32_t)sd[2] << 16 | (uint32_t)sd[3] << 24;
                    delete[] sd;
                    if(sdirect > 0)
                        blocklist->push_back(sdirect);
                }
            }
            sinlist.clear();
        }
        if(tindirect > 0)
        {
            std::vector<uint32_t> dinlist;
            std::vector<uint32_t> sinlist;
            dinlist.clear();
            sinlist.clear();
            for(unsigned int i=0; i < blocksize / 4; i++)
            {
                uint8_t* did = new uint8_t[4];
                devicebuffer->seekg(tindirect * blocksize + i*4);
                devicebuffer->read((char*)did, 4);
                uint32_t dindirect = (uint32_t)did[0] | (uint32_t)did[1] << 8 | (uint32_t)did[2] << 16 | (uint32_t)did[3] << 24;
                delete[] did;
                if(dindirect > 0)
                    dinlist.push_back(dindirect);
            }
            for(int i=0; i < dinlist.size(); i++)
            {
                for(unsigned int j=0; j < blocksize / 4; j++)
                {
                    uint8_t* sid = new uint8_t[4];
                    devicebuffer->seekg(dinlist.at(i) * blocksize + j*4);
                    devicebuffer->read((char*)sid, 4);
                    uint32_t sindirect = (uint32_t)sid[0] | (uint32_t)sid[1] << 8 | (uint32_t)sid[2] << 16 | (uint32_t)sid[3] << 24;
                    delete[] sid;
                    if(sindirect > 0)
                        sinlist.push_back(sindirect);
                }
                for(int j=0; j < sinlist.size(); j++)
                {
                    for(unsigned int k=0; k < blocksize / 4; k++)
                    {
                        uint8_t* sd = new uint8_t[4];
                        devicebuffer->seekg(sinlist.at(j) * blocksize + k*4);
                        devicebuffer->read((char*)sd, 4);
                        uint32_t sdirect = (uint32_t)sd[0] | (uint32_t)sd[1] << 8 | (uint32_t)sd[2] << 16 | (uint32_t)sd[3] << 24;
                        delete[] sd;
                        if(sdirect > 0)
                            blocklist->push_back(sdirect);
                    }
                }
            }
        }
    }
}

std::string ConvertBlocksToExtents(std::vector<uint32_t>* blocklist, uint32_t blocksize)
{
    std::string extentstr = "";
    int blkcnt = 1;
    unsigned int startvalue = blocklist->at(0);
    for(int i=1; i < blocklist->size(); i++)
    {
        unsigned int oldvalue = blocklist->at(i-1);
        unsigned int newvalue = blocklist->at(i);
        if(newvalue - oldvalue == 1)
            blkcnt++;
        else
        {
            extentstr += std::to_string(startvalue * blocksize) + "," + std::to_string(blkcnt * blocksize) + ";";
            startvalue = blocklist->at(i);
            blkcnt = 1;
        }
        if(i == blocklist->size() - 1)
        {
            extentstr += std::to_string(startvalue * blocksize) + "," + std::to_string(blkcnt * blocksize) + ";";
            startvalue = blocklist->at(i);
            blkcnt = 1;
        }
    }
    if(blocklist->size() == 1)
    {
        extentstr += std::to_string(startvalue * blocksize) + "," + std::to_string(blkcnt * blocksize) + ";";
    }

    return extentstr;
}

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
    std::string dirlayout = ConvertBlocksToExtents(&blocklist, blocksize);
    std::cout << "dir layout: " << dirlayout << std::endl;
    blocklist.clear();


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
     */ 
}

