#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include <iostream>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDataStream>
#include <QTextStream>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDebug>
#include <QtEndian>
//#include "../blake3.h"
#include <lz4.h>
#include <lz4frame.h>

/*
void PopulateFile(QFileInfo* tmpfileinfo, bool blake3bool, bool catsigbool, QDataStream* out, QTextStream* logout)
{
    if(tmpfileinfo->isDir()) // its a directory, need to read its contents..
    {
        QDir tmpdir(tmpfileinfo->absoluteFilePath());
        if(tmpdir.isEmpty())
            qDebug() << "dir" << tmpfileinfo->fileName() << "is empty and will be skipped.";
        else
        {
            QFileInfoList infolist = tmpdir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
            for(int i=0; i < infolist.count(); i++)
            {
                QFileInfo tmpinfo = infolist.at(i);
                PopulateFile(&tmpinfo, blake3bool, catsigbool, out, logout);
            }
        }
    }
    else if(tmpfileinfo->isFile()) // its a file, so read the info i need...
    {
        // name << path << size << created << accessed << modified << status changed << b3 hash << category << signature
        // other info to gleam such as groupid, userid, permission, 
        *out << (QString)tmpfileinfo->fileName(); // FILENAME
        *out << (QString)tmpfileinfo->absolutePath(); // FULL PATH
        *out << (qint64)tmpfileinfo->size(); // FILE SIZE (8 bytes)
        *out << (qint64)tmpfileinfo->birthTime().toSecsSinceEpoch(); // CREATED (8 bytes)
        *out << (qint64)tmpfileinfo->lastRead().toSecsSinceEpoch(); // ACCESSED (8 bytes)
        *out << (qint64)tmpfileinfo->lastModified().toSecsSinceEpoch(); // MODIFIED (8 bytes)
        *out << (qint64)tmpfileinfo->metadataChangeTime().toSecsSinceEpoch(); // STATUS CHANGED (8 bytes)
        *logout << "Processed:" << tmpfileinfo->absoluteFilePath() << Qt::endl;
        qDebug() << "Processed: " << tmpfileinfo->fileName();
        if(blake3bool)
        {
            //qDebug() << "calculate blake3 hash for contents here...";
        }
        if(catsigbool)
        {
            //qDebug() << "calculate the signature and category for the contents here...";
        }
        // Get the File Content and compress it using lz4 as a single frame ???
        //qDebug() << "lz4 compress file contents and add to the logical image file..";
    }
}
*/

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("wombatexport");
    QCoreApplication::setApplicationVersion("0.1");
    QCommandLineParser parser;
    parser.setApplicationDescription("Export the files and directory structure for a wombat logical forensic image");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("image", QCoreApplication::translate("main", "Logical forensic image file name."));
    parser.addPositionalArgument("directory", QCoreApplication::translate("main", "Directory where the contents will be exported."));

    // ADD OPTIONS TO HASH, CATEGORY/SIGNATURE ANALYSIS FOR IMPORTING INTO WOMBAT...
    
    // MAYBE SET AN OPTION TO ORIGINAL FILE PATH, OR HERE, AND THEN AN OPTION FOR LIST FILES ONLY

    //QCommandLineOption blake3option(QStringList() << "b" << "compute-hash", QCoreApplication::translate("main", "Compute the Blake3 Hash for the file."), QCoreApplication::translate("main", "blake3"));
    //QCommandLineOption signatureoption(QStringList() << "s" << "cat-sig", QCoreApplication::translate("main", "Compute the category/signature for the file."), QCoreApplication::translate("main", "catsig"));
    //QCommandLineOption compressionleveloption(QStringList() << "l" << "compress-level", QCoreApplication::translate("main", "Set compression level, default=3."), QCoreApplication::translate("main", "clevel"));
    //QCommandLineOption casenumberoption(QStringList() << "c" << "case-number", QCoreApplication::translate("main", "List the case number."), QCoreApplication::translate("main", "casenum"));
    //QCommandLineOption evidencenumberoption(QStringList() << "e" << "evidence-number", QCoreApplication::translate("main", "List the evidence number."), QCoreApplication::translate("main", "evidnum"));
    //QCommandLineOption examineroption(QStringList() << "x" << "examiner", QCoreApplication::translate("main", "Examiner creating forensic image."), QCoreApplication::translate("main", "examiner"));
    //QCommandLineOption descriptionoption(QStringList() << "d" << "description", QCoreApplication::translate("main", "Enter description of evidence to be imaged."), QCoreApplication::translate("main", "desciption"));
    //parser.addOption(compressionleveloption);
    //parser.addOption(blake3option);
    //parser.addOption(signatureoption);
    //parser.addOption(casenumberoption);
    //parser.addOption(evidencenumberoption);
    //parser.addOption(examineroption);
    //parser.addOption(descriptionoption);

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    //QString casenumber = parser.value(casenumberoption);
    //QString evidencenumber = parser.value(evidencenumberoption);
    //QString examiner = parser.value(examineroption);
    //QString description = parser.value(descriptionoption);
    //bool blake3bool = parser.isSet(blake3option);
    //bool catsigbool = parser.isSet(signatureoption);
    //QString clevel = parser.value(compressionleveloption);
    //qDebug() << "casenumber:" << casenumber << "evidencenumber:" << evidencenumber << "examiner:" << examiner << "descrption:" << description;
    QString imgfile = args.at(0);
    //QString logfile = args.at(0) + ".log";
    //QStringList filelist;
    //filelist.clear();
    //for(int i=1; i < args.count(); i++)
    //    filelist.append(args.at(i));

    // Initialize the datastream and header for the custom forensic image
    QFile wli(imgfile);
    if(!wli.isOpen())
        wli.open(QIODevice::ReadOnly);
    QDataStream in(&wli);
    quint64 header;
    quint8 version;
    QString casenumber;
    QString examiner;
    QString description;
    in >> header >> version >> casenumber >> examiner >> description;
    while(!in.atEnd())
    {
        QString filename;
        QString filepath;
        qint64 filesize;
        qint64 filecreate;
        qint64 filemodify;
        qint64 fileaccess;
        qint64 filestatus;
        QString srchash;
        QString catsig;
        in >> filename >> filepath >> filesize >> filecreate >> fileaccess >> filemodify >> filestatus >> srchash >> catsig;
        qDebug() << "filename:" << filename;
        if(!srchash.isEmpty())
            qDebug() << "srchash:" << srchash;
        if(!catsig.isEmpty())
            qDebug() << "catsig:" << catsig;
        // setFileTime(QDateTime(secssinceepoch, Qt::UTC), QFileDevice::FileBirthTime);
        // QFileDevice::FileAccessTime
        //QFileDevice::FileBirthTime
        //QFileDevice::FileMetadataChangeTime
        //QFileDevice::FileModificationTime

    }
    /*
    out.setVersion(QDataStream::Qt_5_15);
    out << (quint64)0x776f6d6261746c69; // wombatli - wombat logical image signature (8 bytes)
    out << (quint8)0x01; // version 1 (1 byte)
    out << (QString)casenumber;
    //out << (QString)evidencenumber;
    out << (QString)examiner;
    out << (QString)description;
    */

    // Initialize the log file
    //QFile log(logfile);
    //log.open(QIODevice::WriteOnly | QIODevice::Text);
    //QTextStream logout(&log);

    // IF I KEEP AN INDEX OF THE STARTING OFFSET OF EACH FILE, I CAN GENERATE A LISTING WITH THE WOMBATINFO FUNCTION TO LIST THE CONTENTS OF THE FILES WITHIN THE IMAGE SUCH AS WHAT ZIP WOULD DO...

    /*
    // Initiate the Loop over all source files to write their info the logical image file...
    for(int i=0; i < filelist.count(); i++)
    {
        QFileInfo tmpstat(filelist.at(i));
        PopulateFile(&tmpstat, blake3bool, catsigbool, &out, &logout);
    }
    */
    // WRITE THE INDEX STRING TO THE FILE FOR READING LATER...

    wli.close();
    // NEED TO REPOPEN THE WLI FILE AS READONLY AND THEN HASH THE CONTENTS TO SPIT OUT A HASH... TO VERIFY THE LOGICAL IMAGE
    // OF COURSE, IF I STICK THE HASH BACK INTO THE END OF THE FILE, I WOULD THEN HAVE TO READ THE ENTIRE FILE MINUS THE HASH VALUE TO VERIFY IT AT A LATER DATE WITH WOMBATVERIFY
    // HASHING THE LOGICAL IMAGE IN THIS WAY WOULDN'T BE COMPATIBLE WITH B3SUM, BUT THIS IS DIFFERENT AND DOESN'T NEED TO BE COMPATIBLE. WITH THAT IN MIND, I'LL HAVE TO FIGURE OUT WHAT TO PASS...
    // AND HOW TO VERIFY
    //log.close();
    /*
    // Initialize the block device for byte reading
    QFile blkdev(blockdevice);
    blkdev.open(QIODevice::ReadOnly);
    QDataStream in(&blkdev);

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
/*
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
/*
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
*/
    return 0;
}
