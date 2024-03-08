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

#include "walafus/wltg_packer.h"

// Copyright 2021-2024 Pasquale J. Rinaldi, Jr.
// Distributed under the terms of CC0-1.0: Creative Commons Zero v1.0 Universal

/*
void ShowUsage(int outtype)
{
    if(outtype == 0)
    {
        printf("Generates a Wombat Hash List (whl) or compare files against a hash list.\n\n");
        printf("Usage :\n");
        printf("\twombathasher [OPTIONS] files\n\n");
        printf("Options :\n");
	printf("-c <new list>\t: Create new hash list.\n");
	printf("-a <existing list>\t: Append to an existing hash list.\n");
	printf("-r\t: Recurse sub-directories.\n");
	printf("-k <file>\t: Compare computed hashes to known list.\n");
	printf("-m\t: Matching mode. Requires -k.\n");
	printf("-n\t: Negative (Inverse) matching mode. Requires -k.\n");
	printf("-w\t: Display which known file was matched, requires -m.\n");
	printf("-l\t: Print relative paths for filenames.\n");
        printf("-v\t: Prints Version information\n");
        printf("-h\t: Prints help information\n\n");
	printf("Arguments:\n");
	printf("files\t: Files to hash for comparison or to add to a hash list.\n");
    }
    else if(outtype == 1)
    {
        printf("wombathasher v0.4\n");
	printf("License CC0-1.0: Creative Commons Zero v1.0 Universal\n");
        printf("This software is in the public domain\n");
        printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
        printf("Written by Pasquale Rinaldi\n");
    }
};
*/

int main(int argc, char* argv[])
{
    WltgPacker packer;
    std::string realpath = std::filesystem::canonical(argv[1]).string();
    std::string virtpath = "/" + std::filesystem::canonical(argv[1]).filename().string();
    std::cout << "virtpath: " << virtpath << std::endl;
    packer.index_real_dir(virtpath.c_str(), realpath.c_str());
    std::string wfistr = std::string(argv[1]) + ".wltg";

    packer.write_fs_blob(wfistr.c_str(), 11, "", true);
    /*
    std::vector<std::filesystem::path> filevector;
    filevector.clear();
    std::filesystem::path imgdir = std::filesystem::canonical(argv[1]);
    for(auto const& dir_entry : std::filesystem::recursive_directory_iterator(imgdir))
    {
	if(std::filesystem::is_regular_file(dir_entry))
	{
	    if(dir_entry.path().string().find(".log") == std::string::npos)
		filevector.push_back(dir_entry);
	}
    }

    // need to keep a running total of the bytes read for each file to update the hashing
    // function as it verifies the contents..
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    for(int i=0; i < filevector.size(); i++)
    {
	std::ifstream fin(filevector.at(i).string().c_str());
	char tmpchar[65536];
	while(fin)
	{
	    fin.read(tmpchar, 65536);
	    size_t cnt = fin.gcount();
	    blake3_hasher_update(&hasher, tmpchar, cnt);
	    if(!cnt)
		break;
	}
    }
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
