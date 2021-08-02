#pragma once
#include "Arduino.h"
#include "vector"
#include "EnOceanDataTypes.h"

namespace EnOcean {


void hexStringToByteArray(std::string stringInput, byte* output, uint8_t byteLength);

std::vector<Parameter> parsePayloadParameters(byte* payload, const uint8_t size);

void printBuffer(const byte* buff, const uint8_t size, const boolean asChars, const char* header);
char* printBuffer(const byte* buff, const uint8_t size, const boolean asChars, const char* header, char* output);

} // namespace EnOcean 
