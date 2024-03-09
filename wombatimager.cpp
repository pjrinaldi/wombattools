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
    //char outbuf[size];
    //memset(outbuf, 0, sizeof(outbuf));
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
    /*
    lseek(infile, 0, SEEK_SET);
    lseek(outfile, 0, SEEK_SET);
    uint8_t sourcehash[BLAKE3_OUT_LEN];
    uint8_t forimghash[BLAKE3_OUT_LEN];
    int i;
    blake3_hasher srchash;
    blake3_hasher imghash;
    blake3_hasher_init(&srchash);
    blake3_hasher_init(&imghash);
    while(curpos < totalbytes)
    {
	char bytebuf[sectorsize];
	memset(bytebuf, 0, sizeof(bytebuf));
	char imgbuf[sectorsize];
	memset(imgbuf, 0, sizeof(imgbuf));
	ssize_t bytesread = read(infile, bytebuf, sectorsize);
	if(bytesread == -1)
	{
	    memset(bytebuf, 0, sizeof(bytebuf));
	    errorcount++;
	    perror("Read Error, Writing zeros instead.");
	}
	ssize_t byteswrite = write(outfile, bytebuf, sectorsize);
	if(byteswrite == -1)
	    perror("Write error, I haven't accounted for this yet so you probably want to use dc3dd instead.");
	blake3_hasher_update(&srchash, bytebuf, bytesread);
	ssize_t byteswrote = pread(outfile, imgbuf, sectorsize, curpos);
	blake3_hasher_update(&imghash, imgbuf, byteswrote);
	curpos = curpos + sectorsize;
	printf("Wrote %llu out of %llu bytes\r", curpos, totalbytes);
	fflush(stdout);
    }
    close(infile);
    close(outfile);
    */ 
}

/*
void HashFile(std::string filename, std::string whlfile)
{
    std::ifstream fin(filename.c_str());
    char tmpchar[65536];
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    while(fin)
    {
	fin.read(tmpchar, 65536);
	size_t cnt = fin.gcount();
	blake3_hasher_update(&hasher, tmpchar, cnt);
	if(!cnt)
	    break;
    }
    uint8_t output[BLAKE3_OUT_LEN];
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    std::stringstream ss;
    for(int i=0; i < BLAKE3_OUT_LEN; i++)
        ss << std::hex << (int)output[i]; 
    std::string srcmd5 = ss.str();
    std::string whlstr = srcmd5 + "," + filename + "\n";
    if(whlfile.compare("") == 0)
        std::cout << whlstr;
    else
    {
        FILE* whlptr = NULL;
        whlptr = fopen(whlfile.c_str(), "a");
        fwrite(whlstr.c_str(), strlen(whlstr.c_str()), 1, whlptr);
        fclose(whlptr);
    }
}
*/ 

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
                infocontent += "Case Number: \t" + std::string(argv[i+1]) + "\n";
            else if(strcmp(argv[i], "-e") == 0)
                infocontent += "Examiner: \t" + std::string(argv[i+1]) + "\n";
            else if(strcmp(argv[i], "-n") == 0)
                infocontent += "Evidence Number: " + std::string(argv[i+1]) + "\n";
            else if(strcmp(argv[i], "-d") == 0)
                infocontent += "Description:\t " + std::string(argv[i+1]) + "\n";
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
	std::cout << "image path: " << imagepath << std::endl;
	std::cout << "log path: " << logpath << std::endl;
	std::cout << "info path: " << infopath << std::endl;
	std::cout << "wfi path: " << wfipath << std::endl;
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
	FILE* fin = NULL;
	FILE* fout = NULL;
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
	    //fin = fopen(devicepath.c_str(), "rb");

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
	    std::cout << "thread count: " << threadcount << std::endl;

	    // GET THE TOTAL SYSTEM RAM
	    struct sysinfo info;
	    int syserr = sysinfo(&info);
	    uint64_t totalram = info.totalram;
	    std::cout << "total ram: " << totalram << std::endl;

	    std::cout << "device bytes: " << totalbytes << std::endl;

	    // MAX PIECE SIZE
	    uint64_t maxpiecesize = totalram / threadcount;
	    std::cout << "max piece size: " << maxpiecesize << std::endl;

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
	    std::cout << "piece size: " << piecesize << std::endl;

	    uint64_t piececount = totalbytes / piecesize;
	    std::cout << "piece count: " << piececount << std::endl;


	    /*
	    // OPEN FILEINFO AND START POPULATING THE INFORMATION
	    fileinfo = fopen(infopath.c_str(), "w+");
	    if(fileinfo == NULL)
	    {
		printf("Error opening info file.\n");
		return 1;
	    }
	    */

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
            
	    //fseek(fin, 0, SEEK_SET);
	    //fout = fopen(imagepath.c_str(), "wb");
	    //fseek(fout, 0, SEEK_SET);
	    
	    // MULTI THREAD THE raw image creation process
	    for(int i=0; i < piececount; i++)
	    {
		std::thread tmp(WritePiece, i*piecesize, piecesize, devicepath, imagepath);
		tmp.join();
	    }

	    /*
	    parallel -j $threadcount --bar dd if=$1 bs=$curpiecesize of=$2.dd skip={} seek={} count=1 status=none ::: $(seq 0 $piececount)
	    for(int i=0; i < filelist.size(); i++)
	    {
		std::thread tmp(HashFile, filelist.at(i).string(), whlstr);
		tmp.join();
	    }
	    */ 

            // BLAKE3_OUT_LEN IS 32 BYTES LONG
            //uint8_t srchash[BLAKE3_OUT_LEN];
            //uint8_t wfihash[BLAKE3_OUT_LEN];

	    //blake3_hasher srchasher;
	    //blake3_hasher_init(&srchasher);
	    

/*
    size_t read, toRead = buffInSize;
    while( (read = fread_orDie(buffIn, toRead, fin)) ) {
        ZSTD_inBuffer input = { buffIn, read, 0 };
        while (input.pos < input.size) {
            ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
            toRead = ZSTD_seekable_compressStream(cstream, &output , &input);   // toRead is guaranteed to be <= ZSTD_CStreamInSize()
            if (ZSTD_isError(toRead)) { fprintf(stderr, "ZSTD_seekable_compressStream() error : %s \n", ZSTD_getErrorName(toRead)); exit(12); }
            if (toRead > buffInSize) toRead = buffInSize;   // Safely handle case when `buffInSize` is manually changed to a value < ZSTD_CStreamInSize()
            fwrite_orDie(buffOut, output.pos, fout);
        }
    }
    while (1) {
        ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
        size_t const remainingToFlush = ZSTD_seekable_endStream(cstream, &output);   // close stream
        if (ZSTD_isError(remainingToFlush)) { fprintf(stderr, "ZSTD_seekable_endStream() error : %s \n", ZSTD_getErrorName(remainingToFlush)); exit(13); }
        fwrite_orDie(buffOut, output.pos, fout);
        if (!remainingToFlush) break;
    }
    ZSTD_seekable_freeCStream(cstream);
    fclose_orDie(fout);
    fclose_orDie(fin);
    free(buffIn);
    free(buffOut);
*/ 
	    /*
	    // USE ZSTD STREAM COMPRESSION
	    size_t bufinsize = ZSTD_CStreamInSize();
	    void* bufin = malloc_orDie(bufinsize);
	    size_t bufoutsize = ZSTD_CStreamOutSize();
	    void* bufout = malloc_orDie(bufoutsize);
	    size_t writecount = 0;

	    ZSTD_seekable_CStream* const cstream = ZSTD_seekable_createCStream();
	    size_t const initresult = ZSTD_seekable_initCStream(cstream, ZSTD_CLEVEL_DEFAULT, 1, 0);
	    //ZSTD_CCtx* cctx = ZSTD_createCCtx();
	    //CHECK(cctx != NULL, "ZSTD_createCCtx() failed");
	    size_t read = bufinsize;
	    size_t toread = bufinsize;
	    while((read = fread_orDie(bufin, toread, fin)))
	    {
		writecount = writecount + read;
		printf("Writing %llu of %llu bytes\r", writecount, totalbytes);
		fflush(stdout);

		blake3_hasher_update(&srchasher, bufin, read);

		ZSTD_inBuffer input = { bufin, read, 0 };
		while(input.pos < input.size)
		{
		    ZSTD_outBuffer output = { bufout, bufoutsize, 0 };
		    toread = ZSTD_seekable_compressStream(cstream, &output, &input);
		    if(toread > bufinsize)
			toread = bufinsize;
		    fwrite_orDie(bufout, output.pos, fout);
		}
	    }
	    //ZSTD_outBuffer output;
	    while(1)
	    {
		ZSTD_outBuffer output = { bufout, bufoutsize, 0 };
		//size_t const remainingtoflush = ZSTD_seekable_endFrame(cstream, &output);
		size_t const remainingtoflush = ZSTD_seekable_endStream(cstream, &output);
		fwrite_orDie(bufout, output.pos, fout);
		if(!remainingtoflush) break;
	    }

	    ZSTD_seekable_freeCStream(cstream);
	    */

	    /*
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
	    */

	    //blake3_hasher_finalize(&srchasher, srchash, BLAKE3_OUT_LEN);
            //memcpy(wfimd.devhash, srchash, BLAKE3_OUT_LEN);
	    // NEED TO WRITE SKIPPABLE FRAME CONTENT HERE
	    //fmd = fopen_orDie(mdpath.c_str(), "wb");
	    //fseek(fmd, 0, SEEK_SET);
	    //fwrite_orDie(&wfimd, sizeof(struct wfi_metadata), fmd);
	    //fclose_orDie(fmd);
	    //fwrite_orDie(&wfimd, sizeof(struct wfi_metadata), fout);
	    
	    //size_t const remainingtoflush = ZSTD_seekable_endStream(cstream, &output);
	    //ZSTD_seekable_freeCStream(cstream);

	    time_t endtime = time(NULL);
            fprintf(filelog, "Wrote %llu out of %llu bytes\n", curpos, totalbytes);
            fprintf(filelog, "%llu blocks replaced with zeroes\n", errcnt);
            fprintf(filelog, "Forensic Image: %s\n", imagepath.c_str());
            fprintf(filelog, "Forensic Image finished at: %s\n", GetDateTime(dtbuf));
            fprintf(filelog, "Forensic Image created in: %f seconds\n\n", difftime(endtime, starttime));
            printf("\nForensic Image Creation Finished\n");
	    /*
            for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
            {
                fprintf(filelog, "%02x", srchash[i]);
                printf("%02x", srchash[i]);
            }
	    */
            printf(" - BLAKE3 Source Device\n");
            fprintf(filelog, " - BLAKE3 Source Device\n");

	    //fclose(fin);
	    //fclose(fout);

	    /*
	    fclose_orDie(fout);
	    fclose_orDie(fin);
	    free(bufin);
	    free(bufout);
	    */

	    if(verify == 1) // start verification
	    {
                fprintf(filelog, "Verification started at: %s\n", GetDateTime(dtbuf));
                printf("Verification Started\n");
                uint8_t forimghash[BLAKE3_OUT_LEN];
                //blake3_hasher imghasher;
                //blake3_hasher_init(&imghasher);
                
		/*
		fout = fopen_orDie(imagepath.c_str(), "rb");
		size_t bufinsize = ZSTD_DStreamInSize();
		void* bufin = malloc_orDie(bufinsize);
		size_t bufoutsize = ZSTD_DStreamOutSize();
		void* bufout = malloc_orDie(bufoutsize);
		*/
/*
    FILE* const fin  = fopen_orDie(fname, "rb");
    FILE* const fout = stdout;
    size_t const buffOutSize = ZSTD_DStreamOutSize();  // Guarantee to successfully flush at least one complete compressed block in all circumstances.
    void*  const buffOut = malloc_orDie(buffOutSize);

    ZSTD_seekable* const seekable = ZSTD_seekable_create();
    if (seekable==NULL) { fprintf(stderr, "ZSTD_seekable_create() error \n"); exit(10); }

    size_t const initResult = ZSTD_seekable_initFile(seekable, fin);
    if (ZSTD_isError(initResult)) { fprintf(stderr, "ZSTD_seekable_init() error : %s \n", ZSTD_getErrorName(initResult)); exit(11); }

    while (startOffset < endOffset) {
        size_t const result = ZSTD_seekable_decompress(seekable, buffOut, MIN(endOffset - startOffset, buffOutSize), startOffset);
        if (!result) {
            break;
        }

        if (ZSTD_isError(result)) {
            fprintf(stderr, "ZSTD_seekable_decompress() error : %s \n",
                    ZSTD_getErrorName(result));
            exit(12);
        }
        fwrite_orDie(buffOut, result, fout);
        startOffset += result;
    }

    ZSTD_seekable_free(seekable);
    fclose_orDie(fin);
    fclose_orDie(fout);
    free(buffOut);
*/ 
		/*
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
		*/

		/*
                blake3_hasher_finalize(&imghasher, forimghash, BLAKE3_OUT_LEN);

		for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
		{
		    fprintf(filelog, "%02x", srchash[i]);
		    printf("%02x", srchash[i]);
		}
		*/
		printf(" - BLAKE3 Source Device\n");
		fprintf(filelog, " - BLAKE3 Source Device\n");

		/*
                for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
                {
		    fprintf(filelog, "%02x", forimghash[i]);
                    printf("%02x", forimghash[i]);
                }
		*/
                printf(" - Forensic Image Hash\n");
                fprintf(filelog, " - Forensic Image Hash\n");
		printf("\n");
		/*
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
		*/
	    }
	}
	else
	{
	    printf("error opening device: %d %s\n", infile, errno);
	    return 1;
	}
	fclose(filelog);
	fclose(fileinfo);
	/*
	std::uintmax_t n{fs::remove_all(tmp / "abcdef")};
	std::cout << "Deleted " << n << " files or directories\n";
	*/
    }
    else
    {
	ShowUsage(0);
	return 1;
    }

    return 0;
}
