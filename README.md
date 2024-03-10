# Wombat Tools
A set of command line tools for working with the wombatforensics forensic image format. These custom images will be integrated into wombatforensics which will be able to read and parse the images as well as create and verify them.

The wombat forensic image (wfi) will be a walafus read only zstd compressed file system. The wfi image file contains the raw forensic image as well as the log file from the creation of the image and the info file which contains the forensic propertis such as case number, examiner, evidence number, description, and blake3 hashes for the source device. If verification was enabled, the forensic image hash and the verification status will be included.

The wombat logical image (wli), will be a zstd compressed tar file with the forensic metadata stored somewhere as yet to be determined, so the wli file will be 100% interoprable with tar and fuse mounting tools based on libarchive.

## Version 0.2 Tools

* 
