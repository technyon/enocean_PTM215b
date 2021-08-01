#pragma once
#include "Arduino.h"
#include "vector"

namespace EnOcean {

struct PayloadVariable {
  uint8_t type;
  uint32_t value;
};

void hexStringToByteArray(std::string stringInput, byte* output, uint8_t byteLength);

std::vector<PayloadVariable> parsePayload(byte* payload, const uint8_t size);

void printBuffer(const byte* buff, const uint8_t size, const boolean asChars, const char* header);
char* printBuffer(const byte* buff, const uint8_t size, const boolean asChars, const char* header, char* output);

} // namespace EnOcean 
