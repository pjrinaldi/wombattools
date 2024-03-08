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
    std::string virtpath = "/" + wltgimg.substr(0, found) + "/" + wltgrawimg;
    
    //std::unique_ptr<BaseFileStream> handle = wltgfilesystem.open_file_read(virtpath.c_str());
    std::cout << virtpath << std::endl;
    /*
    if(!handle)
    {
	std::cout << "failed to open file" << std::endl;
	return 1;
    }
    std::cout << handle->size() << std::endl;
    */

    //blake3_hasher hasher;
    //blake3_hasher_init(&hasher);

    std::vector<ubyte> data = wltgfilesystem.read_file(virtpath.c_str());
    std::cout << "bytes read: " << data.size() << std::endl;
    std::string virtlog = "/" + wltgimg.substr(0, found) + "/" + wltgimg.substr(0, found) + ".log";
    std::vector<ubyte> data2 = wltgfilesystem.read_file(virtlog.c_str());
    std::cout << "bytes read: " << data2.size() << std::endl;
    std::cout << "data content:" << std::endl;
    for(int i=0; i < data2.size(); i++)
	std::cout << data2[i];
    std::cout << std::endl;
    //std::vector<ubyte> Filesystem::read_file(std::string_view filename) {
    //std::vector<ubyte> data = handle->read();
    //std::cout << "data read size: " << data.size() << std::endl;
    //char* b3buf = new char[data.size() + 1];
    //b3buf = (char*)static_cast<unsigned char*>(&data[0]);
    //blake3_hasher_update(&hasher, b3buf, data.size());
    /*
    uint64_t curoff = 0;
    while(curoff < handle->size()+1)
    {
//2147483648 1073741824 
//1016332288
	handle->seek(curoff);
	std::vector<ubyte> data = handle->read(1016332288);
	char* b3buf = (char*)static_cast<unsigned char*>(&data[0]);
	blake3_hasher_update(&hasher, b3buf, 1016332288);
	curoff += 1016332288;
	printf("Read %llu of %llu bytes\r", curoff, handle->size());
	fflush(stdout);
    }
    */

    /*
    uint8_t output[BLAKE3_OUT_LEN];
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    std::stringstream ss;
    for(int i=0; i < BLAKE3_OUT_LEN; i++)
        ss << std::hex  << std::setfill('0') << std::setw(2) << (int)output[i]; 
    //printf("%02x", srchash[i]);
    std::string srcmd5 = ss.str();
    std::cout << ss.str() << std::endl;
    */

    return 0;
}
