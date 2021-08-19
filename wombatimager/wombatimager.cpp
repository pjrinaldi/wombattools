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

#include <iostream>
#include <string>
#include <QFile>
#include <QDataStream>
#include <QTextStream>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDebug>
#include "../blake3.h"
//#include <zstd.h>
//#include "zstdpp.hpp"
#include <snappy.h>
#include <lz4.h>

/*
#define DTTMFMT "%F %T %z"
#define DTTMSZ 35

static char* GetDateTime(char *buff)
{
    time_t t = time(0);
    strftime(buff, DTTMSZ, DTTMFMT, localtime(&t));
    return buff;
};

// NEED TO SWITCH INPUTS FROM IF= AND OF= TO -i -o -e -n -c and -d and then capture between the qoutes some how for each switch, where -i or --input, -o or --output -e --examiner -n --evidence-number -c --case-number -d --description
void ShowUsage(int outtype)
{
    if(outtype == 0)
    {
	printf("Usage: blake3dd if=/dev/sdX of=img.dd\n");
        printf("Create forensic image IMG.DD, log file IMG.LOG from device /DEV/SDX, automatically generates blake3 hash and validates forensic image.\n\n");
        printf("Flags:\n");
        printf("-h\tPrints help information\n");
        printf("-v\tPrints version information\n");
    }
    else if(outtype == 1)
    {
        printf("blake3dd v0.2\n");
	printf("License CC0-1.0: Creative Commons Zero v1.0 Universal\n");
        printf("This software is in the public domain\n");
        printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
        printf("Written by Pasquale Rinaldi\n");
    }
};
*/

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("wombatimager");
    QCoreApplication::setApplicationVersion("0.1");
    QCommandLineParser parser;
    parser.setApplicationDescription("Create a wombat forensic image from a block device");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("source", QCoreApplication::translate("main", "Block device to copy."));
    parser.addPositionalArgument("image", QCoreApplication::translate("main", "Desination forensic image without a file name w/o extension."));

    QCommandLineOption compressionleveloption(QStringList() << "c" << "compress", QCoreApplication::translate("main", "Set compression level default=3."), QCoreApplication::translate("main", "clevel"));
    parser.addOption(compressionleveloption);

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    QString clevel = parser.value(compressionleveloption);
    QString blockdevice = args.at(0);
    QString imgfile = args.at(1) + ".wfi";
    QString logfile = args.at(1) + ".log";
    int compressionlevel = 3;
    if(clevel.toInt() > 0)
        compressionlevel = clevel.toInt();
    qDebug() << "compression level:" << compressionlevel;

    // Initialize the datastream and header for the custom forensic image
    QFile wfi(imgfile);
    wfi.open(QIODevice::WriteOnly);
    QDataStream out(&wfi);
    //QByteArray imgarray;
    //imgarray.clear();
    out.setVersion(QDataStream::Qt_5_15);
    out << (quint64)0x776f6d6261746669; // wombatfi - wombat forensic image signature
    //out << (quint64)0x776f6d6261746c69; // wombatli - wombat logical image signature
    out << (uint8_t)0x1; // version 1
    //wfi.close();

    // Initialize the block device file for ioctl
    int infile = open(blockdevice.toStdString().c_str(), O_RDONLY | O_NONBLOCK);
    // Get Basic Device Information
    uint64_t totalbytes = 0;
    uint16_t sectorsize = 0;
    ioctl(infile, BLKGETSIZE64, &totalbytes);
    ioctl(infile, BLKSSZGET, &sectorsize);
    close(infile);
    qDebug() << "sector size:" << sectorsize;

    // Initialize the block device for byte reading
    QFile blkdev(blockdevice);
    blkdev.open(QIODevice::ReadOnly);
    QDataStream in(&blkdev);
    //QByteArray blkarray;
    //blkarray.clear();
    //FILE* const fin = fopen(blockdevice.toStdString().c_str(), "rb");
    //FILE* const fout = fopen(imgfile.toStdString().c_str(), "ab+");

    // Initialize the log file
    QFile log(logfile);
    log.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream logout(&log);
    
    logout << "wombatimager v0.1 Forensic Imager started at: " << QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss ap") << Qt::endl;
    logout << "Source Device" << Qt::endl;
    logout << "-------------" << Qt::endl;



    //Initialize ZSTD Stream Compressor
    //size_t const buffinsize = ZSTD_CStreamInSize();
    //size_t const buffoutsize = ZSTD_CStreamOutSize();
    //void* const buffin = malloc(buffinsize);
    //void* const buffout = malloc(buffoutsize);
    //ZSTD_CCtx* const cctx = ZSTD_createCCtx();
    //ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, 3);
    //ZSTD_CStream* zcs = ZSTD_createCStream();
    //size_t zcssizer = ZSTD_initCStream(zcs, compressionlevel);

    /*
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1);
    ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, 1);
    */
    //size_t const toread = buffinsize;
    //size_t const toread = buffinsize;
    //qDebug() << "buffinsize:" << buffinsize << "buffoutsize:" << buffoutsize;

    // Initialize Blake3 Hasher for block device and forensic image
    uint8_t sourcehash[BLAKE3_OUT_LEN];
//    uint8_t forimghash[BLAKE3_OUT_LEN];
    blake3_hasher blkhasher;
//    blake3_hasher imghasher;
    blake3_hasher_init(&blkhasher);
//    blake3_hasher_init(&imghasher);

    // GET UDEV INFORMATION FOR THE LOG
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
            if(strcmp(tmp, blockdevice.toStdString().c_str()) == 0)
            {
                logout << "Device:";
                const char* devvendor = udev_device_get_property_value(dev, "ID_VENDOR");
                if(devvendor != NULL)
                    logout << " " << devvendor;
                const char* devmodel = udev_device_get_property_value(dev, "ID_MODEL");
                if(devmodel != NULL)
                    logout << " " << devmodel;
                const char* devname = udev_device_get_property_value(dev, "ID_NAME");
                if(devname != NULL)
                    logout << " " << devname;
                logout << Qt::endl;
                tmp = udev_device_get_devnode(dev);
                logout << "Device Path: " << tmp << Qt::endl;
                tmp = udev_device_get_property_value(dev, "ID_SERIAL_SHORT");
                logout << "Serial Number: " << tmp << Qt::endl;
                logout << "Size: " << totalbytes << " bytes" << Qt::endl;
                logout << "Block Size: " << sectorsize << " bytes" << Qt::endl;
            }
        }
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);
    udev_unref(udev);


    qDebug() << "start reading block device and send to hasher and zstd";
    // start reading the block device and send to the hasher and the zstd_stream compressor
    uint64_t curpos = 0;
    char* bytebuf = new char[sectorsize];
    int dstsize = LZ4_compressBound(sectorsize);
    char* outbuf = new char[dstsize];
    //char bytebuf[sectorsize];
    //memset(bytebuf, 0, sizeof(bytebuf));
    //size_t cbufsize = ZSTD_compressBound(sectorsize);
    //char outbuf[cbufsize];
    //bytebuf = { 0 };
    //outbuf = { 0 };
    int bytesread = 0;
    uint64_t errorcount = 0;
    //size_t dstsize = ZSTD_compressBound(sectorsize);
    //char outbuf[sectorsize];
    //memset(outbuf, 0, sizeof(outbuf));
    //qDebug() << "dstsize:" << dstsize;

    while(curpos < totalbytes)
    {
        // NEED TO MOVE THIS WHILE LOOP BIT INTO THE FOR LOOP BIT DOWN THERE...
        bytesread = in.readRawData(bytebuf, sectorsize);
        if(bytesread == -1)
        {
            //bytebuf = { 0 };
            //memset(bytebuf, 0, sizeof(bytebuf));
            errorcount++;
            perror("Read Error, writing zeros instead.\n");
        }
        //std::string inbuffer = std::string(bytebuf, bytesread);

        // WORKING SNAPPY COMPRESSION
        //std::string outbuffer;
        //size_t csize = snappy::Compress(bytebuf, bytesread, &outbuffer);


        // LZ4 COMPRESSION
        int bwrote = LZ4_compress_default(bytebuf, outbuf, bytesread, dstsize);

        //size_t csize = snappy::Compress(inbuffer.data(), inbuffer.size(), &outbuffer);
        //std::string inbuffer = std::string(bytebuf);
        //char* outbuffer = new char[snappy::MaxCompressedLength(sectorsize)];
        //size_t outlength;
        //snappy::RawCompress(bytebuf, strlen(bytebuf), outbuffer, &outlength);
        //std::string outbuffer;
        //size_t csize = snappy::Compress(inbuffer.c_str(), inbuffer.size(), &outbuffer);
        /*
        ZSTD_inBuffer input = { bytebuf, 512, 0 };
        ZSTD_outBuffer output = { outbuf, cbufsize, 0 };
        //ZSTD_compressStream2(zcs, output, &emptyInput, ZSTD_e_flush);
        //ZSTD_compressStream2(zcs, output, &emptyInput, ZSTD_e_end);
        size_t cssize = ZSTD_compressStream(zcs, &output, &input);
        //! Equivalent to ZSTD_compressStream2(zcs, output, &emptyInput, ZSTD_e_flush). 
        size_t fsize = ZSTD_flushStream(zcs, &output);
        //! Equivalent to ZSTD_compressStream2(zcs, output, &emptyInput, ZSTD_e_end).
        size_t esize = ZSTD_endStream(zcs, &output);
        */


        //size_t csize = ZSTD_compress(outbuf, cbufsize, bytebuf, bytesread, compressionlevel);
        //CHECK_ZSTD(csize);
        /*
         *    size_t fSize;
    void* const fBuff = mallocAndLoadFile_orDie(fname, &fSize);
    size_t const cBuffSize = ZSTD_compressBound(fSize);
    void* const cBuff = malloc_orDie(cBuffSize);
    size_t const cSize = ZSTD_compress(cBuff, cBuffSize, fBuff, fSize, 1);
    CHECK_ZSTD(cSize);

    saveFile_orDie(oname, cBuff, cSize);

    // success
    printf("%25s : %6u -> %7u - %s \n", fname, (unsigned)fSize, (unsigned)cSize, oname);

    free(fBuff);
    free(cBuff);
         *
         */ 
        //ZSTD_inBuffer input = { bytebuf, bytesread, curpos };
        //ZSTD_outBuffer output = { outbuf, bytesread, curpos };
        blake3_hasher_update(&blkhasher, bytebuf, bytesread); // add bytes read to block device source hasher

        //std::string buffer;
        //zstdpp::buff_compress(std::string(bytebuf), buffer, compressionlevel);

        /*
        size_t estcompresssize = ZSTD_compressBound(sizeof(bytebuf));
        std::string buffer;
        buffer.resize(estcompresssize);
        auto compresssize = ZSTD_compress((void*)buffer.data(), estcompresssize, bytebuf, sizeof(bytebuf), compressionlevel);
        buffer.resize(compresssize);
        buffer.shrink_to_fit();
        */

        /*
        inline std::string& buff_compress(const std::string data, std::string& buffer, int compress_level) {
          size_t est_compress_size = ZSTD_compressBound(data.size());

          buffer.resize(est_compress_size);

          auto compress_size = ZSTD_compress((void*)buffer.data(), est_compress_size,
                                             data.data(), data.size(), compress_level);

          buffer.resize(compress_size);
          buffer.shrink_to_fit();

          return buffer;
        }
         */ 
        //auto compressed = zstdpp::compress(std::string(bytebuf), compressionlevel);
        //size_t csize = ZSTD_compress(outbuf, bytesread, bytebuf, bytesread, compressionlevel);
        //size_t remaining = ZSTD_compressStream2(cctx, &output, &input, ZSTD_e_end);
        //if(curpos + bytesread == totalbytes)
        //size_t remaining = ZSTD_compressStream2_simpleArgs(cctx, outbuf, sectorsize, curpos, bytebuf, sectorsize, curpos, ZSTD_e_end);
        //else
        //    size_t remaining = ZSTD_compressStream2_simpleArgs(cctx, outbuf, sectorsize, curpos, bytebuf, sectorsize, curpos, ZSTD_e_continue);
        //size_t remaining = ZSTD_compressStream2_simpleArgs(cctx, buffout, buffoutsize, 0, 
        //size_t ZSTD_compressStream2_simpleArgs ( ZSTD_CCtx* cctx, void* dst, size_t dstCapacity, size_t* dstPos, const void* src, size_t srcSize, size_t* srcPos, ZSTD_EndDirective endOp);
        //size_t csize = ZSTD_compress2(cctx, outbuf, bytesread, bytebuf, bytesread);
        //qDebug() << "csize:" << csize << ZSTD_isError(csize);
        // size_t ZSTD_compress2( ZSTD_CCtx* cctx, void* dst, size_t dstCapacity, const void* src, size_t srcSize);
        curpos = curpos + bytesread;
        //ssize_t remaining = 0;
        
        // SNAPPY WRITE COMMAND
        //size_t byteswrite = out.writeRawData(outbuffer.data(), outbuffer.size());

        // LZ4 WRITE COMMAND
        size_t byteswrite = out.writeRawData(outbuf, bwrote);

        //size_t byteswrite = out.writeRawData(outbuf, outlength);
        //size_t byteswrite = out.writeRawData(outbuffer.c_str(), csize);
        //size_t byteswrite = out.writeRawData(buffer.c_str(), buffer.size());

        //ssize_t byteswrite = out.writeRawData(compressed.c_str(), strlen(compressed.c_str()));
        //ssize_t byteswrite = out.writeRawData(outbuf, cbufsize); // writes zstd compressed stream data to wfi file.
        printf("Wrote %llu of %llu bytes\r", curpos, totalbytes);
        fflush(stdout);
    }
    /*
    for (;;)
    {
        //size_t const readsize = in.readRawData(buffin, toread);
        size_t const readsize = fread(buffin, 1, toread, fin);
        //qDebug() << "readsize:" << readsize << "toread:" << toread;
        //qDebug() << "(readsize < toread):" << (readsize < toread);
        int const lastchunk = (readsize < toread);
        //qDebug() << "lastchunk:" << lastchunk;
        ZSTD_EndDirective const mode = lastchunk ? ZSTD_e_end : ZSTD_e_continue;
        ZSTD_inBuffer input = { buffin, readsize, 0 };
        int finished;
        do
        {
            ZSTD_outBuffer output = { buffout, readsize, 0 };
            //size_t const remaining = ZSTD_compressStream2(cctx, &output, &input, mode);
            //size_t remaining = ZSTD_compressStream2_simpleArgs(cctx, buffout, buffoutsize, 0, 
            //size_t ZSTD_compressStream2_simpleArgs ( ZSTD_CCtx* cctx, void* dst, size_t dstCapacity, size_t* dstPos, const void* src, size_t srcSize, size_t* srcPos, ZSTD_EndDirective endOp);

            blake3_hasher_update(&blkhasher, buffin, readsize);

            //ssize_t byteswrite = out.writeRawData((char*)buffout, readsize);
            size_t const writtensize = fwrite(buffout, 1, output.pos, fout);
            finished = lastchunk ? (remaining == 0) : (input.pos == input.size);
            qDebug() << "lastchunk:" << lastchunk << "finished:" << finished;
            //printf("compressing %ld\r", remaining);
            //fflush(stdout);
        }
        while(!finished);

        if(lastchunk)
            break;
    }
    */

        /*
        ssize_t byteswrite = write(outfile, bytebuf, sectorsize);
        if(byteswrite == -1)
            perror("Write error, I haven't accounted for this yet so you probably want to use dc3dd instead.");
        blake3_hasher_update(&srchash, bytebuf, bytesread);
        ssize_t byteswrote = pread(outfile, imgbuf, sectorsize, curpos);
        blake3_hasher_update(&imghash, imgbuf, byteswrote);
        curpos = curpos + sectorsize;
        printf("Wrote %llu out of %llu bytes\r", curpos, totalbytes);
        fflush(stdout);
         */ 

    //size_t zcssize = ZSTD_freeCStream(zcs);
    //ZSTD_freeCCtx(cctx);
    //fclose(fin);
    //fclose(fout);
    //free(buffin);
    //free(buffout);
    delete bytebuf;
    delete outbuf;

    blake3_hasher_finalize(&blkhasher, sourcehash, BLAKE3_OUT_LEN);
    for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
    {
        //logout << 
        //fprintf(filelog, "%02x", sourcehash[i]);
        printf("%02x", sourcehash[i]);
    }
    printf("\n");


    wfi.close();
    blkdev.close();
    log.close();

    /*
    char* inputstr = NULL;
    char* outputstr = NULL;
    char* logfilestr = NULL;
    char* logstr = NULL;
    if(argc == 1 || (argc == 2 && strcmp(argv[1], "-h") == 0))
    {
	ShowUsage(0);
	return 1;
    }
    else if(argc == 2 && strcmp(argv[1], "-v") == 0)
    {
	ShowUsage(1);
	return 1;
    }
    else if(argc == 3)
    {
	printf("Command called: %s %s %s\n", argv[0], argv[1], argv[2]);
	inputstr = strstr(argv[1], "if=");
	if(inputstr == NULL)
	{
	    ShowUsage(0);
	    return 1;
	}
	if(strlen(inputstr) > 3)
        {
	    //printf("Input Device: %s\n", inputstr+3);
        }
	else
	{
	    printf("Input error.\n");
	    return 1;
	}
	outputstr = strstr(argv[2], "of=");
	if(outputstr == NULL)
	{
	    ShowUsage(0);
	    return 1;
	}
	if(strlen(outputstr) > 3)
	{
	    logstr = strrchr(outputstr+3, '.');
	    char* midname = strndup(outputstr+3, strlen(outputstr+3) - strlen(logstr));
	    logfilestr = strcat(midname, ".log");

            FILE* const fin = fopen(inputstr+3, "rb");
            FILE* const fout = fopen(outputstr+3, "wb");
            size_t const buffinsize = ZSTD_CStreamInSize();
            void* const buffin = malloc(buffinsize);
            size_t const buffoutsize = ZSTD_CStreamOutSize();
            void* const buffout = malloc(buffoutsize);
            ZSTD_CCtx* const cctx = ZSTD_createCCtx();
            ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, 1);
            ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1);
            ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, 4);
            size_t const toread = buffinsize;
            printf("buffinsize: %d buffoutsize %d\n", buffinsize, buffoutsize);

	    uint64_t totalbytes = 0;
	    uint16_t sectorsize = 0;
	    uint64_t curpos = 0;
            uint64_t errorcount = 0;
	    int infile = open(inputstr+3, O_RDONLY | O_NONBLOCK);
	    //int outfile = open(outputstr+3, O_RDWR | O_CREAT | O_TRUNC | O_APPEND, S_IRWXU);
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
		printf("Sector Size: %u Total Bytes: %u\n", sectorsize, totalbytes);
                time_t starttime = time(NULL);
                char buff[35];
                fprintf(filelog, "blake3dd v0.1 Raw Forensic Image started at: %s\n", GetDateTime(buff));
                fprintf(filelog, "\nSource Device\n");
                fprintf(filelog, "--------------\n");

		// GET UDEV DEVICE PROPERTIES FOR LOG...
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
			if(strcmp(tmp, inputstr+3) == 0)
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
                            fprintf(filelog, "Block Size: %u bytes\n", sectorsize);
			}
		    }
		    udev_device_unref(dev);
		}
		udev_enumerate_unref(enumerate);
		udev_unref(udev);
		close(infile);
                //lseek(infile, 0, SEEK_SET);
                //lseek(outfile, 0, SEEK_SET);
	*/
                /*
                uint8_t sourcehash[BLAKE3_OUT_LEN];
                uint8_t forimghash[BLAKE3_OUT_LEN];
                int i;
                blake3_hasher srchash;
                blake3_hasher imghash;
                blake3_hasher_init(&srchash);
                blake3_hasher_init(&imghash);
                */
	/*
                for (;;)
                {
                    size_t const readsize = fread(buffin, 1, toread, fin);
                    int const lastchunk = (readsize < toread);
                    ZSTD_EndDirective const mode = lastchunk ? ZSTD_e_end : ZSTD_e_continue;
                    ZSTD_inBuffer input = { buffin, readsize, 0 };
                    int finished;
                    do
                    {
                        ZSTD_outBuffer output = { buffout, buffoutsize, 0 };
                        size_t const remaining = ZSTD_compressStream2(cctx, &output, &input, mode);

                        size_t const writtensize = fwrite(buffout, 1, output.pos, fout);
                        finished = lastchunk ? (remaining == 0) : (input.pos == input.size);
                    }
                    while(!finished);

                    if(lastchunk)
                        break;
                }

                ZSTD_freeCCtx(cctx);
                fclose(fout);
                fclose(fin);
                free(buffin);
                free(buffout);
                /*
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
                */

                /*
                blake3_hasher_finalize(&srchash, sourcehash, BLAKE3_OUT_LEN);
                blake3_hasher_finalize(&imghash, forimghash, BLAKE3_OUT_LEN);
                time_t endtime = time(NULL);
                fprintf(filelog, "Wrote %llu out of %llu bytes\n", curpos, totalbytes);
                fprintf(filelog, "%llu blocks replaced with zeroes\n", errorcount);
                fprintf(filelog, "Forensic Image: %s\n", outputstr+3);
                fprintf(filelog, "Forensic Image finished at: %s\n", GetDateTime(buff));
                fprintf(filelog, "Forensic Image created in: %f seconds\n\n", difftime(endtime, starttime));
                printf("\nForensic Image Creation Finished\n");
                for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
                {
                    fprintf(filelog, "%02x", sourcehash[i]);
                    printf("%02x", sourcehash[i]);
                }
                fprintf(filelog, " - BLAKE3 Source Device\n");
                printf(" - BLAKE3 Source Device\n");
                for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
                {
                    fprintf(filelog, "%02x", forimghash[i]);
                    printf("%02x", forimghash[i]);
                }
                fprintf(filelog, " - BLAKE3 Forensic Image\n");
                printf(" - BLAKE3 Forensic Image\n");
                if(memcmp(sourcehash, forimghash, BLAKE3_OUT_LEN) == 0)
                {
                    printf("\nVerification Successful\n");
                    fprintf(filelog, "\nVerification Successful\n");
                }
                else
                {
                    printf("\nVerification Failed\n");
                    fprintf(filelog, "\nVerification Failed\n");
                }
                fprintf(filelog, "\nFinished Forensic Image Verification at: %s\n", GetDateTime(buff));
                fprintf(filelog, "Finished Forensic Image Verification in: %f seconds\n", difftime(time(NULL), starttime));
                */
	/*	fclose(filelog);
                //close(infile);
		//close(outfile);
	    }
	    else
	    {
		printf("error opening device: %d %s\n", infile, errno);
		return 1;
	    }
	}
	else
	{
	    printf("Output error.\n");
	    return 1;
	}
    }
    else
    {
	ShowUsage(0);
	return 1;
    }
    */

    return 0;
}
