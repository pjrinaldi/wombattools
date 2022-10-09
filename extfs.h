#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

void GetContentBlocks(std::ifstream* devicebuffer, uint32_t blocksize, uint64_t curoffset, uint32_t incompatflags, std::vector<uint32_t>* blocklist);

void ParseExtForensics(std::string filename, std::string mntptstr, std::string devicestr);
//void ParseExtForensics(std::string filename, std::string mntptstr, std::string devicestr, uint64_t curextinode);
std::string ConvertBlocksToExtents(std::vector<uint32_t>* blocklist, uint32_t blocksize);

// MAYBE PUT INITIAL INFORMATION IN A STRUCT, THAT I POPULATE AND CAN USE FROM RUNNING THE ParseExtInit();
struct extinfo
{
    uint32_t blocksize = 0;
    uint16_t inodesize = 0;
    uint64_t curoffset = 0;
    uint32_t incompatflags = 0;
    uint32_t inodestartingblock = 0;
    uint8_t bgnumber = 0;
    uint32_t blkgrpinodecnt = 0;
    float revision = 0.0;
    std::vector<uint64_t> inodeaddrtables;
    std::string dirlayout = "";
};

void ParseExtInit(std::ifstream* devicebuffer, extinfo* curextinfo);
//void ParseExtInit(std::ifstream* devicebuffer, extinfo* curextinfo, uint64_t curinode);

uint64_t ParseExtPath(std::ifstream* devicebuffer, extinfo* curextinfo, uint64_t nextinode, std::string childpath);

void ParseExtFile(std::ifstream* devicebuffer, uint64_t curextinode);

/*
struct wfi_metadata
{
    uint32_t skipframeheader; // skippable frame header - 4
    uint32_t skipframesize; // skippable frame content size (not including header and this size) - 4
    uint16_t sectorsize; // raw forensic image sector size - 2
    uint16_t version; // version # for forensic image format
    uint16_t reserved1;
    uint32_t reserved2;
    //int64_t reserved; // reserved
    int64_t totalbytes; // raw forensic image total size - 8
    char casenumber[24]; // 24 character string - 24
    char evidencenumber[24]; // 24 character string - 24
    char examiner[24]; // 24 character string - 24
    char description[128]; // 128 character string - 128
    uint8_t devhash[32]; // blake3 source hash - 32
} wfimd; // 256
*/
