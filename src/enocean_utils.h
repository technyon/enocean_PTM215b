#pragma once
#include "Arduino.h"
#include <string>
#include <algorithm>

 
  /**
  * @brief Convert std::string holding character representation of hex bytes into a byte array
  * 
  * Hex representation should be separated by a single character, any character allowed
  * "E2 A3 07 28" => [226, 163, 7, 40]
  * 
  * @param stringInput input string
  * @param output pointer to resulting byte array
  * @param byteLength Number of hex numbers in stringInput
  */
void hexStringToByteArray(std::string stringInput, byte* output, uint8_t byteLength) {
  for(uint8_t i = 0; i < byteLength; i++){
    output[i] = strtol(stringInput.substr(i*2,2).c_str(), NULL, 16);
  }
}

std::string str_tolower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), 
                   [](unsigned char c){ return std::tolower(c); }
                  );
    return s;
}

void printBuf(const byte* buff, const uint8_t size, const boolean asChars, const char* header) {
  delay(100); //delay otherwise first part of print will not be shown 
  char tmp[4];
  
  if (strlen(header) > 0) {
    Serial.print(header);
    Serial.print(": ");
  }
  for (int i = 0; i < size; i++) {
    if (asChars) {
      Serial.print((char)buff[i]);
    } else {
      sprintf(tmp, "%02x", buff[i]);
      Serial.print(tmp);
      Serial.print(" ");
    }
  }
  Serial.println();
}
