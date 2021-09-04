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
#include <QtEndian>
#include "../blake3.h"
#include <lz4.h>
#include <lz4frame.h>

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
    uint32_t blocksize = 0;
    ioctl(infile, BLKGETSIZE64, &totalbytes);
    ioctl(infile, BLKSSZGET, &sectorsize);
    close(infile);
    out << (quint16)sectorsize; // forensic image sector size (2 bytes)
    if(totalbytes % 131072 == 0)
        blocksize = 131072;
    else if(totalbytes % 65536 == 0)
        blocksize = 65536;
    else if(totalbytes % 32768 == 0)
        blocksize = 32768;
    else if(totalbytes % 16384 == 0)
        blocksize = 16384;
    else if(totalbytes % 8192 == 0)
        blocksize = 8192;
    else if(totalbytes % 4096 == 0)
        blocksize = 4096;
    else if(totalbytes % 2048 == 0)
        blocksize = 2048;
    else if(totalbytes % 1024 == 0)
        blocksize = 1024;
    else if(totalbytes % 512 == 0)
        blocksize = 512;
    out << (quint32)blocksize; // block size used for uncompressed frames (4 bytes)
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

    // Initialize the index vector
    QString indxstr = "";
    
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
                logout << "Sector Size: " << sectorsize << " bytes" << Qt::endl;
                logout << "Block Size: " << blocksize << " bytes" << Qt::endl;
            }
        }
        udev_device_unref(dev);
    }
    udev_enumerate_unref(enumerate);
    udev_unref(udev);
    
    // ATTEMPT USING A LZ4FRAME PER BLOCK.
    LZ4F_frameInfo_t lz4frameinfo = LZ4F_INIT_FRAMEINFO;
    LZ4F_preferences_t lz4prefs = LZ4F_INIT_PREFERENCES;
    lz4frameinfo.blockMode = LZ4F_blockIndependent;
    lz4frameinfo.blockSizeID = LZ4F_max256KB;
    lz4prefs.frameInfo = lz4frameinfo;
    quint64 curpos = 0;
    size_t destsize = LZ4F_compressFrameBound(blocksize, &lz4prefs);
    //qDebug() << "destsize:" << destsize;
    char* dstbuf = new char[destsize];
    char* srcbuf = new char[blocksize];
    int dstbytes = 0;

    int compressedsize = 0;
    LZ4F_cctx* lz4cctx;
    LZ4F_errorCode_t errcode;
    errcode = LZ4F_createCompressionContext(&lz4cctx, LZ4F_getVersion());
    if(LZ4F_isError(errcode))
        printf("%s\n", LZ4F_getErrorName(errcode));
    while(curpos < totalbytes)
    {
        //qDebug() << "compress begin frameoffset:" << compressedsize;
        indxstr += QString::number(compressedsize) + ",";
        dstbytes = LZ4F_compressBegin(lz4cctx, dstbuf, destsize, &lz4prefs);
        compressedsize += dstbytes;
        if(LZ4F_isError(dstbytes))
            printf("%s\n", LZ4F_getErrorName(dstbytes));
        out.writeRawData(dstbuf, dstbytes);
        int bytesread = in.readRawData(srcbuf, blocksize);
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
        printf("Writing %llu of %llu bytes\r", curpos, totalbytes);
        fflush(stdout);
    }

    delete[] srcbuf;
    delete[] dstbuf;
    errcode = LZ4F_freeCompressionContext(lz4cctx);
    blkdev.close();
    // SKIPPABLE FRAME WITH INDEX STRING
    out << (quint32)0x5F2A4D18 << (quint32)indxstr.size() << (QString)indxstr;

    blake3_hasher_finalize(&blkhasher, sourcehash, BLAKE3_OUT_LEN);
    QString srchash = "";
    for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
    {
        srchash.append(QString("%1").arg(sourcehash[i], 2, 16, QChar('0')));
        printf("%02x", sourcehash[i]);
    }
    printf(" - Source Device Hash\n");
    out << srchash;
    wfi.close();

    uint8_t forimghash[BLAKE3_OUT_LEN];
    blake3_hasher imghasher;
    blake3_hasher_init(&imghasher);
    char* cmpbuf = new char[2*blocksize];
    
    LZ4F_dctx* lz4dctx;
    //LZ4F_frameInfo_t lz4frameinfo;

    errcode = LZ4F_createDecompressionContext(&lz4dctx, LZ4F_getVersion());
    if(LZ4F_isError(errcode))
        printf("%s\n", LZ4F_getErrorName(errcode));

    curpos = 0;
    QFile cwfi(imgfile);
    if(!cwfi.isOpen())
	cwfi.open(QIODevice::ReadOnly);
    QDataStream cin(&cwfi);

    /* METHOD TO GET THE SKIPPABLE FRAME INDX CONTENT !!!!!
    cwfi.seek(cwfi.size() - 128 - 10000);
    QByteArray skiparray = cwfi.read(10000);
    //int isskiphead = skiparray.lastIndexOf(QString::number(0x5f2a4d18, 16).toStdString().c_str());
    int isskiphead = skiparray.lastIndexOf("_*M");
    qDebug() << "isskiphead:" << isskiphead << skiparray.mid(isskiphead, 4).toHex();
    QString getindx = "";
    if(qFromBigEndian<quint32>(skiparray.mid(isskiphead, 4)) == 0x5f2a4d18)
    {
        qDebug() << "skippable frame containing the index...";
        cwfi.seek(cwfi.size() - 128 - 10000 + isskiphead + 8);
        cin >> getindx;
    }
    qDebug() << "getindx:" << getindx;
    cwfi.seek(0);
    */

    quint64 header;
    uint8_t version;
    quint16 sectorsize2;
    quint32 blocksize2;
    quint64 totalbytes2;
    QString cnum;
    QString evidnum;
    QString examiner2;
    QString description2;
    cin >> header >> version >> sectorsize2 >> blocksize2 >> totalbytes2 >> cnum >> evidnum >> examiner2 >> description2;
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

    QByteArray framearray;
    framearray.clear();
    quint64 frameoffset = 0;
    quint64 framesize = 0;
    //qDebug() << "current position before for loop:" << cwfi.pos();
    size_t ret = 1;
    size_t bread = 0;
    quint64 curread = 0;
    QStringList indxlist = indxstr.split(",", Qt::SkipEmptyParts);
    for(int i=0; i < (totalbytes / blocksize); i++)
    {
        frameoffset = indxlist.at(i).toULongLong();
        if(i == ((totalbytes / blocksize) - 1))
            framesize = totalbytes - frameoffset;
        else
            framesize = indxlist.at(i+1).toULongLong() - frameoffset;
        int bytesread = cin.readRawData(cmpbuf, framesize);
        bread = bytesread;
        size_t rawbufsize = blocksize;
        char* rawbuf = new char[rawbufsize];
        size_t dstsize = rawbufsize;
        ret = LZ4F_decompress(lz4dctx, rawbuf, &dstsize, cmpbuf, &bread, NULL);
        if(LZ4F_isError(ret))
            printf("decompress error %s\n", LZ4F_getErrorName(ret));
        blake3_hasher_update(&imghasher, rawbuf, dstsize);
        curread = curread + dstsize;
        printf("Verifying %llu of %llu bytes\r", curread, totalbytes);
        fflush(stdout);
    }
    blake3_hasher_finalize(&imghasher, forimghash, BLAKE3_OUT_LEN);
    for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
    {
        printf("%02x", forimghash[i]);
        logout << QString("%1").arg(forimghash[i], 2, 16, QChar('0'));
    }
    printf(" - Forensic Image Hash\n");
    logout << " - Forensic Image Hash" << Qt::endl;

    /*
    //HOW TO GET HASH OUT OF THE IMAGE FOR THE WOMBATVERIFY FUNCTION...
    cwfi.seek(cwfi.size() - 128);
    QString readhash;
    QByteArray tmparray = cwfi.read(128);
    for(int i=1; i < 128; i++)
    {
        if(i % 2 != 0)
            readhash.append(tmparray.at(i));
    }
    qDebug() << "readhash:" << readhash;
    */

    cwfi.close();
    delete[] cmpbuf;
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

    log.close();
    return 0;
}
