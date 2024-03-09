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

#include "walafus/filesystem.h"
#include "walafus/wltg_packer.h"
#include "walafus/wltg_reader.h"

int main(int argc, char* argv[])
{
    Filesystem wltgfilesystem;
    WltgReader pack_wltg(argv[1]);
    //std::cout << "argv1: " << argv[1] << std::endl;

    wltgfilesystem.add_source(&pack_wltg);

    std::string wltgimg = std::filesystem::path(argv[1]).filename().string();
    size_t found = wltgimg.rfind(".");
    std::string wltgrawimg = wltgimg.substr(0, found) + ".dd";
    std::string virtpath = "/" + wltgimg.substr(0, found) + "/" + wltgrawimg;
    //std::cout << virtpath << std::endl;
    
    std::unique_ptr<BaseFileStream> handle = wltgfilesystem.open_file_read(virtpath.c_str());
    if(!handle)
    {
	std::cout << "failed to open file" << std::endl;
	return 1;
    }
    //std::cout << handle->size() << std::endl;

    FILE* fout = stdout;
    //fout = fopen(stdout, "w+");

    char buf[131072];
    uint64_t curoffset = 0;
    while(curoffset < handle->size())
    {
	handle->seek(curoffset);
	uint64_t bytesread = handle->read_into(buf, 131072);
	//std::cout << "1st 4 bytes read at curoffset: " << std::hex << (uint)buf[0] << (uint)buf[1] << (uint)buf[2] << (uint)buf[3] << std::dec << std::endl;
	curoffset += bytesread;
	fwrite(buf, 1, bytesread, fout);
    }
    fclose(fout);

    return 0;
}
