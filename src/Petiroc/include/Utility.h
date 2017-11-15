#pragma once
#include<string>

union converter{unsigned char c; uint32_t i;};
std::string IntToBin(int value, int len);

std::string strRev(const std::string & s); // To reverse a string
