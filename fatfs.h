#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

//void GetNextCluster(ForImg* curimg, uint32_t clusternum, uint8_t fstype, qulonglong fatoffset, QList<uint>* clusterlist);

void ParseFatForensics(std::string filename, std::string mntptstr, std::string devicestr, uint8_t ftype);

struct fatinfo
{
    uint16_t bytespersector = 0;
    uint8_t sectorspercluster = 0;
    std::string dootdirlayout = "";
    uint32_t fatoffset = 0;
    uint16_t fatsize16 = 0;
    uint32_t fatsize32 = 0;
    uint64_t clusterareastart = 0;
    /*
    out << "FAT Offset|" << QString::number((qulonglong)(curstartsector*512 + reservedareasize * bytespersector)) << "|Byte offset to the start of the first FAT" << Qt::endl;
    uint16_t fatsize = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 22, 2));
    uint16_t bytespersector = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 11, 2));
    uint8_t fatcount = qFromLittleEndian<uint8_t>(curimg->ReadContent(curstartsector*512 + 16, 1));
    uint8_t sectorspercluster = qFromLittleEndian<uint8_t>(curimg->ReadContent(curstartsector*512 + 13, 1));
    //uint16_t bytesperclutser = bytespersector * sectorspercluster;
    uint16_t reservedareasize = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 14, 2));
    uint16_t rootdirmaxfiles = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 17, 2));
    out << "Cluster Area Start|" << QString::number((qulonglong)(curstartsector + reservedareasize + fatcount * fatsize + ((rootdirmaxfiles * 32) + (bytespersector - 1)) / bytespersector)) << "|Byte offset to the start of the cluster area." << Qt::endl;
    */
};
    /*
    out << "FAT Offset|" << QString::number((qulonglong)(curstartsector*512 + reservedareasize * bytespersector)) << "|Byte offset to the start of the first FAT" << Qt::endl;
    uint16_t fatsize = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 22, 2));
    uint16_t bytespersector = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 11, 2));
    uint8_t fatcount = qFromLittleEndian<uint8_t>(curimg->ReadContent(curstartsector*512 + 16, 1));
    uint8_t sectorspercluster = qFromLittleEndian<uint8_t>(curimg->ReadContent(curstartsector*512 + 13, 1));
    //uint16_t bytesperclutser = bytespersector * sectorspercluster;
    uint16_t reservedareasize = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 14, 2));
    uint16_t rootdirmaxfiles = qFromLittleEndian<uint16_t>(curimg->ReadContent(curstartsector*512 + 17, 2));
    */


void ParseFatInfo(std::ifstream* rawcontent, fatinfo* cfat16, uint8_t ftype);
