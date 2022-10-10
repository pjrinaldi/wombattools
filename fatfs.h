#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

//void GetNextCluster(ForImg* curimg, uint32_t clusternum, uint8_t fstype, qulonglong fatoffset, QList<uint>* clusterlist);

void ParseFatForensics(std::string filename, std::string mntptstr, std::string devicestr, uint8_t ftype);
