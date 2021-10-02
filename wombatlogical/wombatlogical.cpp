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
#include "../blake3.h"
#include <lz4.h>
#include <lz4frame.h>
#include <magic.h>

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
        *out << (qint64)0x776c69696e646578; // wombat logical image index entry
        *out << (QString)tmpfileinfo->fileName(); // FILENAME
        *out << (QString)tmpfileinfo->absolutePath(); // FULL PATH
        *out << (qint64)tmpfileinfo->size(); // FILE SIZE (8 bytes)
        *out << (qint64)tmpfileinfo->birthTime().toSecsSinceEpoch(); // CREATED (8 bytes)
        *out << (qint64)tmpfileinfo->lastRead().toSecsSinceEpoch(); // ACCESSED (8 bytes)
        *out << (qint64)tmpfileinfo->lastModified().toSecsSinceEpoch(); // MODIFIED (8 bytes)
        *out << (qint64)tmpfileinfo->metadataChangeTime().toSecsSinceEpoch(); // STATUS CHANGED (8 bytes)
        QFile tmpfile(tmpfileinfo->absoluteFilePath());
        if(!tmpfile.isOpen())
            tmpfile.open(QIODevice::ReadOnly);
        // Initialize Blake3 Hasher for block device and forensic image
        uint8_t sourcehash[BLAKE3_OUT_LEN];
        blake3_hasher blkhasher;
        blake3_hasher_init(&blkhasher);
        QString srchash = "";
        QString mimestr = "";
        while(!tmpfile.atEnd())
        {
            QByteArray tmparray = tmpfile.read(2048);
            if(blake3bool)
                blake3_hasher_update(&blkhasher, tmparray.data(), tmparray.count());
        }
        if(blake3bool)
        {
            blake3_hasher_finalize(&blkhasher, sourcehash, BLAKE3_OUT_LEN);
            for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
            {
                srchash.append(QString("%1").arg(sourcehash[i], 2, 16, QChar('0')));
            }
        }
        *out << (QString)srchash; // BLAKE3 HASH FOR FILE CONTENTS OR EMPTY
        if(catsigbool)
        {
            if(tmpfileinfo->fileName().startsWith("$UPCASE_TABLE"))
                mimestr = "System File/Up-case Table";
            else if(tmpfileinfo->fileName().startsWith("$ALLOC_BITMAP"))
                mimestr = "System File/Allocation Bitmap";
            else if(tmpfileinfo->fileName().startsWith("$UpCase"))
                mimestr = "Windows System/System File";
            else if(tmpfileinfo->fileName().startsWith("$MFT") || tmpfileinfo->fileName().startsWith("$MFTMirr") || tmpfileinfo->fileName().startsWith("$LogFile") || tmpfileinfo->fileName().startsWith("$Volume") || tmpfileinfo->fileName().startsWith("$AttrDef") || tmpfileinfo->fileName().startsWith("$Bitmap") || tmpfileinfo->fileName().startsWith("$Boot") || tmpfileinfo->fileName().startsWith("$BadClus") || tmpfileinfo->fileName().startsWith("$Secure") || tmpfileinfo->fileName().startsWith("$Extend"))
                mimestr = "Windows System/System File";
            else
            {
                // NON-QT WAY USING LIBMAGIC
                tmpfile.seek(0);
                QByteArray sigbuf = tmpfile.read(2048);
                magic_t magical;
                const char* catsig;
                magical = magic_open(MAGIC_MIME_TYPE);
                magic_load(magical, NULL);
                catsig = magic_buffer(magical, sigbuf.data(), sigbuf.count());
                std::string catsigstr(catsig);
                mimestr = QString::fromStdString(catsigstr);
                magic_close(magical);
                for(int i=0; i < mimestr.count(); i++)
                {
                    if(i == 0 || mimestr.at(i-1) == ' ' || mimestr.at(i-1) == '-' || mimestr.at(i-1) == '/')
                        mimestr[i] = mimestr[i].toUpper();
                }
            }
            //else if(tmpfileinfo->fileName().startsWith("$INDEX_ROOT:") || tmpfileinfo->fileName().startsWith("$DATA:") || tmpfileinfo->fileName().startWith("$INDEX_ALLOCATION:"))
        }
        *out << (QString)mimestr; // CATEGORY/SIGNATURE STRING FOR FILE CONTENTS OR EMPTY
        *out << (qint8)5; // itemtype
        *out << (qint8)0; // deleted

        // ATTEMPT USING A SINGLE LZ4FRAME SIMILAR TO BLAKE3 HASHING
        tmpfile.seek(0);
        qint64 curpos = 0;
        size_t destsize = LZ4F_compressFrameBound(2048, NULL);
        char* dstbuf = new char[destsize];
        char* srcbuf = new char[2048];
        int dstbytes = 0;
        int compressedsize = 0;
        LZ4F_cctx* lz4cctx;
        LZ4F_errorCode_t errcode;
        errcode = LZ4F_createCompressionContext(&lz4cctx, LZ4F_getVersion());
        if(LZ4F_isError(errcode))
            qDebug() << "LZ4 Create Error:" << LZ4F_getErrorName(errcode);
        dstbytes = LZ4F_compressBegin(lz4cctx, dstbuf, destsize, NULL);
        compressedsize += dstbytes;
        if(LZ4F_isError(dstbytes))
            qDebug() << "LZ4 Begin Error:" << LZ4F_getErrorName(dstbytes);
        out->writeRawData(dstbuf, dstbytes);
        while(curpos < tmpfile.size())
        {
            int bytesread = tmpfile.read(srcbuf, 2048);
            dstbytes = LZ4F_compressUpdate(lz4cctx, dstbuf, destsize, srcbuf, bytesread, NULL);
            if(LZ4F_isError(dstbytes))
                qDebug() << "LZ4 Update Error:" << LZ4F_getErrorName(dstbytes);
            dstbytes = LZ4F_flush(lz4cctx, dstbuf, destsize, NULL);
            if(LZ4F_isError(dstbytes))
                qDebug() << "LZ4 Flush Error:" << LZ4F_getErrorName(dstbytes);
            compressedsize += dstbytes;
            out->writeRawData(dstbuf, dstbytes);
            curpos = curpos + bytesread;
            //printf("Wrote %llu of %llu bytes\r", curpos, totalbytes);
            //fflush(stdout);
        }
        dstbytes = LZ4F_compressEnd(lz4cctx, dstbuf, destsize, NULL);
        compressedsize += dstbytes;
        out->writeRawData(dstbuf, dstbytes);
        delete[] srcbuf;
        delete[] dstbuf;
        errcode = LZ4F_freeCompressionContext(lz4cctx);
        tmpfile.close();
        *logout << "Processed:" << tmpfileinfo->absoluteFilePath() << Qt::endl;
        qDebug() << "Processed: " << tmpfileinfo->fileName();
    }
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("wombatlogical");
    QCoreApplication::setApplicationVersion("0.1");
    QCommandLineParser parser;
    parser.setApplicationDescription("Create a wombat logical forensic image from files and directories");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("image", QCoreApplication::translate("main", "Desination logical forensic image file name w/o extension."));
    parser.addPositionalArgument("source", QCoreApplication::translate("main", "Multitple files and directories to copy to the forensic image."));

    // ADD OPTIONS TO HASH, CATEGORY/SIGNATURE ANALYSIS FOR IMPORTING INTO WOMBAT...
    
    QCommandLineOption blake3option(QStringList() << "b" << "compute-hash", QCoreApplication::translate("main", "Compute the Blake3 Hash for the file."));
    QCommandLineOption signatureoption(QStringList() << "s" << "cat-sig", QCoreApplication::translate("main", "Compute the category/signature for the file."));
    //QCommandLineOption compressionleveloption(QStringList() << "l" << "compress-level", QCoreApplication::translate("main", "Set compression level, default=3."), QCoreApplication::translate("main", "clevel"));
    QCommandLineOption casenumberoption(QStringList() << "c" << "case-number", QCoreApplication::translate("main", "List the case number."), QCoreApplication::translate("main", "casenum"));
    //QCommandLineOption evidencenumberoption(QStringList() << "e" << "evidence-number", QCoreApplication::translate("main", "List the evidence number."), QCoreApplication::translate("main", "evidnum"));
    QCommandLineOption examineroption(QStringList() << "x" << "examiner", QCoreApplication::translate("main", "Examiner creating forensic image."), QCoreApplication::translate("main", "examiner"));
    QCommandLineOption descriptionoption(QStringList() << "d" << "description", QCoreApplication::translate("main", "Enter description of evidence to be imaged."), QCoreApplication::translate("main", "desciption"));
    //parser.addOption(compressionleveloption);
    parser.addOption(blake3option);
    parser.addOption(signatureoption);
    parser.addOption(casenumberoption);
    //parser.addOption(evidencenumberoption);
    parser.addOption(examineroption);
    parser.addOption(descriptionoption);

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if(args.count() <= 1)
    {
        qInfo() << "No image and/or source files/dirs provided.\n";
        parser.showHelp(1);
    }
    QString casenumber = parser.value(casenumberoption);
    //QString evidencenumber = parser.value(evidencenumberoption);
    QString examiner = parser.value(examineroption);
    QString description = parser.value(descriptionoption);
    bool blake3bool = parser.isSet(blake3option);
    bool catsigbool = parser.isSet(signatureoption);
    //QString clevel = parser.value(compressionleveloption);
    //qDebug() << "casenumber:" << casenumber << "evidencenumber:" << evidencenumber << "examiner:" << examiner << "descrption:" << description;
    QString imgfile = args.at(0) + ".wli";
    QString logfile = args.at(0) + ".log";
    QStringList filelist;
    filelist.clear();
    for(int i=1; i < args.count(); i++)
        filelist.append(args.at(i));

    // Initialize the datastream and header for the custom forensic image
    QFile wli(imgfile);
    if(!wli.isOpen())
        wli.open(QIODevice::WriteOnly);
    QDataStream out(&wli);
    out.setVersion(QDataStream::Qt_5_15);
    out << (quint64)0x776f6d6261746c69; // wombatli - wombat logical image signature (8 bytes)
    out << (quint8)0x01; // version 1 (1 byte)
    out << (QString)casenumber;
    //out << (QString)evidencenumber;
    out << (QString)examiner;
    out << (QString)description;

    // Initialize the log file
    QFile log(logfile);
    log.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream logout(&log);

    logout << "wombatlogical v0.1 Logical Forensic Imager started at: " << QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss ap") << Qt::endl;

    // Initiate the Loop over all source files to write their info the logical image file...
    for(int i=0; i < filelist.count(); i++)
    {
        QFileInfo tmpstat(filelist.at(i));
        PopulateFile(&tmpstat, blake3bool, catsigbool, &out, &logout);
    }
    // WRITE THE INDEX STRING TO THE FILE FOR READING LATER...

    wli.close();
    // NEED TO REPOPEN THE WLI FILE AS READONLY AND THEN HASH THE CONTENTS TO SPIT OUT A HASH... TO VERIFY THE LOGICAL IMAGE
    // OF COURSE, IF I STICK THE HASH BACK INTO THE END OF THE FILE, I WOULD THEN HAVE TO READ THE ENTIRE FILE MINUS THE HASH VALUE TO VERIFY IT AT A LATER DATE WITH WOMBATVERIFY
    // HASHING THE LOGICAL IMAGE IN THIS WAY WOULDN'T BE COMPATIBLE WITH B3SUM, BUT THIS IS DIFFERENT AND DOESN'T NEED TO BE COMPATIBLE. WITH THAT IN MIND, I'LL HAVE TO FIGURE OUT WHAT TO PASS...
    // AND HOW TO VERIFY
    blake3_hasher logicalhasher;
    blake3_hasher_init(&logicalhasher);
    wli.open(QIODevice::ReadOnly);
    wli.seek(0);
    while(!wli.atEnd())
    {
        QByteArray tmparray = wli.read(65536);
        blake3_hasher_update(&logicalhasher, tmparray.data(), tmparray.count());
    }
    wli.close();
    uint8_t output[BLAKE3_OUT_LEN];
    blake3_hasher_finalize(&logicalhasher, output, BLAKE3_OUT_LEN);
    QString logicalhash = "";
    for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
        logicalhash.append(QString("%1").arg(output[i], 2, 16, QChar('0')));
    wli.open(QIODevice::WriteOnly | QIODevice::Append);
    wli.seek(wli.size());
    QDataStream wout(&wli);
    wout << (QString)logicalhash;
    wli.close();

    logout << logicalhash << " - Logical Image Hash" << Qt::endl;
    // WHEN VERIFYING THE LOGICAL IMAGE, I NEED TO DO 128 FOR THE HASH TO COMPARE AND READ THE IMAGE FROM 0 UP TO 132 (128 FOR HASH + 4 FOR SIZE OF THE QSTRING);
    log.close();

    return 0;
}
