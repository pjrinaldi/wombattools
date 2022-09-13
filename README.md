# Wombat Tools
A set of command line tools for working with the wombatforensics forensic image format. These custom images will be integrated into wombatforensics which will be able to read and parse the images as well as create and verify them.

The wombat forensic image (wfi) will be a zstd compressed raw image with the metadata in a zstd skippable frame at the end of the file. This makes the wfi file act just like a zst file and regular zstd tools can be used to operate on the wfi file, same as a zst file.

The wombat logical image (wli), will be a zstd compressed tar file with the forensic metadata stored somewhere as yet to be determined, so the wli file will be 100 interoprable with tar and fuse mounting tools based on libarchive.

* A command line forensic imager zstd for compression, blake3 for hashing, and udev to get the device information. (v0.1)
* A command line verification tool. (v0.1)
* A command line info tool to provide information about the wombat forensic (v0.1) and wombat logical image. (TODO)
* A command line logical imager using zst for compression, blake3 for hashing, and tar for file/dir storage (TODO)
* A command line export tool to export the files contained in the wombat logical image. (TODO)
* A command line fuse tool to fuse mount just the uncompressed raw dd image. The fuse tool will allow other forensic tools to parse the image such as the sleuthkit or xways, adlab, etc...
