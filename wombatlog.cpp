#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "walafus/filesystem.h"
#include "walafus/wltg_packer.h"
#include "walafus/wltg_reader.h"

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
	    /*
	    Filesystem wltgfilesystem;
	    WltgReader pack_wltg(argv[1]);
	    //std::cout << "argv1: " << argv[1] << std::endl;

	    wltgfilesystem.add_source(&pack_wltg);

	    std::string wltgimg = std::filesystem::path(argv[1]).filename().string();
	    size_t found = wltgimg.rfind(".");
	    std::string wltgrawimg = wltgimg.substr(0, found) + ".dd";
	    std::string virtpath = "/" + wltgimg.substr(0, found) + "/" + wltgrawimg;
	    //std::cout << virtpath << std::endl;
	    
	    std::unique_ptr<BaseFileStream> handle = wltgfilesystem.open_file_read(virtpath.c_str());
	    if(!handle)
	    {
		std::cout << "failed to open file" << std::endl;
		return 1;
	    }
	    //std::cout << handle->size() << std::endl;

	    FILE* fout = stdout;
	    //fout = fopen(stdout, "w+");

	    char buf[131072];
	    uint64_t curoffset = 0;
	    while(curoffset < handle->size())
	    {
		handle->seek(curoffset);
		uint64_t bytesread = handle->read_into(buf, 131072);
		//std::cout << "1st 4 bytes read at curoffset: " << std::hex << (uint)buf[0] << (uint)buf[1] << (uint)buf[2] << (uint)buf[3] << std::dec << std::endl;
		curoffset += bytesread;
		fwrite(buf, 1, bytesread, fout);
	    }
	    fclose(fout);

	     */ 

	    return 0;
	}
    }
}
