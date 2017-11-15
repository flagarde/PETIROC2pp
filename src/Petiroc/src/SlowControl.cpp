#include"SlowControl.h"
#include"Parameters.h" 
#include"Utility.h"
#include"Firmware.h"
#include<vector>
 std::string SlowControl::strDefSC="0000000000000000000000000000000000000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100000001100010000000000000000000000000000000000000000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000010000011101001011000111110100110011110100000111111111011111111010011111111001111111111011011011";
        std::string SlowControl::strSC = SlowControl::strDefSC;

std::string SlowControl::getSC() // ASIC specific
{
        strSC = "";
        for (int i = 0; i < NbChannels; i++) strSC += std::to_string(sc_maskQ[i]);
        for (int i = 0; i < NbChannels; i++) strSC += IntToBin(sc_inputDac[i], 8) + std::to_string(sc_cmdInputDac[i]);
        strSC +=strRev(IntToBin(sc_inputDAC_dummy, 8));
        for (int i = 0; i < NbChannels; i++)strSC += std::to_string(sc_maskT[i]);
        for (int i = 0; i < NbChannels; i++)strSC += strRev(IntToBin(sc_sixBitDac[i], 6));
        strSC += std::to_string(sc_enDac)
            + std::to_string(sc_ppDac)
            + IntToBin(sc_dacVthQ, 10)
            + IntToBin(sc_dacVthRf, 10)
            + std::to_string(sc_enAdc)
            + std::to_string(sc_ppAdc)
            + std::to_string(sc_selStartbRampAdcExt)
            + std::to_string(sc_usebCompensation)
            + std::to_string(sc_enBiasDac)
            + std::to_string(sc_ppBiasDac)
            + std::to_string(sc_enBiasRamp)
            + std::to_string(sc_ppBiasRamp)
            + strRev(IntToBin(sc_dacDelay, 8))
            + std::to_string(sc_enBiasDiscriDelay)
            + std::to_string(sc_ppBiasDiscriDelay)
            + std::to_string(sc_ppBiasSensor)
            + std::to_string(sc_enBiasSensor)
            + std::to_string(sc_enBiasCE)
            + std::to_string(sc_ppBiasCE)
            + std::to_string(sc_enBiasDiscri)
            + std::to_string(sc_ppBiasDiscri)
            + std::to_string(sc_cmdPolarity)
            + std::to_string(sc_latch)
            + std::to_string(sc_enBiasDac)
            + std::to_string(sc_ppBiasDac)
            + std::to_string(sc_enBiasTdc)
            + std::to_string(sc_ppBiasTdc)
            + std::to_string(sc_onBiasInputDac)
            + std::to_string(sc_enBiasQ)
            + std::to_string(sc_ppBiasQ)
            + strRev(IntToBin(sc_Cf, 2))
            + IntToBin(sc_Cin, 2)
            + std::to_string(sc_enBiasSca)
            + std::to_string(sc_ppBiasSca)
            + std::to_string(sc_enBiasDiscriQ)
            + std::to_string(sc_ppBiasDiscriQ)
            + std::to_string(sc_enDiscriAdcT)
            + std::to_string(sc_ppDiscriAdcT)
            + std::to_string(sc_enDiscriAdcQ)
            + std::to_string(sc_ppDiscriAdcQ)
            + std::to_string(sc_disRazChnInt)
            + std::to_string(sc_disRazChnExt)
            + std::to_string(sc_sel80M)
            + std::to_string(sc_en80M)
            + std::to_string(sc_enSlowRec)
            + std::to_string(sc_ppSlowRec)
            + std::to_string(sc_enFastRec)
            + std::to_string(sc_ppFastRec)
            + std::to_string(sc_enBiasTransmitter)
            + std::to_string(sc_ppBiasTransmitter)
            + std::to_string(sc_on1mA)
            + std::to_string(sc_on2mA)
            + std::to_string(sc_onOtaQ)
            + std::to_string(sc_onOtaMux)
            + std::to_string(sc_onOtaProbe)
            + std::to_string(sc_disTrigMux)
            + std::to_string(sc_enNor32T)
            + std::to_string(sc_enNor32Q)
            + std::to_string(sc_disTriggers)
            + std::to_string(sc_enDoutOc)
            + std::to_string(sc_enTransmit);
            return strSC;
}

bool SlowControl::sendSC(int usbDevId, std::string& strSC,Firmware& firm)
{
            // Initialize result as false
            bool result = false;
            // Get standard length of slow control bitstream
            size_t SCLenght = SlowControl::strDefSC.size();
            // Get length of current slow control bitstream
            size_t intLenStrSC = SlowControl::strSC.size();
            std::vector<unsigned char> bytSC;
            // reverse slow control string before loading
            strSC = strRev(strSC);
            // If the length of the current bitstream is not OK, return false
            // else store the slow control in byte array
            if (intLenStrSC == SCLenght)
            {
                for (int i = 0; i < (SCLenght / 8); i++)
                {
                    std::string strScCmdTmp = strSC.substr(i*8,8);
                    strScCmdTmp = strRev(strScCmdTmp);
                    union converter conv;
                    conv.i=std::strtoull(strScCmdTmp.c_str(),nullptr,2);
                    bytSC.push_back((unsigned char)(conv.i));
                }
            }
            else return result;
            // Select slow control parameters to FPGA
            firm.sendWord(1, "10" + ((checkBox_en160M_checked == true) ? std::string("1") : std::string("0")) + "10000", usbDevId);
            // Send slow control parameters to FPGA
            int intLenBytSC = bytSC.size();
            firm.sendWord(10, &bytSC[0], intLenBytSC, usbDevId);
            // Start shift parameters to ASIC
            firm.sendWord(1, "10" + ((checkBox_en160M_checked == true) ? std::string("1") : std::string("0")) + "10110", usbDevId);
            //wait(20000);
            // Stop shift parameters to ASIC
            firm.sendWord(1, "10" + ((checkBox_en160M_checked == true) ? std::string("1") : std::string("0")) + "10100", usbDevId);
            // Slow control test checksum test query
            firm.sendWord(0, "10" + ((checkBox_dout_checked == true) ? std::string("1") : std::string("0")) + 
                          ((checkBox_RAZ_checked == true) ? std::string("1") : std::string("0")) + 
                          ((checkBox_trigExt_checked == true) ? std::string("1") : std::string("0")) + 
                          ((checkBox_userValEvt_checked == true) ? std::string("1") : std::string("0")) + 
                          ((checkBox_userRAZ_checked == true) ? std::string("1") : std::string("0")) + 
                          ((checkBox_valEvt_checked == true) ? std::string("1") : std::string("0")), usbDevId);
            // Load slow control
            firm.sendWord(1, "10" + ((checkBox_en160M_checked == true) ? std::string("1") : std::string("0")) + "10101", usbDevId);
            //wait(20000);
            firm.sendWord(1, "10" + ((checkBox_en160M_checked == true) ? std::string("1") : std::string("0")) + "10100", usbDevId);
            // Send slow control parameters to FPGA
            firm.sendWord(10, &bytSC[0], intLenBytSC, usbDevId);
            // Start shift parameters to ASIC
            firm.sendWord(1, "10" + ((checkBox_en160M_checked == true) ? std::string("1") : std::string("0")) + "10110", usbDevId);
            //wait(20000);
            // Stop shift parameters to ASIC
            firm.sendWord(1, "10" + ((checkBox_en160M_checked == true) ? std::string("1") : std::string("0")) + "10100", usbDevId);
            // Slow Control Correlation Test Result
            if (firm.readWord(4, usbDevId) == "00000000") result = true;
            // Reset slow control test checksum test query
            firm.sendWord(0, "00" + ((checkBox_dout_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_RAZ_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_trigExt_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_userValEvt_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_userRAZ_checked == true) ? std::string("1") : std::string("0")) + ((checkBox_valEvt_checked == true) ? std::string("1") : std::string("0")), usbDevId);
            return result;
}

