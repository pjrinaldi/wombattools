#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "../common.h"
#include <zstd.h>

// ATTEMPT TO BUILD TEST CODE FOR THE FUSE WOMBAT_READ FUNCTION SO I CAN GET SOME
// ERROR RETURN VALUES AND DO SOME TESTING...


int main(int argc, char* argv[])
{
    if(argc == 1 || (argc == 2 && strcmp(argv[1], "-h") == 0))
    {
	//ShowUsage(0);
	return 1;
    }
    else if(strcmp(argv[1], "-v") == 0)
    {
        //ShowUsage(1);
        return 1;
    }
    else if(argc == 4)
    {        
        
        //printf("command run: %s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);

	char* buffer = getcwd(NULL, 0);
	if(buffer != NULL)
	    printf("%s \tLength: %zu\n", buffer, strlen(buffer));
	free(buffer);

	FILE* fout = NULL;
	fout = fopen_orDie(argv[1], "rb");
	size_t bufinsize = ZSTD_DStreamInSize();
	void* bufin = malloc_orDie(bufinsize);
	size_t bufoutsize = ZSTD_DStreamOutSize();
	void* bufout = malloc_orDie(bufoutsize);
	//printf("bufin size: %ld, bufout size: %ld\n", bufinsize, bufoutsize);

	ZSTD_DCtx* dctx = ZSTD_createDCtx();
	CHECK(dctx != NULL, "ZSTD_createDCtx() failed");

	char* offstr;
	char* sizstr;
	long curoff = strtol(argv[2], &offstr, 10);
	long bufsize = strtol(argv[3], &sizstr, 10);
	//printf("offset: %s | %ld\n", argv[2], curoff);
	//printf("size: %s | %ld\n", argv[3], bufsize);
	
	size_t toread = bufinsize;
	size_t read;
	size_t lastret = 0;
	int isempty = 1;

	//uint64_t blkcnt = 0;
	uint64_t indxstart = curoff / bufoutsize;
	//uint64_t posodd = curoff % bufoutsize;
	uint64_t relpos = curoff - (indxstart * bufoutsize);
        //printf("relative starting position: %ld\n", relpos);
        //uint8_t overlap = 0;
        // determine asked for block count
        //uint64_t blocktotal = bufsize / bufoutsize;
        //blocktotal++;
        //printf("number of blocks to contain asked for data %ld\n", blocktotal);
        // determine if content flows into next out buffer.
        /*
        if(curoff + bufsize > bufoutsize)
        {
            overlap = 1;
            blkcnt++;
        }
        */
        // NEED TO DETERMINE IF THE CONTENT FLOWS INTO THE NEXT BUFFER
        /*
         * working on attempting to determine the outbuf blocks necessary to cover the user requested offset and size...
         * so i need to get the offset, and determine the block, then get the (offset + size) and determine which block
         * that is in and then i can get the respective buffers and store them in a tmpbuffer big enough to store it all, 
         * so if it's 3 outbufs, i can create a tmpbuf big enough for it and then once i get the tmp buf, use the relative 
         * offset and size to return the user's offset+size request
         */ 
        uint64_t startingblock = curoff / bufoutsize;
        uint64_t endingblock = (curoff + bufsize) / bufoutsize;
        int posodd = (curoff + bufsize) % bufoutsize;
        if(posodd > 0)
            endingblock++;
        //printf("starting block: %ld | ending block: %ld\n", startingblock, endingblock);
        size_t tmpbufsize = bufoutsize * (endingblock - startingblock + 1);
        //printf("tmpbufsize: %ld\n", tmpbufsize);
        char* tmpbuffer = malloc_orDie(tmpbufsize);

        //printf("tmpbuffer size: %ld\n", bufoutsize & (endingblock - startingblock + 1));
        //tmpbuffer = (char*)malloc(sizeof(char)*(bufoutsize*(endingblock - startingblock + 1)));
        //printf("tmp buffer size: %ld\n", strlen(tmpbuffer));
        //frameindex = (uint64_t*)malloc(sizeof(uint64_t)*framecount);

        //overlap = relpos + bufsize - bufoutsize
        //uint64_t overlap = labs(bufoutsize - (relpos + bufsize)); // 510 + 10 = 520 which is 8 from next section
	//printf("blkcnt: %ld | indxstart: %ld\n", blkcnt, indxstart);
        // CREATE A BUFFER LARGE ENOUGH TO CONTAIN THE REQUIRED BLOCKS FROM THE DECOMPRESSED, SO IF WE NEED THREE, THEN STORE 3.
        // THEN I CAN JUST PULL THE RESPECTIVE CONTENT FROM THE RIGHT OFFSET AND SIZE FROM THIS TMP BUFFER
	//printf("posodd %ld | relpos: %ld\n", posodd, relpos);

        int blkcnt = 0;
        int bufblkoff = 0;
	size_t readcount = 0;
        size_t outcount = 0;
	char* buf = malloc_orDie(bufsize);
	//printf("buf length: %ld\n", strlen(buf));

	while( (read = fread_orDie(bufin, toread, fout)) )
	{
            readcount = readcount + read;
            //printf("read so far: %ld\n", readcount);
	    isempty = 0;
            //size_t outsize = 0;
            //size_t outpos = 0;

	    ZSTD_inBuffer input = { bufin, read, 0 };
	    while(input.pos < input.size)
	    {
                ZSTD_outBuffer output = { bufout, bufoutsize, 0 };
		size_t ret = ZSTD_decompressStream(dctx, &output, &input);
                if(blkcnt >= startingblock && blkcnt <= endingblock)
                {
                    //printf("cur block: %ld | cur tmpbuffer section: %ld\n", blkcnt, (bufblkoff*bufoutsize));
                    memcpy(tmpbuffer+(bufblkoff*bufoutsize), bufout, bufoutsize);
                    bufblkoff++;
                }
                /*
                if(curoff >= outcount)// && curoff + bufsize <= outcount + output.pos)
                {
                    printf("bufoutsize: %ld | offset: %ld | size: %ld | relpos: %ld | overlap: %d\n", bufoutsize, curoff, bufsize, relpos, overlap);
                    if(overlap == 0)
                        printf("the selected content is here.\n");
                    else
                        printf("part of the content is here, and part is in the next block\n");
                    //return 0;
                }
                */
                outcount = outcount + output.pos;
                blkcnt++;
                //printf("outcount: %ld\n", outcount);
		//printf("input pos: %ld | output pos: %ld\n", input.pos, output.pos);
		//printf("input size: %ld | output size: %ld\n", input.size, output.size);
                //outsize = output.size;
                //outpos = output.pos;
		CHECK_ZSTD(ret);
		lastret = ret;
	    }
		//if(curoff >= readcount && curoff+bufsize <= readcount + output.pos)
		//printf("blkcnt: %ld | indxstart: %ld\n", blkcnt, indxstart);
                /*
		if(blkcnt == indxstart)
		{
		    uint curbufsize = 0;
		    //printf("bufsize: %ld\n", bufsize);
		    //memcpy(buf, bufout+relpos, bufsize);
		    curbufsize = curbufsize + bufsize;
		    fwrite(bufout+relpos, sizeof(char), bufsize, stdout);

		    //printf("blkcnt: %ld\n", blkcnt);
		    //printf("read count: %ld\n", readcount);
		    //printf("input size: %ld | output size: %ld\n", input.pos, output.pos);
		    //printf("input size: %ld | output size: %ld\n", input.size, output.size);
		    //printf("buf size: %ld\n", strlen(buf));
		    //fflush(stdout);
		    // THIS WORKS TO STREAM THE WHOLE FILE TO STDOUT... JUST NEED TO TRUNCATE TO JUST WHAT I CALL
		    //fwrite(bufout, sizeof(char), output.pos, stdout);
		    //return bufsize;
		    return 0;
		}
                */
		// NEED TO MOVE THE IF(BLKCNT == INDXSTART) STUFF TO HERE... SO ALL INPUT HAS BEEN PUT INTO OUTPUT
		// THIS SHOULD WORK UNLESS, THE OUTPUT RELPOS + SIZE > BUFOUTSIZE, THEN I NEED TO PULL THE NEXT
		// BLOCK AND GET THE REMAINDER OF THE CONTENT...
	        //readcount = readcount + output.pos;
            //printf("outpos: %ld | outsize: %ld\n", outpos, outsize);
            /*
            if(blkcnt == indxstart)
            {
                //fwrite(bufout+relpos, sizeof(char), bufsize, stdout);
            }
            readcount = readcount + bufoutsize;
	    blkcnt++;
            */
	}
        fflush(stdout);
        fwrite(tmpbuffer+relpos, sizeof(char), bufsize, stdout);

	if(isempty)
	{
	    //printf("input is empty\n");
	    return 0;
	}
	if(lastret != 0)
	{
	    //printf("EOF before end of stream: %zu\n", lastret);
	    return 0;
	}
	ZSTD_freeDCtx(dctx);
	fclose_orDie(fout);
	free(bufin);
	free(bufout);
        free(tmpbuffer);
	free(buf);
	/*
	uint64_t blkcnt = 0;
	uint64_t indxstart = offset / bufoutsize;
	uint64_t posodd = offset % bufoutsize;
	uint64_t relpos = offset - (indxstart * bufoutsize);
	if(offset >= readcount && offset <= readcount + output.pos)
	{
	    if(posodd == 0)
		memcpy(buf, bufout, size);
	    else
		memcpy(buf, bufout+relpos, size);
	    return size;
	}
	readcount = readcount + output.pos;
	if(blkcnt == indxstart)
	{
	    memcpy(buf, bufout+relpos, size);
	    return size;
	}
	readcount = readcount + output.pos;
	blkcnt++;
	*/
        
        return 0;
    }
    else
    {
        //ShowUsage(0);
        return 1;
    }
}
/*
    // MIGHT BE ABLE TO CHEAT WITH ZSTDCAT AND PIPES...
    // I.E. ztdcat test.wfi | dd bs=1 skip=offset count=size

    int fd[2], nbytes;
    pid_t childpid;
    pipe(fd);
    char string[] = "Hello World!\n";
    char readbuffer[80];
    if((childpid == for()) == -1)
    {
        perror("fork");
        exit(1);
    }
    if(childpid == 0)
    {
        // child closes up input side to send data...
        close(fd[0]);
        write(fd[1], string, strlen(string)+1);
        exit(0);
    }
    else
    {
        // parent closes output side to receive data
        close(fd[1]);
        nbytes = read(fd[0], readbuffer, sizeof(readbuffer));
        printf("received string %s", readbuffer);
    }
    */

    /*
    // bypass test code...
    // sort of works
    char popenstr[256];
    //off_t offset = 1048576;
    //size_t size = 512;
    size_t cnt = size / 512;
    int j = snprintf(popenstr, sizeof(popenstr), "zstd -dcf %s | dd bs=512 skip=%ld count=%ld", wfistr, offset, cnt);
    //printf("popen str: %s\n", popenstr);
    FILE* pipefile;
    if((pipefile = popen(popenstr, "r")) == NULL)
    {
        perror("popen error");
    }
    //if(feof(pipefile) != 0)
    //while(offset < rawsize)
        fread(buf, 1, size, pipefile);
    pclose(pipefile);
    */

/*
    FILE* fout = NULL;
    fout = fopen_orDie(wfistr, "rb");

    size_t bufinsize = ZSTD_DStreamInSize();
    void* bufin = malloc_orDie(bufinsize);
    size_t bufoutsize = ZSTD_DStreamOutSize();
    void* bufout = malloc_orDie(bufoutsize);

    ZSTD_DCtx* dctx = ZSTD_createDCtx();
    CHECK(dctx != NULL, "ZSTD_createDCtx() failed");

    uint64_t blkcnt = 0;
    uint64_t indxstart = offset / bufoutsize;
    uint64_t posodd = offset % bufoutsize;
    uint64_t relpos = offset - (indxstart * bufoutsize);

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
            lastret = ret;
            //memcpy(buf, bufout, size);
	    
	    if(offset >= readcount && offset <= readcount + output.pos)
            {
                if(posodd == 0)
                    memcpy(buf, bufout, size);
                else
                    memcpy(buf, bufout+relpos, size);
                return size;
            }
            readcount = readcount + output.pos;

            if(blkcnt == indxstart)
            {
                memcpy(buf, bufout+relpos, size);
                return size;
            }
            readcount = readcount + output.pos;
            blkcnt++;
        }
    }

    if(isempty)
    {
        printf("input is empty\n");
        return 0;
    }

    if(lastret != 0)
    {
        printf("EOF before end of stream: %zu\n", lastret);
        return 0;
    }
    ZSTD_freeDCtx(dctx);
    fclose_orDie(fout);
    free(bufin);
    free(bufout);

    return size;
*/

	/*
        wfistr = argv[1];
        printf("wfistr: %s\n", wfistr);
        //wfistr = strndup(argv[1], strlen(argv[1]));
        //mntstr = strndup(argv[2], strlen(argv[2]));

        // get wfimd.totalbytes
        FILE* imgfile = NULL;
        imgfile = fopen(argv[1], "rb");
        fseek(imgfile, 0, SEEK_END);
        fseek(imgfile, -264, SEEK_CUR);
        fread(&wfimd, sizeof(struct wfi_metadata), 1, imgfile);
        fclose(imgfile);
        rawsize = wfimd.totalbytes;
        printf("totalbytes %ld\n", rawsize);

        //char* imgstr = NULL;
        //imgstr = strrchr(argv[1], '/');
        //if(imgstr == NULL)
        //    imgstr = argv[1];
        //strcat(imgstr, ".dd");
        //printf("imgstr: %s\n", imgstr);
        //char* rawfilename = NULL;
        //rawfilename = strndup(imgstr, strlen(imgstr));
        //printf("raw filename \"%s\"\n", rawfilename);
        //printf("totalbytes: %ld\n", wfimd.totalbytes);

        char** fargv = NULL;
        fargv = (char**)calloc(2, sizeof(char*));
        int fargc = 2;
        //fargv[0] = wfistr;
        fargv[0] = argv[1];
        fargv[1] = argv[2];
        struct fuse_args fuseargs = FUSE_ARGS_INIT(fargc, fargv);

        int ret;

        //printf("fuseargs: %s %s\n", fuseargs.argv[0], fuseargs.argv[1]);
        //fuse_opt_parse(NULL, NULL, NULL, NULL);
        ret = fuse_main(fuseargs.argc, fuseargs.argv, &wombat_oper, NULL);
        
        fuse_opt_free_args(&fuseargs);

        return ret;

        //printf("Fuse Mount Here\n");
	*/

