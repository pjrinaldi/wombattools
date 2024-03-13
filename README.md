# Wombat Tools
A set of command line tools for working with the wombatforensics forensic image format. These custom images will be integrated into wombatforensics which will be able to read and parse the images as well as create and verify them.

The wombat forensic image (wfi) will be a walafus read only zstd compressed file system. The wfi image file contains the raw forensic image as well as the log file from the creation of the image and the info file which contains the forensic propertis such as case number, examiner, evidence number, description, and blake3 hashes for the source device. If verification was enabled, the forensic image hash and the verification status will be included.

The wombat logical image (wli), will be a zstd compressed tar file with the forensic metadata stored somewhere as yet to be determined, so the wli file will be 100% interoprable with tar and fuse mounting tools based on libarchive.

## Version 0.2 Tools

- wombatimager  - create a walafus read only zstd compressed forensic image given a source device and an image format name.
- wombatlist    - displays the files and their sizes within the wombat forensic image 
- wombatinfo    - displays the forensic image metadata information
- wombatlog     - displays the log from the creation of the forensic image
- wombaverify   - verifies the raw forensic image within the wombat forensic image to ensure nothing has changed
- wombatreader  - reads the raw forensic image and sends to stdout for use with other tools such as b3sum, xxd, etc.
- wombatrestore - restores the raw forensic image to a physical device and can optionally verify the device when done
- wombatmount   - working on a fuse module to enable fuse mounting the forensic image and accessing the files within
