#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "walafus/filesystem.h"
#include "walafus/wltg_packer.h"
#include "walafus/wltg_reader.h"

#include "fusepp/Fuse.h"
#include "fusepp/Fuse-impl.h"

static const char* rootpath = NULL;
static const char* imgpath = NULL;
static const char* logpath = NULL;
static const char* infopath = NULL;
static off_t imgsize = 0;
static off_t logsize = 0;
static off_t infosize = 0;
static const char* vimg = NULL;
static const char* vlog = NULL;
static const char* vinf = NULL;
std::unique_ptr<BaseFileStream> handle = NULL;

class WombatFileSystem : public Fusepp::Fuse<WombatFileSystem>
{
    public:
	WombatFileSystem() {};
	~WombatFileSystem() {};

	static int getattr(const char* path, struct stat *stbuf, struct fuse_file_info*)
	{
	    int res = 0;
	    memset(stbuf, 0, sizeof(struct stat));
	    if(strcmp(path, "/") == 0)
	    {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	    }
	    else if(strcmp(path, imgpath) == 0)
	    {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = imgsize;
	    }
	    else if(strcmp(path, logpath) == 0)
	    {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = logsize;
	    }
	    else if(strcmp(path, infopath) == 0)
	    {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = infosize;
	    }
	    else
		res = -ENOENT;

	    return res;
	};

	static int readdir(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags)
	{
	    if(strcmp(path, "/") == 0)
	    {
		filler(buf, ".", NULL, 0, FUSE_FILL_DIR_PLUS);
		filler(buf, "..", NULL, 0, FUSE_FILL_DIR_PLUS);
		filler(buf, imgpath + 1, NULL, 0, FUSE_FILL_DIR_PLUS);
		filler(buf, logpath + 1, NULL, 0, FUSE_FILL_DIR_PLUS);
		filler(buf, infopath + 1, NULL, 0, FUSE_FILL_DIR_PLUS);
	    }
	    else
		return -ENOENT;
	    /*
	    if(path != rootpath)
		return -ENOENT;
	    */

	    return 0;
	};

	static int open(const char* path, struct fuse_file_info* fi)
	{
	    if(strcmp(path, imgpath) == 0)
	    {
	    }
	    else if(strcmp(path, logpath) == 0)
	    {
		//Filesystem wltgfilesystem;
		//WltgReader pack_wltg(rootpath);
		//wltgfilesystem.add_source(&pack_wltg);
		//handle = wltgfilesystem.open_file_read(vlog);
		//if(!handle)
		//    return -ENOENT;
	    }
	    else if(strcmp(path, infopath) == 0)
	    {
	    }
	    else
		return -ENOENT;

	    if((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	    return 0;
	};

	static int read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* fi)
	{
	    Filesystem wltgfilesystem;
	    WltgReader pack_wltg(rootpath);
	    /*
	    wltgfilesystem.add_source(&pack_wltg);
	    */

	    /*
	    std::string wltgimg = std::filesystem::path(argv[1]).filename().string();
	    size_t found = wltgimg.rfind(".");
	    std::string wltgrawimg = wltgimg.substr(0, found) + ".dd";
	    std::string virtpath = "/" + wltgimg.substr(0, found) + "/" + wltgrawimg;
	    */
	    
	    /*
	    uint64_t bufsize = 131072;
	    if(size < bufsize)
		bufsize = size;
	    char tmpbuf[bufsize];
	    
	    */
	    //char tmpbuf[size];
	    //char buf[131072];
	    if(strcmp(path, imgpath) == 0)
	    {
		/*
		std::unique_ptr<BaseFileStream> handle = wltgfilesystem.open_file_read(vimg);
		if(!handle)
		    return -ENOENT;
		uint64_t curoffset = offset;
		uint64_t totalsize = handle->size();
		if(size < handle->size())
		    totalsize = size;
		while(curoffset < totalsize)
		{
		    handle->seek(curoffset);
		    uint64_t bytesread = handle->read_into(buf, 131072);
		    curoffset += bytesread;
		}
		//handle->seek(offset);
		//uint64_t bytesread = handle->read_into(tmpbuf, size);
		*/
	    }
	    else if(strcmp(path, logpath) == 0)
	    {
		/*
		std::unique_ptr<BaseFileStream> handle = wltgfilesystem.open_file_read(vlog);
		if(!handle)
		    return -ENOENT;
		*/
		/*
		char tmpbuf[handle->size()];
		memset(tmpbuf, 0, handle->size());
		//char buf[handle->size()];
		uint64_t bytesread = handle->read_into(tmpbuf, handle->size());
		for(int i=offset; i < size; i++)
		    buf[i] = tmpbuf[i];

		//fwrite(buf, 1, bytesread, fout);
		//fclose(fout);
		//handle->seek(offset);
		//uint64_t bytesread = handle->read_into(tmpbuf, size);
		//std::stringstream ss;
		//for(int i=0; i < size; i++)
		//    buf[i] = tmpbuf[i];

		/*
		FILE* tmpstrm;
		tmpstrm  = fmemopen(buf, size, "w");
		fwrite(tmpbuf, 1, size, tmpstrm);
		fclose(tmpstrm);
		*/
		/*
		char* buffer = new char[255];

		  {
		    std::ifstream file("myfile.txt");
		    read_into_buffer(file, buffer, 10); // reads 10 bytes from the file
		  }
		  {
		    std::string s("Some very long and useless message and ...");
		    std::stringstream stream(s);
		    read_into_buffer(stream, buffer, 10); // reads 10 bytes from the string
		 */ 
		/*
		std::unique_ptr<BaseFileStream> handle = wltgfilesystem.open_file_read(vlog);
		uint64_t totalsize = handle->size();
		if(size < handle->size())
		    totalsize = size;
		uint64_t curoffset = offset;
		while(curoffset < totalsize)
		{
		    handle->seek(curoffset);
		    uint64_t bytesread = handle->read_into(tmpbuf, bufsize);
		    buf = &tmpbuf[bytesread - curoffset];
		    curoffset += bytesread;

		}
		*/
		//if(!handle)
		//    return -ENOENT;
		//handle->seek(offset);
		//uint64_t bytesread = handle->read_into(tmpbuf, size);
	    }
	    else if(strcmp(path, infopath) == 0)
	    {
		/*
		std::unique_ptr<BaseFileStream> handle = wltgfilesystem.open_file_read(vinf);
		if(!handle)
		    return -ENOENT;
		//handle->seek(offset);
		//uint64_t bytesread = handle->read_into(tmpbuf, size);
		*/
	    }

	    //buf = &tmpbuf[0];

	    //FILE* fout = stdout;

	    //memcpy(buf, tmpbuffer+relpos, size);
	    //char buf[131072];
	    //uint64_t curoffset = 0;
	    //while(curoffset < handle->size())
	    //{
		//curoffset += bytesread;
		//fwrite(buf, 1, bytesread, fout);
	    //}
	    //fclose(fout);

	    /*
	if (path != hello_path)
		return -ENOENT;

	size_t len;
	len = hello_str.length();
	if ((size_t)offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str.c_str() + offset, size);
	} else
		size = 0;
	     */ 

	    return size;
	};
};

void ShowUsage(int outtype)
{
    if(outtype == 0)
    {
        printf("Mount the raw forensic image contents to a folder.\n\n");
        printf("Usage :\n");
        printf("\twombatmount IMAGE_FILE MOUNT_POINT\n\n");
        printf("IMAGE_FILE\t: the file name for the wombat forensic image.\n");
	printf("MOUNT_POINT\t: the empty directory where the wombat forensic image will be mounted.\n");
        printf("Arguments :\n");
        printf("-v\t: Prints version information\n");
        printf("-h\t: Prints help information\n\n");
        printf("Example Usage :\n");
        printf("wombatmount item1.wfi /mnt/\n");
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
	std::string wfipath = argv[1];
	std::string mntpath = argv[2];
	//std::cout << "wfipath: " << wfipath << " mntpath: " << mntpath << std::endl;
	std::string wltgimg = std::filesystem::path(argv[1]).filename().string();
	size_t found = wltgimg.rfind(".");
	//std::string proot = "/" + wltgimg.substr(0, found);
	rootpath = wfipath.c_str();
	std::cout << "root path: " << rootpath << std::endl;
	//rootpath = proot.c_str();
	std::string pimg = "/" + wltgimg.substr(0, found) + ".dd";
	std::string plog = "/" + wltgimg.substr(0, found) + ".log";
	std::string pinfo = "/" + wltgimg.substr(0, found) + ".info";
	//std::cout << "p's: " << pimg << " " << plog << " " << pinfo << std::endl;
	//std::cout << "root path: " << rootpath << std::endl;
	imgpath = pimg.c_str();
	//std::cout << "img path: " << imgpath << std::endl;
	logpath = plog.c_str();
	std::cout << "log path: " << logpath << std::endl;
	infopath = pinfo.c_str();
	//std::cout << "info path: " << infopath << std::endl;
	std::string virtimg = "/" + wltgimg.substr(0, found) + "/" + wltgimg.substr(0, found) + ".dd";
	std::string virtlog = "/" + wltgimg.substr(0, found) + "/" + wltgimg.substr(0, found) + ".log";
	std::string virtinfo = "/" + wltgimg.substr(0, found) + "/" + wltgimg.substr(0, found) + ".info";
	vimg = virtimg.c_str();
	vlog = virtlog.c_str();
	vinf = virtinfo.c_str();
	std::cout << "virtual logpath: " << vlog << std::endl; 
	
	Filesystem wltgfilesystem;
	WltgReader pack_wltg(argv[1]);
	wltgfilesystem.add_source(&pack_wltg);
	std::unique_ptr<BaseFileStream> imghandle = wltgfilesystem.open_file_read(virtimg.c_str());
	if(!imghandle)
	{
	    std::cout << "failed to open file" << std::endl;
	    return 1;
	}
	imgsize = imghandle->size();
	std::unique_ptr<BaseFileStream> loghandle = wltgfilesystem.open_file_read(virtlog.c_str());
	if(!loghandle)
	{
	    std::cout << "failed to open file" << std::endl;
	    return 1;
	}
	logsize = loghandle->size();
	std::unique_ptr<BaseFileStream> infohandle = wltgfilesystem.open_file_read(virtinfo.c_str());
	if(!infohandle)
	{
	    std::cout << "failed to open file" << std::endl;
	    return 1;
	}
	infosize = infohandle->size();

        char** fargv = NULL;
        fargv = (char**)calloc(2, sizeof(char*));
        int fargc = 2;
        fargv[0] = argv[1];
        fargv[1] = argv[2];

	WombatFileSystem wfs;
	int status = wfs.run(fargc, fargv);

	return status;
	/*
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
	*/
    }
}
