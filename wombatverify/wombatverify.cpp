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

void FindNextFrame(qint64 initialindex, QList<qint64>* framelist, QFile* wfi)
{
    //qDebug() << "initial index:" << initialindex;
    if(!wfi->isOpen())
        wfi->open(QIODevice::ReadOnly);
    wfi->seek(initialindex);
    QByteArray srcharray = wfi->peek(131072);
    int srchindx = srcharray.toHex().indexOf("04224d18");
    if(srchindx == -1)
    {
        //qDebug() << "this should occur after the last frame near the end of the file";
    }
    //int srchindx = srcharray.toHex().indexOf("04224d18", initialindex*2);
    wfi->seek(initialindex + srchindx/2);
    if(qFromBigEndian<qint32>(wfi->peek(4)) == 0x04224d18)
    {
        //qDebug() << "frame found:" << srchindx/2;
        framelist->append(initialindex + srchindx/2);
        FindNextFrame(initialindex + srchindx/2 + 1, framelist, wfi);
    }
    //else
    //    qDebug() << "frame error:" << srchindx/2;
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("wombatverify");
    QCoreApplication::setApplicationVersion("0.1");
    QCommandLineParser parser;
    parser.setApplicationDescription("Verify a wombat forensic image and wombat logical forensic image.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("image", QCoreApplication::translate("main", "Wombat forensic/logical image file name."));

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    QString imgfile = args.at(0);
    QFile wfi(imgfile);
    if(!wfi.isOpen())
        wfi.open(QIODevice::ReadOnly);
    QDataStream in(&wfi);
    if(in.version() != QDataStream::Qt_5_15)
    {
        qDebug() << "Wrong Qt Data Stream version:" << in.version();
        return 1;
    }
    quint64 header;
    in >> header;
    if(header == 0x776f6d6261746669) // wombat forensic image
    {
        // HOW TO GET FRAME INDEX LIST OUT OF THE WFI FILE 
        QList<qint64> frameindxlist;
        frameindxlist.clear();
        FindNextFrame(0, &frameindxlist, &wfi);

        wfi.seek(0);

        uint8_t version;
        quint16 sectorsize;
        quint32 blocksize;
        quint64 totalbytes;
        QString casenumber;
        QString evidnumber;
        QString examiner;
        QString description;
        in >> header >> version >> sectorsize >> blocksize >> totalbytes >> casenumber >> evidnumber >> examiner >> description;
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
        
        printf("wombatverify v0.1 started at: %s\n", QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss ap").toStdString().c_str());
        uint8_t imghash[BLAKE3_OUT_LEN];
        blake3_hasher imghasher;
        blake3_hasher_init(&imghasher);
        LZ4F_dctx* lz4dctx;
        LZ4F_errorCode_t errcode;
        errcode = LZ4F_createDecompressionContext(&lz4dctx, LZ4F_getVersion());
        if(LZ4F_isError(errcode))
            printf("%s\n", LZ4F_getErrorName(errcode));
        char* cmpbuf = new char[2*blocksize];
        quint64 frameoffset = 0;
        quint64 framesize = 0;
        quint64 curpos = 0;

        //QStringList indxlist = getindx.split(",", Qt::SkipEmptyParts);
        //qDebug() << "current position before for loop:" << wfi.pos();

        size_t ret = 1;
        size_t bread = 0;
        for(int i=0; i < (totalbytes / blocksize); i++)
        {
            frameoffset = frameindxlist.at(i);
            //frameoffset = indxlist.at(i).toULongLong();
            if(i == ((totalbytes / blocksize) - 1))
                framesize = totalbytes - frameoffset;
            else
            {
                framesize = frameindxlist.at(i+1) - frameoffset;
                //framesize = indxlist.at(i+1).toULongLong() - frameoffset;
            }
            int bytesread = in.readRawData(cmpbuf, framesize);
            bread = bytesread;
            size_t rawbufsize = blocksize;
            char* rawbuf = new char[rawbufsize];
            size_t dstsize = rawbufsize;
            ret = LZ4F_decompress(lz4dctx, rawbuf, &dstsize, cmpbuf, &bread, NULL);
            if(LZ4F_isError(ret))
                printf("decompress error %s\n", LZ4F_getErrorName(ret));
            blake3_hasher_update(&imghasher, rawbuf, dstsize);
            curpos += dstsize;
            printf("Verifying %llu of %llu bytes\r", curpos, totalbytes);
            fflush(stdout);
        }
        blake3_hasher_finalize(&imghasher, imghash, BLAKE3_OUT_LEN);
        QString calchash = "";
        for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
        {
            printf("%02x", imghash[i]);
            calchash.append(QString("%1").arg(imghash[i], 2, 16, QChar('0')));
        }
        printf(" - Forensic Image Hash\n");

        //HOW TO GET HASH OUT OF THE IMAGE FOR THE WOMBATVERIFY FUNCTION...
        wfi.seek(wfi.size() - 132);
        QString readhash = "";
        in >> readhash;
        wfi.close();
        delete[] cmpbuf;

        errcode = LZ4F_freeDecompressionContext(lz4dctx);

        // VERIFY HASHES HERE...
        if(calchash == readhash)
        {
            printf("\nVerification Successful\n");
        }
        else
        {
            printf("\nVerification Failed\n");
        }
        printf("Finished Forensic Image Verification at %s\n", QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss ap").toStdString().c_str());
    }
    else if(header == 0x776f6d6261746c69) // wombat logical image
    {
        printf("wombatverify v0.1 started at: %s\n", QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss ap").toStdString().c_str());
        wfi.seek(0);
        qint64 imgsize = wfi.size() - 132;
        blake3_hasher logicalhasher;
        blake3_hasher_init(&logicalhasher);
        qint64 curpos = 0;
        while(!wfi.atEnd())
        {
            QByteArray tmparray = wfi.read(65536);
            curpos += tmparray.count();
            if(curpos > imgsize)
                tmparray.chop(132);
            blake3_hasher_update(&logicalhasher, tmparray.data(), tmparray.count());
            printf("Verifying %llu of %llu bytes\r", curpos, imgsize);
            fflush(stdout);
        }
        // read existing hash
        wfi.seek(imgsize);
        QString readhash;
        in >> readhash;
        wfi.close();
        uint8_t output[BLAKE3_OUT_LEN];
        blake3_hasher_finalize(&logicalhasher, output, BLAKE3_OUT_LEN);
        QString calchash = "";
        for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
            calchash.append(QString("%1").arg(output[i], 2, 16, QChar('0')));

        printf("%s - Logical Image Hash\n", calchash.toStdString().c_str());
        if(calchash == readhash)
            printf("\nVerification Successful\n");
        else
            printf("\nVerification Failed\n");
        printf("Finished Logical Image Verification at %s\n", QDateTime::currentDateTime().toString("MM/dd/yyyy hh:mm:ss ap").toStdString().c_str());
    }

    return 0;
}
