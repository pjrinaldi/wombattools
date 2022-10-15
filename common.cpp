#include "common.h"

void ReadContent(std::ifstream* rawcontent, uint8_t* tmpbuf, uint64_t offset, uint64_t size)
{
    rawcontent->seekg(offset);
    rawcontent->read((char*)tmpbuf, size);
}

void ReadContent(std::ifstream* rawcontent, int8_t* tmpbuf, uint64_t offset, uint64_t size)
{
    rawcontent->seekg(offset);
    rawcontent->read((char*)tmpbuf, size);
}

void ReturnUint32(uint32_t* tmp32, uint8_t* tmp8)
{
    *tmp32 = (uint32_t)tmp8[0] | (uint32_t)tmp8[1] << 8 | (uint32_t)tmp8[2] << 16 | (uint32_t)tmp8[3] << 24;
}

void ReturnUint16(uint16_t* tmp16, uint8_t* tmp8)
{
    *tmp16 = (uint16_t)tmp8[0] | (uint16_t)tmp8[1] << 8;
}

void ReturnUint64(uint64_t* tmp64, uint8_t* tmp8)
{
    *tmp64 = (uint64_t)tmp8[0] | (uint64_t)tmp8[1] << 8 | (uint64_t)tmp8[2] << 16 | (uint64_t)tmp8[3] << 24 | (uint64_t)tmp8[4] << 32 | (uint64_t)tmp8[5] << 40 | (uint64_t)tmp8[6] << 48 << (uint64_t)tmp8[7] << 56;
}

void ReturnUint(unsigned int* tmp, uint8_t* tmp8, unsigned int length)
{
    std::cout << "integer length: " << length << std::endl;
    unsigned int* tmpint = NULL;
    for(unsigned int i=0; i < length; i++)
    {
        std::cout << i << " " << length - 1 - i << " " << std::hex << (unsigned int)tmp8[length - 1 - i] << std::dec << std::endl;
        //tmpint[i] = (unsigned int)tmp8[length - 1 - i];
    }
    std::cout << "tmpint: " << tmpint << std::endl;
    *tmp = *tmpint;
}

void ReturnInt(int* tmp, int8_t* tmp8, unsigned int length)
{
    for(unsigned int i=0; i < length; i++)
        tmp[i] = (int)tmp8[i] << i * 8;
}
