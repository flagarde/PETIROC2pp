#include "Firmware.h"
#include "LALUsb.h"
#include <string>
#include <cstdlib>
#include <iostream>
#include <string.h>
#include "Utility.h"
#include <array>

void Firmware::sendWord(unsigned char subAd, std::string word, int usbDevId)
{
    int y=0;
    union converter conv;
    conv.i= std::strtoull(word.c_str(),nullptr,2);
    while(y!=1)y=UsbWrt(usbDevId,subAd,&(conv.c), 1);
}

void Firmware::sendWord(unsigned char subAd, unsigned char* stream, int nbSend, int usbDevId)
{
    UsbWrt(usbDevId, subAd, stream, nbSend);
}

std::string Firmware::readWord(unsigned char subAd, int usbDevId)
{
    unsigned char rdAdd = 0;
    int32_t rdByte=0;
    while(rdByte!=1)rdByte = UsbRd(usbDevId, subAd, &rdAdd, 1);
    return IntToBin(int(rdAdd), 8);
}

unsigned char* Firmware::readWord(unsigned char subAd, int nbRead, int usbDevId)
{
    unsigned char * rdAdd = new unsigned char[nbRead];
    int rdByte = UsbRd(usbDevId, subAd,rdAdd, nbRead);
    return rdAdd;
}

std::string Firmware::strFtDataSetBit(unsigned char signal, int data)
{
    std::string strData = "";
    strData += (char)((unsigned char)((unsigned char)(signal * data)));
    return strData;
}

std::string Firmware::strFtDataMsb(unsigned char signal)
{
    std::string strData = "";
    int bit = 0;
    unsigned char lsb = byteReverse(signal);

    if (signal == AS_READ_SILICON_ID)
    {
        for (int i = 0; i < 8; i++)
        {
            bit = lsb >> i;
            bit = bit & 0x1;

            strData += (char)((unsigned char)((unsigned char)(bit * ASDI) | NCE | NCONFIG | (Firmware::DCLK * 0)));
            strData += (char)((unsigned char)((unsigned char)(bit * ASDI) | NCE | NCONFIG | (Firmware::DCLK * 1)));
            strData += (char)((unsigned char)((unsigned char)(bit * ASDI) | NCE | NCONFIG | (Firmware::DCLK * 0)));
        }
    }
    else if (signal == AS_READ_BYTES ||
             signal == AS_WRITE_ENABLE ||
             signal == AS_ERASE_BULK ||
             signal == AS_READ_STATUS ||
             signal == AS_PAGE_PROGRAM)
    {
        for (int i = 0; i < 8; i++)
        {
            bit = lsb >> i;
            bit = bit & 0x1;

            strData += (char)((unsigned char)((unsigned char)(bit * ASDI) | NCE | (DCLK * 0)));
            strData += (char)((unsigned char)((unsigned char)(bit * ASDI) | NCE | (DCLK * 1)));
            strData += (char)((unsigned char)((unsigned char)(bit * ASDI) | NCE | (DCLK * 0)));
        }
    }
    else if (signal == AS_NOP)
    {
        for (int i = 0; i < 8; i++)
        {
            strData += (char)((unsigned char)(NCE | (DCLK * 0)));
            strData += (char)((unsigned char)(NCE | (DCLK * 1)));
            strData += (char)((unsigned char)(NCE | (DCLK * 0)));
        }
    }
    return strData;
}

std::string Firmware::strFtDataLsb(unsigned char signal)
{
    std::string strData = "";
    int bit = 0;
    unsigned char lsb = signal;

    for (int i = 0; i < 8; i++)
    {
        bit = lsb >> i;
        bit = bit & 0x1;

        strData += (char)((unsigned char)((unsigned char)(bit * ASDI) | NCE | (DCLK * 0)));
        strData += (char)((unsigned char)((unsigned char)(bit * ASDI) | NCE | (DCLK * 1)));
        strData += (char)((unsigned char)((unsigned char)(bit * ASDI) | NCE | (DCLK * 0)));
    }
    return strData;
}

std::string Firmware::strFtDataEpcs(unsigned char signal)
{
    std::string strData = "";
    int bit = 0;
    unsigned char lsb = byteReverse(signal);

    for (int i = 0; i < 8; i++)
    {
        bit = lsb >> i;
        bit = bit & 0x1;

        strData += (char)((unsigned char)((unsigned char)(bit * ASDI) | NCE | (DCLK * 0)));
        strData += (char)((unsigned char)((unsigned char)(bit * ASDI) | NCE | (DCLK * 1)));
        strData += (char)((unsigned char)((unsigned char)(bit * ASDI) | NCE | (DCLK * 0)));
    }
    return strData;
}
/*
static FT_STATUS Firmware::ftBitWrite(unsigned char signal, int data, FTDI myFtdiDevice)
{
    byte mask = (byte)~signal;
    byte[] dataBuffer = new byte[1];
    byte pinState = 0x00;
    uint numByteWr = 0;
    FTDI.FT_STATUS status = FTDI.FT_STATUS.FT_OK;

    status = myFtdiDevice.GetPinStates(ref pinState);

    dataBuffer[0] = (byte)((pinState & mask) | (data * signal));
    status = myFtdiDevice.Write(dataBuffer, 1, ref numByteWr);
    return status;
}

static FT_STATUS Firmware::ftBitRead(unsigned char signal, int& data, FTDI myFtdiDevice)
{
    FTDI.FT_STATUS status = FTDI.FT_STATUS.FT_OK;
    byte pinState = 0x00;
    byte[] rdBuf = new byte[1];

    status = myFtdiDevice.GetPinStates(ref pinState);

    if (status == FTDI.FT_STATUS.FT_OK)
        data = pinState & signal;

    return status;
}

static FT_STATUS Firmware::ftByteRead(unsigned char& data, FTDI myFtdiDevice)
{
    FTDI.FT_STATUS status = FTDI.FT_STATUS.FT_OK;
    int bit = 0;
    int mask = 0x01;

    status = ftBitWrite(DCLK, 0, myFtdiDevice);
    if (status != FTDI.FT_STATUS.FT_OK) return status;

    for (int i = 0; i < 8; i++)
    {
        status = ftBitWrite(DCLK, 1, myFtdiDevice);
        if (status != FTDI.FT_STATUS.FT_OK) return status;
        status = ftBitRead(DATAOUT, ref bit, myFtdiDevice);
        if (bit != 0)
            data |= (byte)(mask << i);

        status = ftBitWrite(DCLK, 0, myFtdiDevice);
        if (status != FTDI.FT_STATUS.FT_OK) return status;
    }
    return status;
}*/

unsigned char Firmware::byteReverse(unsigned char inByte)
{
    unsigned char result = 0x00;
    unsigned char mask = 0x00;

    for (mask = 0x80; int32_t(mask) > 0; mask >>= 1)
    {
        result >>= 1;
        unsigned char tempbyte = (unsigned char)(inByte & mask);
        if (tempbyte != 0x00)
            result |= 0x80;
    }
    return result;
}

/*static int Firmware::eraseEpcsDev(FTDI myFtdiDevice)
{
    int status = 0;

    // Set FTDI to Bit-bang mode
    ftStatus = myFtdiDevice.SetBitMode(0x4f, 0x1); //0x4f -- 01001111
    ftStatus = myFtdiDevice.SetBaudRate(153600);

    // Reset PINs;
    uint numByteWr = 0;
    byte[] byteWr = new byte[1];
    byteWr[0] = DEF_VALUE;
    ftStatus = myFtdiDevice.Write(byteWr, 1, ref numByteWr);

    //Set NCS to 0
    string strFtWr = strFtDataSetBit(NCS, 0);

    // Send Write enable
    strFtWr += strFtDataMsb(AS_WRITE_ENABLE);

    // Set NCS to 1
    strFtWr += strFtDataSetBit(NCS, 1);

    //Set NCS to 0
    strFtWr += strFtDataSetBit(NCS, 0);

    // Send Erase bulk
    strFtWr += strFtDataMsb(AS_ERASE_BULK);

    //Set NCS to 1
    strFtWr += strFtDataSetBit(NCS, 1);

    //Set NCS to 0
    strFtWr += strFtDataSetBit(NCS, 0);

    // Send Read status
    strFtWr += strFtDataMsb(AS_READ_STATUS);
    uint numFtWr = 0;
    ftStatus = myFtdiDevice.Write(strFtWr, strFtWr.Length, ref numFtWr);

    // Read status
    byte byteStatusReg = 0;
    ftStatus = ftByteRead(ref byteStatusReg, myFtdiDevice);
    byteStatusReg = byteReverse(byteStatusReg);

    while ((byteStatusReg & 0x01) == 1)
    {
        byteStatusReg = 0;
        ftStatus = ftByteRead(ref byteStatusReg, myFtdiDevice);
        byteStatusReg = byteReverse(byteStatusReg);
    }

    //Set NCS to 1
    ftStatus = myFtdiDevice.Write(strFtDataSetBit(NCS, 1), 1, ref numFtWr);

    if (ftStatus == FTDI.FT_STATUS.FT_OK)
        status = 0;
    else
        status = -1;

    return status;
}*/
