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
#include <limits.h>
#include <stdlib.h>

#include <string>
#include <filesystem>
#include <iostream>

#include "blake3.h"
#include "common.h"
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
    std::string devicepath;
    std::string imagepath;
    std::string logpath;
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
                verify=1;
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
	//printf("Command called: %s %s %s\n", argv[0], argv[1], argv[2]);
        devicepath = argv[1];
        std::string filestr = argv[2];
        std::size_t found = filestr.find_last_of("/");
        std::string pathname = filestr.substr(0, found);
        std::string filename = filestr.substr(found+1);
        std::filesystem::path initpath = std::filesystem::canonical(pathname + "/");
        imagepath = initpath.string() + "/" + filename + ".wfi";
        logpath = imagepath + ".log";
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

        int64_t totalbytes = 0;
        int16_t sectorsize = 0;
        int64_t curpos = 0;
        int64_t errcnt = 0;
        int infile = open(devicepath.c_str(), O_RDONLY | O_NONBLOCK);
	FILE* fin = NULL;
	FILE* fout = NULL;
        FILE* filelog = NULL;
        filelog = fopen(logpath.c_str(), "w+");
        if(filelog == NULL)
        {
            printf("Error opening log file.\n");
            return 1;
        }

        if(infile >= 0)
        {
            ioctl(infile, BLKGETSIZE64, &totalbytes);
	    ioctl(infile, BLKSSZGET, &sectorsize);
            close(infile);
	    fin = fopen_orDie(devicepath.c_str(), "rb");
	    fout = fopen_orDie(imagepath.c_str(), "wb");

	    wfimd.skipframeheader = 0x184d2a5f;
            wfimd.skipframesize = 256;
	    wfimd.sectorsize = sectorsize;
            wfimd.reserved  = 0x0;
	    wfimd.totalbytes = totalbytes;

            time_t starttime = time(NULL);
            char dtbuf[35];
            fprintf(filelog, "wombatimager v0.1 zstd Compressed Raw Forensic Image started at: %s\n", GetDateTime(dtbuf));
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
                    if(strcmp(tmp, devicepath.c_str()) == 0)
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
	    
	    time_t endtime = time(NULL);
            fprintf(filelog, "Wrote %llu out of %llu bytes\n", curpos, totalbytes);
            fprintf(filelog, "%llu blocks replaced with zeroes\n", errcnt);
            fprintf(filelog, "Forensic Image: %s\n", imagepath.c_str());
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
                fprintf(filelog, "Verification started at: %s\n", GetDateTime(dtbuf));
                printf("Verification Started\n");
                uint8_t forimghash[BLAKE3_OUT_LEN];
                blake3_hasher imghasher;
                blake3_hasher_init(&imghasher);
                
		fout = fopen_orDie(imagepath.c_str(), "rb");
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
