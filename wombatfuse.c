#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "common.h"
#include <zstd.h>

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>

struct wfi_metadata
{
    uint32_t skipframeheader; // skippable frame header
    uint32_t skipframesize; // skippable frame content size (not including header and this size
    uint16_t sectorsize; // raw forensic image sector size
    int64_t totalbytes; // raw forensic image total size
    char casenumber[24]; // 24 character string
    char evidencenumber[24]; // 24 character string
    char examiner[24]; // 24 character string
    char description[128]; // 128 character string
    uint8_t devhash[32]; // blake3 source hash
} wfimd;

static const char* wfistr = NULL;
static off_t rawsize = 0;

void ShowUsage(int outtype)
{
    if(outtype == 0)
    {
        printf("Loop mount the wombat forensic image IMAGE_FILE as a raw forensic image at the designated MOUNT PATH.\n\n");
        printf("Usage :\n");
        printf("\twombatmount IMAGE_FILE MOUNT_PATH\n\n");
        printf("IMAGE_FILE\t: Wombat forensic image file name.\n");
        printf("MOUNT_PATH\t: Mount point to mount the loop mounted raw forensic image.\n");
        printf("Arguments :\n");
        printf("-v\t: Prints Version information\n");
        printf("-h\t: Prints help information\n\n");
        printf("Example Usage :\n");
        printf("wombatmount item1.wfi /mnt/wfi/\n");
    }
    else if(outtype == 1)
    {
        printf("wombatmount v0.1\n");
	printf("License CC0-1.0: Creative Commons Zero v1.0 Universal\n");
        printf("This software is in the public domain\n");
        printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
        printf("Written by Pasquale Rinaldi\n");
    }
};


static int wombat_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi)
{

    //(void) fi;
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0)
    {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else if(strcmp(path, "/wfi.dd") == 0)
    {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_size = rawsize;
    }
    //else
    //    res = -ENOENT;

    return res;
}

static int wombat_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
    (void) offset;
    //(void) fi;
    (void) flags;

    if (strcmp(path, "/") != 0)
            return -ENOENT;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);
    filler(buf, "wfi.dd", NULL, 0, 0);

    return 0;
}

static int wombat_open(const char *path, struct fuse_file_info *fi)
{
    if(strcmp(path, "/wfi.dd") != 0)
            return -ENOENT;


    return 0;
}

static int wombat_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    (void)fi;

    FILE* fout = NULL;
    fout = fopen_orDie(wfistr, "rb");
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

    uint64_t indxstart = offset / bufoutsize;
    uint64_t relpos = offset - (indxstart * bufoutsize);
    uint64_t startingblock = offset / bufoutsize;
    uint64_t endingblock = (offset + size) / bufoutsize;
    int posodd = (offset + size) % bufoutsize;
    if(posodd > 0)
        endingblock++;
    size_t tmpbufsize = bufoutsize * (endingblock - startingblock + 1);
    char* tmpbuffer = malloc_orDie(tmpbufsize);

    int blkcnt = 0;
    int bufblkoff = 0;
    size_t readcount = 0;
    size_t outcount = 0;

    while( (read = fread_orDie(bufin, toread, fout)) )
    {
        readcount = readcount + read;
        isempty = 0;

        ZSTD_inBuffer input = { bufin, read, 0 };
        while(input.pos < input.size)
        {
            ZSTD_outBuffer output = { bufout, bufoutsize, 0 };
            size_t ret = ZSTD_decompressStream(dctx, &output, &input);
            if(blkcnt >= startingblock && blkcnt <= endingblock)
            {
                memcpy(tmpbuffer+(bufblkoff*bufoutsize), bufout, bufoutsize);
                bufblkoff++;
            }
            outcount = outcount + output.pos;
            blkcnt++;

            CHECK_ZSTD(ret);
            lastret = ret;
        }
    }
    memcpy(buf, tmpbuffer+relpos, size);

    if(isempty)
    {
        return 0;
    }
    if(lastret != 0)
    {
        return 0;
    }
    ZSTD_freeDCtx(dctx);
    fclose_orDie(fout);
    free(bufin);
    free(bufout);
    free(tmpbuffer);

    return size;
}

static const struct fuse_operations wombat_oper = {
	.getattr	= wombat_getattr,
	.open		= wombat_open,
	.read		= wombat_read,
	.readdir	= wombat_readdir,
};

int main(int argc, char* argv[])
{
    if(argc == 1 || (argc == 2 && strcmp(argv[1], "-h") == 0))
    {
	ShowUsage(0);
	return 1;
    }
    else if(strcmp(argv[1], "-v") == 0)
    {
        ShowUsage(1);
        return 1;
    }
    else if(argc == 3)
    {
        
        
        printf("command run: %s %s %s\n", argv[0], argv[1], argv[2]);
        
	char wfistr2[256];
	realpath(argv[1], wfistr2);
	printf("wfistr: \"%s\"\n", wfistr2);
	
	wfistr = malloc(sizeof(char)*strlen(wfistr2));
	wfistr = wfistr2;

        // get wfimd.totalbytes
        FILE* imgfile = NULL;
        imgfile = fopen(argv[1], "rb");
        fseek(imgfile, 0, SEEK_END);
        fseek(imgfile, -256, SEEK_CUR);
        fread(&wfimd, sizeof(struct wfi_metadata), 1, imgfile);
        fclose(imgfile);
        rawsize = wfimd.totalbytes;
        printf("totalbytes %ld\n", rawsize);


        char** fargv = NULL;
        fargv = (char**)calloc(2, sizeof(char*));
        int fargc = 2;
        fargv[0] = argv[1];
        fargv[1] = argv[2];
        struct fuse_args fuseargs = FUSE_ARGS_INIT(fargc, fargv);

        int ret;

        ret = fuse_main(fuseargs.argc, fuseargs.argv, &wombat_oper, NULL);
        
        fuse_opt_free_args(&fuseargs);

        return ret;

        return 0;
    }
    else
    {
        ShowUsage(0);
        return 1;
    }

    return 0;
}
