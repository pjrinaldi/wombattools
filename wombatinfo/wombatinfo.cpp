#include <stdint.h>
#include <fcntl.h>
#include <linux/fs.h>
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
    QCoreApplication::setApplicationName("wombatinfo");
    QCoreApplication::setApplicationVersion("0.1");
    QCommandLineParser parser;
    parser.setApplicationDescription("Print Information for a wombat forensic image.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("image", QCoreApplication::translate("main", "Wombat forensic image file name."));

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

    printf("wombatinfo v0.1\n\n");
    quint64 header;
    uint8_t version;
    quint64 totalbytes;
    QString casenumber;
    QString evidnumber;
    QString examiner;
    QString description;
    in >> header >> version >> totalbytes >> casenumber >> evidnumber >> examiner >> description;
    if(header == 0x776f6d6261746669)
        printf("File Format:\t\tWombat Forensic Image\n"); 
    else
    {
        qDebug() << "Wrong file type, not a wombat forensic image.";
        return 1;
    }
    if(version == 1)
        printf("Format Version:\t\tVersion %u\n", version);
    else
    {
        qDebug() << "Not the correct wombat forensic image format.";
        return 1;
    }
    QString lz4ver = QString::number(LZ4_VERSION_NUMBER);
    lz4ver.replace("0", ".");
    printf("Raw Media Size:\t\t%llu\n", totalbytes);
    printf("LZ4 Version:\t\t%s\n", lz4ver.toStdString().c_str()); 
    
    #define IN_CHUNK_SIZE (16*1024)

    char* cmpbuf = new char[IN_CHUNK_SIZE];

    LZ4F_dctx* lz4dctx;
    LZ4F_frameInfo_t lz4frameinfo;
    LZ4F_errorCode_t errcode;

    errcode = LZ4F_createDecompressionContext(&lz4dctx, LZ4F_getVersion());
    if(LZ4F_isError(errcode))
        printf("%s\n", LZ4F_getErrorName(errcode));
    
    int bytesread = in.readRawData(cmpbuf, IN_CHUNK_SIZE);

    size_t consumedsize = bytesread;
    size_t framesize = LZ4F_getFrameInfo(lz4dctx, &lz4frameinfo, cmpbuf, &consumedsize);
    if(LZ4F_isError(framesize))
        printf("frameinfo error: %s\n", LZ4F_getErrorName(framesize));
    
    printf("LZ4 Block Size:\t\t%ld\n", GetBlockSize(&lz4frameinfo));
    printf("\n");
    printf("Case Number:\t\t%s\n", casenumber.toStdString().c_str());
    printf("Evidence Number:\t%s\n", evidnumber.toStdString().c_str());
    printf("Examiner:\t\t%s\n", examiner.toStdString().c_str());
    printf("Description:\t\t%s\n", description.toStdString().c_str());

    wfi.seek(wfi.size() - 128);
    QString readhash;
    QByteArray tmparray = wfi.read(128);
    for(int i=1; i < 128; i++)
    {
        if(i % 2 != 0)
            readhash.append(tmparray.at(i));
    }
    wfi.close();
    delete[] cmpbuf;
    errcode = LZ4F_freeDecompressionContext(lz4dctx);
    
    printf("BLAKE3 Hash:\t\t%s\n", readhash.toStdString().c_str());

    return 0;
}
