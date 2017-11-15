#pragma once
#include<string>
#include<array>
#include<algorithm>
#include"Firmware.h"
class SlowControl
{
        public:
        //region slow control parameters declaration
        int NbChannels=32;
        std::array<int,32> sc_maskQ;
        std::array<int,32>sc_inputDac;
        std::array<int,32>sc_cmdInputDac;
        int sc_inputDAC_dummy = 0;
        std::array<int,32>sc_maskT;
        std::array<int,32>sc_sixBitDac;
        int sc_enDac;
        int sc_ppDac;
        int sc_dacVthQ;
        int sc_dacVthRf;
        int sc_enAdc;
        int sc_ppAdc;
        int sc_selStartbRampAdcExt;
        int sc_usebCompensation;
        int sc_enBiasDac;
        int sc_ppBiasDac;
        int sc_enBiasRamp;
        int sc_ppBiasRamp;
        int sc_dacDelay;
        int sc_enBiasDiscriDelay;
        int sc_ppBiasDiscriDelay;
        int sc_ppBiasSensor;
        int sc_enBiasSensor;
        int sc_enBiasCE;
        int sc_ppBiasCE;
        int sc_enBiasDiscri;
        int sc_ppBiasDiscri;
        int sc_cmdPolarity;
        int sc_latch;
        int sc_enBias6Dac;
        int sc_ppBias6Dac;
        int sc_enBiasTdc;
        int sc_ppBiasTdc;
        int sc_onBiasInputDac;
        int sc_enBiasQ;
        int sc_ppBiasQ;
        int sc_Cf;
        int sc_Cin;
        int sc_enBiasSca;
        int sc_ppBiasSca;
        int sc_enBiasDiscriQ;
        int sc_ppBiasDiscriQ;
        int sc_enDiscriAdcT;
        int sc_ppDiscriAdcT;
        int sc_enDiscriAdcQ;
        int sc_ppDiscriAdcQ;
        int sc_disRazChnInt;
        int sc_disRazChnExt;
        int sc_sel80M;
        int sc_en80M;
        int sc_enSlowRec;
        int sc_ppSlowRec;
        int sc_enFastRec;
        int sc_ppFastRec;
        int sc_enBiasTransmitter;
        int sc_ppBiasTransmitter;
        int sc_on1mA;
        int sc_on2mA;
        int sc_onOtaQ;
        int sc_onOtaMux;
        int sc_onOtaProbe;
        int sc_disTrigMux;
        int sc_enNor32T;
        int sc_enNor32Q;
        int sc_disTriggers;
        int sc_enDoutOc;
        int sc_enTransmit;
        static std::string strDefSC;
        static std::string strSC;
        //endregion
        
        //region slow control management

        std::string getSC(); // ASIC specific
        //region mask
        bool sendSC(int usbDevId, std::string& strSC,Firmware& firm);
};
