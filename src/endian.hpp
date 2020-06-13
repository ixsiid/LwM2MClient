#ifndef _ENDIAN_H
#define _ENDIAN_H

#include <sys/types.h>

namespace LwM2M {
int putUint16ToBytes(uint16_t input, uint8_t *output);
int putUint24ToBytes(uint32_t input, uint8_t *output);
int putUint32ToBytes(uint32_t input, uint8_t *output);
int putUint48ToBytes(uint64_t input, uint8_t *output);
int putUint64ToBytes(uint64_t input, uint8_t *output);
int putFloat64ToBytes(double input, uint8_t *output);
uint16_t getUint16FromBytes(const uint8_t *input);
uint32_t getUint24FromBytes(const uint8_t *input);
uint32_t getUint32FromBytes(const uint8_t *input);
uint64_t getUint48FromBytes(const uint8_t *input);
uint64_t getUint64FromBytes(const uint8_t *input);
double getFloat64FromBytes(const uint8_t *input);
uint32_t getReverseUint32(uint32_t input);
uint64_t getReverseUint64(uint64_t input);
}  // namespace LwM2M

#endif
