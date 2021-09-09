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
    quint8 islogical = 0;
    quint64 header;
    uint8_t version;
    quint16 sectorsize;
    quint32 blocksize;
    quint64 totalbytes;
    QString casenumber;
    QString evidnumber;
    QString examiner;
    QString description;
    in >> header >> version;
    if(header == 0x776f6d6261746669)
        printf("File Format:\t\tWombat Forensic Image\n"); 
    else if(header == 0x776f6d6261746c69)
    {
        printf("File Format:\t\tWombat Logical Image\n"); 
        islogical = 1;
    }
    else
    {
        qDebug() << "Wrong file type, not a wombat forensic image.";
        return 1;
    }
    if(version == 1)
        printf("Format Version:\t\tVersion %u\n", version);
    else
    {
        qDebug() << "Not the correct wombat forensic/logical image format.";
        return 1;
    }
    if(islogical == 0)
        in >> sectorsize >> blocksize >> totalbytes >> casenumber >> evidnumber >> examiner >> description;
    if(islogical == 1)
        in >> casenumber >> examiner >> description;
    qint64 curoffset = wfi.pos();
    QString lz4ver = QString::number(LZ4_VERSION_NUMBER);
    lz4ver.replace("0", ".");
    if(islogical == 0)
        printf("Raw Media Size:\t\t%llu\n", totalbytes);
    printf("LZ4 Version:\t\t%s\n", lz4ver.toStdString().c_str()); 
    if(islogical == 0)
        printf("LZ4 Block Size:\t\t%u\n", blocksize);
    
    printf("Case Number:\t\t%s\n", casenumber.toStdString().c_str());
    if(islogical == 0)
        printf("Evidence Number:\t%s\n", evidnumber.toStdString().c_str());
    printf("Examiner:\t\t%s\n", examiner.toStdString().c_str());
    printf("Description:\t\t%s\n", description.toStdString().c_str());

    if(islogical == 0)
    {
        wfi.seek(wfi.size() - 128);
        QString readhash;
        QByteArray tmparray = wfi.read(128);
        for(int i=1; i < 128; i++)
        {
            if(i % 2 != 0)
                readhash.append(tmparray.at(i));
        }
        printf("BLAKE3 Hash:\t\t%s\n", readhash.toStdString().c_str());
    }
    if(islogical == 1)
    {
        QList<qint64> fileindxlist;
        while(!in.atEnd())
        {
            QByteArray skiparray = wfi.read(1000);
            int isindx = skiparray.indexOf("wliindex");
            if(isindx >= 0)
                fileindxlist.append(curoffset + isindx);
            curoffset += 1000;
        }
        printf("\nContains the following %d files:\n", fileindxlist.count());
        for(int i=0; i < fileindxlist.count(); i++)
        {
            qint64 fileoffset = fileindxlist.at(i);
            /*
            qint64 lzfilesize = 0;
            if(i == fileindxlist.count() - 1)
                lzfilesize = wfi.size() - fileindxlist.at(i);
            else
                lzfilesize = fileindxlist.at(i+1) - fileoffset;
            */
            //qDebug() << "index:" << i << "fileoffset:" << fileoffset + 8 << "filesize:" << filesize;
            wfi.seek(fileoffset);
            wfi.read(8);
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
            printf("%s\n%d %s\n%s\n", std::string(filename.count() + 5, '-').c_str(), i+1, filename.toStdString().c_str(), std::string(filename.count() + 5, '-').c_str());
            printf("PATH:        %s\n", filepath.toStdString().c_str());
            printf("SIZE:        %lld\n", filesize);
            printf("CREATE:      %s\n", QDateTime::fromSecsSinceEpoch(filecreate).toString("MM/dd/yyyy hh:mm:ss AP").toStdString().c_str());
            if(!srchash.isEmpty())
                printf("BLAKE3 HASH: %s\n", srchash.toStdString().c_str());
            if(!catsig.isEmpty())
            {
                printf("CATEGORY:    %s\n", catsig.split("/").at(0).toStdString().c_str());
                printf("SIGNATURE:   %s\n", catsig.split("/").at(1).toStdString().c_str());
            }
            if(i < fileindxlist.count() - 1)
                printf("\n");
        }
    }
    wfi.close();

    return 0;
}
