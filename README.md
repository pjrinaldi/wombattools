# wombatimagers
* a custom command line forensic imager and logical imager using zstd for compression and blake3 for verification hashing. These custom images will be integrated into wombatforensics which will be able to read and parse the images as well as create and verify them.
* I will also work on building a command line verification tool and maybe a command line fuse tool to fuse mount just the uncompressed raw dd image.
* The fuse tool will allow other forensic tools to parse the image such as the sleuthkit or xways, adlab, etc...
