#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

void GetContentBlocks(std::ifstream* devicebuffer, uint32_t blocksize, uint64_t curoffset, uint32_t* incompatflags, std::vector<uint32_t>* blocklist);

void ParseExtForensics(std::string filename, std::string mntptstr, std::string devicestr, uint64_t curextinode);
