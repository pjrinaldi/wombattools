#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <map>
#include <vector>
#include <string>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <thread>

#include "blake3/blake3.h"

#include "walafus/filesystem.h"
#include "walafus/wltg_packer.h"
#include "walafus/wltg_reader.h"

int main(int argc, char* argv[])
{
    Filesystem wltgfilesystem;
    WltgReader pack_wltg(argv[1], "");

    wltgfilesystem.add_source(&pack_wltg);

    std::string wltgimg = std::filesystem::path(argv[1]).filename().string();
    size_t found = wltgimg.rfind(".");
    std::string wltgrawimg = wltgimg.substr(0, found) + ".dd";
    std::string virtpath = "/" + wltgrawimg;
    
    std::unique_ptr<BaseFileStream> handle = wltgfilesystem.open_file_read(virtpath.c_str());
    std::cout << virtpath << std::endl;
    if(!handle)
    {
	std::cout << "failed to open file" << std::endl;
	return 1;
    }
    std::cout << handle->size() << std::endl;

    blake3_hasher hasher;
    blake3_hasher_init(&hasher);

    uint64_t curoff = 0;
    while(curoff < handle->size())
    {
	handle->seek(512);
	std::vector<ubyte> data = handle->read(512);
	char* b3buf = (char*)static_cast<unsigned char*>(&data[0]);
	blake3_hasher_update(&hasher, b3buf, 512);
	curoff += 512;
	printf("Read %llu of %llu bytes\r", curoff, handle->size());
	fflush(stdout);
    }
    uint8_t output[BLAKE3_OUT_LEN];
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    std::stringstream ss;
    for(int i=0; i < BLAKE3_OUT_LEN; i++)
        ss << std::hex  << std::setfill('0') << std::setw(2) << (int)output[i]; 
    //printf("%02x", srchash[i]);
    std::string srcmd5 = ss.str();
    std::cout << ss.str() << std::endl;

    return 0;
}
