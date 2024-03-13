#include <stdint.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
//#include <sys/stat.h>
#include <sys/sysinfo.h>
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
#include <fstream>
#include <thread>

#include "blake3/blake3.h"

#include "walafus/filesystem.h"
#include "walafus/wltg_packer.h"
#include "walafus/wltg_reader.h"

#define DTTMFMT "%F %T %z"
#define DTTMSZ 35

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
        printf("Create forensic image IMAGE_NAME.wfi, info file IMAGE_NAME.info, log file IMAGE_NAME.log from device DEVICE_PATH, automatically generates blake3 hash, packs into a read-only zstd-compressed walafus fs, and optionally validates the forensic image.\n\n");
        printf("Usage :\n");
        printf("\twombatimager DEVICE_PATH IMAGE_NAME [arg]\n\n");
        printf("DEVICE_PATH\t: a device to image such as /dev/sdX\n");
        printf("IMAGE_NAME\t: the file name for the forensic image without an extension.\n");
        printf("Arguments :\n");
        printf("-c\t: Provide a case number\n");
        printf("-e\t: Provide an examiner name\n");
        printf("-n\t: Provide an evidence number\n");
        printf("-d\t: Provide a description\n");
	printf("-v\t: Perform image verification.\n");
        printf("-V\t: Prints Version information\n");
        printf("-h\t: Prints help information\n\n");
        printf("Example Usage :\n");
        printf("wombatimager /dev/sda item1 -v -c \"Case 1\" -e \"My Name\" -n \"Item 1\" -d \"Forensic Image of a 32MB SD Card\"\n");
    }
    else if(outtype == 1)
    {
        printf("wombatimager v0.2\n");
	printf("License CC0-1.0: Creative Commons Zero v1.0 Universal\n");
        printf("This software is in the public domain\n");
        printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
        printf("Written by Pasquale Rinaldi\n");
    }
};

void WritePiece(uint64_t offset, uint64_t size, std::string devpath, std::string imgpath)
{
    int infile = open(devpath.c_str(), O_RDONLY | O_NONBLOCK);
    int outfile = open(imgpath.c_str(), O_RDWR, S_IRWXU);
    lseek(infile, offset, SEEK_SET);
    lseek(outfile, 0, SEEK_SET);
    lseek(outfile, offset, SEEK_SET);
    char inbuf[size];
    memset(inbuf, 0, sizeof(inbuf));
    ssize_t bytesread = read(infile, inbuf, size);
    close(infile);
    if(bytesread == -1)
    {
	memset(inbuf, 0, sizeof(inbuf));
    }
    ssize_t byteswrite = write(outfile, inbuf, size);
    close(outfile);
    printf("Wrote %llu bytes\r", offset + size);
    fflush(stdout);
}

int main(int argc, char* argv[])
{
    // START DEBUGGING
    //argc = 3;
    //argv[1] = (char*)std::string("/dev/mmcbl0").c_str();
    //argv[2] = (char*)std::string("WFI").c_str();
    // END DEBUGGING
    std::string devicepath;
    std::string imagepath;
    std::string logpath;
    std::string infopath;
    std::string wfipath;
    std::string infocontent = "";
    int64_t totalbytes = 0;
    int16_t sectorsize = 0;
    uint8_t verify = 0;

    int infile = 0;
    FILE* fileinfo = NULL;

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
	devicepath = argv[1];
        infile = open(devicepath.c_str(), O_RDONLY | O_NONBLOCK);
        ioctl(infile, BLKGETSIZE64, &totalbytes);
	ioctl(infile, BLKSSZGET, &sectorsize);
        close(infile);
	infocontent = "wombatimager v0.2]\n\n";
	infocontent += "Wombat Forensic Image v0.2 - Raw Forensic Image with Log and Info File Stored in a Read-Only ZSTD Compressed Walafus File System.\n\n";
	infocontent += "Raw Media Size:  " + std::to_string(totalbytes) + " bytes\n";
        for(int i=3; i < argc; i++)
        {
            if(strcmp(argv[i], "-v") == 0)
                verify=1;
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
            else if(strcmp(argv[i], "-c") == 0)
                infocontent += "Case Number: \t " + std::string(argv[i+1]) + "\n";
            else if(strcmp(argv[i], "-e") == 0)
                infocontent += "Examiner: \t " + std::string(argv[i+1]) + "\n";
            else if(strcmp(argv[i], "-n") == 0)
                infocontent += "Evidence Number: " + std::string(argv[i+1]) + "\n";
            else if(strcmp(argv[i], "-d") == 0)
                infocontent += "Description:\t " + std::string(argv[i+1]) + "\n\n";
        }
	//printf("Command called: %s %s %s\n", argv[0], argv[1], argv[2]);
        std::string filestr = argv[2];
        std::size_t found = filestr.find_last_of("/");
        std::string pathname = filestr.substr(0, found);
        std::string filename = filestr.substr(found+1);
	if((int)found == -1)
	    pathname = ".";
        std::filesystem::path initpath = std::filesystem::canonical(pathname + "/");
	std::string imgdirpath = initpath.string() + "/" + filename;
	std::filesystem::create_directory(imgdirpath);
	wfipath = initpath.string() + "/" + filename + ".wfi";
        imagepath = imgdirpath + "/" + filename + ".dd";
        logpath = imgdirpath + "/" + filename + ".log";
	infopath = imgdirpath + "/" + filename + ".info";
	//std::cout << "device path: " << devicepath << std::endl;
	//std::cout << "image path: " << imagepath << std::endl;
	//std::cout << "log path: " << logpath << std::endl;
	//std::cout << "info path: " << infopath << std::endl;
	//std::cout << "wfi path: " << wfipath << std::endl;
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

        int64_t curpos = 0;
        int64_t errcnt = 0;
        infile = open(devicepath.c_str(), O_RDONLY | O_NONBLOCK);
	//FILE* fin = NULL;
	//FILE* fout = NULL;
	FILE* fmd = NULL;
        FILE* filelog = NULL;
        filelog = fopen(logpath.c_str(), "w+");
        if(filelog == NULL)
        {
            printf("Error opening log file.\n");
            return 1;
        }
        if(infile >= 0)
        {
	    close(infile);

	    // CREATE THE SPARSE IMAGE FILE
	    int file = 0;
	    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
	    file = open(imagepath.c_str(), O_WRONLY | O_CREAT, mode);
	    if(file == -1)
	    {
		printf("Error opening image file.\n");
		return -1;
	    }
	    ftruncate(file, totalbytes);
	    close(file);
	    
	    // GET THE CPU THREAD COUNT FOR CONCURRENCY PURPOSES
	    unsigned int threadcount = std::thread::hardware_concurrency();
	    threadcount = threadcount - 2;
	    //std::cout << "thread count: " << threadcount << std::endl;

	    // GET THE TOTAL SYSTEM RAM
	    struct sysinfo info;
	    int syserr = sysinfo(&info);
	    uint64_t totalram = info.totalram;
	    //std::cout << "total ram: " << totalram << std::endl;

	    //std::cout << "device bytes: " << totalbytes << std::endl;

	    // MAX PIECE SIZE
	    uint64_t maxpiecesize = totalram / threadcount;
	    //std::cout << "max piece size: " << maxpiecesize << std::endl;

	    // IF DEVICE SIZE < MAX PIECE SIZE, MAKE MAX PIECE SIZE = DEVICE SIZE
	    if(totalbytes < maxpiecesize)
		maxpiecesize = totalbytes;
	    
	    // CALCULATE THE CORRECT PIECE SIZE
	    std::vector<uint64_t> piecesizechoice = { 8589934592, 4294967296, 2147483648, 1073741824, 536870912, 268435456, 134217728, 67108864, 33554432, 16777216, 8388608, 4194304, 1048576, 524288, 131072, 65536, 32768, 16384, 8192, 4096, 2048, 1024, 512 };

	    uint64_t piecesize = 0;
	    for(int i=0; i < piecesizechoice.size(); i++)
	    {
		//std::cout << i << ": " << piecesizechoice.at(i) << " " << totalbytes / piecesizechoice.at(i) << " " << totalbytes % piecesizechoice.at(i) << std::endl;
		if(maxpiecesize > piecesizechoice.at(i) && totalbytes % piecesizechoice.at(i) == 0)
		{
		    piecesize = piecesizechoice.at(i);
		    break;
		}
	    }
	    //std::cout << "piece size: " << piecesize << std::endl;

	    uint64_t piececount = totalbytes / piecesize;
	    //std::cout << "piece count: " << piececount << std::endl;

	    // GET HASH PIECE SIZE
	    uint64_t hashsize = piecesize;
	    for(int i=0; i < piecesizechoice.size(); i++)
	    {
		if(totalbytes % piecesizechoice.at(i) == 0)
		{
		    hashsize = piecesizechoice.at(i);
		    break;
		}
	    }
	    //std::cout << "hash size: " << hashsize << std::endl;

            time_t starttime = time(NULL);
            char dtbuf[35];
            fprintf(filelog, "wombatimager v0.2 Raw Forensic Image started at: %s\n", GetDateTime(dtbuf));
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
	    
	    // MULTI THREAD THE raw image creation process
	    for(int i=0; i < piececount; i++)
	    {
		std::thread tmp(WritePiece, i*piecesize, piecesize, devicepath, imagepath);
		tmp.join();
	    }
	    time_t endtime = time(NULL);
            fprintf(filelog, "Wrote %llu bytes\n", totalbytes);
            //fprintf(filelog, "%llu blocks replaced with zeroes\n", errcnt);
            fprintf(filelog, "Forensic Image: %s\n", imagepath.c_str());
            fprintf(filelog, "Forensic Image finished at: %s\n", GetDateTime(dtbuf));
            fprintf(filelog, "Forensic Image created in: %5.2f seconds\n\n", difftime(endtime, starttime));
            printf("\nForensic Image Creation Finished\n");

	    time_t devhashstart = time(NULL);
	    // HASH THE DEVICE | BLAKE3_OUT_LEN IS 32 BYTES LONG
            uint8_t devhash[BLAKE3_OUT_LEN];
	    blake3_hasher devhasher;
	    blake3_hasher_init(&devhasher);
	    int infile = open(devicepath.c_str(), O_RDONLY | O_NONBLOCK);
	    lseek(infile, 0, SEEK_SET);
	    int curpos = 0;
	    while(curpos < totalbytes)
	    {
		char inbuf[hashsize];
		memset(inbuf, 0, sizeof(inbuf));
		ssize_t bytesread = read(infile, inbuf, hashsize);
		curpos = curpos + bytesread;
		blake3_hasher_update(&devhasher, inbuf, bytesread);
		printf("Hashing %llu of %llu bytes\r", curpos, totalbytes);
		fflush(stdout);
	    }
	    close(infile);
	    blake3_hasher_finalize(&devhasher, devhash, BLAKE3_OUT_LEN);

	    // OPEN FILEINFO AND START POPULATING THE INFORMATION
	    fileinfo = fopen(infopath.c_str(), "w+");
	    if(fileinfo == NULL)
	    {
		printf("Error opening info file.\n");
		return 1;
	    }

	    fprintf(fileinfo, "%s", infocontent.c_str());
	    //std::cout << infocontent << std::endl;

            for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
            {
                fprintf(filelog, "%02x", devhash[i]);
                printf("%02x", devhash[i]);
		fprintf(fileinfo, "%02x", devhash[i]);
            }
            printf(" - BLAKE3 Source Device\n");
            fprintf(filelog, " - BLAKE3 Source Device\n");
            fprintf(fileinfo, " - BLAKE3 Source Device\n");
	    time_t devhashend = time(NULL);
            fprintf(filelog, "Hashed %llu bytes\n", totalbytes);
            fprintf(filelog, "Hash of Source Device finished at: %s\n", GetDateTime(dtbuf));
            fprintf(filelog, "Hash of Source Device created in: %5.2f seconds\n\n", difftime(devhashend, devhashstart));
            printf("\nSource Device Hashing Finished\n");

	    // START VERIFYING THE FORENSIC IMAGE IF ENABLED
	    if(verify == 1) // start verification
	    {
		time_t imghashstart = time(NULL);
                fprintf(filelog, "Verification started at: %s\n", GetDateTime(dtbuf));
                printf("Verification Started\n");
                uint8_t forimghash[BLAKE3_OUT_LEN];
                blake3_hasher imghasher;
                blake3_hasher_init(&imghasher); 
		int outfile = open(imagepath.c_str(), O_RDONLY, O_NONBLOCK);
		lseek(outfile, 0, SEEK_SET);
		int curpos = 0;
		while(curpos < totalbytes)
		{
		    char inbuf[hashsize];
		    memset(inbuf, 0, sizeof(inbuf));
		    ssize_t bytesread = read(outfile, inbuf, hashsize);
		    curpos = curpos + bytesread;
		    blake3_hasher_update(&imghasher, inbuf, bytesread);
		    printf("Hashing %llu of %llu bytes\r", curpos, totalbytes);
		    fflush(stdout);
		}
		close(outfile);
		blake3_hasher_finalize(&imghasher, forimghash, BLAKE3_OUT_LEN);
                for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
                {
		    fprintf(filelog, "%02x", forimghash[i]);
                    printf("%02x", forimghash[i]);
		    fprintf(fileinfo, "%02x", forimghash[i]);
                }
                printf(" - BLAKE3 Forensic Image\n");
                fprintf(filelog, " - BLAKE3 Forensic Image\n");
                fprintf(fileinfo, " - BLAKE3 Forensic Image\n");
		time_t imghashend = time(NULL);
		fprintf(filelog, "Hashed %llu bytes\n", totalbytes);
		fprintf(filelog, "Hash of Forensic Image finished at: %s\n", GetDateTime(dtbuf));
		fprintf(filelog, "Hash of Forensic Image created in: %5.2f seconds\n\n", difftime(imghashend, imghashstart));
		printf("\nForensic Image Hashing Finished\n");
		printf("\n");
		if(memcmp(&devhash, &forimghash, BLAKE3_OUT_LEN) == 0)
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
	fprintf(filelog, "Packing files into the Wombat Forensic Image and cleaning up.\n");
	printf("Packing files into the Wombat Forensic Image and cleaning up.\n");
	fclose(filelog);
	fclose(fileinfo);

	// CREATE WALAFUS FILE HERE
	WltgPacker packer;
	std::string virtpath = "/" + filename + "/";
	//std::cout << "virtpath: " << virtpath << std::endl;
	//std::cout << "realpath: " << imgdirpath << std::endl;
	packer.index_real_dir(virtpath.c_str(), imgdirpath.c_str());
	packer.write_fs_blob(wfipath.c_str());
	printf("Wombat Forensic Image created successfully.\n");

	// DELETE RAW IMG TMP DIRECTORY
	std::uintmax_t rmcnt = std::filesystem::remove_all(imgdirpath);
	//std::cout << "Deleted: " << rmcnt << " files or directories" << std::endl;
    }
    else
    {
	ShowUsage(0);
	return 1;
    }

    return 0;
}
