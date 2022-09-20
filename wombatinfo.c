#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

void ShowUsage(int outtype)
{
    if(outtype == 0)
    {
        printf("Print metadata information for a wombat forensic and wombat logical image.\n\n");
        printf("Usage :\n");
        printf("\twombatinfo IMAGE_FILE\n\n");
        printf("IMAGE_FILE\t: the file name for the wombat forensic or wombat logical image.\n");
        printf("Arguments :\n");
        printf("-v\t: Prints version information\n");
        printf("-h\t: Prints help information\n\n");
        printf("Example Usage :\n");
        printf("wombatinfo item1.wfi\n");
    }
    else if(outtype == 1)
    {
        printf("wombatinfo v0.1\n");
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
	    fseek(imgfile, -256, SEEK_CUR);
	    fread(&wfimd, sizeof(struct wfi_metadata), 1, imgfile);
	    fclose(imgfile);
	    printf("\nwombatinfo v0.1\n\n");
	    printf("Raw Media Size:  %llu bytes\n", wfimd.totalbytes);
	    printf("Case Number:\t%s\n", wfimd.casenumber);
	    printf("Examiner:\t\t%s\n", wfimd.examiner);
	    printf("Evidence Number: %s\n", wfimd.evidencenumber);
	    printf("Description:\t %s\n", wfimd.description);
	    printf("BLAKE3 Hash:\t ");
	    for(size_t i=0; i < 32; i++)
		printf("%02x", wfimd.devhash[i]);
	    printf("\n");

	    return 0;
	}
    }
}
