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
        printf("Print the contents for a wombat forensic and wombat logical image.\n\n");
        printf("Usage :\n");
        printf("\twombatlist IMAGE_FILE\n\n");
        printf("IMAGE_FILE\t: the file name for the wombat forensic or wombat logical image.\n");
        printf("Arguments :\n");
        printf("-v\t: Prints version information\n");
        printf("-h\t: Prints help information\n\n");
        printf("Example Usage :\n");
        printf("wombatlist item1.wfi\n");
    }
    else if(outtype == 1)
    {
        printf("wombatlog v0.2\n");
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
	    Filesystem wltgfilesystem;
	    WltgReader pack_wltg(argv[1]);

	    wltgfilesystem.add_source(&pack_wltg);

	    std::string wltgimg = std::filesystem::path(argv[1]).filename().string();
	    size_t found = wltgimg.rfind(".");
	    //std::string wltgrawimg = wltgimg.substr(0, found) + ".dd";
	    //std::string wltglog = wltgimg.substr(0, found) + ".log";
	    //std::string wltginfo = wltgimg.substr(0, found) + ".info";
	    std::string virtimg = "/" + wltgimg.substr(0, found) + "/" + wltgimg.substr(0, found) + ".dd";
	    std::string virtlog = "/" + wltgimg.substr(0, found) + "/" + wltgimg.substr(0, found) + ".log";
	    std::string virtinfo = "/" + wltgimg.substr(0, found) + "/" + wltgimg.substr(0, found) + ".info";
	    
	    std::unique_ptr<BaseFileStream> imghandle = wltgfilesystem.open_file_read(virtimg.c_str());
	    if(!imghandle)
	    {
		std::cout << "failed to open file" << std::endl;
		return 1;
	    }
	    std::unique_ptr<BaseFileStream> loghandle = wltgfilesystem.open_file_read(virtlog.c_str());
	    if(!loghandle)
	    {
		std::cout << "failed to open file" << std::endl;
		return 1;
	    }
	    std::unique_ptr<BaseFileStream> infohandle = wltgfilesystem.open_file_read(virtinfo.c_str());
	    if(!infohandle)
	    {
		std::cout << "failed to open file" << std::endl;
		return 1;
	    }
	    std::string headerstring = "Contents for the Forensic Image: " + wltgimg;
	    std::cout << headerstring << std::endl;
	    for(int i=0; i < headerstring.size(); i++)
		std::cout << "-";
	    std::cout << std::endl;
	    std::cout << virtimg << "   | " << imghandle->size() << " bytes" << std::endl;
	    std::cout << virtlog << "  | " << loghandle->size() << " bytes" << std::endl;
	    std::cout << virtinfo << " | " << infohandle->size() << " bytes" << std::endl;

	    //FILE* fout = stdout;

	    //char buf[handle->size()];
	    //uint64_t bytesread = handle->read_into(buf, handle->size());
	    //fwrite(buf, 1, bytesread, fout);
	    //fclose(fout);

	    return 0;
	}
    }
}
