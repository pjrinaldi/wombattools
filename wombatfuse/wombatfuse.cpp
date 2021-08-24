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
#include <QDebug>
#include <lz4.h>
#include <lz4frame.h>

#define FUSE_USE_VERSION 31

#include <fuse3/fuse.h>

static QString wfimg;
static QString imgfile;
static QString ifile;
static QString mntpt;
static const char* relativefilename = NULL;
static const char* rawfilename = NULL;
static std::string lz4filename;
//static quint64 totalbytes = 0;
static FILE* infile = NULL;
static off_t lz4size = 0;
static off_t rawsize = 0;
static off_t curoffset = 0;
static char* curbuffer = NULL;

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
qint64 Uncompress(char* buf, off_t offset, size_t size)
{
    #define IN_CHUNK_SIZE  (16*1024)

    char* cmpbuf = new char[IN_CHUNK_SIZE];
    
    LZ4F_dctx* lz4dctx;
    LZ4F_frameInfo_t lz4frameinfo;
    LZ4F_errorCode_t errcode;
    
    errcode = LZ4F_createDecompressionContext(&lz4dctx, LZ4F_getVersion());
    if(LZ4F_isError(errcode))
        printf("%s\n", LZ4F_getErrorName(errcode));

    quint64 curpos = 0;
    QFile cwfi(wfimg);
    if(!cwfi.isOpen())
	cwfi.open(QIODevice::ReadOnly);
    QDataStream cin(&cwfi);
    QFile rawdd(imgfile);
    rawdd.open(QIODevice::WriteOnly);
    QDataStream cout(&rawdd);

    quint64 header;
    uint8_t version;
    QString cnum;
    QString evidnum;
    QString examiner2;
    QString description2;
    cin >> header >> version >> totalbytes >> cnum >> evidnum >> examiner2 >> description2;
    if(header != 0x776f6d6261746669)
    {
        qDebug() << "Wrong file type, not a wombat forensic image.";
        return -EIO;
    }
    if(version != 1)
    {
        qDebug() << "Not the correct wombat forensic image format.";
        return -EIO;
    }

    int bytesread = cin.readRawData(cmpbuf, IN_CHUNK_SIZE);
    
    size_t consumedsize = bytesread;
    size_t framesize = LZ4F_getFrameInfo(lz4dctx, &lz4frameinfo, cmpbuf, &consumedsize);
    if(LZ4F_isError(framesize))
        printf("frameinfo error: %s\n", LZ4F_getErrorName(framesize));
    
    size_t rawbufsize = GetBlockSize(&lz4frameinfo);
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
            //write here
            int byteswrote = cout.writeRawData(rawbuf, dstsize);
            srcptr = (const char*)srcptr + srcsize;
        }
    }
    //qDebug() << "ret should be zero:" << ret;

    cwfi.close();
    rawdd.close();
    delete[] cmpbuf;
    delete[] rawbuf;

    errcode = LZ4F_freeDecompressionContext(lz4dctx);
    rawdd.open(QIODevice::ReadOnly);
    rawdd.seek(offset);
    int byteswritten = rawdd.read(buf, size);
    rawdd.close();

    return size;
}
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

    #define IN_CHUNK_SIZE  (16*1024)

    char* cmpbuf = new char[IN_CHUNK_SIZE];
    curbuffer = new char[size];
    
    LZ4F_dctx* lz4dctx;
    LZ4F_frameInfo_t lz4frameinfo;
    LZ4F_errorCode_t errcode;
    
    errcode = LZ4F_createDecompressionContext(&lz4dctx, LZ4F_getVersion());
    if(LZ4F_isError(errcode))
        printf("%s\n", LZ4F_getErrorName(errcode));

    fseek(infile, curoffset, SEEK_SET);
    //fseek(infile, offset, SEEK_SET);
    //int insize = fread(buf, 1, size, infile);

    off_t curoff = 0;
    int bytesread = fread(cmpbuf, 1, IN_CHUNK_SIZE, infile);
    
    size_t consumedsize = bytesread;
    size_t framesize = LZ4F_getFrameInfo(lz4dctx, &lz4frameinfo, cmpbuf, &consumedsize);
    if(LZ4F_isError(framesize))
        printf("frameinfo error: %s\n", LZ4F_getErrorName(framesize));
    
    size_t rawbufsize = GetBlockSize(&lz4frameinfo);
    char* rawbuf = new char[rawbufsize];
    size_t filled = bytesread - consumedsize;
    int firstchunk = 1;
    size_t ret = 1;
    while(ret != 0)
    {
	size_t readsize = firstchunk ? filled : fread(cmpbuf, 1, IN_CHUNK_SIZE, infile);
        firstchunk = 0;
        const void* srcptr = (const char*)cmpbuf + consumedsize;
        consumedsize = 0;
        const void* const srcend = (const char*)srcptr + readsize;
        while(srcptr < srcend && ret != 0)
        {
            size_t dstsize = rawbufsize;
            size_t srcsize = (const char*)srcend - (const char*)srcptr;
	    /*
            if(curoffset == offset)
            {
                if(size <= dstsize)
                    memcpy(buf, rawbuf, size);
                //else need to add to a buffer
            }
            else if(curoffset > offset)
            {
                int newoff = curoffset - offset;
                memcpy(buf, rawbuf+newoff, size);
            }
            if(curoffset >= offset && size <= dstsize)
            {
                if(curoffset == offset)
                    memcpy(buf, rawbuf, size);
                //buf 
            }
	    */
            ret = LZ4F_decompress(lz4dctx, rawbuf, &dstsize, srcptr, &srcsize, NULL);
	    curoff += dstsize;
	    //if(curoff >= offset)
		//memcpy(curbuffer, rawbuf, size);

            if(LZ4F_isError(ret))
            {
                printf("decompression error: %s\n", LZ4F_getErrorName(ret));
            }
	    memcpy(buf, rawbuf, sizeof(rawbuf));
            //write here
            //int byteswrote = cout.writeRawData(rawbuf, dstsize);
            srcptr = (const char*)srcptr + srcsize;
        }
    }
    //qDebug() << "ret should be zero:" << ret;

    delete[] cmpbuf;

    errcode = LZ4F_freeDecompressionContext(lz4dctx);
    //memcpy(buf, curbuffer, sizeof(curbuffer));

    //fseek(infile, offset, SEEK_SET);
    //int insize = fread(buf, 1, size, infile);
    //printf("insize: %d\n", insize);

    return size;
}

static void wombat_destroy(void* param)
{
    fclose(infile);
    delete[] curbuffer;
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

    lz4filename = wfimg.toStdString();
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

    //printf("infile size: %ld\n", lz4size);

    QFile cwfile(wfimg);
    cwfile.open(QIODevice::ReadOnly);
    QDataStream cin(&cwfile);
    qint64 header;
    uint8_t version;
    qint64 totalbytes;
    QString cnum;
    QString evidnum;
    QString examiner;
    QString description;
    cin >> header >> version >> totalbytes >> cnum >> evidnum >> examiner >> description;
    curoffset += 17 + cnum.size() + evidnum.size() + examiner.size() + description.size();
    rawsize = (off_t)totalbytes;
    cwfile.close();

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