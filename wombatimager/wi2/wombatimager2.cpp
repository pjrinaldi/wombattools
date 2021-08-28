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
#include "../../blake3.h"
#include <lz4.h>
#include <lz4frame.h>

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

    //QCommandLineOption compressionleveloption(QStringList() << "l" << "compress-level", QCoreApplication::translate("main", "Set compression level, default=3."), QCoreApplication::translate("main", "clevel"));
    QCommandLineOption casenumberoption(QStringList() << "c" << "case-number", QCoreApplication::translate("main", "List the case number."), QCoreApplication::translate("main", "casenum"));
    QCommandLineOption evidencenumberoption(QStringList() << "e" << "evidence-number", QCoreApplication::translate("main", "List the evidence number."), QCoreApplication::translate("main", "evidnum"));
    QCommandLineOption examineroption(QStringList() << "x" << "examiner", QCoreApplication::translate("main", "Examiner creating forensic image."), QCoreApplication::translate("main", "examiner"));
    QCommandLineOption descriptionoption(QStringList() << "d" << "description", QCoreApplication::translate("main", "Enter description of evidence to be imaged."), QCoreApplication::translate("main", "desciption"));
    //parser.addOption(compressionleveloption);
    parser.addOption(casenumberoption);
    parser.addOption(evidencenumberoption);
    parser.addOption(examineroption);
    parser.addOption(descriptionoption);

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    QString casenumber = parser.value(casenumberoption);
    QString evidencenumber = parser.value(evidencenumberoption);
    QString examiner = parser.value(examineroption);
    QString description = parser.value(descriptionoption);
    //QString clevel = parser.value(compressionleveloption);
    //qDebug() << "casenumber:" << casenumber << "evidencenumber:" << evidencenumber << "examiner:" << examiner << "descrption:" << description;
    QString blockdevice = args.at(0);
    QString imgfile = args.at(1) + ".wfi";
    QString logfile = args.at(1) + ".log";
    QString ndxfile = args.at(1) + ".ndx";
    //int compressionlevel = 3;
    //if(clevel.toInt() > 0)
    //    compressionlevel = clevel.toInt();
    //qDebug() << "compression level:" << compressionlevel;

    // Initialize the datastream and header for the custom forensic image
    QFile wfi(imgfile);
    if(!wfi.isOpen())
	wfi.open(QIODevice::WriteOnly);
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
    //qDebug() << "sector size:" << sectorsize;
    out << (quint16)sectorsize; // forensic image sector size (2 bytes)
    out << (quint64)totalbytes; // forensic image size (8 bytes)
    out << (QString)casenumber;
    out << (QString)evidencenumber;
    out << (QString)examiner;
    out << (QString)description;

    // Initialize the block device for byte reading
    QFile blkdev(blockdevice);
    blkdev.open(QIODevice::ReadOnly);
    QDataStream in(&blkdev);

    // Initialize the log file
    QFile log(logfile);
    log.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream logout(&log);

    // Initialize the index file
    QFile indx(ndxfile);
    indx.open(QIODevice::WriteOnly);
    QDataStream ndxout(&indx);
    out.setVersion(QDataStream::Qt_5_15);
    
    logout << "wombatimager v0.1 Forensic Imager started at: " << QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss ap") << Qt::endl;
    logout << "Source Device" << Qt::endl;
    logout << "-------------" << Qt::endl;

    // Initialize Blake3 Hasher for block device and forensic image
    uint8_t sourcehash[BLAKE3_OUT_LEN];
    blake3_hasher blkhasher;
    blake3_hasher_init(&blkhasher);

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
    
    // ATTEMPT USING A LZ4FRAME PER BLOCK.
    quint64 curpos = 0;
    size_t destsize = LZ4F_compressFrameBound(sectorsize, NULL);
    qDebug() << "destsize:" << destsize;
    char* dstbuf = new char[destsize];
    char* srcbuf = new char[sectorsize];
    int dstbytes = 0;

    int compressedsize = 0;
    //qDebug() << "frameoffset:" << compressedsize;
    //QVector<quint64> framevectors;
    //framevectors.clear();
    LZ4F_frameInfo_t lz4frameinfo = LZ4F_INIT_FRAMEINFO;
    lz4frameinfo.blockMode = LZ4F_blockIndependent;
    LZ4F_cctx* lz4cctx;
    LZ4F_errorCode_t errcode;
    errcode = LZ4F_createCompressionContext(&lz4cctx, LZ4F_getVersion());
    if(LZ4F_isError(errcode))
        printf("%s\n", LZ4F_getErrorName(errcode));
    while(curpos < totalbytes)
    {
        //qDebug() << "compress begin frameoffset:" << compressedsize;
        //framevectors.append(compressedsize);
        ndxout << (quint64)compressedsize;
        dstbytes = LZ4F_compressBegin(lz4cctx, dstbuf, destsize, NULL);
        compressedsize += dstbytes;
        if(LZ4F_isError(dstbytes))
            printf("%s\n", LZ4F_getErrorName(dstbytes));
        out.writeRawData(dstbuf, dstbytes);
        int bytesread = in.readRawData(srcbuf, sectorsize);
        blake3_hasher_update(&blkhasher, srcbuf, bytesread);
        dstbytes = LZ4F_compressUpdate(lz4cctx, dstbuf, destsize, srcbuf, bytesread, NULL);
        if(LZ4F_isError(dstbytes))
            printf("%s\n", LZ4F_getErrorName(dstbytes));
        dstbytes = LZ4F_flush(lz4cctx, dstbuf, destsize, NULL);
        if(LZ4F_isError(dstbytes))
            printf("%s\n", LZ4F_getErrorName(dstbytes));
        compressedsize += dstbytes;
        //qDebug() << " compress frameoffset:" << compressedsize;
        out.writeRawData(dstbuf, dstbytes);
        curpos = curpos + bytesread;
        dstbytes = LZ4F_compressEnd(lz4cctx, dstbuf, destsize, NULL);
        compressedsize += dstbytes;
        //qDebug() << "frameoffset:" << compressedsize;
        out.writeRawData(dstbuf, dstbytes);
    }

    delete[] srcbuf;
    delete[] dstbuf;
    errcode = LZ4F_freeCompressionContext(lz4cctx);
    //out << framevectors;
    blkdev.close();
    indx.close();
    //qDebug() << "framevectors:" << framevectors;
    //qDebug() << "framevector count:" << framevectors.count();

    /*
    quint64 curpos = 0;
    size_t destsize = LZ4F_compressFrameBound(sectorsize, NULL);
    //qDebug() << "destsize:" << destsize;
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
    out.writeRawData(dstbuf, dstbytes);
    //size_t byteswrite = out.writeRawData(dstbuf, dstbytes);

    while(curpos < totalbytes)
    {
        int bytesread = in.readRawData(srcbuf, sectorsize);
	blake3_hasher_update(&blkhasher, srcbuf, bytesread);
	dstbytes = LZ4F_compressUpdate(lz4cctx, dstbuf, destsize, srcbuf, bytesread, NULL);
        if(LZ4F_isError(dstbytes))
            printf("%s\n", LZ4F_getErrorName(dstbytes));
	dstbytes = LZ4F_flush(lz4cctx, dstbuf, destsize, NULL);
        if(LZ4F_isError(dstbytes))
            printf("%s\n", LZ4F_getErrorName(dstbytes));
        compressedsize += dstbytes;
	//qDebug() << "update dstbytes:" << dstbytes;
	out.writeRawData(dstbuf, dstbytes);
	//byteswrite = out.writeRawData(dstbuf, dstbytes);
	curpos = curpos + bytesread;
        printf("Wrote %llu of %llu bytes\r", curpos, totalbytes);
        fflush(stdout);
    }
    */
    blake3_hasher_finalize(&blkhasher, sourcehash, BLAKE3_OUT_LEN);
    QString srchash = "";
    for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
    {
        //logout << 
        //fprintf(filelog, "%02x", sourcehash[i]);
        //logout << QString("%1").arg(forimghash[i], 2, 16, QChar('0'));
        srchash.append(QString("%1").arg(sourcehash[i], 2, 16, QChar('0')));
        printf("%02x", sourcehash[i]);
    }
    printf(" - Source Device Hash\n");
    out << srchash;
    wfi.close();
    /*
    //QString srchash = QString::fromUtf8(sourcehash);
    //qDebug() << "srchash as string:" << srchash;
    dstbytes = LZ4F_compressEnd(lz4cctx, dstbuf, destsize, NULL);
    compressedsize += dstbytes;
    //qDebug() << "compressed size:" << compressedsize;
    //qDebug() << "compressend dstbytes:" << dstbytes;
    out.writeRawData(dstbuf, dstbytes);
    out << srchash;
    //byteswrite = out.writeRawData(dstbuf, dstbytes);
    delete[] srcbuf;
    delete[] dstbuf;
    errcode = LZ4F_freeCompressionContext(lz4cctx);
    wfi.close();
    blkdev.close();
    */


    /*
    #define IN_CHUNK_SIZE  (16*1024)

    uint8_t forimghash[BLAKE3_OUT_LEN];
    blake3_hasher imghasher;
    blake3_hasher_init(&imghasher);
    char* cmpbuf = new char[IN_CHUNK_SIZE];
    
    LZ4F_dctx* lz4dctx;
    LZ4F_frameInfo_t lz4frameinfo;
    
    errcode = LZ4F_createDecompressionContext(&lz4dctx, LZ4F_getVersion());
    if(LZ4F_isError(errcode))
        printf("%s\n", LZ4F_getErrorName(errcode));

    curpos = 0;
    QFile cwfi(imgfile);
    if(!cwfi.isOpen())
	cwfi.open(QIODevice::ReadOnly);
    QDataStream cin(&cwfi);
    //QFile rawdd(imgfile.split(".").first() + ".dd");
    //rawdd.open(QIODevice::WriteOnly);
    //QDataStream cout(&rawdd);
    */
    /*
    int skipbytes = cin.skipRawData(17);
    if(skipbytes == -1)
        qDebug() << "skip failed";
    */
    /*
    quint64 header;
    uint8_t version;
    QString cnum;
    QString evidnum;
    QString examiner2;
    QString description2;
    cin >> header >> version >> totalbytes >> cnum >> evidnum >> examiner2 >> description2;
    if(header != 0x776f6d6261746669)
    {
        qDebug() << "Wrong file type, not a wombat forensic image.";
        return 1;
    }
    if(version != 1)
    {
        qDebug() << "Not the correct wombat forensic image format.";
        return 1;
    }

    int bytesread = cin.readRawData(cmpbuf, IN_CHUNK_SIZE);
    
    size_t consumedsize = bytesread;
    size_t framesize = LZ4F_getFrameInfo(lz4dctx, &lz4frameinfo, cmpbuf, &consumedsize);
    if(LZ4F_isError(framesize))
        printf("frameinfo error: %s\n", LZ4F_getErrorName(framesize));
    
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
	    blake3_hasher_update(&imghasher, rawbuf, dstsize);
            printf("Verifying %ld of %llu bytes\r", dstsize, totalbytes);
            fflush(stdout);
            //write here
            //int byteswrote = cout.writeRawData(rawbuf, dstsize);
            srcptr = (const char*)srcptr + srcsize;
        }
    }
    //qDebug() << "ret should be zero:" << ret;
    blake3_hasher_finalize(&imghasher, forimghash, BLAKE3_OUT_LEN);
    for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
    {
        printf("%02x", forimghash[i]);
        logout << QString("%1").arg(forimghash[i], 2, 16, QChar('0'));
    }
    printf(" - Forensic Image Hash\n");
    logout << " - Forensic Image Hash" << Qt::endl;
    */

    /* HOW TO GET HASH OUT OF THE IMAGE FOR THE WOMBATVERIFY FUNCTION...
    cwfi.seek(cwfi.size() - 128);
    QString readhash;
    QByteArray tmparray = cwfi.read(128);
    for(int i=1; i < 128; i++)
    {
        if(i % 2 != 0)
            readhash.append(tmparray.at(i));
    }
    qDebug() << "readhash:" << readhash;
    cwfi.close();
    //rawdd.close();
    delete[] cmpbuf;
    delete[] rawbuf;
    */

    /*
    errcode = LZ4F_freeDecompressionContext(lz4dctx);

    // VERIFY HASHES HERE...
    if(memcmp(sourcehash, forimghash, BLAKE3_OUT_LEN) == 0)
    {
        printf("\nVerification Successful\n");
        logout << "Verification Successful" << Qt::endl;
    }
    else
    {
        printf("\nVerification Failed\n");
        logout << "Verification Failed" << Qt::endl;
    }
    printf("Finished Forensic Image Verification at %s\n", QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss ap").toStdString().c_str());
    logout << "Finished Forensic Image Verification at:" << QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss ap") << Qt::endl;
    */
    log.close();
    return 0;
}
