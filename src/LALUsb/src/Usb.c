//
//	MODULE: USB.C
//			LalUsb Library Main module
//			Author: Chafik CHEIKALI
//			Created: February 2006
//			Last modified :
//				17/02/2011 : Added Synchronous i/o mode management for 2232H
//				11/01/2011 : Error codes management within USB_GetLastError(), two error codes added see LALUsb.h
//				11/01/2011 : Both timeouts (rx an tx) set to 1s in Usb_Init + spell-checking and printing 
//				18/05/2010 : InitializeEEPROMData() adapted to ftd2XX (2232H and4 232H eeprom programming)
//				25/03/2008 : Added Eprom programming support for 2232C
//				04/04/2008 : Adapted to the new version of ftdi Library ftd2xx

#ifndef __USB_C__
#define __USB_C__

//#include "LALUsb.p.h"
#include "LALUsb.h"

/******************************* Variables ******************************/

static USB_Error	USB_Errno = FT_OK;
static PerrorFunc 	UsbpfPerror = (PerrorFunc) USB_PrintErrMsg;
PrintfFunc   UsbPrintf = (PrintfFunc) printf;
static UCHAR ucBitMode = 0x00;
static int numDevs = 0;
static char *BufPtrs[MAXDEVS + 1]; // pointer to array of 'MAXDEVS + 1' pointers
static long CurrentIndex = 0;

static  FT_DEVICE ftDeviceType = FT_DEVICE_BM;

//EEPROM data

static FT_PROGRAM_DATA EpromData;
static char Manufacturer[32] = "LAL-SE";
static char ManufacturerId[16] = "LA";
static char SerialNumber[16]="SERNUM01";
//static char Description[65]="LAL-USBFrame based board";
static char Description[65]="Nouvelle carte LAL";
static DWORD ua_size = 0;
static WORD MaxPower = 90;
static WORD SelfPowered = 0;

// Error messages
static char *USB_ErrMsg[Max_USB_Errors] =
{
		"Ok",
		"Invalid handle",
		"Device not found",
		"Device not opened",
		"IO error",
		"Insufficient resources",
		"Invalid parameter",
		"Invalid baud rate",
		"Device not opened for erase",
		"Device not opened for write",
		"Failed to write device",
		"Failed to read eeprom",
		"Failed to write eeprom",
		"Failed to erase eeprom",
		"Eeprom not present",
		"Eeprom not programmed",
		"Invalid arguments",
		"Not supported",
		"Unknown error",
		"Device list not ready",
		"Interrupt frame received",
		"Timeout in a write operation",
		"Timeout in a read operation",
		"Header is missing",
		"Trailer is missing",
		"Received sub-address doesn't match",
		"Description string pointer is null",
		"Serial number string pointer is null"
};

FT_HANDLE UsbDevice[MAXDEVS];

static FT_HANDLE USB_GetDeviceHandle(int index);

//Get the number of devices currently connected

/* =========================================================================== */
FT_STATUS GetNumberOfDevices(int *numDevs)
/* =========================================================================== */
{
 FT_STATUS ftStatus;

	ftStatus = FT_ListDevices(numDevs,NULL,FT_LIST_NUMBER_ONLY);
	if (ftStatus != FT_OK)
	{
   		USB_Errno = ftStatus;
	}

	return ftStatus;
}

/* =========================================================================== */
BOOL USB_GetNumberOfDevices(int *num_devs)
/* =========================================================================== */
{
 return (GetNumberOfDevices(num_devs) == FT_OK);
}

//Get serial number of first device
/* =========================================================================== */
FT_STATUS GetDeviceSerNum(char *buffer, int index)
/* =========================================================================== */
{
 FT_STATUS ftStatus;
 DWORD devIndex = index;

	ftStatus = FT_ListDevices((PVOID)devIndex, buffer, FT_LIST_BY_INDEX|FT_OPEN_BY_SERIAL_NUMBER);

	if (ftStatus != FT_OK)
	{
   		USB_Errno = ftStatus;
	}

	return ftStatus;
}

/* =========================================================================== */
BOOL USB_GetDeviceSerNum(char *buffer, int index)
/* =========================================================================== */
{
 return (GetDeviceSerNum(buffer, index) == FT_OK);
}


/* =========================================================================== */
FT_STATUS GetDeviceDesc(char *buffer, int index)
/* =========================================================================== */
{
 DWORD devIndex = index; // first device
 FT_STATUS ftStatus;

	ftStatus = FT_ListDevices((PVOID)devIndex, buffer, FT_LIST_BY_INDEX|FT_OPEN_BY_DESCRIPTION);

	if (ftStatus != FT_OK)
	{
   		USB_Errno = ftStatus;
	}

	return ftStatus;
}

/* =========================================================================== */
FT_STATUS GetAllDeviceDesc(char **strArray)
/* =========================================================================== */
{
 FT_STATUS fts;
 int i, num_dev = MAXDEVS;

 // initialize the array of pointers
	
	if(strArray == NULL)
		return FT_OTHER_ERROR;

	for(i = 0; i < MAXDEVS; i++)
	{
		strArray[i] = malloc(64);
		if(strArray[i] == NULL)
		{
			while(i)
			{
				free(strArray[--i]);
			}
			return FT_OTHER_ERROR;
		}
	}
	strArray[MAXDEVS] = NULL; // last entry should be NULL

	fts = FT_ListDevices(strArray, &num_dev, FT_LIST_ALL | FT_OPEN_BY_DESCRIPTION);

	if (fts != FT_OK)
	{
   		USB_Errno = fts;
	}
	else
	{
	 int i;

		for(i = 0; i < num_dev; i++)
		{
			char buffer[64];

			UsbPrintf("Device # %d : %s\n",  i, strArray[i]);
			fts = GetDeviceDesc(buffer, i);
			if(fts == FT_OK)
			{
//				UsbPrintf("Device description : %s\n", buffer);
				fts = GetDeviceSerNum(buffer, i);
				if(fts == FT_OK)
					UsbPrintf("Serial number : %s\n", buffer);
			}
			UsbPrintf("----\n");
		}
	}
	return fts;
}

/* =========================================================================== */
int USB_GetNumberOfDevs(void)
/* =========================================================================== */
{
 FT_STATUS fts = GetNumberOfDevices(&numDevs);

	if(fts != FT_OK)		// Can't get the number of connected devices
		return -1;		// Errmsg --> TO BE DONE
	else
		return numDevs;
}


/* =========================================================================== */
BOOL USB_FindDevices(char *DeviceDescriptionStr)
/* =========================================================================== */
{
 int index = 0;
 FT_STATUS fts;

	USB_CloseAll(); // if descriptors were already open let's close them.

	fts = GetNumberOfDevices(&numDevs);
	if(fts != FT_OK)		// Can't get the number of connected devices
   		return FALSE;		// Errmsg --> TO BE DONE
	else
	{
		if(numDevs == 0)
		{
			UsbPrintf("No device found\n"); // There is no ftdi-chip based device connected to the system
	   		USB_Errno = FT_DEVICE_NOT_FOUND;
   			return FALSE;
		}
		else
		{		
			UsbPrintf("*************************************************\n");
			UsbPrintf("%d usb device(s) found\n", numDevs);
			UsbPrintf("*************************************************\n");
			
			if(DeviceDescriptionStr)
			{
				char buffer[64];
				for(index = 0; index < MAXDEVS; index++)
				{
					fts = GetDeviceDesc(buffer, index);
					if(fts == FT_OK)
					{
						if(!strcmp(DeviceDescriptionStr, buffer))
						{
							return TRUE;
						}
					}
				}
			}
			else
                fts = GetAllDeviceDesc(BufPtrs);

			if(fts!=FT_OK)
			{
				UsbPrintf( "Cannot get device description strings\n");
				return FALSE;
			}
			return TRUE;
		}
	}
}

/* =========================================================================== */
int OpenUsbDevice(char *sernumStr)
/* =========================================================================== */
{
	FT_STATUS fts;
	FT_HANDLE fth;
	int index;

	for( index = 0; index < numDevs; index++)
	{	
		if(!UsbDevice[index])
		{
			fts = FT_OpenEx(sernumStr, FT_OPEN_BY_SERIAL_NUMBER, &fth);
			if(fts != FT_OK)
			{
   				USB_Errno = fts;
			}
			else
			{
				UsbDevice[index] = fth;
//				CurrentIndex = index;
				UsbPrintf("*************************************************\n");
				UsbPrintf("Connected to the device with Serial Number %s\nOpened with handle : %08X - internal ID : %d\n",
							sernumStr, fth, index + 1);
				UsbPrintf("*************************************************\n");
				return index + 1;
			}
		}
	}
	return -1; 
}

/* =========================================================================== */
void CloseUsbDevice(int id)
/* =========================================================================== */
{
	int i;

	if(UsbDevice[id-1] != NULL)
	{
		FT_Close (UsbDevice[id-1]);
		UsbDevice[id-1] = NULL;
//		CurrentIndex = 0;
	}

	for(i = 0; i < MAXDEVS; i++)
	{
		if(BufPtrs[i] != NULL)
		{
			free(BufPtrs[i]);
			BufPtrs[i] = NULL;
		}
	}
}

/* =========================================================================== */
BOOL USB_Init(int usb_id, BOOL verbose)
/* =========================================================================== */
{
 UCHAR lat_timer;
 FT_STATUS fts;
 DWORD deviceID, EventDWord, RxBytes, TxBytes;
 char SerialNumber[16];
 char Description[64];
 
	if(verbose)
	{
		FT_GetLatencyTimer(UsbDevice[usb_id -1], &lat_timer);
		UsbPrintf("Latency timer : %d ms\n", (int) lat_timer);
		
		fts = FT_GetDeviceInfo(UsbDevice[usb_id -1],
						&ftDeviceType,
						&deviceID,
						SerialNumber,
						Description,
						NULL
						);
		if(fts != FT_OK)
		{
			CloseUsbDevice(usb_id);
			UsbDevice[usb_id -1] = NULL;
			USB_Errno = fts;
			return FALSE;
		}

	if(FT_EE_UASize(USB_GetDeviceHandle(usb_id), &ua_size) == FT_OK)
		UsbPrintf("Eeprom UA size %d\n", ua_size);
	else
		UsbPrintf("Cannot retrieve user area EEPROM size\n"); // modified 11/01/11 (spell_checking)
		
		switch(ftDeviceType)
		{
			case FT_DEVICE_AM :
			case FT_DEVICE_BM :
				UsbPrintf("Device type : %s\n",  (ftDeviceType == FT_DEVICE_BM) ? "FT2XXBM" : "FT8U232AM");
				break;
			case FT_DEVICE_232R :
				UsbPrintf("Device type : FT2XXR\n");
				break;
			case FT_DEVICE_2232C :
				UsbPrintf("Device type : FT2232C\n");
				break;
			case FT_DEVICE_2232H :
				UsbPrintf("Device type : FT2232H\n");
				break;
			case FT_DEVICE_4232H :
				UsbPrintf("Device type : FT4232H\n"); // modified 11/01/11 (replaced FT2232H with FT4232H)
				break;
			default :
				UsbPrintf("Unknown device type\n");
				break;
		}
		
		UsbPrintf("Desc. : %s\nS/N : %s\n", Description, SerialNumber);
		FT_GetStatus(UsbDevice[usb_id -1], &RxBytes, &TxBytes, &EventDWord);
		UsbPrintf("Rx Queue : %d bytes left\nTx Queue : %d bytes left\nEvents : %d\n\n",RxBytes, TxBytes, EventDWord);
	}

	if(!USB_ResetDevice(usb_id))
		return FALSE;
	if(!USB_PurgeBuffers(usb_id))
		return FALSE;
	if(!USB_ResetDevice(usb_id))
		return FALSE;
	if(!USB_SetLatencyTimer(usb_id, 2))
		return FALSE;
	if(!USB_SetTimeouts(usb_id, 1000, 1000)) // modified 11/01/11 (previously 2ms)
		return FALSE;
	if(!USB_SetXferSize(usb_id, 0x1000, 0x8000))
		return FALSE;

	return TRUE;
}

/* =========================================================================== */
static FT_HANDLE USB_GetDeviceHandle(int index)
/* =========================================================================== */
{
	return UsbDevice[index - 1];
}

/* =========================================================================== */
void USB_CloseAll(void)
/* =========================================================================== */
{
	int index;

		for(index = 0; index < MAXDEVS; index++)
		{		
			if(UsbDevice[index] != NULL)
			{
				FT_Close (UsbDevice[index]);
			}
			UsbDevice[index] = NULL;
			
			if(BufPtrs[index] != NULL)
			{
				free(BufPtrs[index]);
				BufPtrs[index] = NULL;
			}
		}

//		CurrentIndex = 0;
}

/* =========================================================================== */
int USB_GetStatus(int id, TXRX_STATUS stat)
/* =========================================================================== */
{
	FT_STATUS fts;
	DWORD EventDWord, RxBytes=0, TxBytes=0;
	FT_HANDLE fth = USB_GetDeviceHandle(id);

	fts = FT_GetStatus(fth, &RxBytes, &TxBytes, &EventDWord);
	
	if (fts != FT_OK)
	{
   		USB_Errno = fts;
		return -1;
	}
	else
	{
//		UsbPrintf("RxQueue : %d bytes left, TxQueue : %d bytes left\n",RxBytes, TxBytes);
	}
	if(stat == RXSTATUS)
		return RxBytes;
	else
		return TxBytes;
}

/* =========================================================================== */
int USB_GetStatusEx(int id, TXRX_STATUS stat, DWORD *pEventDWord)
/* =========================================================================== */
{
	FT_STATUS fts;
	DWORD RxBytes, TxBytes;
	FT_HANDLE fth = USB_GetDeviceHandle(id);

	fts = FT_GetStatus(fth, &RxBytes, &TxBytes, pEventDWord);
	
	if (fts != FT_OK)
	{
   		USB_Errno = fts;
		return -1;
	}
	else
	{
//		UsbPrintf("RxQueue : %d bytes left, TxQueue : %d bytes left\n",RxBytes, TxBytes);
	}
	if(stat == RXSTATUS)
		return RxBytes;
	else
		return TxBytes;
}

/* =========================================================================== */
BOOL USB_Read(int id, void *buf, int maxcnt, int *rdcnt)
/* =========================================================================== */
{
 FT_STATUS fts =FT_OK;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		fts = FT_Read(fth, (void*) buf, maxcnt, rdcnt); 

		if(fts != FT_OK)
		{
			USB_Errno = fts;
			return FALSE;
		}
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}

/* =========================================================================== */
BOOL USB_Write(int id, void *buf, int count, int *written)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{	
		fts = FT_Write(fth, (LPVOID) buf, count, written); 

		if(fts != FT_OK)
		{
            USB_Errno = fts;
			return FALSE;
		}
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	} 
	return TRUE;
}

/* =========================================================================== */
BOOL USB_ResetDevice(int id)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		fts = FT_ResetDevice(fth);
		if(fts != FT_OK)
		{
   			USB_Errno = fts;
   			return FALSE;
		}
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
 }

/* =========================================================================== */
BOOL USB_PurgeBuffers(int id)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		fts = FT_Purge(fth, FT_PURGE_TX | FT_PURGE_RX);
		if(fts != FT_OK)
		{
   			USB_Errno = fts;
   			return FALSE;
		}
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}

/* =========================================================================== */
BOOL USB_SetTimeouts(int id, int tx_timeout, int rx_timeout)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		fts = FT_SetTimeouts(fth, rx_timeout, tx_timeout);
		if(fts != FT_OK)
		{
   			USB_Errno = fts;
   			return FALSE;
		}
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}


/* =========================================================================== */
BOOL USB_SetBaudRate(int id, int baud)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		fts = FT_SetBaudRate(fth, baud);
		if(fts != FT_OK)
		{
	   		USB_Errno = fts;
   			return FALSE;
		}
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}

/* =========================================================================== */
BOOL USB_SetXferSize(int id, unsigned long txsize, unsigned long rxsize)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		fts = FT_SetUSBParameters(fth, rxsize, txsize);
		if(fts != FT_OK)
		{
	   		USB_Errno = fts;
   			return FALSE;
		}
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}

/* =========================================================================== */
BOOL USB_SetLatencyTimer(int id, UCHAR msecs)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		fts = FT_SetLatencyTimer(fth, msecs);
		if(fts != FT_OK)
		{
	   		USB_Errno = fts;
   			return FALSE;
		}
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}
/* =========================================================================== */
BOOL USB_GetLatencyTimer(int id, PUCHAR msecs)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		fts = FT_GetLatencyTimer(fth, msecs);
		if(fts != FT_OK)
		{
	   		USB_Errno = fts;
   			return FALSE;
		}
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}


//******* IO transfer mode management (asynchronous or synchronous)
// Created 17/02/2011
/* =========================================================================== */
BOOL USB_ResetMode(int id)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		fts = FT_SetBitMode(fth, 0xFF, 0x00);
		if(fts != FT_OK)
		{
   			USB_Errno = fts;
			return FALSE;
		}
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	
	return TRUE;
}

/* =========================================================================== */
BOOL USB_SetSynchronousMode(int id, int sleep_time)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(ftDeviceType != FT_DEVICE_2232H)
		return FALSE;

	if(sleep_time)
	{
		if(fth)
		{
			if(USB_ResetMode(id))
			{
//				Sleep(sleep_time);
				fts = FT_SetBitMode(fth, 0xFF, 0x40);
				if(fts != FT_OK)
				{
					USB_Errno = fts;
					return FALSE;
				}

			}
			else
				return FALSE;
		}
		else
		{
			USB_Errno = FT_INVALID_HANDLE;
			return FALSE;
		}
	}

	return TRUE;
}

/* =========================================================================== */
BOOL USB_SetFlowControlToHardware(int id)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		fts = FT_SetFlowControl(fth, FT_FLOW_RTS_CTS, 0, 0);
		if(fts != FT_OK)
		{
			USB_Errno = fts;
			return FALSE;
		}
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	
	return TRUE;
}

//****************************** EEPROM management *****************************


/* =========================================================================== */
void InitializeEEPROMData(void)
/* =========================================================================== */
{
	memset(&EpromData, 0, sizeof(FT_PROGRAM_DATA));
	EpromData.Signature1 = 0x00000000;
	EpromData.Signature2 = 0xffffffff;

	EpromData.Version = 0x00000000; // modified 25/03/08
	EpromData.VendorId = 0x0403;

	EpromData.Manufacturer = Manufacturer;
	EpromData.ManufacturerId = ManufacturerId;
	EpromData.Description = Description;
	EpromData.SerialNumber = SerialNumber;
	EpromData.MaxPower = MaxPower;
	EpromData.PnP = 1;
	EpromData.SelfPowered = SelfPowered;

	switch(ftDeviceType)
	{

		case FT_DEVICE_AM :
		case FT_DEVICE_BM :
		default :
			EpromData.ProductId = 0x6001;		// modified 18/05/10			
			EpromData.Rev4 = 1;
			EpromData.USBVersionEnable = 0x0001;
			EpromData.PullDownEnable = 0;
			EpromData.SerNumEnable = 1;
			EpromData.USBVersion = 0x0200;
			break;

		case FT_DEVICE_232R :
			EpromData.ProductId = 0x6001;		// modified 18/05/10			
			EpromData.Version = 0x00000002;		// modified 25/03/08
			EpromData.USBVersionEnable = 0x0001;
			EpromData.USBVersion = 0x0200;
			EpromData.UseExtOsc = 0;			// Use External Oscillator
			EpromData.HighDriveIOs = 0;			// High Drive I/Os
			EpromData.EndpointSize = 64;			// Endpoint size
//			EpromData.MaxPower = 0x5a;
			EpromData.PnP = 1;
			EpromData.PullDownEnableR = 0;		// non-zero if pull down enabled
			EpromData.SerNumEnableR = 1;		// non-zero if serial number to be used
			EpromData.RemoteWakeup = 0;

			EpromData.Cbus0 = 3;				// Cbus Mux control
			EpromData.Cbus1 = 2;				// Cbus Mux control
			EpromData.Cbus2 = 0;				// Cbus Mux control
			EpromData.Cbus3 = 1;				// Cbus Mux control
			EpromData.Cbus4 = 5;				// Cbus Mux control

			EpromData.RIsD2XX = 1;				// non-zero if using D2XX drivers
			break;

		case FT_DEVICE_2232C :
			EpromData.ProductId = 0x6010;		// modified 18/05/10			
			EpromData.Version = 0x00000001; // modified 25/03/08
			EpromData.Rev5 = 1;
			EpromData.USBVersionEnable5 = 0x0001;
			EpromData.PullDownEnable5 = 0;
			EpromData.SerNumEnable5 = 1;
			EpromData.USBVersion5 = 0x0200;
			EpromData.IFAIsFifo = 1;
			EpromData.IFBIsFifo = 0;
			break;

		case FT_DEVICE_2232H :
			EpromData.ProductId = 0x6010;		// modified 18/05/10			
			EpromData.Version = 0x00000003;		// modified 18/05/10
			EpromData.PullDownEnable7 = 0;
			EpromData.SerNumEnable7 = 1;
			EpromData.IFAIsFifo7 = 1;
			EpromData.IFAIsFifoTar7 = 0;
			EpromData.IFBIsFifo7 = 1;
			EpromData.IFBIsFifoTar7 = 0;
			EpromData.AIsVCP7 = 0;
			EpromData.BIsVCP7 = 0;
			break;

		case FT_DEVICE_4232H :
			EpromData.ProductId = 0x6011;		// modified 18/05/10			
			EpromData.Version = 0x00000004;		// modified 18/05/10
			EpromData.PullDownEnable8 = 0;
			EpromData.SerNumEnable8 = 1;
			EpromData.AIsVCP8 = 0;
			EpromData.BIsVCP8 = 0;
			EpromData.CIsVCP8 = 0;
			EpromData.DIsVCP8 = 0;
			break;
	}
}

/* =========================================================================== */
void ShowEEPROMData(void)
/* =========================================================================== */
{
	UCHAR USBVersionEnable, SerNumEnable, PullDownEnable;
	WORD USBVersion; //, i, data[1024];
	DWORD dwBytesRead = 0;
	
	switch(ftDeviceType)
	{
		case FT_DEVICE_2232C :
			USBVersionEnable = EpromData.USBVersionEnable5;
			SerNumEnable = EpromData.SerNumEnable5;
			USBVersion = EpromData.USBVersion5;
			PullDownEnable = EpromData.PullDownEnable5;
			break;
		case FT_DEVICE_2232H :
			USBVersionEnable = 1;
			USBVersion = 0x0200;
			SerNumEnable = EpromData.SerNumEnable7;
			PullDownEnable = EpromData.PullDownEnable7;
			break;

		case FT_DEVICE_232R :
			USBVersionEnable = EpromData.USBVersionEnable;
			SerNumEnable = EpromData.SerNumEnableR;
			USBVersion = EpromData.USBVersion;
			PullDownEnable = EpromData.PullDownEnableR;
			break;
		case FT_DEVICE_AM :
		case FT_DEVICE_BM :
		default :
			USBVersionEnable = EpromData.USBVersionEnable;
			SerNumEnable = EpromData.SerNumEnable;
			USBVersion = EpromData.USBVersion;
			PullDownEnable = EpromData.PullDownEnable;
			break;
	}

	UsbPrintf("*********************************************\n");
	UsbPrintf("Product ID       %04X\n",EpromData.ProductId);
	UsbPrintf("Vendor ID        %04X\n",EpromData.VendorId);
	UsbPrintf("Manufacturer     %s\n",EpromData.Manufacturer);
	UsbPrintf("Manufacturer ID  %s\n",EpromData.ManufacturerId);
	UsbPrintf("Description      %s\n",EpromData.Description);
	UsbPrintf("Serial number    %s (%s)\n",EpromData.SerialNumber, 
											(SerNumEnable == 1)? "Enabled" : "Disabled");
	UsbPrintf("Power type       %s\n", (EpromData.SelfPowered)? "Self powered":"Bus powered");
	UsbPrintf("Max power        %dmA\n",EpromData.MaxPower);
	UsbPrintf("USB Version  is  %s\n",(USBVersionEnable)? "Enabled" : "Disabled");
	UsbPrintf("USB Version      %04X\n",USBVersion);
	UsbPrintf("Pull down are %s\n",(PullDownEnable)? "Enabled" : "Disabled");

	if(EpromData.Rev4 == 1)
	{
		UsbPrintf("Rev4 device\n");
	}

	if(EpromData.Rev5 == 1)
	{
		UsbPrintf("2232C ports :\n");
		UsbPrintf("2232C A port is in  %s mode\n",(EpromData.IFAIsFifo == 1)? "FIFO":"SERIAL");
		UsbPrintf("2232C B port is in  %s mode\n",(EpromData.IFBIsFifo == 1)? "FIFO":"SERIAL");
	}
	
	if(ftDeviceType == FT_DEVICE_2232H)
	{
		UsbPrintf("2232H ports :\n");
		if(EpromData.IFAIsFastSer7 == 1)
		{
			UsbPrintf("Port A is in Fast Serial mode\n");
		}
		else
		{
			if(EpromData.IFAIsFifo7 == 1)
				UsbPrintf("Port A is in  245 FIFO mode\n");
			else
				if(EpromData.IFAIsFifoTar7 == 1)
					UsbPrintf("Port A is in  CPU FIFO mode\n");
				else
					UsbPrintf("Port A : unknown\n");
		}

		if(EpromData.IFBIsFastSer7 == 1)
		{
			UsbPrintf("Port B is in Fast Serial mode\n");
		}
		else
		{
			if(EpromData.IFBIsFifo7 == 1)
				UsbPrintf("Port B is in  245 FIFO mode\n");
			else
				if(EpromData.IFBIsFifoTar7 == 1)
					UsbPrintf("Port B is in CPU FIFO mode\n");
				else
					UsbPrintf("Port B : unknown\n");
		}

	}
/*	
	for(i = 0; i < 0x80; i++)
	{
		FT_ReadEE(USB_GetDeviceHandle(1), i, &data[i]); 
		if(!(i%16))
			UsbPrintf("\n");
		UsbPrintf("%04X ", data[i]);

	}*/
	UsbPrintf("\n*********************************************\n");
}

/* =========================================================================== */
BOOL USB_EraseEEpromData(int id)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
//		InitializeEEPROMData();
		UsbPrintf("\nErasing EEPROM...");
		fts = FT_EraseEE(fth);

		if(fts != FT_OK)
		{
	   		USB_Errno = fts;
			UsbPrintf("Error\n\n");
   			return FALSE;
		}
		UsbPrintf("Done\n\n");
	
/*		UsbPrintf("\nTrying to unplug and re-plug your device...");
		fts = FT_CyclePort(fth);
		if(fts != FT_OK)
		{
	   		UsbPrintf("failed\n");
			USB_Errno = fts;
   			return FALSE;
		}
*/		UsbPrintf("Done\n");
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}

/* =========================================================================== */
BOOL USB_ReadEEpromData(int id, BOOL verbose)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		InitializeEEPROMData();
		if(verbose)
			UsbPrintf("\nReading data from EEPROM...");

		fts = FT_EE_Read(fth, &EpromData);
		if(fts != FT_OK)
		{
	   		USB_Errno = fts;
			if(verbose) 
				UsbPrintf("Error\n\n");
   			return FALSE;
		}

		if(verbose)
		{
			UsbPrintf("\n");
			ShowEEPROMData();
			UsbPrintf("Ok\n\n");
		}
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}

/* =========================================================================== */
BOOL USB_WriteEEpromData(int id)
/* =========================================================================== */
{
 FT_STATUS fts;
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		InitializeEEPROMData();
//		ShowEEPROMData();
		UsbPrintf("\nWriting data to EEPROM...");
		fts = FT_EE_Program(fth, &EpromData);
		if(fts != FT_OK)
		{
	   		UsbPrintf("failed\n");
			USB_Errno = fts;
   			return FALSE;
		}   		
		UsbPrintf("Ok\n");

/*		UsbPrintf("\nTrying to unplug and re-plug your device...");
		fts = FT_CyclePort(fth);
		if(fts != FT_OK)
		{
	   		UsbPrintf("failed\n");
			USB_Errno = fts;
   			return FALSE;
		}
		UsbPrintf("Done\n");
*/
		ShowEEPROMData();

	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}

/* =========================================================================== */
BOOL USB_SetSerialNumber(int id, char *sernum)
/* =========================================================================== */
{
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		if(sernum == NULL)
		{
			USB_Errno = NULL_SERNUM_PTR;
			return FALSE;
		}
		
		strncpy(SerialNumber, sernum, strlen(sernum));
		SerialNumber[strlen(sernum)] = 0;
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}

/* =========================================================================== */
char *USB_GetDefaultSerialNum(void)
/* =========================================================================== */
{
	return SerialNumber;
}

/* =========================================================================== */
char *USB_GetEpromSerialNum(void)
/* =========================================================================== */
{
	return EpromData.SerialNumber;
}

/* =========================================================================== */
BOOL USB_SetDescStr(int id, char *devdesc)
/* =========================================================================== */
{
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		if(devdesc == NULL)
		{
			USB_Errno = NULL_DESCSTR_PTR;
			return FALSE;
		}
		strncpy(Description, devdesc, strlen(devdesc));
		Description[strlen(devdesc)] = 0;

//		if(!USB_WriteEEpromData(id))
//			return FALSE;
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}

/* =========================================================================== */
BOOL USB_SetPowerSource(int id, unsigned short powersrc)
/* =========================================================================== */
{
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		SelfPowered = powersrc;
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}

/* =========================================================================== */
BOOL USB_SetMaxPower(int id, unsigned short current)
/* =========================================================================== */
{
 FT_HANDLE fth = USB_GetDeviceHandle(id);

	if(fth)
	{
		MaxPower = current;
	}
	else
	{
		USB_Errno = FT_INVALID_HANDLE;
		return FALSE;
	}
	return TRUE;
}


/* =========================================================================== */
char *USB_GetDefaultDeviceDesc(void)
/* =========================================================================== */
{
	return Description;
}

/* =========================================================================== */
char *USB_GetEpromDeviceDesc(void)
/* =========================================================================== */
{
	return EpromData.Description;
}

/*********************** Error handling and printng ***************************/

/* =========================================================================== */
char *GetErrMsg(USB_Error err_code)
/* =========================================================================== */
{
 if(err_code < Max_USB_Errors && err_code >= 0)
	 return(USB_ErrMsg[err_code]);
 else
	 return USB_ErrMsg[FT_OTHER_ERROR];
}

/* =========================================================================== */
void USB_PrintErrMsg(char *msg)
/* =========================================================================== */
{
	printf("Error : %s\n", msg);
}

/* =========================================================================== */
void USB_Perror(USB_Error err_code)
/* =========================================================================== */
{
	char *msg = GetErrMsg(err_code);
	if(msg)
		UsbpfPerror(msg);
}

/* =========================================================================== */
void USB_SetPerrorFunc(PerrorFunc func)
/* =========================================================================== */
{
   if(func != NULL)
   		UsbpfPerror = func;
}

/* =========================================================================== */
USB_Error USB_GetLastError(void)
/* =========================================================================== */
{
 USB_Error err;
	
	if((USB_Errno >= 0) && (USB_Errno < Max_USB_Errors))
		err = USB_Errno;
	else
		err = FT_OTHER_ERROR;

	USB_Errno = FT_OK;
	
	return err;
}

/* =========================================================================== */
void SetErrorToInvalidHandle(void)
/* =========================================================================== */
{
	USB_Errno = FT_INVALID_HANDLE;
}

/* =========================================================================== */
void SetUsbErrno(USB_Error err_code)
/* =========================================================================== */
{
	if(err_code < Max_USB_Errors && err_code >= 0)
		USB_Errno = err_code;
	else
		USB_Errno = FT_OTHER_ERROR;
}

/* =========================================================================== */
USB_Error GetUSB_Errno(void)
/* =========================================================================== */
{
	return USB_Errno;
}

/* =========================================================================== */
void USB_Printf(char *format, ...)
/* =========================================================================== */
{
	printf(format);
}

/* =========================================================================== */
void USB_SetPrintfFunc(PrintfFunc func)
/* =========================================================================== */
{
   if(func != NULL)
   		UsbPrintf = func;
}

#endif // __USB_C__
