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
    QString restorepath = args.at(1);
    //qDebug() << "restorepath:" << args.at(1);
    QDir restoredir(restorepath);
    //qDebug() << "restore dir path:" << restoredir.absolutePath();
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
    qint64 initoffset = wli.pos();
    qint64 curoffset = wli.pos();
    qDebug() << curoffset;
    QList<qint64> fileindxlist;
    while(!in.atEnd())
    {
        QByteArray skiparray = wli.read(1000);
        int isindx = skiparray.indexOf("wliindex");
        if(isindx >= 0)
            fileindxlist.append(curoffset + isindx);
        curoffset += 1000;
        /*
        QByteArray skiparray = wfi.read(10000);
        int isskiphead = skiparray.lastIndexOf("_*M");
        QString getindx = "";
        if(qFromBigEndian<quint32>(skiparray.mid(isskiphead, 4)) == 0x5f2a4d18)
        {
            wfi.seek(wfi.size() - 128 - 10000 + isskiphead + 8);
            in >> getindx;
        }
        wfi.seek(0);
         */ 
    }
    qDebug() << "fileindxlist:" << fileindxlist;
    qDebug() << "fileindxlist count:" << fileindxlist.count();
    for(int i=0; i < fileindxlist.count(); i++)
    {
        qint64 fileoffset = fileindxlist.at(i);
        qint64 lzfilesize = 0;
        if(i == fileindxlist.count() - 1)
            lzfilesize = wli.size() - fileindxlist.at(i);
        else
            lzfilesize = fileindxlist.at(i+1) - fileoffset;
        //qDebug() << "index:" << i << "fileoffset:" << fileoffset + 8 << "filesize:" << filesize;
        wli.seek(fileoffset);
        wli.read(8);
        //qDebug() << "wlistart:" << QString(wli.read(8));
        //qDebug() << "wli start:" << wli.read(8).toHex()
        qDebug() << "fileoffset:" << fileoffset << "lzfilesize:" << lzfilesize;
        //while(fileoffset < fileoffset + filesize)
        //{
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
            qDebug() << "cur pos before frame:" << wli.pos();
            qDebug() << "filename:" << filename << "filesize:" << filesize;
            //qDebug() << "new restorepath:" << restoredir.absolutePath() + filepath + "/" + filename;
            QDir tmpdir;
            if(tmpdir.mkpath(restoredir.absolutePath() + filepath) == false)
                qDebug() << "Failed to create restored directory for current file.";
            QFile restorefile(restoredir.absolutePath() + filepath + "/" + filename);
            if(!restorefile.isOpen())
                restorefile.open(QIODevice::WriteOnly);
            QDataStream out(&restorefile);
            // Decompress and Write Contents to the restored file
            char* cmpbuf = new char[16384];
            LZ4F_dctx* lz4dctx;
            LZ4F_frameInfo_t lz4frameinfo;
            LZ4F_errorCode_t errcode;
            errcode = LZ4F_createDecompressionContext(&lz4dctx, LZ4F_getVersion());
            if(LZ4F_isError(errcode))
                qDebug() << "Create Error:" << LZ4F_getErrorName(errcode);
            //qint64 curpos = 0;
            int bytesread = in.readRawData(cmpbuf, 16384);
            qDebug() << "init bytesread:" << bytesread;
            size_t consumedsize = bytesread;
            //fileoffset += bytesread;
            size_t framesize = LZ4F_getFrameInfo(lz4dctx, &lz4frameinfo, cmpbuf, &consumedsize);
            if(LZ4F_isError(framesize))
                qDebug() << "GetFrameInfo Error:" << LZ4F_getErrorName(framesize);
            size_t rawbufsize = GetBlockSize(&lz4frameinfo);
            char* rawbuf = new char[rawbufsize];
            size_t filled = bytesread - consumedsize;
            int firstchunk = 1;
            size_t ret = 1;
            while(ret != 0)
            {
                size_t readsize = firstchunk ? filled : in.readRawData(cmpbuf, 16384);
                qDebug() << "readsize:" << readsize;
                //fileoffset += readsize;
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
                        qDebug() << "Decompress Error:" << LZ4F_getErrorName(ret);
                    int byteswrote = out.writeRawData(rawbuf, dstsize);
                    qDebug() << "byteswrote:" << byteswrote;
                    srcptr = (const char*)srcptr + srcsize;
                }
                qDebug() << "ret:" << ret;
            }
            restorefile.close();
            if(!restorefile.isOpen())
                restorefile.open(QIODevice::WriteOnly);
            restorefile.setFileTime(QDateTime::fromSecsSinceEpoch(filecreate, Qt::UTC), QFileDevice::FileBirthTime);
            restorefile.setFileTime(QDateTime::fromSecsSinceEpoch(fileaccess, Qt::UTC), QFileDevice::FileAccessTime);
            restorefile.setFileTime(QDateTime::fromSecsSinceEpoch(filemodify, Qt::UTC), QFileDevice::FileModificationTime);
            restorefile.setFileTime(QDateTime::fromSecsSinceEpoch(filestatus, Qt::UTC), QFileDevice::FileMetadataChangeTime);
            restorefile.close();
            qDebug() << "Restored" << QString(filepath + "/" + filename) << "to" << restoredir.absolutePath();
            //qDebug() << "filename:" << filename;
            //if(!srchash.isEmpty())
            //    qDebug() << "srchash:" << srchash;
            //if(!catsig.isEmpty())
            //    qDebug() << "catsig:" << catsig;
        //}
    }
    /*
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
        //qDebug() << "new restorepath:" << restoredir.absolutePath() + filepath + "/" + filename;
        QDir tmpdir;
        if(tmpdir.mkpath(restoredir.absolutePath() + filepath) == false)
            qDebug() << "Failed to create restored directory for current file.";
        QFile restorefile(restoredir.absolutePath() + filepath + "/" + filename);
        if(!restorefile.isOpen())
            restorefile.open(QIODevice::WriteOnly);
        QDataStream out(&restorefile);
        // Decompress and Write Contents to the restored file
        char* cmpbuf = new char[16384];
        LZ4F_dctx* lz4dctx;
        LZ4F_frameInfo_t lz4frameinfo;
        LZ4F_errorCode_t errcode;
        errcode = LZ4F_createDecompressionContext(&lz4dctx, LZ4F_getVersion());
        if(LZ4F_isError(errcode))
            qDebug() << "Create Error:" << LZ4F_getErrorName(errcode);
        //qint64 curpos = 0;
        int bytesread = in.readRawData(cmpbuf, 16384);
        size_t consumedsize = bytesread;
        size_t framesize = LZ4F_getFrameInfo(lz4dctx, &lz4frameinfo, cmpbuf, &consumedsize);
        if(LZ4F_isError(framesize))
            qDebug() << "GetFrameInfo Error:" << LZ4F_getErrorName(framesize);
        size_t rawbufsize = GetBlockSize(&lz4frameinfo);
        char* rawbuf = new char[rawbufsize];
        size_t filled = bytesread - consumedsize;
        int firstchunk = 1;
        size_t ret = 1;
        while(ret != 0)
        {
            size_t readsize = firstchunk ? filled : in.readRawData(cmpbuf, 16384);
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
                    qDebug() << "Decompress Error:" << LZ4F_getErrorName(ret);
                int byteswrote = out.writeRawData(rawbuf, dstsize);
                srcptr = (const char*)srcptr + srcsize;
            }
        }
        restorefile.close();
        if(!restorefile.isOpen())
            restorefile.open(QIODevice::WriteOnly);
        restorefile.setFileTime(QDateTime::fromSecsSinceEpoch(filecreate, Qt::UTC), QFileDevice::FileBirthTime);
        restorefile.setFileTime(QDateTime::fromSecsSinceEpoch(fileaccess, Qt::UTC), QFileDevice::FileAccessTime);
        restorefile.setFileTime(QDateTime::fromSecsSinceEpoch(filemodify, Qt::UTC), QFileDevice::FileModificationTime);
        restorefile.setFileTime(QDateTime::fromSecsSinceEpoch(filestatus, Qt::UTC), QFileDevice::FileMetadataChangeTime);
        restorefile.close();
        qDebug() << "Restored" << QString(filepath + "/" + filename) << "to" << restoredir.absolutePath();
        //qDebug() << "filename:" << filename;
        //if(!srchash.isEmpty())
        //    qDebug() << "srchash:" << srchash;
        //if(!catsig.isEmpty())
        //    qDebug() << "catsig:" << catsig;
    }
    */
    wli.close();

    // IF I KEEP AN INDEX OF THE STARTING OFFSET OF EACH FILE, I CAN GENERATE A LISTING WITH THE WOMBATINFO FUNCTION TO LIST THE CONTENTS OF THE FILES WITHIN THE IMAGE SUCH AS WHAT ZIP WOULD DO...
    // WRITE THE INDEX STRING TO THE FILE FOR READING LATER...

    /*
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
            printf("Verifying %ld of %llu bytes\r", dstsize, totalbytes);
            fflush(stdout);
            //write here
            //int byteswrote = cout.writeRawData(rawbuf, dstsize);
            srcptr = (const char*)srcptr + srcsize;
        }
    }
    //qDebug() << "ret should be zero:" << ret;
    cwfi.close();
    //rawdd.close();
    delete[] cmpbuf;
    delete[] rawbuf;

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
