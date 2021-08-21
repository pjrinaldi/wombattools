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
//#include <snappy.h>
#include <lz4.h>
#include <lz4frame.h>

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

static size_t GetBlockSize(const LZ4F_frameInfo_t* info)
{
    switch (info->blockSizeID)
    {
        case LZ4F_default:
        case LZ4F_max64KB:  return 1 << 16;
        case LZ4F_max256KB: return 1 << 18;
        case LZ4F_max1MB:   return 1 << 20;
        case LZ4F_max4MB:   return 1 << 22;
        default:
            printf("Impossible with expected frame specification (<=v1.6.1)\n");
            exit(1);
    }
}

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
    parser.addPositionalArgument("image", QCoreApplication::translate("main", "Desination forensic image file name w/o extension."));

    QCommandLineOption compressionleveloption(QStringList() << "l" << "compress-level", QCoreApplication::translate("main", "Set compression level, default=3."), QCoreApplication::translate("main", "clevel"));
    QCommandLineOption casenumberoption(QStringList() << "c" << "case-number", QCoreApplication::translate("main", "List the case number."), QCoreApplication::translate("main", "casenum"));
    QCommandLineOption evidencenumberoption(QStringList() << "e" << "evidence-number", QCoreApplication::translate("main", "List the evidence number."), QCoreApplication::translate("main", "evidnum"));
    QCommandLineOption examineroption(QStringList() << "x" << "examiner", QCoreApplication::translate("main", "Examiner creating forensic image."), QCoreApplication::translate("main", "examiner"));
    QCommandLineOption descriptionoption(QStringList() << "d" << "description", QCoreApplication::translate("main", "Enter description of evidence to be imaged."), QCoreApplication::translate("main", "desciption"));
    parser.addOption(compressionleveloption);
    parser.addOption(casenumberoption);
    parser.addOption(evidencenumberoption);
    parser.addOption(examineroption);
    parser.addOption(descriptionoption);

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    QString clevel = parser.value(compressionleveloption);
    QString blockdevice = args.at(0);
    QString imgfile = args.at(1) + ".wfi";
    QString logfile = args.at(1) + ".log";
    int compressionlevel = 3;
    if(clevel.toInt() > 0)
        compressionlevel = clevel.toInt();
    //qDebug() << "compression level:" << compressionlevel;

    // Initialize the datastream and header for the custom forensic image
    QFile wfi(imgfile);
    if(!wfi.isOpen())
	wfi.open(QIODevice::WriteOnly);
    if(wfi.isOpen())
	qDebug() << "wfi is open...";
    QDataStream out(&wfi);
    out.setVersion(QDataStream::Qt_5_15);
    out << (quint64)0x776f6d6261746669; // wombatfi - wombat forensic image signature (8 bytes)
    //out << (quint64)0x776f6d6261746c69; // wombatli - wombat logical image signature (8 bytes)
    out << (uint8_t)0x01; // version 1 (1 byte)

    // Initialize the block device file for ioctl
    int infile = open(blockdevice.toStdString().c_str(), O_RDONLY | O_NONBLOCK);
    // Get Basic Device Information
    quint64 totalbytes = 0;
    uint16_t sectorsize = 0;
    ioctl(infile, BLKGETSIZE64, &totalbytes);
    ioctl(infile, BLKSSZGET, &sectorsize);
    close(infile);
    qDebug() << "sector size:" << sectorsize;
    out << (quint64)totalbytes; // forensic image size (8 bytes)

    // Initialize the block device for byte reading
    QFile blkdev(blockdevice);
    blkdev.open(QIODevice::ReadOnly);
    QDataStream in(&blkdev);

    // Initialize the log file
    QFile log(logfile);
    log.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream logout(&log);
    
    logout << "wombatimager v0.1 Forensic Imager started at: " << QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss ap") << Qt::endl;
    logout << "Source Device" << Qt::endl;
    logout << "-------------" << Qt::endl;

    // Initialize Blake3 Hasher for block device and forensic image
    uint8_t sourcehash[BLAKE3_OUT_LEN];
    uint8_t forimghash[BLAKE3_OUT_LEN];
    blake3_hasher blkhasher;
    blake3_hasher imghasher;
    blake3_hasher_init(&blkhasher);
    blake3_hasher_init(&imghasher);

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
    
    // ATTEMPT USING A SINGLE LZ4FRAME SIMILAR TO BLAKE3 HASHING SETUP.
    qint64 curpos = 0;
    size_t destsize = LZ4F_compressFrameBound(sectorsize, NULL);
    qDebug() << "destsize:" << destsize;
    char* dstbuf = new char[destsize];
    char* srcbuf = new char[sectorsize];
    int dstbytes = 0;

    int compressedsize = 0;
    LZ4F_cctx* lz4cctx;
    LZ4F_errorCode_t errcode;
    errcode = LZ4F_createCompressionContext(&lz4cctx, LZ4F_getVersion());
    if(LZ4F_isError(errcode))
        printf("%s\n", LZ4F_getErrorName(errcode));
    dstbytes = LZ4F_compressBegin(lz4cctx, dstbuf, destsize, NULL);
    compressedsize += dstbytes;
    if(LZ4F_isError(dstbytes))
        printf("%s\n", LZ4F_getErrorName(dstbytes));
    //qDebug() << "compressbegin dstbytes:" << dstbytes;
    size_t byteswrite = out.writeRawData(dstbuf, dstbytes);

    while(curpos < totalbytes)
    {
        int bytesread = in.readRawData(srcbuf, sectorsize);
	dstbytes = LZ4F_compressUpdate(lz4cctx, dstbuf, destsize, srcbuf, bytesread, NULL);
        if(LZ4F_isError(dstbytes))
            printf("%s\n", LZ4F_getErrorName(dstbytes));
	dstbytes = LZ4F_flush(lz4cctx, dstbuf, destsize, NULL);
        if(LZ4F_isError(dstbytes))
            printf("%s\n", LZ4F_getErrorName(dstbytes));
        compressedsize += dstbytes;
	//qDebug() << "update dstbytes:" << dstbytes;
	byteswrite = out.writeRawData(dstbuf, dstbytes);
	curpos = curpos + bytesread;
    }
    dstbytes = LZ4F_compressEnd(lz4cctx, dstbuf, destsize, NULL);
    compressedsize += dstbytes;
    qDebug() << "compressed size:" << compressedsize;
    //qDebug() << "compressend dstbytes:" << dstbytes;
    byteswrite = out.writeRawData(dstbuf, dstbytes);
    delete[] srcbuf;
    delete[] dstbuf;
    errcode = LZ4F_freeCompressionContext(lz4cctx);
    wfi.close();
    log.close();
    blkdev.close();
    #define IN_CHUNK_SIZE  (16*1024)

    char* cmpbuf = new char[IN_CHUNK_SIZE];
    //char* rawbuf = new char[3*sectorsize];
    
    LZ4F_dctx* lz4dctx;
    LZ4F_frameInfo_t lz4frameinfo;
    
    errcode = LZ4F_createDecompressionContext(&lz4dctx, LZ4F_getVersion());
    if(LZ4F_isError(errcode))
        printf("%s\n", LZ4F_getErrorName(errcode));

    curpos = 0;
    QFile cwfi(imgfile);
    if(!cwfi.isOpen())
	cwfi.open(QIODevice::ReadOnly);
    if(cwfi.isOpen())
	qDebug() << "wfi is open to decompress...";
    QDataStream cin(&cwfi);
    QFile rawdd(imgfile.split(".").first() + ".dd");
    rawdd.open(QIODevice::WriteOnly);
    QDataStream cout(&rawdd);
    errcode = cin.skipRawData(17);
    if(errcode == -1)
        qDebug() << "skip failed";
    //size_t rawbufsize = 3*sectorsize;
    size_t cmpbufsize = IN_CHUNK_SIZE;

    int bytesread = cin.readRawData(cmpbuf, IN_CHUNK_SIZE);
    //bytesread = in.readRawData(bytebuf, sectorsize);
    
    size_t consumedsize = bytesread;
    /*
    size_t headersize = LZ4F_headerSize(cmpbuf, LZ4F_HEADER_SIZE_MAX);
    if(LZ4F_isError(headersize))
        printf("headersize error: %s\n", LZ4F_getErrorName(headersize));
    else
        qDebug() << "header size:" << headersize;
    */
    size_t framesize = LZ4F_getFrameInfo(lz4dctx, &lz4frameinfo, cmpbuf, &consumedsize);
    if(LZ4F_isError(framesize))
        printf("frameinfo error: %s\n", LZ4F_getErrorName(framesize));
    else
    {
        qDebug() << "framesize:" << framesize << "block size:" << GetBlockSize(&lz4frameinfo);
        qDebug() << "frame content size:" << lz4frameinfo.contentSize;
    }
    size_t rawbufsize = GetBlockSize(&lz4frameinfo);
    char* rawbuf = new char[rawbufsize];
    size_t filled = bytesread - consumedsize;
    int firstchunk = 1;
    size_t ret = 1;
    while(ret != 0)
    {
        size_t readsize = firstchunk ? filled : cin.readRawData(cmpbuf, IN_CHUNK_SIZE);
        firstchunk = 0;
        const void* srcptr = (const char*)cmpbuf + consumedsize;
        consumedsize = 0;
        const void* const srcend = (const char*)srcptr + readsize;
        while(srcptr < srcend && ret != 0)
        {
            size_t dstsize = rawbufsize;
            size_t srcsize = (const char*)srcend - (const char*)srcptr;
            ret = LZ4F_decompress(lz4dctx, rawbuf, &dstsize, srcptr, &srcsize, NULL);
            if(LZ4F_isError(ret))
            {
                printf("decompression error: %s\n", LZ4F_getErrorName(ret));
            }
            //write here
            int byteswrote = cout.writeRawData(rawbuf, dstsize);
            srcptr = (const char*)srcptr + srcsize;
        }
    }
    qDebug() << "ret should be zero:" << ret;

    cwfi.close();
    rawdd.close();
    delete[] cmpbuf;
    delete[] rawbuf;

    errcode = LZ4F_freeDecompressionContext(lz4dctx);

    //uint64_t curpos = 0;
    //char* bytebuf = new char[sectorsize];
    //memset(bytebuf, 0, sizeof(bytebuf));

    //int dstsize = LZ4_compressBound(sectorsize); // lz4 compression
    //size_t dstsize = ZSTD_compressBound(sectorsize); // zstd compression

    //char* outbuf = new char[dstsize];
    //char bytebuf[sectorsize];
    //memset(bytebuf, 0, sizeof(bytebuf));
    //bytebuf = { 0 };
    //int bytesread = 0;
    //uint64_t errorcount = 0;
    //quint64 totalcompressedsize = 0;
    //qint64 stringcount = 0;

    /*
    while(curpos < totalbytes)
    {
        // NEED TO MOVE THIS WHILE LOOP BIT INTO THE FOR LOOP BIT DOWN THERE...
	std::string instring = blkdev.read(sectorsize).toStdString();
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
        std::string outbuffer;
	size_t csize = snappy::Compress(instring.data(), instring.size(), &outbuffer);
        //size_t csize = snappy::Compress(bytebuf, bytesread, &outbuffer);

        // LZ4 COMPRESSION
        //int bwrote = LZ4_compress_default(bytebuf, outbuf, bytesread, dstsize);

        // ZSTD COMPRESSION
        //size_t bwrote = ZSTD_compress(outbuf, dstsize, bytebuf, bytesread, compressionlevel);
        //totalcompressedsize += bwrote;

	blake3_hasher_update(&blkhasher, instring.data(), instring.size());
        //blake3_hasher_update(&blkhasher, bytebuf, bytesread); // add bytes read to block device source hasher

        curpos = curpos + instring.size();
        //curpos = curpos + bytesread;
        
	// WHETHER I GO WITH ZSTD, SNAPPY, OR LZ4, I CAN WRITE EACH BUFFER AS A QSTRING RATHER THAN A RAWDATA, KEEP TRACK OF THE COUNT OF 
	// QSTRINGS, AND THEN WRITE THE COUNT AT THE END OF THE FILE AFTER THE HASH. WHEN READING IMAGE, I CAN SKIP TO THE END, THEN SKIP
	// BACK THE QINT64 SIZE (8), THEN READ THE << QINT64 SO I HAVE MY LOOP OF STRINGS... THEN I DON'T NEED THE COMPRESSED SIZE...
	// ANOTHER OPTION IS TO WRITE THE INT WITH THE COMPRESSED SIZE THEN THE QSTRING SO I CAN SIMPLY READ EACH SEGMENT AND HAVE THE END SEGMENT BE SOMETHING
	// ELSE LIKE -1
	

        // SNAPPY WRITE COMMAND
	//out << QString::fromStdString(outbuffer);
	//stringcount++;
        size_t byteswrite = out.writeRawData(outbuffer.data(), outbuffer.size());
        totalcompressedsize += outbuffer.size();
        

	// LZ4 WRITE COMMAND
        //size_t byteswrite = out.writeRawData(outbuf, bwrote);

        // ZSTD WRITE COMMAND
        //size_t byteswrite = out.writeRawData(outbuf, bwrote);

        char* imgbuf = new char[bwrote];
        wfi.seek(curpos - bytesread);
        qint64 bytesread = wfi.read(imgbuf, bwrote);
        char* dimgbuf = new char[1024];
        size_t dsize = ZSTD_decompress(dimgbuf, 1024, imgbuf, bwrote);
        blake3_hasher_update(&imghasher, dimgbuf, dsize);

        //size_t const dSize = ZSTD_decompress(rBuff, rSize, cBuff, cSize);

        ssize_t byteswrite = write(outfile, bytebuf, sectorsize);
        if(byteswrite == -1)
            perror("Write error, I haven't accounted for this yet so you probably want to use dc3dd instead.");
        blake3_hasher_update(&srchash, bytebuf, bytesread);
        ssize_t byteswrote = pread(outfile, imgbuf, sectorsize, curpos);
        blake3_hasher_update(&imghash, imgbuf, byteswrote);

        printf("Wrote %llu of %llu bytes\r", curpos, totalbytes);
        fflush(stdout);
        //delete[] imgbuf;
        //delete[] dimgbuf;
    }
    //qDebug() << "string count:" << stringcount;
    //out << (qint64)stringcount;
    //delete[] bytebuf;
    //delete[] outbuf;

    qDebug() << "totalcompressed size:" << totalcompressedsize;

    blake3_hasher_finalize(&blkhasher, sourcehash, BLAKE3_OUT_LEN);
    //blake3_hasher_finalize(&imghasher, forimghash, BLAKE3_OUT_LEN);

    for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
    {
        //logout << 
        //fprintf(filelog, "%02x", sourcehash[i]);
        printf("%02x", sourcehash[i]);
    }
    printf(" - Source Device Hash\n");
    for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
    {
        printf("%02x", forimghash[i]);
    }
    printf("\n");


    wfi.close();
    blkdev.close();
    log.close();

    QFile verwfi(imgfile);
    verwfi.open(QIODevice::ReadOnly);
    QDataStream vin(&verwfi);
    quint64 getheader;
    uint8_t getversion;
    quint64 getimgsize;
    vin >> getheader >> getversion >> getimgsize;
    qint64 compressedread = 0;
    qDebug() << QString::number(getheader, 16) << getversion << getimgsize;

    qDebug() << "totalcompressedsize / sectorsize:" << totalcompressedsize / sectorsize;
    qDebug() << "mostly compressed size:" << (totalcompressedsize / sectorsize) * sectorsize;
    qDebug() << "stringcount:" << stringcount;
    QFile rawdd("test.dd");
    rawdd.open(QIODevice::WriteOnly);

    verwfi.seek(17);
    //qDebug() << "position 17:" << verwfi.read(1).toHex();
    //while(compressedread < ((totalcompressedsize / sectorsize) * sectorsize))
    {
	std::string instring = verwfi.read(sectorsize).toStdString();
	std::string uncomp;
	snappy::Uncompress(instring.data(), instring.size(), &uncomp);
	int byteswrote = rawdd.write(uncomp.data(), uncomp.size());
	blake3_hasher_update(&imghasher, uncomp.data(), uncomp.size());

	compressedread += sectorsize;
    }
    std::string instring = verwfi.read((totalcompressedsize - ((totalcompressedsize/sectorsize) * sectorsize))).toStdString();
    std::string uncomp;
    snappy::Uncompress(instring.data(), instring.size(), &uncomp);
    int bytewrote = rawdd.write(uncomp.data(), uncomp.size());
    blake3_hasher_update(&imghasher, uncomp.data(), uncomp.size());
    for(qint64 i=0; i < stringcount; i++)
    {
	QString compstring;
	vin >> compstring;
	std::string uncomp;
	snappy::Uncompress(compstring.toStdString().data(), compstring.toStdString().size(), &uncomp);
	int byteswrote = rawdd.write(uncomp.data(), uncomp.size());
    }
    rawdd.close();
    char* imgbuf = new char[sectorsize];
    while(compressedread < ((totalcompressedsize / sectorsize) * sectorsize))
    {
        int bread = vin.readRawData(imgbuf, sectorsize);
        std::string uncomp;
        snappy::Uncompress(imgbuf, bread, &uncomp);
        //LZ4_decompress_safe(src, dst, comp_size, src_size);
        blake3_hasher_update(&imghasher, uncomp.data(), uncomp.size());
        compressedread += bread;
        printf("Verifying %d of %d bytes\r", compressedread, totalcompressedsize);
        fflush(stdout);
    }
    qDebug() << "compressed read:" << compressedread;
    // MIGHT BE ABLE TO DO SKIPRAWDATA() AND SKIP BACK BY BYTESWRITE AND THEN READ THE DATA I JUST WROTE SO BYTESWRITE TO A CHAR WHICH I PUSH TO THE HASH, ASSUMING THE CURRENT SEPARATE ATTEMPT WORKS.
    int bread = vin.readRawData(imgbuf, totalcompressedsize - ((totalcompressedsize / sectorsize) * sectorsize));
    qDebug() << "final bread:" << bread;
    std::string uncomp;
    snappy::Uncompress(imgbuf, bread, &uncomp);
    qDebug() << "uncomp.size():" << uncomp.size();
    blake3_hasher_update(&imghasher, uncomp.data(), uncomp.size());
    compressedread += bread;
    qDebug() << "final compressedread:" << compressedread;
    blake3_hasher_finalize(&imghasher, forimghash, BLAKE3_OUT_LEN);
    for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
    {
        printf("%02x", forimghash[i]);
    }
    printf(" - Forensic Image Hash\n");
    */

    //char* imgbuf = new char[sectorsize];
    //char* dimgbuf = new char[2*sectorsize];

    //snappy::Uncompress(input.data(), input.size(), &output);

    //qint64 bread = vin.readRawData(imgbuf, sectorsize);
    //quint64 rSize = ZSTD_getFrameContentSize(imgbuf, sectorsize);
    //qDebug() << "rsize: " << rSize;
    /*
    while(compressedread < totalcompressedsize)
    {
        int bread = vin.readRawData(imgbuf, sectorsize);
        size_t dsize = ZSTD_decompress(dimgbuf, 2*sectorsize, imgbuf, bread);
        qDebug() << "dsize:" << dsize;
        compressedread += bread;
    }*/
    /*
    char* imgbuf = new char[sectorsize];
    char* dimgbuf = new char[2*sectorsize];
    wfi.open(QIODevice::ReadOnly);
    while(curpos < wfi.size())
    {
        wfi.seek(curpos);
        qint64 bread = wfi.read(imgbuf, sectorsize);
        qDebug() << "start decompress." << curpos;
        size_t dsize = ZSTD_decompress(dimgbuf, 2*sectorsize, imgbuf, bread);
        qDebug() << "end decompress." << curpos;
        //blake3_hasher_update(&imghasher, dimgbuf, dsize);
        curpos = curpos + sectorsize;
        
        printf("Verifying %llu of %llu bytes\r", curpos, totalbytes);
        fflush(stdout);
    }
    //delete[] imgbuf;
    //delete[] dimgbuf;

    blake3_hasher_finalize(&imghasher, forimghash, BLAKE3_OUT_LEN);
    for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
    {
        printf("%02x", forimghash[i]);
    }
    printf(" - Forensic Image Hash\n");
    */

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
