# Wombat Tools
A set of command line tools for working with the wombatforensics forensic image format. These custom images will be integrated into wombatforensics which will be able to read and parse the images as well as create and verify them.

* A command line forensic imager using Qt5 for serialization, lz4 for compression, blake3 for hashing, and udev to get the device information.
* A command line verification tool .
* A command line info tool to provide information about the wombat forensic and wombat logical image.
* A command line logical imager using Qt5 for serialization, lz4 for compression, blake3 for hashing.
* A command line export tool to export the files contained in the wombat logical image.
* A command line fuse tool to fuse mount just the uncompressed raw dd image.
* The fuse tool will allow other forensic tools to parse the image such as the sleuthkit or xways, adlab, etc...
