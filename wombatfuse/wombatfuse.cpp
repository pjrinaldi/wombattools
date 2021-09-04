#include <stdint.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

#include <iostream>
#include <string>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QDataStream>
#include <QTextStream>
#include <QCommandLineParser>
#include <QStringList>
#include <QDateTime>
#include <QtEndian>
#include <QDebug>
#include <lz4.h>
#include <lz4frame.h>

#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>

static QString wfimg;
static QString imgfile;
static QString ifile;
//static QString ndxstr;
static QString mntpt;
static QStringList indxlist;
static const char* relativefilename = NULL;
static const char* rawfilename = NULL;
//static std::string lz4filename;
//static std::string ndxfilename;
//static FILE* infile = NULL;
//static FILE* ndxfile = NULL;
static off_t lz4size = 0;
static off_t rawsize = 0;
static size_t framecnt = 0;
static off_t curoffset = 0;
static off_t blocksize = 0;

static void *wombat_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
    (void) conn;
    cfg->kernel_cache = 1;
    return NULL;
}

static int wombat_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{

    (void) fi;
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if(strcmp(path, relativefilename) == 0)
    {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        //stbuf->st_size = lz4size;
        stbuf->st_size = rawsize;
    }
    else
        res = -ENOENT;

    return res;
}

static int wombat_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
    (void) offset;
    (void) fi;
    (void) flags;

    if (strcmp(path, "/") != 0)
            return -ENOENT;

    filler(buf, ".", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "..", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, rawfilename, NULL, 0, (fuse_fill_dir_flags)0);

    return 0;
}

static int wombat_open(const char *path, struct fuse_file_info *fi)
{
    if(strcmp(path, relativefilename) != 0)
            return -ENOENT;

    if ((fi->flags & O_ACCMODE) != O_RDONLY)
            return -EACCES;

    return 0;
}

static int wombat_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    (void) fi;
    if(strcmp(path, relativefilename) != 0)
        return -ENOENT;

    QFile wfi(wfimg);
    wfi.open(QIODevice::ReadOnly);
    QDataStream in(&wfi);
    qint64 lz4start = curoffset;
    LZ4F_dctx* lz4dctx;
    LZ4F_errorCode_t errcode;
    errcode = LZ4F_createDecompressionContext(&lz4dctx, LZ4F_getVersion());
    char* cmpbuf = new char[2*blocksize];
    QByteArray framearray;
    framearray.clear();
    quint64 frameoffset = 0;
    quint64 framesize = 0;
    size_t ret = 1;
    size_t bread = 0;
    size_t rawbufsize = blocksize;
    size_t dstsize = rawbufsize;
    char* rawbuf = new char[rawbufsize];

    qint64 indxstart = offset / blocksize;
    qint8 posodd = offset % blocksize;
    qint64 relpos = offset - (indxstart * blocksize);
    qint64 indxcnt = size / blocksize;
    if(indxcnt == 0)
        indxcnt = 1;
    if(posodd != 0 && (relpos + size) > blocksize)
        indxcnt++;
    qint64 indxend = indxstart + indxcnt;
    //if(indxend > rawsize / blocksize)
    for(int i=indxstart; i < indxend; i++)
    {
        frameoffset = indxlist.at(i).toULongLong();
        if(i == ((rawsize / blocksize) - 1))
            framesize = rawsize - frameoffset;
        else
            framesize = indxlist.at(i+1).toULongLong() - frameoffset;
        wfi.seek(lz4start + frameoffset);
        int bytesread = in.readRawData(cmpbuf, framesize);
        bread = bytesread;
        ret = LZ4F_decompress(lz4dctx, rawbuf, &dstsize, cmpbuf, &bread, NULL);
        QByteArray blockarray(rawbuf, dstsize);
        framearray.append(blockarray);
    }
    if(posodd == 0)
	memcpy(buf, framearray.mid(0, size).data(), size);
    else
	memcpy(buf, framearray.mid(relpos, size).data(), size);
    
    //wfi.seek(offset);
    //qint64 bytesread = wfi.read(buf, size);
    wfi.close();
    delete[] cmpbuf;
    delete[] rawbuf;
    framearray.clear();
    /*
	for(int i=indxstart; i < indxstart + indxcnt; i++)
	{
            frameoffset = indxlist.at(i).toULongLong();
	    if(i == ((totalbytes / sectorsize) - 1))
		framesize = totalbytes - frameoffset;
	    else
                framesize = indxlist.at(i+1).toULongLong() - frameoffset;
	    wfi.seek(lz4start + frameoffset);
	    int bytesread = in.readRawData(cmpbuf, framesize);
	    bread = bytesread;
	    ret = LZ4F_decompress(lz4dctx, rawbuf, &dstsize, cmpbuf, &bread, NULL);
	    QByteArray blockarray(rawbuf, dstsize);
	    framearray.append(blockarray);
	}
        /*
	qDebug() << "framearray size:" << framearray.size();
	if(posodd == 0)
	    qDebug() << "framearray zero:" << framearray.mid(0, size).toHex();
	else
	    qDebug() << "framearray rel:" << framearray.mid(relpos, size).toHex();
        */

	//qDebug() << "size requested:" << size << "size read:" << dstsize;
/*        errcode = LZ4F_freeDecompressionContext(lz4dctx);
        delete[] cmpbuf;
        delete[] rawbuf;
        wfi.close();
	tmparray.clear();
	if(posodd == 0)
            tmparray = framearray.mid(0, size);
	else
            tmparray = framearray.mid(relpos, size);
	//qDebug() << "tmparray:" << tmparray.toHex();
    }

     */ 

    /*
    QFile wfi(wfimg);
    wfi.open(QIODevice::ReadOnly);
    QDataStream in(&wfi);

    LZ4F_dctx* lz4dctx;
    LZ4F_errorCode_t errcode;
    errcode = LZ4F_createDecompressionContext(&lz4dctx, LZ4F_getVersion());
    char* cmpbuf = new char[2*blocksize];
    QByteArray framearray;
    framearray.clear();
    quint64 frameoffset = 0;
    quint64 nextoffset = 0;
    quint64 framesize = 0;

    size_t ret = 1;
    size_t bread = 0;
    size_t rawbufsize = blocksize;
    size_t dstsize = rawbufsize;
    char* rawbuf = new char[rawbufsize];
    qint64 indxstart = offset / blocksize;
    qint8 posodd = offset % blocksize;
    qint64 relpos = offset - (indxstart * blocksize);
    qint64 indxcnt = size / blocksize;
    //qint64 indxcnt = framecnt;
    if(indxcnt == 0)
        indxcnt = 1;
    if(posodd != 0 && (relpos + size) > blocksize)
        indxcnt++;
    wfi.seek(curoffset);
    for(int i=indxstart; i < indxstart + indxcnt; i++)
    {
        frameoffset = indxlist.at(i).toULongLong();
        if(i == (framecnt - 1))
            framesize = rawsize - frameoffset;
        else
            framesize = indxlist.at(i+1).toULongLong() - frameoffset;
        wfi.seek(curoffset + frameoffset);
        int bytesread = in.readRawData(cmpbuf, framesize);
        bread = bytesread;
        ret = LZ4F_decompress(lz4dctx, rawbuf, &dstsize, cmpbuf, &bread, NULL);
        QByteArray blockarray(rawbuf, dstsize);
        framearray.append(blockarray);
    }

    /*
    //qint64 indxcnt = rawsize / blocksize;
    qint64 indxcnt = framecnt - indxstart;
    //qint64 indxcnt = framecnt;
    //if(indxcnt == 0)
	//indxcnt = 1;
    if(posodd != 0 && (relpos + size) > blocksize)
	indxcnt++;
    qint64 indxend = indxstart + indxcnt;
    //if(indxend > rawsize / blocksize)
	//return -ENOENT;
    //QStringList indxlist = getindx.split(",", Qt::SkipEmptyParts);
    wfi.seek(curoffset);
    for(int i=indxstart; i < indxend; i++)
    {
	//ndx.seek(i*8);
	//frameoffset = qFromBigEndian<quint64>(ndx.read(8));
        frameoffset = indxlist.at(i).toULongLong();
	if(i == ((rawsize / blocksize) - 1))
	    framesize = rawsize - frameoffset;
	else
            framesize = indxlist.at(i+1).toULongLong() - frameoffset;
	    //framesize = qFromBigEndian<quint64>(ndx.peek(8)) - frameoffset;
	wfi.seek(curoffset + frameoffset);
	int bytesread = in.readRawData(cmpbuf, framesize);
	bread = bytesread;
	ret = LZ4F_decompress(lz4dctx, rawbuf, &dstsize, cmpbuf, &bread, NULL);
	QByteArray blockarray(rawbuf, dstsize);
	framearray.append(blockarray);
    }
    */
 /*   errcode = LZ4F_freeDecompressionContext(lz4dctx);
    wfi.close();
    if(posodd == 0)
	memcpy(buf, framearray.mid(0, size).data(), size);
    else
	memcpy(buf, framearray.mid(relpos, size).data(), size);

    delete[] cmpbuf;
    delete[] rawbuf;
    framearray.clear();
*/

    return size;
}

static void wombat_destroy(void* param)
{
    //fclose(infile);
    //fclose(ndxfile);
    //delete[] curbuffer;
    return;
}

static const struct fuse_operations wombat_oper = {
	.getattr	= wombat_getattr,
	.open		= wombat_open,
	.read		= wombat_read,
	.readdir	= wombat_readdir,
	.init           = wombat_init,
        .destroy        = wombat_destroy,
};

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("wombatfuse");
    QCoreApplication::setApplicationVersion("0.1");
    QCommandLineParser parser;
    parser.setApplicationDescription("Fuse mount a wombat forensic image to access raw forensic image");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("image", QCoreApplication::translate("main", "Wombat forensic image file name."));
    parser.addPositionalArgument("mountpoint", QCoreApplication::translate("main", "Mount Point."));

    parser.process(app);

    const QStringList args = parser.positionalArguments();

    if(args.count() == 0)
    {
        printf("Usage: wombatfuse imagefile mountpoint.\n");
        return 1;
    }
    wfimg = args.at(0);
    mntpt = args.at(1);
    imgfile = wfimg.split("/").last().split(".").first() + ".dd";
    ifile = "/" + imgfile;
    relativefilename = ifile.toStdString().c_str();
    rawfilename = imgfile.toStdString().c_str();

    //lz4filename = wfimg.toStdString();

    QFile cwfile(wfimg);
    cwfile.open(QIODevice::ReadOnly);
    QDataStream cin(&cwfile);
    
    //curpos = 0;
    // METHOD TO GET THE SKIPPABLE FRAME INDX CONTENT !!!!!
    cwfile.seek(cwfile.size() - 128 - 10000);
    QByteArray skiparray = cwfile.read(10000);
    int isskiphead = skiparray.lastIndexOf("_*M");
    qDebug() << "isskiphead:" << isskiphead << skiparray.mid(isskiphead, 4).toHex();
    QString getindx = "";
    if(qFromBigEndian<quint32>(skiparray.mid(isskiphead, 4)) == 0x5f2a4d18)
    {
        qDebug() << "skippable frame containing the index...";
        cwfile.seek(cwfile.size() - 128 - 10000 + isskiphead + 8);
        cin >> getindx;
    }
    //qDebug() << "getindx:" << getindx;
    indxlist.clear();
    indxlist = getindx.split(",", Qt::SkipEmptyParts);
    //qDebug() << "indxlist:" << indxlist;
    qDebug() << "indxlist count:" << indxlist.count();
    cwfile.seek(0);


    qint64 header;
    uint8_t version;
    quint16 sectorsize;
    quint32 blksize;
    qint64 totalbytes;
    QString cnum;
    QString evidnum;
    QString examiner;
    QString description;
    cin >> header >> version >> sectorsize >> blksize >> totalbytes >> cnum >> evidnum >> examiner >> description;
    qDebug() << "current position before for loop:" << cwfile.pos();
    curoffset = cwfile.pos();
    framecnt = totalbytes / blksize;
    qDebug() << "framecnt:" << framecnt;
    //framecnt = totalbytes / sectorsize;
    /*
    curoffset = 17;
    if(!cnum.isEmpty())
        curoffset += 2*cnum.length() + 4;
    if(!evidnum.isEmpty())
        curoffset += 2*evidnum.length() + 4;
    if(!examiner.isEmpty())
        curoffset += 2*examiner.length() + 4;
    if(!description.isEmpty())
        curoffset += 2*description.length() + 4;
    //curoffset += 17 + 2*(cnum.length() + evidnum.length() + examiner.length() + description.length()) + 16;
    */
    rawsize = (off_t)totalbytes;
    blocksize = (size_t)blksize;
    //qDebug() << "blocksize:" << blocksize << "framecnt:" << framecnt;
    //blocksize = (size_t)sectorsize;
    //printf("curoffset: %ld\n", curoffset);
    cwfile.close();
    //printf("rawsize: %ld\n", rawsize);

    char** fargv = NULL;
    fargv = (char**)calloc(2, sizeof(char*));
    int fargc = 2;
    fargv[0] = argv[1];
    fargv[1] = argv[2];
    struct fuse_args fuseargs = FUSE_ARGS_INIT(fargc, fargv);

    int ret;

    //fuse_opt_parse(NULL, NULL, NULL, NULL);
    ret = fuse_main(fuseargs.argc, fuseargs.argv, &wombat_oper, NULL);
    
    fuse_opt_free_args(&fuseargs);

    return ret;
}
