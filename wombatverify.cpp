#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "blake3/blake3.h"

#include "walafus/filesystem.h"
#include "walafus/wltg_packer.h"
#include "walafus/wltg_reader.h"

#define DTTMFMT "%F %T %z"
#define DTTMSZ 35

struct wfi_metadata
{
    uint32_t skipframeheader; // skippable frame header
    uint32_t skipframesize; // skippable frame content size (not including header and this size
    uint16_t sectorsize; // raw forensic image sector size
    int64_t reserved; // reserved
    int64_t totalbytes; // raw forensic image total size
    char casenumber[24]; // 24 character string
    char evidencenumber[24]; // 24 character string
    char examiner[24]; // 24 character string
    char description[128]; // 128 character string
    uint8_t devhash[32]; // blake3 source hash
} wfimd;

static char* GetDateTime(char *buff)
{
    time_t t = time(0);
    strftime(buff, DTTMSZ, DTTMFMT, localtime(&t));
    return buff;
};

void ShowUsage(int outtype)
{
    if(outtype == 0)
    {
        printf("Verify the BLAKE3 hash for a wombat forensic or wombat logical image.\n\n");
        printf("Usage :\n");
        printf("\twombatverify IMAGE_FILE\n\n");
        printf("IMAGE_FILE\t: the file name for the wombat forensic or wombat logical image.\n");
        printf("Arguments :\n");
        printf("-v\t: Prints Version information\n");
        printf("-h\t: Prints help information\n\n");
        printf("Example Usage :\n");
        printf("wombatverify item1.wfi\n");
    }
    else if(outtype == 1)
    {
        printf("wombatverify v0.2\n");
	printf("License CC0-1.0: Creative Commons Zero v1.0 Universal\n");
        printf("This software is in the public domain\n");
        printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
        printf("Written by Pasquale Rinaldi\n");
    }
};

int main(int argc, char* argv[])
{
    if(argc == 1 || (argc == 2 && strcmp(argv[1], "-h") == 0))
    {
	ShowUsage(0);
	return 1;
    }
    else if(argc == 2)
    {
	if(strcmp(argv[1], "-v") == 0)
	{
	    ShowUsage(1);
	    return 1;
	}
	else
	{
	    Filesystem wltgfilesystem;
	    WltgReader pack_wltg(argv[1]);
	    //std::cout << "argv1: " << argv[1] << std::endl;

	    wltgfilesystem.add_source(&pack_wltg);

	    std::string wltgimg = std::filesystem::path(argv[1]).filename().string();
	    size_t found = wltgimg.rfind(".");
	    std::string wltgrawimg = wltgimg.substr(0, found) + ".dd";
	    std::string virtpath = "/" + wltgimg.substr(0, found) + "/" + wltgrawimg;
	    std::string wltginfo = wltgimg.substr(0, found) + ".info";
	    std::string virtinfo = "/" + wltgimg.substr(0, found) + "/" + wltginfo;
	    //std::cout << virtpath << std::endl;
	    
	    std::unique_ptr<BaseFileStream> handle = wltgfilesystem.open_file_read(virtpath.c_str());
	    if(!handle)
	    {
		std::cout << "failed to open file" << std::endl;
		return 1;
	    }
	    //std::cout << handle->size() << std::endl;

	    blake3_hasher hasher;
	    blake3_hasher_init(&hasher);

	    char buf[131072];
	    uint64_t curoffset = 0;
	    while(curoffset < handle->size())
	    {
		handle->seek(curoffset);
		uint64_t bytesread = handle->read_into(buf, 131072);
		blake3_hasher_update(&hasher, buf, bytesread);
		curoffset += bytesread;
		printf("Hashed %llu of %llu bytes\r", curoffset, handle->size());
		fflush(stdout);
	    }

	    std::unique_ptr<BaseFileStream> infohandle = wltgfilesystem.open_file_read(virtinfo.c_str());
	    if(!infohandle)
	    {
		std::cout << "failed to open file" << std::endl;
		return 1;
	    }
	    std::cout << std::endl;

	    char infobuf[infohandle->size()];
	    uint64_t bytesread = infohandle->read_into(buf, infohandle->size());
	    std::string infostring = "";
	    if(bytesread == infohandle->size())
	    {
		std::stringstream iss;
		iss << infobuf;
		infostring = iss.str();
	    }
	    std::size_t infofind = infostring.find(" - BLAKE3 Source Device\n");
	    std::cout << infostring.substr(infofind - 32, 32) << " - BLAKE3 Source Device" << std::endl;
	    uint8_t input[BLAKE3_OUT_LEN];
	    for(int i=0; i < BLAKE3_OUT_LEN; i++)
		input[i] = (uint8_t)infostring.substr(infofind - 32, 32).at(i);

	    uint8_t output[BLAKE3_OUT_LEN];
	    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
	    std::stringstream ss;
	    for(int i=0; i < BLAKE3_OUT_LEN; i++)
		ss << std::hex << std::setfill('0') << std::setw(2) << (int)output[i]; 
	    std::string srcmd5 = ss.str();
	    std::cout << std::endl << ss.str() << " - BLAKE3 Forensic Image" << std::endl << std::endl;
	    
	    if(memcmp(&input, output, BLAKE3_OUT_LEN) == 0)
		std::cout << "Verification Successful" << std::endl;
	    else
		std::cout << "Verification Failed" << std::endl;

	    return 0;
	}
    }
}
