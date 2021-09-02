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
#include <QDataStream>
#include <QTextStream>
#include <QCommandLineParser>
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
static QString ndxstr;
static QString mntpt;
static const char* relativefilename = NULL;
static const char* rawfilename = NULL;
static std::string lz4filename;
static std::string ndxfilename;
//static quint64 totalbytes = 0;
static FILE* infile = NULL;
static FILE* ndxfile = NULL;
static off_t lz4size = 0;
static off_t rawsize = 0;
static size_t framecnt = 0;
static off_t curoffset = 0;
static off_t blocksize = 0;
//static char* curbuffer = NULL;

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

/*
*/

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

    QString lz4str = QString::fromStdString(lz4filename);
    QString ndxstr = lz4str.split(".").first() + ".ndx";
    QFile wfi(lz4str);
    QFile ndx(ndxstr);
    if(!wfi.isOpen())
        wfi.open(QIODevice::ReadOnly);
    QDataStream in(&wfi);
    if(!ndx.isOpen())
	ndx.open(QIODevice::ReadOnly);
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
    qint64 indxcnt = rawsize / blocksize;
    if(indxcnt == 0)
	indxcnt = 1;
    if(posodd != 0 && (relpos + size) > blocksize)
	indxcnt++;
    qint64 indxend = indxstart + indxcnt;
    //if(indxend > rawsize / blocksize)
	//return -ENOENT;
    for(int i=indxstart; i < indxend; i++)
    {
	ndx.seek(i*8);
	frameoffset = qFromBigEndian<quint64>(ndx.read(8));
	if(i == ((rawsize / blocksize) - 1))
	    framesize = rawsize - frameoffset;
	else
	    framesize = qFromBigEndian<quint64>(ndx.peek(8)) - frameoffset;
	wfi.seek(curoffset + frameoffset);
	int bytesread = in.readRawData(cmpbuf, framesize);
	bread = bytesread;
	ret = LZ4F_decompress(lz4dctx, rawbuf, &dstsize, cmpbuf, &bread, NULL);
	QByteArray blockarray(rawbuf, dstsize);
	framearray.append(blockarray);
    }
    errcode = LZ4F_freeDecompressionContext(lz4dctx);
    delete[] cmpbuf;
    delete[] rawbuf;
    ndx.close();
    wfi.close();
    if(posodd == 0)
	memcpy(buf, framearray.mid(0, size).data(), size);
    else
	memcpy(buf, framearray.mid(relpos, size).data(), size);

    framearray.clear();

    return size;
}

static void wombat_destroy(void* param)
{
    fclose(infile);
    fclose(ndxfile);
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
    ndxstr = wfimg.split(".").first() + ".ndx";
    ifile = "/" + imgfile;
    relativefilename = ifile.toStdString().c_str();
    rawfilename = imgfile.toStdString().c_str();

    lz4filename = wfimg.toStdString();
    ndxfilename = ndxstr.toStdString();
    //printf("lz4filename: %s\n", lz4filename.c_str());
    infile = fopen(lz4filename.c_str(), "rb");
    if(infile != NULL)
    {
        fseek(infile, 0, SEEK_END);
        lz4size = ftell(infile);
    }
    else
        printf("file failed to open\n");
    fseek(infile, 0, SEEK_SET);

    ndxfile = fopen(ndxfilename.c_str(), "rb");
    if(ndxfile == NULL)
    {
        printf("index file failed to open\n");
    }

    //printf("infile size: %ld\n", lz4size);

    QFile cwfile(wfimg);
    cwfile.open(QIODevice::ReadOnly);
    QDataStream cin(&cwfile);
    qint64 header;
    uint8_t version;
    quint16 sectorsize;
    qint64 totalbytes;
    QString cnum;
    QString evidnum;
    QString examiner;
    QString description;
    cin >> header >> version >> sectorsize >> totalbytes >> cnum >> evidnum >> examiner >> description;
    qDebug() << "current position before for loop:" << cwfile.pos();
    curoffset = cwfile.pos();
    framecnt = totalbytes / sectorsize;
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
    blocksize = (size_t)sectorsize;
    //printf("curoffset: %ld\n", curoffset);
    cwfile.close();
    printf("rawsize: %ld\n", rawsize);

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
