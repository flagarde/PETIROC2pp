#include "LALUsb.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include "Firmware.h"
#include "SlowControl.h"
#include"Parameters.h"
#include "Utility.h"
#include<map>
#include<vector>
#include<cstring>

void readAllFPGAWords(Firmware& firm,int& usbDevId)
{
    std::cout<<firm.readWord(0, usbDevId)<<"  "<<std::endl<<"  "<<firm.readWord(1, usbDevId)<<"  "
            <<firm.readWord(2, usbDevId)<<"  "<<firm.readWord(3, usbDevId)<<"  "
           <<firm.readWord(5, usbDevId)<<"  "<<firm.readWord(6, usbDevId)<<std::endl;
}


int main()
{

    std::string boardFirmware = "";
    Firmware firm;
    SlowControl slow;
    int usbDevId=0;
    std::cout<<"Welcome ! I found "<<USB_GetNumberOfDevs()<<" USB Device(s)"<<std::endl;
    for(unsigned int i=0;i!=(unsigned int) (USB_GetNumberOfDevs());++i)
    {
        //std::cout<<"rr"<<std::endl;
        char jj[256] ;
        char kk[256] ;
        GetDeviceDesc(&jj[0],i);
        std::cout<<jj<<std::endl;
        GetDeviceSerNum(&kk[0],i);
        std::cout<<kk<<std::endl;
        if(std::string(jj)=="PCB_Petiroc2 A")
        {
           unsigned char *array = new unsigned char[1];
           bool retVerbose=true;
           usbDevId=OpenUsbDevice(&kk[0]);
           bool retUsbOpen=USB_Init(usbDevId,retVerbose);
           bool retSetLT=USB_GetLatencyTimer(usbDevId,array);
           bool retSetBufSize=USB_SetXferSize(usbDevId,8192,32768);
           bool retSetTimeOuts=USB_SetTimeouts(usbDevId,20,20);
           if (retUsbOpen&&retSetLT&&retSetBufSize&&retSetTimeOuts)
           {
              std::string rdSubAdd100="00000000";
              int testCon=0;
              while(rdSubAdd100=="00000000"&&testCon<10)
              {
                  testCon++;
                  rdSubAdd100 = firm.readWord(100, usbDevId);
                  // Sub address 100 contain the firware version. If rdSubAdd100 == 0 the board failed to connect
                  //rdSubAdd100=Firmware.readWord(100,usbDevID);
                  std::this_thread::sleep_for(std::chrono::milliseconds(200));
              }
                  if(rdSubAdd100 == "00000000")
                  {

                      std::cout<<"Looks like there is an issue with the connection. Please verify the board is well plugged and powered and click again."<<std::endl;
                  }
                  else
                  {
                      std::cout<<"Connected"<<std::endl;
                      boardFirmware = std::to_string(std::strtoull(&rdSubAdd100[0],nullptr,2));

                      std::cout<<"Initialize firmware static registers (contains firmware options)"<<std::endl;
                      std::string a="00" + ((checkBox_dout_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_RAZ_checked == true) ? std::string("1") : std::string("0")) +
                                    ((checkBox_trigExt_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_userValEvt_checked == true) ? std::string("1") : std::string("0")) +
                                       ((checkBox_userRAZ_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_valEvt_checked == true) ? std::string("1") : std::string("0"));
                      std::string b="10" + ((checkBox_en160M_checked == true) ? std::string("1") : std::string("0")) + "10100";
                      std::string c="00" + ((checkBox_en40M_checked == true) ? std::string("1") : std::string("0")) + "0" + ((checkBox_userTrigExt_checked == true) ? std::string("1") : std::string("0")) +
                                       ((checkBox_startbRampADCext_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_coinc_checked == true) ? std::string("1") : std::string("0")) + "0";
                      std::string d="000" + ((checkBox_pwronDAC_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_pwronADC_checked == true) ? std::string("1") : std::string("0")) + "0" +
                                        ((checkBox_pwronA_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_pwronD_checked == true) ? std::string("1") : std::string("0"));
                    std::string e=((checkBox_clkRO_checked == true) ? std::string("1") : std::string("0")) + "0100" + ((checkBox_polarity_checked == true) ? std::string("1") : std::string("0")) +
                                       ((checkBox_160Mspeed_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_40Mspeed_checked == true) ? std::string("1") : std::string("0"));
                        std::string f="000" + ((checkBox_enTimeout_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_selOR32hold_checked == true) ? std::string("1") : std::string("0")) + "0" +
                                        ((checkBox_selOR32_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_trigChoice_checked == true) ? std::string("1") : std::string("0"));
                      std::map<int,std::string>g{{0,a},{1,b},{2,c},{3,d},{5,e},{6,f}};
                      for(std::map<int,std::string>::iterator it=g.begin();it!=g.end();++it)
                      {
                        /* do
                        {*/
                            firm.sendWord(it->first,it->second,usbDevId);
                        /*}while(isnotsame(it->first,it->second,firm,usbDevId)==true);*/
                          
                      }
                      std::this_thread::sleep_for(std::chrono::milliseconds(100));
                      readAllFPGAWords(firm,usbDevId);
                      if(slow.sendSC(usbDevId,slow.strDefSC,firm)==true)std::cout<<"SlowControl sent"<<std::endl;
                      int type=3;
                                  switch (type)
            {
                case 0:

                    firm.sendWord(24, "00000001", usbDevId);
                    break;
                case 1:
                 
                    firm.sendWord(24, "00000011", usbDevId);
                    break;
                case 2:
               
                    firm.sendWord(24, "00000010", usbDevId);
                    break;
                case 3:
           
                    firm.sendWord(24, "00000100", usbDevId);
                    break;
            }
            int FIFOAcqLength = 100;
            int nbAcq = 100;
            int nbCycles = nbAcq / FIFOAcqLength;
            if (nbAcq % FIFOAcqLength != 0 || nbCycles == 0) nbCycles++; // add
            for (int cycle = 0; cycle < nbCycles; cycle++)
            {
                std::cout<<cycle<<std::endl;
                int nbAcqInCycle = 0;
                if (nbAcq >= FIFOAcqLength) nbAcqInCycle = FIFOAcqLength;
                else if (nbAcq < FIFOAcqLength) nbAcqInCycle = nbAcq;

                // SubAdd 45 store the number of acquisitions to do and save in FIFO before reading it.
                std::string strNbAcq = IntToBin(nbAcqInCycle, 8);
                firm.sendWord(45, strNbAcq, usbDevId);

                // start DAQ sequence
                firm.sendWord(23, "10000000", usbDevId);
                std::string rd4 = "00000000";

                // bit 7 of subAdd 4 is 1 when the acquisitions are done
                while (rd4.substr(7, 1) == "0") { rd4 = firm.readWord(4, usbDevId);std::cout<<firm.readWord(4, usbDevId)<<std::endl;std::this_thread::sleep_for(std::chrono::milliseconds(100)); }

                std::string strSA13 = firm.readWord(13, usbDevId);
                strSA13 = strRev(strSA13);
                int sumSA13SA14 = 0;

                // Bits 7 and 6 of subAdd 13 are 1 if FIFO is empty or full. If that's the case an error occured and events are discarded.
                if (strSA13.substr(7, 1) == "0" && strSA13.substr(6, 1) == "0")
                {
                    // read sum of subadd 13 & 14
                    sumSA13SA14 = std::strtoull(firm.readWord(13, usbDevId).c_str(),nullptr, 2) * 256 + std::strtoull(firm.readWord(14, usbDevId).c_str(),nullptr, 2);
                    std::cout<<sumSA13SA14<<std::endl;
                    std::vector<unsigned char>tmpFIFO15(nbAcqInCycle * 120);
                    // read FIFO subadd 15
                   // byte[] tmpFIFO15 = new byte[nbAcqInCycle * 120];
                    if (sumSA13SA14 == nbAcqInCycle * 120)
                        //bufferType::size_type size = std::strlen((const char*)buffer);
                        tmpFIFO15=std::vector<unsigned char>(firm.readWord(15, nbAcqInCycle * 120, usbDevId),firm.readWord(15, nbAcqInCycle * 120, usbDevId)+std::strlen((const char*)firm.readWord(15, nbAcqInCycle * 120, usbDevId)));
                        //tmpFIFO15 =firm.readWord(15, nbAcqInCycle * 120, usbDevId);
                    else
                    {
                        sumSA13SA14 = 0;
                        cycle = cycle - 1;
                        continue;
                    }

                    // reset Sub Address 13
                    firm.sendWord(13, "00000000", usbDevId);
                    for(unsigned int k=0;k!=tmpFIFO15.size();++k) std::cout<<tmpFIFO15[k]<<"  ";
                    std::cout<<std::endl;
                    //region raw data conversion

                   /* BitArray bitArrayFIFO15 = new BitArray(tmpFIFO15);

                    int[] data = new int[96];
                    int[] hit = new int[32];

                    for (int i = 0; i < bitArrayFIFO15.Length / 960; i++)
                    {
                        for (int ii = 0; ii < 64; ii++)
                        {
                            bool[] boolArrayData = { bitArrayFIFO15[i * 960 + ii * 10 + 9], bitArrayFIFO15[i * 960 + ii * 10 + 8], bitArrayFIFO15[i * 960 + ii * 10 + 7], bitArrayFIFO15[i * 960 + ii * 10 + 6], bitArrayFIFO15[i * 960 + ii * 10 + 5], bitArrayFIFO15[i * 960 + ii * 10 + 4], bitArrayFIFO15[i * 960 + ii * 10 + 3], bitArrayFIFO15[i * 960 + ii * 10 + 2], bitArrayFIFO15[i * 960 + ii * 10 + 1], bitArrayFIFO15[i * 960 + ii * 10 + 0] };
                            BitArray bitArrayData = new BitArray(boolArrayData);
                            data[ii] = GrayToInt(bitArrayData, 10);
                            if (ii == 0 || ii % 2 == 0) PerChannelTime[ii / 2, data[ii]]++;
                            else PerChannelCharge[ii / 2, data[ii]]++;
                        }

                        for (int ii = 64; ii < 96; ii++)
                        {
                            bool[] boolArrayData = { bitArrayFIFO15[i * 960 + ii * 10 + 8], bitArrayFIFO15[i * 960 + ii * 10 + 7], bitArrayFIFO15[i * 960 + ii * 10 + 6], bitArrayFIFO15[i * 960 + ii * 10 + 5], bitArrayFIFO15[i * 960 + ii * 10 + 4], bitArrayFIFO15[i * 960 + ii * 10 + 3], bitArrayFIFO15[i * 960 + ii * 10 + 2], bitArrayFIFO15[i * 960 + ii * 10 + 1], bitArrayFIFO15[i * 960 + ii * 10 + 0] };
                            hit[ii - 64] = Convert.ToInt32(bitArrayFIFO15[i * 960 + ii * 10 + 9]);
                            BitArray bitArrayData = new BitArray(boolArrayData);
                            data[ii] = GrayToInt(bitArrayData, 9);
                            Hit[ii - 64] += hit[ii - 64];
                        }

                        //if (Hit[1] == 1 && Hit[20] == 1)
                        {
                            string outdata = "";

                            for (int ii = 0; ii < 64; ii++)
                                outdata += data[ii] + " ";

                            for (int ii = 64; ii < 96; ii++)
                                outdata += data[ii] + " " + hit[ii - 64] + " ";

                            tw.WriteLine(outdata);
                        }
                    }*/
                   // object[] par = { data, hit };

                    //endregion
                }
                else
                {
                    cycle = cycle - 1;
                    continue;
                }
                firm.sendWord(23, "00000000", usbDevId);
                nbAcq -= FIFOAcqLength;
            }         
                      
                      
                      
                      
                      
                      
                      if (usbDevId > 0)
            {
                CloseUsbDevice(usbDevId);
                usbDevId = 0;
            }
                      //std::cout<<usbDevId<<"  "<<boardFirmware<<"  "<<rdSubAdd100<<std::endl;
                  }

              }
           }
    }
}
