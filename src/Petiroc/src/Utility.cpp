#include "Utility.h" 
#include<string>
#include<algorithm>
std::string IntToBin(int value, int len)
{
       return (len > 1 ? IntToBin(value >> 1, len - 1) : "") + "01"[value & 1];
}

std::string strRev(const std::string & s) // To reverse a string
{
        std::string temp(s);
        std::reverse(std::begin(temp), std::end(temp));
        return temp;
}
