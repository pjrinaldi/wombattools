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
	/*
	//printf("Command called: %s %s %s\n", argv[0], argv[1], argv[2]);
        devicepath = argv[2];
        std::string filestr = argv[1];
        std::size_t found = filestr.find_last_of("/");
        std::string pathname = filestr.substr(0, found);
        std::string filename = filestr.substr(found+1);
        std::filesystem::path initpath = std::filesystem::canonical(pathname + "/");
        imagepath = initpath.string() + "/" + filename;
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
	*/

	    /*	    
	    Filesystem wltgfilesystem;
	    WltgReader pack_wltg(argv[1]);

	    wltgfilesystem.add_source(&pack_wltg);

	    std::string wltgimg = std::filesystem::path(argv[1]).filename().string();
	    size_t found = wltgimg.rfind(".");
	    std::string wltgrawimg = wltgimg.substr(0, found) + ".dd";
	    std::string virtpath = "/" + wltgimg.substr(0, found) + "/" + wltgrawimg;
	    
	    std::unique_ptr<BaseFileStream> handle = wltgfilesystem.open_file_read(virtpath.c_str());
	    if(!handle)
	    {
		std::cout << "failed to open file" << std::endl;
		return 1;
	    }

	    FILE* fout = stdout;

	    char buf[131072];
	    uint64_t curoffset = 0;
	    while(curoffset < handle->size())
	    {
		handle->seek(curoffset);
		uint64_t bytesread = handle->read_into(buf, 131072);
		curoffset += bytesread;
		fwrite(buf, 1, bytesread, fout);
	    }
	    fclose(fout);
	    */ 

	/*
        // GET IMAGE TOTAL SIZE TO RESTORE
        FILE* imgfile = NULL;
        imgfile = fopen(imagepath.c_str(), "rb");
        fseek(imgfile, 0, SEEK_END);
        fseek(imgfile, -264, SEEK_CUR);
        fread(&wfimd, sizeof(struct wfi_metadata), 1, imgfile);
        fclose(imgfile);
        //printf("Image Size to Restore: %llu bytes\n", wfimd.totalbytes);

        // GET DEVICE TOTAL SIZE
        int64_t devtotalbytes = 0;
        int infile = open(devicepath.c_str(), O_RDONLY | O_NONBLOCK);
        if(infile >= 0)
        {
            ioctl(infile, BLKGETSIZE64, &devtotalbytes);
            close(infile);
        }
        //printf("Device Size: %llu bytes\n", devtotalbytes);

        // ENSURE THE DEVICE IS LARGE ENOUGH TO HOLD IMAGE
        if(devtotalbytes < wfimd.totalbytes)
        {
            printf("ERROR: Device is not large enough to hold the image. Exiting.\n");
            return 1;
        }

        FILE* fin = NULL;
        FILE* fout = NULL;

        fin = fopen_orDie(imagepath.c_str(), "rb");
        fout = fopen_orDie(devicepath.c_str(), "wb");

        fseek(fin, 0, SEEK_SET);
        fseek(fout, 0, SEEK_SET);
        // DECOMPRESS THE RAW IMAGE AND WRITE IT TO THE DEVICE
        size_t bufinsize = ZSTD_DStreamInSize();
        void* bufin = malloc_orDie(bufinsize);
        size_t bufoutsize = ZSTD_DStreamOutSize();
        void* bufout = malloc_orDie(bufoutsize);

        ZSTD_DCtx* dctx = ZSTD_createDCtx();
        CHECK(dctx != NULL, "ZSTD_createDCtx() failed");

        size_t toread = bufinsize;
        size_t read;
        size_t lastret;
        int isempty = 1;
        size_t readcount = 0;
        while( (read = fread_orDie(bufin, toread, fin)) )
        {
            isempty = 0;
            ZSTD_inBuffer input = { bufin, read, 0 };
            while(input.pos < input.size)
            {
                ZSTD_outBuffer output = { bufout, bufoutsize, 0 };
                size_t ret = ZSTD_decompressStream(dctx, &output, &input);
                CHECK_ZSTD(ret);
		fwrite_orDie(bufout, output.pos, fout);
                lastret = ret;
                readcount = readcount + output.pos;
                printf("Written %llu of %llu bytes\r", readcount, wfimd.totalbytes);
                fflush(stdout);
            }
        }

        if(isempty)
        {
            printf("input is empty\n");
            return 1;
        }
        if(lastret != 0)
        {
            printf("EOF before end of stream: %zu\n", lastret);
            return 1;
        }
        ZSTD_freeDCtx(dctx);
        fclose_orDie(fout);
        fclose_orDie(fin);
        free(bufin);
        free(bufout);

        printf("\nForensic Image has been sucessfully restored\n");

        if(verify == 1) // start verification of the device
        {
	    */

	    /*
	    Filesystem wltgfilesystem;
	    WltgReader pack_wltg(argv[1]);

	    wltgfilesystem.add_source(&pack_wltg);

	    std::string wltgimg = std::filesystem::path(argv[1]).filename().string();
	    size_t found = wltgimg.rfind(".");
	    std::string wltgrawimg = wltgimg.substr(0, found) + ".dd";
	    std::string virtpath = "/" + wltgimg.substr(0, found) + "/" + wltgrawimg;
	    std::string wltginfo = wltgimg.substr(0, found) + ".info";
	    std::string virtinfo = "/" + wltgimg.substr(0, found) + "/" + wltginfo;
	    
	    std::unique_ptr<BaseFileStream> handle = wltgfilesystem.open_file_read(virtpath.c_str());
	    if(!handle)
	    {
		std::cout << "failed to open file" << std::endl;
		return 1;
	    }

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
	    std::cout << infostring.substr(infofind - 2*BLAKE3_OUT_LEN, 2*BLAKE3_OUT_LEN) << " - BLAKE3 Source Device" << std::endl;

	    // CALCULATE THE FORENSIC IMAGE BLAKE3 HASH
	    uint8_t output[BLAKE3_OUT_LEN];
	    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
	    std::stringstream ss;
	    for(int i=0; i < BLAKE3_OUT_LEN; i++)
		ss << std::hex << std::setfill('0') << std::setw(2) << (int)output[i]; 
	    std::cout << ss.str() << " - BLAKE3 Forensic Image" << std::endl << std::endl;
	    
	    // COMPARE THE 2 HASHES to VERIFY
	    if(ss.str().compare(infostring.substr(infofind - 2*BLAKE3_OUT_LEN, 2*BLAKE3_OUT_LEN)) == 0)
		std::cout << "Verification Successful" << std::endl;
	    else
		std::cout << "Verification Failed" << std::endl;
	    */


            /*
	    printf("Verification Started\n");
            // GET BLOCKSIZE FOR SPEEDY VERIFICATION
            int64_t bufinsize = GetBlockSize(&wfimd.totalbytes);
            //printf("selected block size for verification: %llu\n", bufinsize);
            uint8_t devhash[BLAKE3_OUT_LEN];
            blake3_hasher devhasher;
            blake3_hasher_init(&devhasher);
	    fin = fopen_orDie(devicepath.c_str(), "rb");
	    fseek(fin, 0, SEEK_SET);
	    size_t writecount = 0;
	    void* bufin = malloc_orDie(bufinsize);
            while(writecount < wfimd.totalbytes)
            {
	        size_t read = fread_orDie(bufin, bufinsize, fin);
		writecount = writecount + read;
		printf("Verifying %llu of %llu bytes\r", writecount, wfimd.totalbytes);
		fflush(stdout);
		blake3_hasher_update(&devhasher, bufin, read);
            }
            printf("\n");
	    blake3_hasher_finalize(&devhasher, devhash, BLAKE3_OUT_LEN);
	    fclose_orDie(fin);
	    free(bufin);
            for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
            {
                printf("%02x", devhash[i]);
            }
            printf(" - BLAKE3 Restored Device\n");
	    for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
		printf("%02x", wfimd.devhash[i]);
	    printf(" - BLAKE3 Source Image\n");
            printf("\n");
	    if(memcmp(&wfimd.devhash, &devhash, BLAKE3_OUT_LEN) == 0)
		printf("Verification Successful\n\n");
	    else
		printf("Verification Failed\n\n");
        }
	*/
    }
    else
    {
	ShowUsage(0);
	return 1;
    }

    return 0;
}
