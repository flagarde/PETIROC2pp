#pragma once
#include "LALUsb.h"
#include <string>
#include "ftd2xx.h"
class Firmware
{
public :
    static FT_STATUS ftStatus;
    
private:
    //region EPCS Operation Code
    const unsigned char AS_NOP = 0x00;
    const unsigned char AS_WRITE_ENABLE = 0x06;
    const unsigned char AS_WRITE_DISABLE = 0x04;
    const unsigned char AS_READ_STATUS = 0x05;
    const unsigned char AS_WRITE_STATUS = 0x01;
    const unsigned char AS_READ_BYTES = 0x03;
    const unsigned char AS_FAST_READ_BYTES = 0x0B;
    const unsigned char AS_PAGE_PROGRAM = 0x02;
    const unsigned char AS_ERASE_SECTOR = 0xD8;
    const unsigned char AS_ERASE_BULK = 0xC7;
    const unsigned char AS_READ_SILICON_ID = 0xAB;
    const unsigned char AS_CHECK_SILICON_ID = 0x9F;
    //endregion

    //region Pin Definition
    const unsigned char CONF_DONE = 0x80;
    const unsigned char ASDI = 0x40;
    const unsigned char DATAOUT = 0x10;
    const unsigned char NCE = 0x08;
    const unsigned char NCS = 0x04;
    const unsigned char NCONFIG = 0x02;
    const unsigned char DCLK = 0x01;
    const unsigned char CUR_DATA = 0x00;
    const unsigned char DEF_VALUE = 0x0E;
    //endregion
    
    
       /* static FT_STATUS ftBitWrite(unsigned char signal, int data, FTDI myFtdiDevice);
 
        static FT_STATUS ftBitRead(unsigned char signal, int& data, FTDI myFtdiDevice);*/
public:

        static void sendWord(unsigned char subAd, std::string word, int usbDevId);

        static void sendWord(unsigned char subAd, unsigned char* stream, int nbSend, int usbDevId);

        static std::string readWord(unsigned char subAd, int usbDevId);

        static unsigned char* readWord(unsigned char subAd, int nbRead, int usbDevId);

        static std::string strFtDataSetBit(unsigned char signal, int data);
    
        std::string strFtDataMsb(unsigned char signal);
 
        std::string strFtDataLsb(unsigned char signal);

        std::string strFtDataEpcs(unsigned char signal);

       /* static FT_STATUS ftByteRead(unsigned char& data, FTDI myFtdiDevice);
*/
        static unsigned char byteReverse(unsigned char inByte);

       /* static int eraseEpcsDev(FTDI myFtdiDevice);*/
};
