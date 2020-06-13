#include "endian.hpp"

#include <string.h>

using namespace LwM2M;

int LwM2M::putUint16ToBytes(uint16_t input, uint8_t *output){
    output[0] = (uint8_t)(input >> 8);
    output[1] = (uint8_t)(input >> 0);
    return 2;
}

int LwM2M::putUint24ToBytes(uint32_t input, uint8_t *output){
    output[0] = (uint8_t)(input >> 16);
    output[1] = (uint8_t)(input >> 8);
    output[2] = (uint8_t)(input >> 0);
    return 3;
}

int LwM2M::putUint32ToBytes(uint32_t input, uint8_t *output){
    output[0] = (uint8_t)(input >> 24);
    output[1] = (uint8_t)(input >> 16);
    output[2] = (uint8_t)(input >> 8);
    output[3] = (uint8_t)(input >> 0);
    return 4;
}

int LwM2M::putUint48ToBytes(uint64_t input, uint8_t *output){
    output[0] = (uint8_t)(input >> 40);
    output[1] = (uint8_t)(input >> 32);
    output[2] = (uint8_t)(input >> 24);
    output[3] = (uint8_t)(input >> 16);
    output[4] = (uint8_t)(input >> 8);
    output[5] = (uint8_t)(input >> 0);
    return 6;
}

int LwM2M::putUint64ToBytes(uint64_t input, uint8_t *output){
    output[0] = (uint8_t)(input >> 56);
    output[1] = (uint8_t)(input >> 48);
    output[2] = (uint8_t)(input >> 40);
    output[3] = (uint8_t)(input >> 32);
    output[4] = (uint8_t)(input >> 24);
    output[5] = (uint8_t)(input >> 16);
    output[6] = (uint8_t)(input >> 8);
    output[7] = (uint8_t)(input >> 0);
    return 8;
}

int LwM2M::putFloat64ToBytes(double input, uint8_t *output){
    uint8_t temp[8];
    memcpy(temp, &input, 8);
    for (int i = 0; i < 8; i++){
      output[i] = temp[7 - i];
    }
    return 8;
}

uint16_t LwM2M::getUint16FromBytes(const uint8_t *input){
    uint16_t ret = 0;
    ret += ((uint16_t)input[0]) << 8;
    ret += ((uint16_t)input[1]) << 0;
    return ret;
}

uint32_t LwM2M::getUint24FromBytes(const uint8_t *input){
    uint32_t ret = 0;
    ret += ((uint32_t)input[0]) << 16;
    ret += ((uint32_t)input[1]) << 8;
    ret += ((uint32_t)input[2]) << 0;
    return ret;
}

uint32_t LwM2M::getUint32FromBytes(const uint8_t *input){
    uint32_t ret = 0;
    ret += ((uint32_t)input[0]) << 24;
    ret += ((uint32_t)input[1]) << 16;
    ret += ((uint32_t)input[2]) << 8;
    ret += ((uint32_t)input[3]) << 0;
    return ret;
}

uint64_t LwM2M::getUint48FromBytes(const uint8_t *input){
    uint64_t ret = 0;
    ret += ((uint64_t)input[0]) << 40;
    ret += ((uint64_t)input[1]) << 32;
    ret += ((uint64_t)input[2]) << 24;
    ret += ((uint64_t)input[3]) << 16;
    ret += ((uint64_t)input[4]) << 8;
    ret += ((uint64_t)input[5]) << 0;
    return ret;
}

uint64_t LwM2M::getUint64FromBytes(const uint8_t *input){
    uint64_t ret = 0;
    ret += ((uint64_t)input[0]) << 56;
    ret += ((uint64_t)input[1]) << 48;
    ret += ((uint64_t)input[2]) << 40;
    ret += ((uint64_t)input[3]) << 32;
    ret += ((uint64_t)input[4]) << 24;
    ret += ((uint64_t)input[5]) << 16;
    ret += ((uint64_t)input[6]) << 8;
    ret += ((uint64_t)input[7]) << 0;
    return ret;
}

double LwM2M::getFloat64FromBytes(const uint8_t *input){
    double ret = 0.0;
    uint8_t temp[8];
    for (int i = 0; i < 8; i++){
      temp[i] = input[7 - i];
    }
    memcpy(&ret, temp, 8);
    return ret;
}

uint32_t LwM2M::getReverseUint32(uint32_t input){
    uint8_t bytes[4];
    putUint32ToBytes(input, bytes);
    uint32_t ret = 0;
    for (int i = 3; i >= 0; i--){
        ret = (ret << 8) + bytes[i];
    }
    return ret;
}

uint64_t LwM2M::getReverseUint64(uint64_t input){
    uint8_t bytes[8];
    putUint64ToBytes(input, bytes);
    uint64_t ret = 0;
    for (int i = 7; i >= 0; i--){
        ret = (ret << 8) + bytes[i];
    }
    return ret;
}
