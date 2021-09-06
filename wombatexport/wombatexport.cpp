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
#include <lz4.h>
#include <lz4frame.h>

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

    //qDebug() << Qt::endl;
    qDebug() << "wombatexport v0.1 Export from a Wombat Logical Forensic Imager started at: " << QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss ap") << Qt::endl;
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
    //qint64 initoffset = wli.pos();
    qint64 curoffset = wli.pos();
    //qDebug() << curoffset;
    QList<qint64> fileindxlist;
    while(!in.atEnd())
    {
        QByteArray skiparray = wli.read(1000);
        int isindx = skiparray.indexOf("wliindex");
        if(isindx >= 0)
            fileindxlist.append(curoffset + isindx);
        curoffset += 1000;
    }
    //qDebug() << "fileindxlist:" << fileindxlist;
    //qDebug() << "fileindxlist count:" << fileindxlist.count();
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
        QString filename;
        QString filepath;
        qint64 filesize;
        qint64 filecreate;
        qint64 filemodify;
        qint64 fileaccess;
        qint64 filestatus;
        QString srchash;
        QString catsig;
        qint8 itemtype;
        qint8 deleted;
        in >> filename >> filepath >> filesize >> filecreate >> fileaccess >> filemodify >> filestatus >> srchash >> catsig >> itemtype >> deleted;
        //qDebug() << "cur pos before frame:" << wli.pos();
        //qDebug() << "filename:" << filename << "filesize:" << filesize;
        //qDebug() << "new restorepath:" << restoredir.absolutePath() + filepath + "/" + filename;
        QDir tmpdir;
        if(tmpdir.mkpath(restoredir.absolutePath() + filepath) == false)
            qDebug() << "Failed to create restored directory for current file.";
        QFile restorefile(restoredir.absolutePath() + filepath + "/" + filename);
        if(!restorefile.isOpen())
            restorefile.open(QIODevice::WriteOnly | QIODevice::Append);
        QDataStream out(&restorefile);
        // Decompress and Write Contents to the restored file
        char* cmpbuf = new char[200];
        LZ4F_dctx* lz4dctx;
        LZ4F_errorCode_t errcode;
        errcode = LZ4F_createDecompressionContext(&lz4dctx, LZ4F_getVersion());
        if(LZ4F_isError(errcode))
            qDebug() << "Create Error:" << LZ4F_getErrorName(errcode);
        size_t consumedsize = 0;
        size_t rawbufsize = 1000;
        char* rawbuf = new char[rawbufsize];
        size_t filled = 0;
        int firstchunk = 1;
        size_t ret = 1;
        while(ret != 0)
        {
            size_t readsize = firstchunk ? filled : in.readRawData(cmpbuf, 100);
            firstchunk = 0;
            const void* srcptr = (const char*)cmpbuf + consumedsize;
            consumedsize = 0;
            const void* const srcend = (const char*)srcptr + readsize;
            while(srcptr < srcend && ret != 0)
            {
                size_t dstsize = rawbufsize;
                size_t srcsize = (const char*)srcend - (const char*)srcptr;
                ret = LZ4F_decompress(lz4dctx, rawbuf, &dstsize, srcptr, &srcsize, NULL);
                //qDebug() << "dstsize:" << dstsize;
                if(LZ4F_isError(ret))
                    qDebug() << "Decompress Error:" << LZ4F_getErrorName(ret);
                if(dstsize > 0)
                {
                    int byteswrote = out.writeRawData(rawbuf, dstsize);
                    //qDebug() << "byteswrote:" << byteswrote;
                }
                srcptr = (const char*)srcptr + srcsize;
            }
            //qDebug() << "ret:" << ret;
        }
        restorefile.close();
        //qDebug() << "restorefile size:" << restorefile.size();
        if(!restorefile.isOpen())
            restorefile.open(QIODevice::ReadOnly);
        restorefile.setFileTime(QDateTime::fromSecsSinceEpoch(filecreate, Qt::UTC), QFileDevice::FileBirthTime);
        restorefile.setFileTime(QDateTime::fromSecsSinceEpoch(fileaccess, Qt::UTC), QFileDevice::FileAccessTime);
        restorefile.setFileTime(QDateTime::fromSecsSinceEpoch(filemodify, Qt::UTC), QFileDevice::FileModificationTime);
        restorefile.setFileTime(QDateTime::fromSecsSinceEpoch(filestatus, Qt::UTC), QFileDevice::FileMetadataChangeTime);
        restorefile.close();
        qDebug() << "Exported" << QString(filepath + "/" + filename) << "to" << restoredir.absolutePath();
    }
    qDebug() << Qt::endl << "wombatexport finished at" << QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss ap") << Qt::endl;
    wli.close();

    return 0;
}
