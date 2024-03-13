#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "walafus/filesystem.h"
#include "walafus/wltg_packer.h"
#include "walafus/wltg_reader.h"

#include "fusepp/Fuse.h"
#include "fusepp/Fuse-impl.h"

class WombatFileSystem : public Fusepp::Fuse<WombatFileSystem>
{
    public:
	WombatFileSystem() {};
	~WombatFileSystem() {};

	static int getattr(const char*, struct stat*, struct fuse_file_info*)
	{
	    int res = 0;

	    return res;
	};

	static int readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags)
	{

	    return 0;
	};

	static int open(const char* path, struct fuse_file_info* fi)
	{

	    return 0;
	};

	static int read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
	{

	    return size;
	};
};

void ShowUsage(int outtype)
{
    if(outtype == 0)
    {
        printf("Write raw forensic image contents to stdout.\n\n");
        printf("Usage :\n");
        printf("\twombatread IMAGE_FILE\n\n");
        printf("IMAGE_FILE\t: the file name for the wombat forensic image.\n");
        printf("Arguments :\n");
        printf("-v\t: Prints version information\n");
        printf("-h\t: Prints help information\n\n");
        printf("Example Usage :\n");
        printf("wombatread item1.wfi\n");
    }
    else if(outtype == 1)
    {
        printf("wombatread v0.2\n");
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
    else if(strcmp(argv[1], "-v") == 0)
    {
	ShowUsage(1);
	return 1;
    }
    else if(argc == 3)
    {
	Filesystem wltgfilesystem;
	WltgReader pack_wltg(argv[1]);

	wltgfilesystem.add_source(&pack_wltg);

	std::string wltgimg = std::filesystem::path(argv[1]).filename().string();
	size_t found = wltgimg.rfind(".");
	std::string wltgrawimg = wltgimg.substr(0, found) + ".dd";
	std::string virtpath = "/" + wltgimg.substr(0, found) + "/" + wltgrawimg;
	
	std::unique_ptr<BaseFileStream> handle = wltgfilesystem.open_file_read(virtpath.c_str());
	if(!handle)
	{
	    std::cout << "failed to open file" << std::endl;
	    return 1;
	}

	FILE* fout = stdout;

	char buf[131072];
	uint64_t curoffset = 0;
	while(curoffset < handle->size())
	{
	    handle->seek(curoffset);
	    uint64_t bytesread = handle->read_into(buf, 131072);
	    curoffset += bytesread;
	    fwrite(buf, 1, bytesread, fout);
	}
	fclose(fout);

	return 0;
    }
}
