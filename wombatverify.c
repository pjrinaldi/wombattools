#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "blake3.h"
#include "common.h"
#include <zstd.h>

#define DTTMFMT "%F %T %z"
#define DTTMSZ 35

struct wfi_metadata
{
    uint32_t skipframeheader; // skippable frame header
    uint32_t skipframesize; // skippable frame content size (not including header and this size
    uint16_t sectorsize; // raw forensic image sector size
    int64_t reserved; // reserved
    int64_t totalbytes; // raw forensic image total size
    char casenumber[24]; // 24 character string
    char evidencenumber[24]; // 24 character string
    char examiner[24]; // 24 character string
    char description[128]; // 128 character string
    uint8_t devhash[32]; // blake3 source hash
} wfimd;

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
        printf("Verify the BLAKE3 hash for a wombat forensic or wombat logical image.\n\n");
        printf("Usage :\n");
        printf("\twombatinfo IMAGE_FILE\n\n");
        printf("IMAGE_FILE\t: the file name for the wombat forensic or wombat logical image.\n");
        printf("Arguments :\n");
        printf("-v\t: Prints Version information\n");
        printf("-h\t: Prints help information\n\n");
        printf("Example Usage :\n");
        printf("wombatverify item1.wfi\n");
    }
    else if(outtype == 1)
    {
        printf("wombatverify v0.1\n");
	printf("License CC0-1.0: Creative Commons Zero v1.0 Universal\n");
        printf("This software is in the public domain\n");
        printf("There is NO WARRANTY, to the extent permitted by law.\n\n");
        printf("Written by Pasquale Rinaldi\n");
    }
};

int main(int argc, char* argv[])
{
    if(argc == 1 || (argc == 2 && strcmp(argv[1], "-h") == 0))
    {
	ShowUsage(0);
	return 1;
    }
    else if(argc == 2)
    {
	if(strcmp(argv[1], "-v") == 0)
	{
	    ShowUsage(1);
	    return 1;
	}
	else
	{
	    FILE* imgfile = NULL;
	    imgfile = fopen(argv[1], "rb");
	    fseek(imgfile, 0, SEEK_END);
	    fseek(imgfile, -264, SEEK_CUR);
	    fread(&wfimd, sizeof(struct wfi_metadata), 1, imgfile);
	    fclose(imgfile);
	    

            time_t starttime = time(NULL);
            char dtbuf[35];
	    printf("Verification Started at %s\n", GetDateTime(dtbuf));

	    uint8_t forimghash[BLAKE3_OUT_LEN];
	    blake3_hasher imghasher;
	    blake3_hasher_init(&imghasher);
	    
	    FILE* fout = NULL;
	    fout = fopen_orDie(argv[1], "rb");
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
		    printf("Read %llu of %llu bytes\r", readcount, wfimd.totalbytes);
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
	    printf("Image Hash:\t   ");
	    for(size_t i=0; i < 32; i++)
		printf("%02x", wfimd.devhash[i]);
	    printf("\n");
	    printf("Verification Hash: ");
	    for(size_t i=0; i < BLAKE3_OUT_LEN; i++)
	    {
		printf("%02x", forimghash[i]);
	    }
	    printf("\n\n");
	    if(memcmp(&wfimd.devhash, &forimghash, BLAKE3_OUT_LEN) == 0)
		printf("Verification Successful\n");
	    else
		printf("Verification Failed\n");

	    return 0;
	}
    }
}
