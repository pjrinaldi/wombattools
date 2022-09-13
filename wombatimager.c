#include <stdint.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <libudev.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "blake3.h"
#include "common.h"
#include <zstd.h>

#define DTTMFMT "%F %T %z"
#define DTTMSZ 35

struct wfi_metadata
{
    uint32_t skipframeheader; // skippable frame header
    uint32_t skipframesize; // skippable frame content size (not including header and this size
    uint16_t sectorsize; // raw forensic image sector size
    int64_t blocksize; // block size used for uncompressed frames
    int64_t totalbytes; // raw forensic image total size
    char casenumber[24]; // 24 character string
    char evidencenumber[24]; // 24 character string
    char examiner[24]; // 24 character string
    char description[128]; // 128 character string
    uint8_t devhash[32]; // blake3 source hash
} wfimd;

// BLOCKSIZE FOR IMAGE VERIFICATION...
int64_t GetBlockSize(int64_t* totalsize)
{
    int64_t blocksize = 512;
    //if(*totalsize % 1073741824 == 0) // 1GB
    //    blocksize = 1073741824;
    if(*totalsize % 4194304 == 0) // 4MB
        blocksize = 4194304;
    else if(*totalsize % 1048576 == 0) // 1MB
        blocksize = 1048576;
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
        printf("Create forensic image IMAGE_NAME.wfi, log file IMAGE_NAME.log from device DEVICE_PATH, automatically generates blake3 hash, applies zstd compression, and optionally validates forensic image.\n\n");
        printf("Usage :\n");
        printf("\twombatimager DEVICE_PATH IMAGE_NAME [arg]\n\n");
        printf("DEVICE_PATH\t: a device to image such as /dev/sdX\n");
        printf("IMAGE_NAME\t: the file name for the forensic image without an extension.\n");
        printf("Arguments :\n");
        printf("-c\t: Provide a case number (24 character limit)\n");
        printf("-e\t: Provide an examiner name (24 character limit)\n");
        printf("-n\t: Provide an evidence number (24 character limit)\n");
        printf("-d\t: Provide a description (128 character limit)\n");
	printf("-v\t: Perform image verification.\n");
        printf("-V\t: Prints Version information\n");
        printf("-h\t: Prints help information\n\n");
        printf("Example Usage :\n");
        printf("wombatimager /dev/sda item1 -v -c \"Case 1\" -e \"My Name\" -n \"Item 1\" -d \"Forensic Image of a 32MB SD Card\"\n");
    }
    else if(outtype == 1)
    {
        printf("wombatimager v0.1\n");
	printf("License CC0-1.0: Creative Commons Zero v1.0 Universal\n");
        printf("This software is in the public domain\n");
        printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
        printf("Written by Pasquale Rinaldi\n");
    }
};

int main(int argc, char* argv[])
{
    char* inputstr = NULL;
    char* outputstr = NULL;
    char* imgfilestr = NULL;
    char* logfilestr = NULL;
    char* extstr = NULL;
    uint8_t verify = 0;

    printf("wfi_metadata struct size is %d\n", sizeof(struct wfi_metadata));

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
            printf("Command option %d, %s\n", i, argv[i]);
            if(strcmp(argv[i], "-v") == 0)
            {
                verify=1;
                printf("verification is set\n");
            }
            else if(strcmp(argv[i], "-c") == 0)
                strcpy(wfimd.casenumber, argv[i+1]);
            else if(strcmp(argv[i], "-e") == 0)
                strcpy(wfimd.examiner, argv[i+1]);
            else if(strcmp(argv[i], "-n") == 0)
                strcpy(wfimd.evidencenumber, argv[i+1]);
            else if(strcmp(argv[i], "-d") == 0)
                strcpy(wfimd.description, argv[i+1]);
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
	printf("Command called: %s %s %s\n", argv[0], argv[1], argv[2]);
        printf("wfimd.examiner: %s\n", wfimd.examiner);
        inputstr = argv[1];
	if(inputstr == NULL)
	{
	    ShowUsage(0);
	    return 1;
	}
        outputstr = argv[2];
	if(outputstr == NULL)
	{
	    ShowUsage(0);
	    return 1;
	}

        extstr = strstr(argv[2], ".wfi");
        if(extstr == NULL)
            imgfilestr = strcat(outputstr, ".wfi");
        else
            imgfilestr = outputstr;

        char* midname = strndup(outputstr, strlen(outputstr));
        logfilestr = strcat(midname, ".log");

        //printf("img str: %s\tlog str: %s\n", imgfilestr, logfilestr);

        int64_t totalbytes = 0;
        int16_t sectorsize = 0;
        int64_t curpos = 0;
        int64_t errcnt = 0;
        int infile = open(inputstr, O_RDONLY | O_NONBLOCK);
	FILE* fin = NULL;
	FILE* fout = NULL;
        FILE* filelog = NULL;
        filelog = fopen(logfilestr, "w+");
        if(filelog == NULL)
        {
            printf("Error opening log file.\n");
            return 1;
        }
        free(midname);

        if(infile >= 0)
        {
            ioctl(infile, BLKGETSIZE64, &totalbytes);
	    ioctl(infile, BLKSSZGET, &sectorsize);
            close(infile);
	    fin = fopen_orDie(inputstr, "rb");
	    fout = fopen_orDie(imgfilestr, "wb");
	    printf("Sector Size: %u Total Bytes: %u\n", sectorsize, totalbytes);
            int64_t blocksize = GetBlockSize(&totalbytes);
            printf("probed lz4 block size: %ld\n", blocksize);
            //uint64_t framecount = totalbytes / blocksize;
            //printf("frame count: %ld\n", framecount);
            //uint64_t* frameindex = NULL;
            //frameindex = (uint64_t*)malloc(sizeof(uint64_t)*framecount);

	    wfimd.skipframeheader = 0x184d2a5f;
            wfimd.skipframesize = 256;
	    wfimd.sectorsize = sectorsize;
	    wfimd.totalbytes = totalbytes;
	    wfimd.blocksize = blocksize;

            time_t starttime = time(NULL);
            char dtbuf[35];
            fprintf(filelog, "wombatimager v0.1 LZ4 Compressed Raw Forensic Image started at: %s\n", GetDateTime(dtbuf));
            fprintf(filelog, "\nSource Device\n");
            fprintf(filelog, "-------------\n");

            // GET UDEV DEVICE PROPERTIES FOR LOG...
            // !!!!! THIS ONLY GET PARENT DEVICES, DOESN'T WORK FOR PARTITIONS OF PARENT DEVICES...
            struct udev* udev;
            struct udev_device* dev;
            struct udev_enumerate* enumerate;
            struct udev_list_entry* devices;
            struct udev_list_entry* devlistentry;
            udev = udev_new();
            enumerate = udev_enumerate_new(udev);
            udev_enumerate_add_match_subsystem(enumerate, "block");
            udev_enumerate_scan_devices(enumerate);
            devices = udev_enumerate_get_list_entry(enumerate);
            udev_list_entry_foreach(devlistentry, devices)
            {
                const char* path;
                const char* tmp;
                path = udev_list_entry_get_name(devlistentry);
                dev = udev_device_new_from_syspath(udev, path);
                if(strncmp(udev_device_get_devtype(dev), "partition", 9) != 0 && strncmp(udev_device_get_sysname(dev), "loop", 4) != 0)
                {
                    tmp = udev_device_get_devnode(dev);
                    if(strcmp(tmp, inputstr) == 0)
                    {
                        fprintf(filelog, "Device:");
                        const char* devvendor = udev_device_get_property_value(dev, "ID_VENDOR");
                        if(devvendor != NULL)
                            fprintf(filelog, " %s", devvendor);
                        const char* devmodel = udev_device_get_property_value(dev, "ID_MODEL");
                        if(devmodel != NULL)
                            fprintf(filelog, " %s", devmodel);
                        const char* devname = udev_device_get_property_value(dev, "ID_NAME");
                        if(devname != NULL)
                            fprintf(filelog, " %s", devname);
                        fprintf(filelog, "\n");
                        tmp = udev_device_get_devnode(dev);
                        fprintf(filelog, "Device Path: %s\n", tmp);
                        tmp = udev_device_get_property_value(dev, "ID_SERIAL_SHORT");
                        fprintf(filelog, "Serial Number: %s\n", tmp);
                        fprintf(filelog, "Size: %u bytes\n", totalbytes);
                        fprintf(filelog, "Sector Size: %u bytes\n", sectorsize);
                    }
                }
                udev_device_unref(dev);
            }
            udev_enumerate_unref(enumerate);
            udev_unref(udev);
            
	    fseek(fin, 0, SEEK_SET);
	    fseek(fout, 0, SEEK_SET);
            //lseek(infile, 0, SEEK_SET);
            //lseek(outfile, 0, SEEK_SET);

            // BLAKE3_OUT_LEN IS 32 BYTES LONG
            uint8_t srchash[BLAKE3_OUT_LEN];
            uint8_t wfihash[BLAKE3_OUT_LEN];

	    blake3_hasher srchasher;
	    blake3_hasher_init(&srchasher);

	    // USE ZSTD STREAM COMPRESSION
	    size_t bufinsize = ZSTD_CStreamInSize();
	    void* bufin = malloc_orDie(bufinsize);
	    size_t bufoutsize = ZSTD_CStreamOutSize();
	    void* bufout = malloc_orDie(bufoutsize);
	    size_t writecount = 0;

	    ZSTD_CCtx* cctx = ZSTD_createCCtx();
	    CHECK(cctx != NULL, "ZSTD_createCCtx() failed");

	    //CHECK_ZSTD(ZSTD_CCtx_setParameter(
	    //CHECK_ZSTD(ZSTD_CCtx_setParamter
	    size_t toread = bufinsize;
	    for(;;)
	    {
		size_t read = fread_orDie(bufin, toread, fin);
		writecount = writecount + read;
		printf("Writing %llu of %llu bytes\r", writecount, totalbytes);
		fflush(stdout);

		blake3_hasher_update(&srchasher, bufin, read);

		int lastchunk = (read < toread);
		ZSTD_EndDirective mode = lastchunk ? ZSTD_e_end : ZSTD_e_continue;
		ZSTD_inBuffer input = { bufin, read, 0 };
		int finished;
		do
		{
		    ZSTD_outBuffer output = { bufout, bufoutsize, 0 };
		    size_t remaining = ZSTD_compressStream2(cctx, &output, &input, mode);
		    CHECK_ZSTD(remaining);
		    fwrite_orDie(bufout, output.pos, fout);
		    finished = lastchunk ? (remaining == 0) : (input.pos == input.size);
		} while (!finished);
		CHECK(input.pos == input.size, "Impossible, zstd only returns 0 when the input is completely consumed");

		if(lastchunk)
		    break;
	    }
	    ZSTD_freeCCtx(cctx);

	    blake3_hasher_finalize(&srchasher, srchash, BLAKE3_OUT_LEN);
            memcpy(wfimd.devhash, srchash, BLAKE3_OUT_LEN);
	    // NEED TO WRITE SKIPPABLE FRAME CONTENT HERE
	    fwrite_orDie(&wfimd, sizeof(struct wfi_metadata), fout);
            //write(outfile, &wfimd, sizeof(struct wfi_metadata));
	    
	    time_t endtime = time(NULL);
            fprintf(filelog, "Wrote %llu out of %llu bytes\n", curpos, totalbytes);
            fprintf(filelog, "%llu blocks replaced with zeroes\n", errcnt);
            fprintf(filelog, "Forensic Image: %s\n", outputstr+3);
            fprintf(filelog, "Forensic Image finished at: %s\n", GetDateTime(dtbuf));
            fprintf(filelog, "Forensic Image created in: %f seconds\n\n", difftime(endtime, starttime));
            printf("\nForensic Image Creation Finished\n");
            for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
            {
                fprintf(filelog, "%02x", srchash[i]);
                printf("%02x", srchash[i]);
            }
            printf(" - BLAKE3 Source Device\n");
            fprintf(filelog, " - BLAKE3 Source Device\n");

	    fclose_orDie(fout);
	    fclose_orDie(fin);
	    free(bufin);
	    free(bufout);

	    if(verify == 1) // start verification
	    {
		//printf("Start Verification by decompressing...");
                fprintf(filelog, "Verification started at: %s\n", GetDateTime(dtbuf));
                //fprintf(filelog, "wombatimager v0.1 LZ4 Compressed Raw Forensic Image started at: %s\n", GetDateTime(dtbuf));
                printf("Verification Started\n");
                uint8_t forimghash[BLAKE3_OUT_LEN];
                blake3_hasher imghasher;
                blake3_hasher_init(&imghasher);
                
		fout = fopen_orDie(imgfilestr, "rb");
		size_t bufinsize = ZSTD_DStreamInSize();
		void* bufin = malloc_orDie(bufinsize);
		size_t bufoutsize = ZSTD_DStreamOutSize();
		void* bufout = malloc_orDie(bufoutsize);

		ZSTD_DCtx* dctx = ZSTD_createDCtx();
		CHECK(dctx != NULL, "ZSTD_createDCtx() failed");

		size_t toread = bufinsize;
		size_t read;
		size_t lastret = 0;
		int isempty = 1;
		size_t readcount = 0;
		while( (read = fread_orDie(bufin, toread, fout)) )
		{
		    isempty = 0;
		    ZSTD_inBuffer input = { bufin, read, 0 };
		    while(input.pos < input.size)
		    {
			ZSTD_outBuffer output = { bufout, bufoutsize, 0 };
			size_t ret = ZSTD_decompressStream(dctx, &output, &input);
			CHECK_ZSTD(ret);
			blake3_hasher_update(&imghasher, bufout, output.pos);
			lastret = ret;
			readcount = readcount + output.pos;
			printf("Read %llu of %llu bytes\r", readcount, totalbytes);
			fflush(stdout);
		    }
		}
		printf("\nVerification Finished\n");

		if(isempty)
		{
		    printf("input is empty\n");
		    return 1;
		}

		if(lastret != 0)
		{
		    printf("EOF before end of stream: %zu\n", lastret);
		    exit(1);
		}
		ZSTD_freeDCtx(dctx);
		fclose_orDie(fout);
		free(bufin);
		free(bufout);

                blake3_hasher_finalize(&imghasher, forimghash, BLAKE3_OUT_LEN);

		for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
		{
		    fprintf(filelog, "%02x", srchash[i]);
		    printf("%02x", srchash[i]);
		}
		printf(" - BLAKE3 Source Device\n");
		fprintf(filelog, " - BLAKE3 Source Device\n");

                for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
                {
		    fprintf(filelog, "%02x", forimghash[i]);
                    printf("%02x", forimghash[i]);
                }
                printf(" - Forensic Image Hash\n");
                fprintf(filelog, " - Forensic Image Hash\n");
		printf("\n");
		if(memcmp(&srchash, &forimghash, BLAKE3_OUT_LEN) == 0)
		{
		    printf("Verification Successful\n");
		    fprintf(filelog, "Verification Successful\n");
		}
		else
		{
		    printf("Verification Failed\n");
		    fprintf(filelog, "Verification Failed\n");
		}
		printf("\n");
	    }
	}
	else
	{
	    printf("error opening device: %d %s\n", infile, errno);
	    return 1;
	}
	fclose(filelog);
    }
    else
    {
	ShowUsage(0);
	return 1;
    }

    return 0;
}
