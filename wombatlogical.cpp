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
#include <limits.h>
#include <stdlib.h>

#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <fstream>

#include <tar.h>
#include <libtar.h>

#include "blake3.h"

#include "common.h"
#include <zstd.h>

#define DTTMFMT "%F %T %z"
#define DTTMSZ 35

struct wfi_metadata
{
    uint32_t skipframeheader; // skippable frame header - 4
    uint32_t skipframesize; // skippable frame content size (not including header and this size) - 4
    uint16_t sectorsize; // raw forensic image sector size - 2
    uint16_t version; // version # for forensic image format
    uint16_t reserved1; // reserved1
    uint32_t reserved2; // reserved2
    //int64_t reserved; // reserved
    int64_t totalbytes; // raw forensic image total size - 8
    char casenumber[24]; // 24 character string - 24
    char evidencenumber[24]; // 24 character string - 24
    char examiner[24]; // 24 character string - 24
    char description[128]; // 128 character string - 128
    uint8_t devhash[32]; // blake3 source hash - 32
} wfimd; // 256

static char* GetDateTime(char *buff)
{
    time_t t = time(0);
    strftime(buff, DTTMSZ, DTTMFMT, localtime(&t));
    return buff;
};

void ShowUsage(int outtype)
{
    if(outtype == 0)
    {
        printf("Create logical forensic image IMAGE_NAME.wli, log file IMAGE_NAME.log from device FILES, automatically generates tar, applies zstd compression, and optionally validates forensic image.\n\n");
        printf("Usage :\n");
        printf("\twombatlogical [args] IMAGE_NAME FILES\n\n");
        printf("IMAGE_NAME\t: the file name for the forensic image without an extension.\n");
        printf("FILES\t: a device to image such as /dev/sdX\n");
        printf("Arguments :\n");
	printf("-c\t: calculate and store hash for each file in logical image.\n");
	//printf("-v\t: Perform image verification.\n");
        printf("-V\t: Prints Version information\n");
        printf("-h\t: Prints help information\n\n");
        printf("Example Usage :\n");
        printf("wombatlogical test.wli item1 dir/ file1 file2\n");
    }
    else if(outtype == 1)
    {
        printf("wombatlogical v0.1\n");
	printf("License CC0-1.0: Creative Commons Zero v1.0 Universal\n");
        printf("This software is in the public domain\n");
        printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
        printf("Written by Pasquale Rinaldi\n");
    }
};

/*
 * GET LIST OF MOUNT POINTS BY DEVICE MNTPT
#include <stdio.h>
#include <stdlib.h>
#include <mntent.h>

int main(void)
{
  struct mntent *ent;
  FILE *aFile;

  aFile = setmntent("/proc/mounts", "r");
  if (aFile == NULL) {
    perror("setmntent");
    exit(1);
  }
  while (NULL != (ent = getmntent(aFile))) {
    printf("%s %s\n", ent->mnt_fsname, ent->mnt_dir);
  }
  endmntent(aFile);
}
*/

/* c++ EXAMPLE TO GET MNTPT AND COMPARE TO PROVIDED PATH STRING
 * CAN MODIFY THIS TO USE STRING.CONTAINS RATHER THAN ==

#include <string_view>
#include <fstream>
#include <optional>

std::optional<std::string> get_device_of_mount_point(std::string_view path)
{
   std::ifstream mounts{"/proc/mounts"};
   std::string mountPoint;
   std::string device;

   while (mounts >> device >> mountPoint)
   {
      if (mountPoint == path)
      {
         return device;
      }
   }

   return std::nullopt;
}
if (const auto device = get_device_of_mount_point("/"))
   std::cout << *device << "\n";
else
   std::cout << "Not found\n";

*/

/*
 *
    // BEGIN TAR METHOD
    QString tmptar = casepath + "/" + wombatvariable.casename + ".wfc";
    QString oldtmptar = tmptar + ".old";
    if(FileExists(tmptar.toStdString()))
    {
        rename(tmptar.toStdString().c_str(), oldtmptar.toStdString().c_str());
    }
    QByteArray tmparray = tmptar.toLocal8Bit();
    QByteArray tmparray2 = wombatvariable.tmpmntpath.toLocal8Bit();
    QByteArray tmparray3 = QString("./" + wombatvariable.casename).toLocal8Bit();
    TAR* casehandle;
    tar_open(&casehandle, tmparray.data(), NULL, O_WRONLY | O_CREAT, 0644, TAR_GNU);
    tar_append_tree(casehandle, tmparray2.data(), tmparray3.data());
    tar_close(casehandle);
    std::remove(oldtmptar.toStdString().c_str());
    // END TAR METHOD

    TAR* tarhand;
    tar_open(&tarhand, tmparray.data(), NULL, O_RDONLY, 0644, TAR_GNU);
    tar_extract_all(tarhand, tmparray2.data());
    tar_close(tarhand);
 */ 
int main(int argc, char* argv[])
{
    std::string imagepath;
    std::string logpath;
    std::vector<std::filesystem::path> filevector;
    filevector.clear();

    if(argc == 1)
    {
	ShowUsage(0);
	return 1;
    }

    int i;
    while((i=getopt(argc, argv, "chrV")) != -1)
    {
        switch(i)
        {
            case 'h':
                ShowUsage(0);
                return 1;
            case 'V':
                ShowUsage(1);
                return 1;
	    case 'c':
		printf("Calculate hash and store in logical image.\n");
		break;
	    case 'r':
		printf("Set  recursive mode for directories.\n");
        }
    }
    // get all the input strings... then run parse directory to get the list of files...
    for(int i=optind; i < argc; i++)
    {
        filevector.push_back(std::filesystem::canonical(argv[i]));
    }
    // when i replace vector with filelist, this works to get the device for reach file, so i can determine the
    // filesystem and then get the forensic properties for the file.
    for(int i=0; i < filevector.size(); i++)
    {
	std::ifstream mounts{"/proc/mounts"};
	std::string mntpt;
	std::string device;
	std::string fileinfo;
	while(mounts >> device >> mntpt)
	{
	    std::size_t found = filevector.at(i).string().find(mntpt);
	    if(found != std::string::npos)
	    {
		fileinfo = filevector.at(i).string() + " file's mountpoint " + mntpt + " and subsequent device " + device;
	    }
	}
	std::cout << fileinfo << "\n";
    }


    /*
        devicepath = argv[1];
        std::string filestr = argv[2];
        std::size_t found = filestr.find_last_of("/");
        std::string pathname = filestr.substr(0, found);
        std::string filename = filestr.substr(found+1);
        std::filesystem::path initpath = std::filesystem::canonical(pathname + "/");
        imagepath = initpath.string() + "/" + filename + ".wfi";
        logpath = imagepath + ".log";
        if(devicepath.empty())
        {
            ShowUsage(0);
            return 1;
        }
        if(imagepath.empty())
        {
            ShowUsage(0);
            return 1;
        }

        int64_t totalbytes = 0;
        int16_t sectorsize = 0;
        int64_t curpos = 0;
        int64_t errcnt = 0;
        int infile = open(devicepath.c_str(), O_RDONLY | O_NONBLOCK);
	FILE* fin = NULL;
	FILE* fout = NULL;
        FILE* filelog = NULL;
        filelog = fopen(logpath.c_str(), "w+");
        if(filelog == NULL)
        {
            printf("Error opening log file.\n");
            return 1;
        }

        if(infile >= 0)
        {
            ioctl(infile, BLKGETSIZE64, &totalbytes);
	    ioctl(infile, BLKSSZGET, &sectorsize);
            close(infile);
	    fin = fopen_orDie(devicepath.c_str(), "rb");
	    fout = fopen_orDie(imagepath.c_str(), "wb");

	    wfimd.skipframeheader = 0x184d2a5f;
            wfimd.skipframesize = 256;
	    wfimd.sectorsize = sectorsize;
            wfimd.reserved  = 0x0;
	    wfimd.totalbytes = totalbytes;

            time_t starttime = time(NULL);
            char dtbuf[35];
            fprintf(filelog, "wombatimager v0.1 zstd Compressed Raw Forensic Image started at: %s\n", GetDateTime(dtbuf));
            fprintf(filelog, "\nSource Device\n");
            fprintf(filelog, "-------------\n");

            // GET UDEV DEVICE PROPERTIES FOR LOG...
            // !!!!! THIS ONLY GET PARENT DEVICES, DOESN'T WORK FOR PARTITIONS OF PARENT DEVICES...
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
                    if(strcmp(tmp, devicepath.c_str()) == 0)
                    {
                        fprintf(filelog, "Device:");
                        const char* devvendor = udev_device_get_property_value(dev, "ID_VENDOR");
                        if(devvendor != NULL)
                            fprintf(filelog, " %s", devvendor);
                        const char* devmodel = udev_device_get_property_value(dev, "ID_MODEL");
                        if(devmodel != NULL)
                            fprintf(filelog, " %s", devmodel);
                        const char* devname = udev_device_get_property_value(dev, "ID_NAME");
                        if(devname != NULL)
                            fprintf(filelog, " %s", devname);
                        fprintf(filelog, "\n");
                        tmp = udev_device_get_devnode(dev);
                        fprintf(filelog, "Device Path: %s\n", tmp);
                        tmp = udev_device_get_property_value(dev, "ID_SERIAL_SHORT");
                        fprintf(filelog, "Serial Number: %s\n", tmp);
                        fprintf(filelog, "Size: %u bytes\n", totalbytes);
                        fprintf(filelog, "Sector Size: %u bytes\n", sectorsize);
                    }
                }
                udev_device_unref(dev);
            }
            udev_enumerate_unref(enumerate);
            udev_unref(udev);
            
	    fseek(fin, 0, SEEK_SET);
	    fseek(fout, 0, SEEK_SET);

            // BLAKE3_OUT_LEN IS 32 BYTES LONG
            uint8_t srchash[BLAKE3_OUT_LEN];
            uint8_t wfihash[BLAKE3_OUT_LEN];

	    blake3_hasher srchasher;
	    blake3_hasher_init(&srchasher);

	    // USE ZSTD STREAM COMPRESSION
	    size_t bufinsize = ZSTD_CStreamInSize();
	    void* bufin = malloc_orDie(bufinsize);
	    size_t bufoutsize = ZSTD_CStreamOutSize();
	    void* bufout = malloc_orDie(bufoutsize);
	    size_t writecount = 0;

	    ZSTD_CCtx* cctx = ZSTD_createCCtx();
	    CHECK(cctx != NULL, "ZSTD_createCCtx() failed");

	    size_t toread = bufinsize;
	    for(;;)
	    {
		size_t read = fread_orDie(bufin, toread, fin);
		writecount = writecount + read;
		printf("Writing %llu of %llu bytes\r", writecount, totalbytes);
		fflush(stdout);

		blake3_hasher_update(&srchasher, bufin, read);

		int lastchunk = (read < toread);
		ZSTD_EndDirective mode = lastchunk ? ZSTD_e_end : ZSTD_e_continue;
		ZSTD_inBuffer input = { bufin, read, 0 };
		int finished;
		do
		{
		    ZSTD_outBuffer output = { bufout, bufoutsize, 0 };
		    size_t remaining = ZSTD_compressStream2(cctx, &output, &input, mode);
		    CHECK_ZSTD(remaining);
		    fwrite_orDie(bufout, output.pos, fout);
		    finished = lastchunk ? (remaining == 0) : (input.pos == input.size);
		} while (!finished);
		CHECK(input.pos == input.size, "Impossible, zstd only returns 0 when the input is completely consumed");

		if(lastchunk)
		    break;
	    }
	    ZSTD_freeCCtx(cctx);

	    blake3_hasher_finalize(&srchasher, srchash, BLAKE3_OUT_LEN);
            memcpy(wfimd.devhash, srchash, BLAKE3_OUT_LEN);
	    // NEED TO WRITE SKIPPABLE FRAME CONTENT HERE
	    fwrite_orDie(&wfimd, sizeof(struct wfi_metadata), fout);
	    
	    time_t endtime = time(NULL);
            fprintf(filelog, "Wrote %llu out of %llu bytes\n", curpos, totalbytes);
            fprintf(filelog, "%llu blocks replaced with zeroes\n", errcnt);
            fprintf(filelog, "Forensic Image: %s\n", imagepath.c_str());
            fprintf(filelog, "Forensic Image finished at: %s\n", GetDateTime(dtbuf));
            fprintf(filelog, "Forensic Image created in: %f seconds\n\n", difftime(endtime, starttime));
            printf("\nForensic Image Creation Finished\n");
            for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
            {
                fprintf(filelog, "%02x", srchash[i]);
                printf("%02x", srchash[i]);
            }
            printf(" - BLAKE3 Source Device\n");
            fprintf(filelog, " - BLAKE3 Source Device\n");

	    fclose_orDie(fout);
	    fclose_orDie(fin);
	    free(bufin);
	    free(bufout);

	    if(verify == 1) // start verification
	    {
                fprintf(filelog, "Verification started at: %s\n", GetDateTime(dtbuf));
                printf("Verification Started\n");
                uint8_t forimghash[BLAKE3_OUT_LEN];
                blake3_hasher imghasher;
                blake3_hasher_init(&imghasher);
                
		fout = fopen_orDie(imagepath.c_str(), "rb");
		size_t bufinsize = ZSTD_DStreamInSize();
		void* bufin = malloc_orDie(bufinsize);
		size_t bufoutsize = ZSTD_DStreamOutSize();
		void* bufout = malloc_orDie(bufoutsize);

		ZSTD_DCtx* dctx = ZSTD_createDCtx();
		CHECK(dctx != NULL, "ZSTD_createDCtx() failed");

		size_t toread = bufinsize;
		size_t read;
		size_t lastret = 0;
		int isempty = 1;
		size_t readcount = 0;
		while( (read = fread_orDie(bufin, toread, fout)) )
		{
		    isempty = 0;
		    ZSTD_inBuffer input = { bufin, read, 0 };
		    while(input.pos < input.size)
		    {
			ZSTD_outBuffer output = { bufout, bufoutsize, 0 };
			size_t ret = ZSTD_decompressStream(dctx, &output, &input);
			CHECK_ZSTD(ret);
			blake3_hasher_update(&imghasher, bufout, output.pos);
			lastret = ret;
			readcount = readcount + output.pos;
			printf("Read %llu of %llu bytes\r", readcount, totalbytes);
			fflush(stdout);
		    }
		}
		printf("\nVerification Finished\n");

		if(isempty)
		{
		    printf("input is empty\n");
		    return 1;
		}

		if(lastret != 0)
		{
		    printf("EOF before end of stream: %zu\n", lastret);
		    exit(1);
		}
		ZSTD_freeDCtx(dctx);
		fclose_orDie(fout);
		free(bufin);
		free(bufout);

                blake3_hasher_finalize(&imghasher, forimghash, BLAKE3_OUT_LEN);

		for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
		{
		    fprintf(filelog, "%02x", srchash[i]);
		    printf("%02x", srchash[i]);
		}
		printf(" - BLAKE3 Source Device\n");
		fprintf(filelog, " - BLAKE3 Source Device\n");

                for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
                {
		    fprintf(filelog, "%02x", forimghash[i]);
                    printf("%02x", forimghash[i]);
                }
                printf(" - Forensic Image Hash\n");
                fprintf(filelog, " - Forensic Image Hash\n");
		printf("\n");
		if(memcmp(&srchash, &forimghash, BLAKE3_OUT_LEN) == 0)
		{
		    printf("Verification Successful\n");
		    fprintf(filelog, "Verification Successful\n");
		}
		else
		{
		    printf("Verification Failed\n");
		    fprintf(filelog, "Verification Failed\n");
		}
		printf("\n");
	    }
	}
	else
	{
	    printf("error opening device: %d %s\n", infile, errno);
	    return 1;
	}
	fclose(filelog);
    }
    else
    {
	ShowUsage(0);
	return 1;
    }
    */
    return 0;
}

// OLD WOMBATLOGICAL
/*
 *
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
*/

/*
 *void ParseDirectory(std::filesystem::path dirpath, std::vector<std::filesystem::path>* filelist, uint8_t isrelative)
{
    for(auto const& dir_entry : std::filesystem::recursive_directory_iterator(dirpath))
    {
	if(std::filesystem::is_regular_file(dir_entry))
        {
            if(isrelative)
                filelist->push_back(std::filesystem::relative(dir_entry, std::filesystem::current_path()));
            else
                filelist->push_back(dir_entry);
        }
    }
}

void HashFile(std::string filename, std::string whlfile)
{
    std::ifstream fin(filename.c_str());
    char tmpchar[65536];
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    while(fin)
    {
	fin.read(tmpchar, 65536);
	size_t cnt = fin.gcount();
	blake3_hasher_update(&hasher, tmpchar, cnt);
	if(!cnt)
	    break;
    }
    uint8_t output[BLAKE3_OUT_LEN];
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    std::stringstream ss;
    for(int i=0; i < BLAKE3_OUT_LEN; i++)
        ss << std::hex << (int)output[i]; 
    std::string srcmd5 = ss.str();
    std::string whlstr = srcmd5 + "," + filename + "\n";
    FILE* whlptr = NULL;
    whlptr = fopen(whlfile.c_str(), "a");
    fwrite(whlstr.c_str(), strlen(whlstr.c_str()), 1, whlptr);
    fclose(whlptr);
}

void CompareFile(std::string filename, std::map<std::string, std::string>* knownhashes, int8_t matchbool, uint8_t isdisplay)
{
    std::ifstream fin(filename.c_str());
    char tmpchar[65536];
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    while(fin)
    {
	fin.read(tmpchar, 65536);
	size_t cnt = fin.gcount();
	blake3_hasher_update(&hasher, tmpchar, cnt);
	if(!cnt)
	    break;
    }
    uint8_t output[BLAKE3_OUT_LEN];
    blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);
    std::stringstream ss;
    for(int i=0; i < BLAKE3_OUT_LEN; i++)
        ss << std::hex << (int)output[i]; 
    std::string srcmd5 = ss.str();
    
    std::string matchstring = filename;
    uint8_t hashash = knownhashes->count(srcmd5);
    if(matchbool == 0 && hashash == 1)
    {
	matchstring += " matches";
        if(isdisplay == 1)
            matchstring += " " + knownhashes->at(srcmd5);
        matchstring += ".\n";
	std::cout << matchstring;
    }
    if(matchbool == 1 && hashash == 0)
    {
	matchstring += " does not match known files.\n";
	std::cout << matchstring;
    }
}

void ShowUsage(int outtype)
{
    if(outtype == 0)
    {
        printf("Generates a Wombat Hash List (whl) or compare files against a hash list.\n\n");
        printf("Usage :\n");
        printf("\twombathasher [OPTIONS] files\n\n");
        printf("Options :\n");
	printf("-c <new list>\t: Create new hash list.\n");
	printf("-a <existing list>\t: Append to an existing hash list.\n");
	printf("-r\t: Recurse sub-directories.\n");
	printf("-k <file>\t: Compare computed hashes to known list.\n");
	printf("-m\t: Matching mode. Requires -k.\n");
	printf("-n\t: Negative (Inverse) matching mode. Requires -k.\n");
	printf("-w\t: Display which known file was matched, requires -m.\n");
	printf("-l\t: Print relative paths for filenames.\n");
        printf("-v\t: Prints Version information\n");
        printf("-h\t: Prints help information\n\n");
	printf("Arguments:\n");
	printf("files\t: Files to hash for comparison or to add to a hash list.\n");
    }
    else if(outtype == 1)
    {
        printf("wombathasher v0.3\n");
	printf("License CC0-1.0: Creative Commons Zero v1.0 Universal\n");
        printf("This software is in the public domain\n");
        printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
        printf("Written by Pasquale Rinaldi\n");
    }
};

int main(int argc, char* argv[])
{
    uint8_t isnew = 0;
    uint8_t isappend = 0;
    uint8_t isrecursive = 0;
    uint8_t isknown = 0;
    uint8_t ismatch = 0;
    uint8_t isnotmatch = 0;
    uint8_t isdisplay = 0;
    uint8_t isrelative = 0;

    std::string newwhlstr = "";
    std::string appendwhlstr = "";
    std::string knownwhlstr = "";
    std::filesystem::path newpath;
    std::filesystem::path appendpath;
    std::filesystem::path knownpath;

    std::vector<std::filesystem::path> filevector;
    filevector.clear();
    std::vector<std::filesystem::path> filelist;
    filelist.clear();
    std::map<std::string, std::string> knownhashes;
    knownhashes.clear();

    //unsigned int threadcount = std::thread::hardware_concurrency();
    //std::cout << threadcount << " concurrent threads are supported.\n";

    if(argc == 1 || (argc == 2 && strcmp(argv[1], "-h") == 0))
    {
	ShowUsage(0);
	return 1;
    }
    else if(argc == 2 && strcmp(argv[1], "-v") == 0)
    {
	ShowUsage(1);
	return 1;
    }
    else if(argc >= 3)
    {
        int i;
        while((i=getopt(argc, argv, "a:c:hk:lmnrvw")) != -1)
        {
            switch(i)
            {
                case 'a':
                    isappend = 1;
                    appendwhlstr = optarg;
                    break;
                case 'c':
                    isnew = 1;
                    newwhlstr = optarg;
                    break;
                case 'h':
                    ShowUsage(0);
                    return 1;
                    break;
                case 'k':
                    isknown = 1;
                    knownwhlstr = optarg;
                    break;
                case 'l':
                    isrelative = 1;
                    break;
                case 'm':
                    ismatch = 1;
                    break;
                case 'n':
                    isnotmatch = 1;
                    break;
                case 'r':
                    isrecursive = 1;
                    break;
                case 'v':
                    ShowUsage(1);
                    return 1;
                    break;
                case 'w':
                    isdisplay = 1;
                    break;
                default:
                    printf("unknown option: %s\n", optarg);
                    return 1;
                    break;
            }
        }
        for(int i=optind; i < argc; i++)
        {
            filevector.push_back(std::filesystem::canonical(argv[i]));
        }
	if(isnew)
        {
	    newpath = std::filesystem::absolute(newwhlstr);
        }
	if(isappend)
        {
	    appendpath = std::filesystem::absolute(appendwhlstr);
        }
	if(isknown)
        {
	    knownpath = std::filesystem::absolute(knownwhlstr);
        }
        // CHECK FOR INCOMPATIBLE OPTIONS
	if(isnew && isappend)
	{
	    printf("You cannot create a new and append to an existing wombat hash list (whl) file.\n");
	    return 1;
	}
	if(ismatch && !isknown || ismatch && isnotmatch || isnotmatch && !isknown)
	{
	    printf("(Not) Match requires a known whl file, cannot match/not match at the same time\n");
	    return 1;
	}
	if(isnew && std::filesystem::exists(newwhlstr))
	{
	    printf("Wombat Hash List (whl) file %s already exists. Cannot Create, try to append (-a)\n", newwhlstr.c_str());
	    return 1;
	}
	if(isappend && !std::filesystem::exists(appendwhlstr))
	{
	    printf("Wombat Hash List (whl) file %s does not exist. Cannot Append, try to create (-c)\n", appendwhlstr.c_str());
	    return 1;
	}
	if(isknown && !std::filesystem::exists(knownwhlstr))
	{
	    printf("Known wombat hash list (whl) file %s does not exist\n", knownwhlstr.c_str());
	    return 1;
	}

	for(int i=0; i < filevector.size(); i++)
	{
	    if(std::filesystem::is_regular_file(filevector.at(i)))
	    {
                if(isrelative)
                    filelist.push_back(std::filesystem::relative(filevector.at(i), std::filesystem::current_path()));
                else
                    filelist.push_back(filevector.at(i));
	    }
	    else if(std::filesystem::is_directory(filevector.at(i)))
	    {
		if(isrecursive)
		    ParseDirectory(filevector.at(i), &filelist, isrelative);
		else
		    printf("Directory %s skipped. Use -r to recurse directory\n", filevector.at(i).c_str());
	    }
	}
    }
    // GOT THE LIST OF FIlES (filelist), NOW I NEED TO HASH AND HANDLE ACCORDING TO OPTIONS 
    if(isnew || isappend)
    {
        std::string whlstr;
        if(isnew) // create a new whl file
        {
            whlstr = newpath.string();
            std::size_t found = whlstr.rfind(".whl");
            if(found == -1)
                whlstr += ".whl";
        }
        if(isappend) // append to existing whl file
            whlstr = appendpath.string();
	for(int i=0; i < filelist.size(); i++)
	{
	    std::thread tmp(HashFile, filelist.at(i).string(), whlstr);
	    tmp.join();
	}
    }
    if(isknown)
    {
        // READ KNOWN HASH LIST FILE INTO A MAP
	std::string matchstring = "";
        std::ifstream knownstream;
        knownstream.open(knownpath.string(), std::ios::in);
        std::string tmpfile;
        while(std::getline(knownstream, tmpfile))
	{
	    std::size_t found = tmpfile.find(",");
	    std::string tmpkey = tmpfile.substr(0, found);
	    std::string tmpval = tmpfile.substr(found+1);
	    knownhashes.insert(std::pair<std::string, std::string>(tmpkey, tmpval));
	    //std::cout << tmpkey << " | " << tmpval << "\n";
	}
        knownstream.close();
	int8_t matchbool = -1;
	if(ismatch)
	    matchbool = 0;
	if(isnotmatch)
	    matchbool = 1;

	for(int i=0; i < filelist.size(); i++)
	{
	    std::thread tmp(CompareFile, filelist.at(i).string(), &knownhashes, matchbool, isdisplay);
	    tmp.join();
	}
    }

    return 0;
}
*/
