#include "common.h"

void GetNextCluster(std::ifstream* rawcontent, uint32_t clusternum, uint8_t ftype, uint32_t fatoffset, std::vector<uint32_t>* clusterlist);
std::string ConvertClustersToExtents(std::vector<uint32_t>* clusterlist, uint32_t clustersize, uint64_t rootdiroffset);

void ParseFatForensics(std::string filename, std::string mntptstr, std::string devicestr, uint8_t ftype);

struct fatinfo
{
    uint16_t bytespersector = 0;
    uint8_t sectorspercluster = 0;
    uint8_t fattype = 0;
    uint32_t fatoffset = 0;
    uint32_t fatsize = 0;
    uint64_t clusterareastart = 0;
    std::string rootdirlayout = "";
};

void ParseFatInfo(std::ifstream* rawcontent, fatinfo* curfat);
uint32_t ParseFatPath(std::ifstream* rawcontent, fatinfo* curfat, std::string childpath);
