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

#include "blake3.h"
#include "zstdcommon.h"
#include <zstd.h>

#define DTTMFMT "%F %T %z"
#define DTTMSZ 35

struct wfi_metadata
{
    uint32_t skipframeheader; // skippable frame header - 4
    uint32_t skipframesize; // skippable frame content size (not including header and this size) - 4
    uint16_t sectorsize; // raw forensic image sector size - 2
    int64_t reserved; // reserved
    int64_t totalbytes; // raw forensic image total size - 8
    char casenumber[24]; // 24 character string - 24
    char evidencenumber[24]; // 24 character string - 24
    char examiner[24]; // 24 character string - 24
    char description[128]; // 128 character string - 128
    uint8_t devhash[32]; // blake3 source hash - 32
} wfimd; // 256

static char* GetDateTime(char *buff)
{
    time_t t = time(0);
    strftime(buff, DTTMSZ, DTTMFMT, localtime(&t));
    return buff;
};

// BLOCKSIZE FOR IMAGE VERIFICATION...
int64_t GetBlockSize(int64_t* totalsize)
{
    int64_t blocksize = 512;
    if(*totalsize % 1073741824 == 0) // 1GB
        blocksize = 1073741824;
    else if(*totalsize % 536870912 == 0) // 512MB
        blocksize = 536870912;
    else if(*totalsize % 268435456 == 0) // 256MB
        blocksize = 268435456;
    else if(*totalsize % 134217728 == 0) // 128MB
        blocksize = 134217728;
    else if(*totalsize % 4194304 == 0) // 4MB
        blocksize = 4194304;
    else if(*totalsize % 1048576 == 0) // 1MB
        blocksize = 1048576;
    else if(*totalsize % 52488 == 0) // 512KB
        blocksize = 52488;
    else if(*totalsize % 131072 == 0) // 128KB
        blocksize = 131072;
    else if(*totalsize % 65536 == 0) //64KB
        blocksize = 65536;
    else if(*totalsize % 32768 == 0) // 32KB
        blocksize = 32768;
    else if(*totalsize % 16384 == 0) //16KB
        blocksize = 16384;
    else if(*totalsize % 8192 == 0) // 8KB
        blocksize = 8192;
    else if(*totalsize % 4096 == 0) // 4KB
        blocksize = 4096;
    else if(*totalsize % 2048 == 0) // 2KB
        blocksize = 2048;
    else if(*totalsize % 1024 == 0) // 1KB
        blocksize = 1024;

    return blocksize;
}

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
        printf("wombatrestore v0.1\n");
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
    }
    else
    {
	ShowUsage(0);
	return 1;
    }

    return 0;
}
