#include <stdint.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#include <string>
#include <filesystem>
#include <iostream>

#include "blake3/blake3.h"

#include "walafus/filesystem.h"
#include "walafus/wltg_packer.h"
#include "walafus/wltg_reader.h"

void ShowUsage(int outtype)
{
    if(outtype == 0)
    {
        printf("Restore a forensic image IMAGE_NAME.wfi to a device DEVICE_PATH, and optionally validates forensic image.\n\n");
        printf("Usage :\n");
        printf("\twombatrestore IMAGE_NAME DEVICE_PATH [arg]\n\n");
        printf("IMAGE_NAME\t: the file name for the forensic image to restore.\n");
        printf("DEVICE_PATH\t: a device to restore the image such as /dev/sdX\n");
        printf("Arguments :\n");
	printf("-v\t: Perform device verification.\n");
        printf("-V\t: Prints Version information\n");
        printf("-h\t: Prints help information\n\n");
        printf("Example Usage :\n");
        printf("wombatrestore test.wfi /dev/sda -v\n");
    }
    else if(outtype == 1)
    {
        printf("wombatrestore v0.2\n");
	printf("License CC0-1.0: Creative Commons Zero v1.0 Universal\n");
        printf("This software is in the public domain\n");
        printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
        printf("Written by Pasquale Rinaldi\n");
    }
};

int main(int argc, char* argv[])
{
    std::string devicepath;
    std::string imagepath;
    uint8_t verify = 0;

    if(argc == 1 || (argc == 2 && strcmp(argv[1], "-h") == 0))
    {
	ShowUsage(0);
	return 1;
    }
    else if(argc == 2 && strcmp(argv[1], "-V") == 0)
    {
	ShowUsage(1);
	return 1;
    }
    else if(argc >= 3)
    {
        for(int i=3; i < argc; i++)
        {
            if(strcmp(argv[i], "-v") == 0)
                verify = 1;
            else if(strcmp(argv[i], "-V") == 0)
            {
                ShowUsage(1);
                return 1;
            }
            else if(strcmp(argv[i], "-h") == 0)
            {
                ShowUsage(0);
                return 1;
            }
        }
        devicepath = argv[2];
	std::filesystem::path imagepath = std::filesystem::path(argv[1]).string();
	std::string imgname = imagepath.filename().string();
	size_t found = imgname.rfind(".");
	std::string virtpath = "/" + imgname.substr(0, found) + "/" + imgname.substr(0, found) + ".dd";
        if(devicepath.empty())
        {
            ShowUsage(0);
            return 1;
        }
        if(imagepath.empty())
        {
            ShowUsage(0);
            return 1;
        }
        // GET DEVICE TOTAL SIZE
        int64_t totalbytes = 0;
        int infile = open(devicepath.c_str(), O_RDONLY | O_NONBLOCK);
        if(infile >= 0)
        {
            ioctl(infile, BLKGETSIZE64, &totalbytes);
            close(infile);
        }
	// LOAD RAW IMAGE FROM WFI FILE
	Filesystem wltgfilesystem;
	WltgReader pack_wltg(argv[1]);
	wltgfilesystem.add_source(&pack_wltg);
	std::unique_ptr<BaseFileStream> handle = wltgfilesystem.open_file_read(virtpath.c_str());
	if(!handle)
	{
	    std::cout << "failed to open file" << std::endl;
	    return 1;
	}
        // ENSURE THE DEVICE IS LARGE ENOUGH TO HOLD IMAGE
        if(totalbytes < handle->size())
        {
            printf("ERROR: Device is not large enough to hold the image. Exiting.\n");
            return 1;
        }
	FILE* fout = fopen(devicepath.c_str(), "wb");
        if(fout == NULL)
        {
            printf("Error opening device.\n");
            return 1;
        }
	char buf[131072];
	uint64_t curoffset = 0;
	printf("Starting to Write Forensic Image to device %s\n", devicepath.c_str());
	while(curoffset < handle->size())
	{
	    handle->seek(curoffset);
	    uint64_t bytesread = handle->read_into(buf, 131072);
	    curoffset = curoffset + bytesread;
	    fwrite(buf, 1, bytesread, fout);
            printf("Written %llu of %llu bytes\r", curoffset, handle->size());
            fflush(stdout);
	}
	fclose(fout);
        printf("\nForensic Image has been sucessfully restored to %s\n", devicepath.c_str());

	printf("Starting Device %s Verification\n", devicepath.c_str());
        if(verify == 1) // start verification of the device
        {
	    // HASH THE DEVICE | BLAKE3_OUT_LEN IS 32 BYTES LONG
            uint8_t devhash[BLAKE3_OUT_LEN];
	    blake3_hasher devhasher;
	    blake3_hasher_init(&devhasher);
	    int infile = open(devicepath.c_str(), O_RDONLY | O_NONBLOCK);
	    lseek(infile, 0, SEEK_SET);
	    int curpos = 0;
	    while(curpos < totalbytes)
	    {
		char inbuf[131072];
		memset(inbuf, 0, sizeof(inbuf));
		ssize_t bytesread = read(infile, inbuf, 131072);
		curpos = curpos + bytesread;
		blake3_hasher_update(&devhasher, inbuf, bytesread);
		printf("Hashing %llu of %llu bytes\r", curpos, totalbytes);
		fflush(stdout);
	    }
	    close(infile);
	    blake3_hasher_finalize(&devhasher, devhash, BLAKE3_OUT_LEN);
	    std::stringstream devstream;
            for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
		devstream << std::hex << std::setfill('0') << std::setw(2) << (int)devhash[i];
	    std::string virtinfopath = "/" + imgname.substr(0, found) + "/" + imgname.substr(0, found) + ".info";
	    std::unique_ptr<BaseFileStream> infohandle = wltgfilesystem.open_file_read(virtinfopath.c_str());
	    if(!infohandle)
	    {
		std::cout << "failed to open file" << std::endl;
		return 1;
	    }
	    // GET ORIGINAL SOURCE DEVICE BLAKE3 HASH
	    char infobuf[infohandle->size()];
	    uint64_t bytesread = infohandle->read_into(infobuf, infohandle->size());
	    std::string infostring = "";
	    std::stringstream iss;
	    if(bytesread == infohandle->size())
	    {
		for(int i=0; i < infohandle->size(); i++)
		    iss << infobuf[i];
		infostring = iss.str();
	    }
	    std::size_t infofind = infostring.find(" - BLAKE3 Source Device\n");
	    std::cout << std::endl;
	    // COMPARE THE 2 HASHES to VERIFY
	    if(devstream.str().compare(infostring.substr(infofind - 2*BLAKE3_OUT_LEN, 2*BLAKE3_OUT_LEN)) == 0)
		std::cout << "Verification Successful" << std::endl;
	    else
		std::cout << "Verification Failed" << std::endl;
	}
    }
    else
    {
	ShowUsage(0);
	return 1;
    }

    return 0;
}
