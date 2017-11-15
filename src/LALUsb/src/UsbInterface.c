//
//	MODULE: UsbInterface.c
//			LAL Frames management module for LalUsb Library 
//			Author: Chafik CHEIKALI
//			Created: February 2006
//			Last modified :
//				

#include "LALUsb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//Frame format : <Header><N-1><(R|W)|SubAddr><B0><B1>...<BN-1><BN><Trailer>
typedef enum
{
	ftDATA,
	ftERROR,
	ftINCOMPLETE,
	ftINTERRUPT
}FrameType;

FrameType ChkFrame(char *buffer,ULONG size, BOOL long_read); // USB receive buffer size
static int FindHeader(unsigned char *buffer, int size); // scans the entire frame size in order to find a header
static int GetDataSize(unsigned char *buffer); // returns the payload size
static BOOL CheckTrailer(unsigned char trailer);
static BOOL IsInterrupt(unsigned char *buffer, int nbytes);

static ULONG TmpPos = 0;
static ULONG CurPos = 0;
static LONG Header_Pos = 0;
static ULONG TotalByteCount = 0;
static ULONG Header_Errors = 0;
static ULONG Trailer_Errors = 0;
static InterruptFrameInfo LastFrame;
static BOOL bCheckInterrupts = TRUE;
static char OutBuffer[32*1024];
static char InBuffer[64*1024];
static char InterruptFrame[5];
static ULONG nLostFrames = 0;
static BOOL bHeaderFakeError = FALSE;
static BOOL bTrailerFakeError = FALSE;
static BOOL bCountFakeError = FALSE;

void		SetErrorToInvalidHandle(void);
void		SetUsbErrno(USB_Error err_code);
int			UsbCmd						(int id, char sub_addr, int inout, void *buffer, int count);
int			ProcessInterrupt			(int id, char sub_addr, char *buf, int inout, int expected_bytes);
int			UsbHostRd					(int id, char sub_addr, void *array, int count);

extern PrintfFunc   UsbPrintf;

#ifdef _USRDLL

FILE *Usblogfile = NULL;

/* =========================================================================== */
BOOL APIENTRY DllMain( HINSTANCE  hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
/* =========================================================================== */
{
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
		InitProcessorTimerCaps();
#ifdef _DEBUG
		printf("attached\n");
#endif
		break;
	case DLL_PROCESS_DETACH:
		USB_CloseAll();
		break;
	}
    return TRUE;
}

/* =========================================================================== */
void dbg_OpenLogFile(void)
/* =========================================================================== */
{
	if(Usblogfile == NULL)
	{
		Usblogfile = fopen("USB_LogFile.bin","wb");
	}
}

/* =========================================================================== */
void dbg_CloseLogFile(void)
/* =========================================================================== */
{
	if(Usblogfile)
	{
		fclose(Usblogfile);
		Usblogfile = NULL;
	}

}
#endif

/* =========================================================================== */
void GenerateFakeError(FakeErrorType fet, BOOL onoff)
/* =========================================================================== */
{
	switch(fet)
	{
		case FK_HEADER:
			bHeaderFakeError = onoff;
			break;
		case FK_TRAILER:
			bTrailerFakeError = onoff;
			break;
		case FK_COUNT:
			bCountFakeError = onoff;
			break;
		case FK_LETSBETRUE:
			if(onoff)
			{
				bHeaderFakeError = FALSE;
				bTrailerFakeError = FALSE;
				bCountFakeError = FALSE;
			}
			break;
	}
}

/* =========================================================================== */
int FindHeader(unsigned char *buffer, int size)
/* =========================================================================== */
{
	int pos = -1, index = 0;

	while(index < size)
	{
		if(buffer[index] == FRAME_HEADER)
		{
			pos = index;
			break;
		}
		else
		{
			index++;
//			buffer++;
		}
	}
	return pos;
}

/* =========================================================================== */
BOOL CheckTrailer(unsigned char trailer)
/* =========================================================================== */
{
	BOOL ret;

	if(trailer == FRAME_TRAILER)
		ret = TRUE;
	else
		ret = FALSE;

	return ret;

//	return (trailer == FRAME_TRAILER);
}

/* =========================================================================== */
int GetDataSize(unsigned char *buffer)
/* =========================================================================== */
{
	return *(buffer+1);
}

/* =========================================================================== */
BOOL IsInterrupt(unsigned char *buffer, int nbytes)
/* =========================================================================== */
{
	if(nbytes == 0) // if more than 1 character --> not an interrupt
	{
		if(!(buffer[2] & 0x80))
			return TRUE;
	}
	return FALSE;
}

/* =========================================================================== */
ULONG GetHeaderErrors(void)
/* =========================================================================== */
{
	return Header_Errors;
}

/* =========================================================================== */
ULONG GetTrailerErrors(void)
/* =========================================================================== */
{
	return Trailer_Errors;
}

/* =========================================================================== */
void ResetFrameErrors(void)
/* =========================================================================== */
{
	Header_Errors = 0;
	Trailer_Errors = 0;
}

/* =========================================================================== */
FrameType ChkFrame(char *buffer, ULONG bufsize, BOOL long_read)
/* =========================================================================== */
{
 FrameType res = ftERROR;
 ULONG tpos, n;
 BOOL done = TRUE;

	do
	{
		Header_Pos = FindHeader(buffer, bufsize);
		if(Header_Pos >= 0)
		{
			if(Header_Pos)				// Header is not at the first position
			{
				Header_Errors++;		// we write it down as a header error
				if(TmpPos)				// An incomplete frame is in progress
				{
					TmpPos = 0;			// Let's throw it...
					nLostFrames++;		// ... because it's lost
				}
				buffer += Header_Pos;		// we reach the new header pos
			}
			
			if(!long_read)
				n = GetDataSize(buffer);	// we get the number of bytes from header_pos + 1
			else
				n = (bufsize - 1) - 4;
			tpos = n + 4; // Header_Pos + n + 4;

			if(tpos < bufsize)
			{
				if(CheckTrailer(buffer[tpos]))
				{
					if(IsInterrupt(buffer, n))
						res = ftINTERRUPT;
					else
					{
						res = ftDATA;
						CurPos += Header_Pos + tpos + 1;
					}
					TmpPos = 0;
					done = TRUE;
				}
				else
				{
					SetUsbErrno(BAD_TRAILER_ERROR);
					Trailer_Errors++;
					CurPos += (Header_Pos + 1);
					buffer += (Header_Pos + 1);
					bufsize -= (Header_Pos + 1); // +1 only ?
					done = FALSE;
					TmpPos = 0;
					nLostFrames++;
				}
			}
			else
			{
				res = ftINCOMPLETE; 
				TmpPos = bufsize - Header_Pos;
				memcpy(InBuffer, &InBuffer[CurPos], TmpPos); // To be tried : memcpy(InBuffer, buffer, TmpPos);
				done = TRUE;
//				if(tpos == bufsize)
//						Trailer_Errors++;
			}
		}
		else
		{
//		USB_PurgeBuffers(usbid);
			Header_Errors++;
			SetUsbErrno(BAD_HEADER_ERROR);
			done = TRUE;
		}
	}while(!done);

	return res;
}

/* =========================================================================== */
int ProcessInterrupt(int id, char sub_addr, char *buffer, int inout, int expected_bytes)
/* =========================================================================== */
{
 int read = 0, in_bytes = 0;

	memset(&LastFrame, 0, sizeof(InterruptFrameInfo));
	LastFrame.source = FE_SOFTWARE;
	LastFrame.err_code = FrameNoErr; 
	LastFrame.io_dir = inout;
	LastFrame.sub_addr = sub_addr;
	LastFrame.type = FE_NONE;
	LastFrame.error_type = FE_NONE;

	if(inout == DATA_WRITE)
	{
//		Sleep(1);
		in_bytes = USB_GetStatus(id, RXSTATUS);

		if((in_bytes != 5)|| (in_bytes == 0))
			return 0;

		if(!USB_Read(id, InterruptFrame, 5, &read))
		{
			LastFrame.err_code = GetUSB_Errno();
			return -1;
		}

		if(read == 0)
			return 0;
	}
	else
	{
		memcpy(InterruptFrame, buffer, 5);
		read = expected_bytes - 5;
		if(read > 0)
		{
			TmpPos = read;
			memcpy(buffer, buffer + 5, read);
			if(!USB_Read(id, buffer + TmpPos, 5, &read))
			{
				LastFrame.err_code = GetUSB_Errno();
				return -1;
			}
			read += TmpPos;
//			CurPos = read + 1; ?
			TmpPos = 0;
		}

	}

	UsbPrintf("Interrupt Frame received at sub_addr %02X (%s mode) ", sub_addr & 0x000000FF, 
							(inout == DATA_WRITE)? "write":"read");
	LastFrame.source = FE_DEVICE;
	LastFrame.last_sub_addr = InterruptFrame[2] & 0x7F;
	LastFrame.status = InterruptFrame[3];

	if(InterruptFrame[2] == 0x7F)
	{
		UsbPrintf("generated by user, status : %02X\n", InterruptFrame[3] & 0x000000FF);
		LastFrame.type = FE_USER;
		SetUsbErrno(USB_INT_FRAME);
		read = -5;
	}
	else
	{
	 char szErrString[16]="";

		if(InterruptFrame[3] & 0x01)
		{
			LastFrame.error_type = FE_HEADER;
			strcpy(szErrString,"Header");
			SetUsbErrno(BAD_HEADER_ERROR);
		}

		if(InterruptFrame[3] & 0x02)
		{
			LastFrame.error_type = FE_TRAILER;
			strcpy(szErrString,"Trailer");
			SetUsbErrno(BAD_TRAILER_ERROR);
		}

		if(InterruptFrame[3] == 0)
		{
			LastFrame.error_type = FE_SOFTWARE;
			strcpy(szErrString,"Unknown");
			SetUsbErrno(USB_INT_FRAME);
		}

		UsbPrintf("%s error - status %02X\n", szErrString,InterruptFrame[3]);
		read = -5;
//		return -1;
	}

	if(inout == DATA_READ)
	{
		if(expected_bytes) 
			if(read != expected_bytes) 
				read = -1;
	}

	return read;
}

/* =========================================================================== */
void UsbSetIntCheckingState(BOOL truefalse)
/* =========================================================================== */
{
	bCheckInterrupts = truefalse;
}

/* =========================================================================== */
int UsbCmd(int id, char sub_addr, int inout, void *buffer, int count)
/* =========================================================================== */
{
	int written = 0, maxcnt = (inout == DATA_WRITE)? 256 : 65536;
	char trailer = (bTrailerFakeError) ? 0x38 : FRAME_TRAILER;

	if(count > maxcnt)
		count = maxcnt;

	memset(OutBuffer, 0, count + 4);
	OutBuffer[0] = (bHeaderFakeError) ? 0xAB:FRAME_HEADER;						// Header
	OutBuffer[1] = (bCountFakeError) ? count + 5 : (count - 1) & 0x000000FF;	// WordCount - 1
	OutBuffer[2] = sub_addr;													// Sub-address

	if(inout == DATA_WRITE)
	{
		OutBuffer[2] &= DATA_WRITE;
		memcpy(&OutBuffer[3], (char*) buffer, count);							// data copy
		OutBuffer[count + 3] = trailer;											// Trailer
	}
	else 
	{
		OutBuffer[2] |= DATA_READ;
		OutBuffer[3] = (char) ((((count-1) & 0xFFFF) & 0xFF00) >> 8);			// Extended count byte
		OutBuffer[4] = trailer;													// Trailer
	}

	if(id > 0 && id < MAXDEVS)
	{
		int bytesToWrite = (inout == DATA_WRITE) ? count + 4 : 5;
		
		if(USB_Write(id, OutBuffer, bytesToWrite, &written))
		{	
			if(written != (bytesToWrite))
			{
				UsbPrintf("Warning : only %d bytes of %d have actually been written\n",
					written, bytesToWrite);
				SetUsbErrno(IO_TIMEOUT_WRITE);
				return -1;
			}
			return(written - 4);
		}
		else
		{
			UsbPrintf("Lib info : Write error\n");
			return -1;
		}
	}
	else
		SetErrorToInvalidHandle();

	return -1;
}

/* =========================================================================== */
int UsbWrt(int id, char sub_addr, void *buffer, int count)
/* =========================================================================== */
{
 int bytes = UsbCmd(id, sub_addr, DATA_WRITE, buffer, count);
 long double t = 0.;

	if(bytes < 0)
		return bytes;

	if(bCheckInterrupts)
	{
	 int result = ProcessInterrupt(id, sub_addr, InBuffer, DATA_WRITE, 0); 
	
		if(result < 0)
		{
			bytes = result;
			if(result == -1)
				USB_PurgeBuffers(id);
		}
	}

	return bytes;
}

/* =========================================================================== */
int UsbRead(int id, char sub_addr, void *array, int usercount)
/* =========================================================================== */
{
 char saddr;
 int readbytes = -1;
 FrameType ft;
 BOOL long_read = FALSE;
 ULONG actual_count = usercount + 4; // Add a mechanism to check the upper limit of user count
 long double t = 0.;

	if(id > 0 && id < MAXDEVS)
	{
		nLostFrames = 0;
		memset(InBuffer, 0, actual_count);
		if(usercount > 256)
			long_read = TRUE;
		if(UsbCmd(id, sub_addr, DATA_READ, InBuffer, usercount) < 0)
		{
			return -1;
		}
// GetStatus should be inserted here and its result must be greater or equal than usercount + 4
		if(!USB_Read(id, InBuffer + TmpPos, actual_count, &readbytes))
		{
			LastFrame.err_code = GetUSB_Errno();
			return -1;
		}
		
		TmpPos = 0;
		if(readbytes == 0)
		{
			return 0;
		}
		else
			actual_count = readbytes;

		ft = ChkFrame(InBuffer, actual_count, long_read);

		if(ft == ftERROR)
		{
			USB_PurgeBuffers(id);
			return -1;
		}

		if(ft == ftINTERRUPT)
		{
			readbytes = ProcessInterrupt(id, sub_addr, InBuffer, DATA_READ, actual_count);
			if(	readbytes <= 0)  // if(readbytes < 0)
				return readbytes;  // return -1;
			actual_count = readbytes;
		}
	}
	else
	{
		SetErrorToInvalidHandle();
		return -1;
	}

	if(actual_count != (usercount + 4 ))
	{
		UsbPrintf("Warning : only %d bytes of %d have actually been read\n", actual_count, usercount + 4);
		SetUsbErrno(IO_TIMEOUT_READ);
		USB_PurgeBuffers(id);
			return -1;
	}
	else
	{
		saddr = InBuffer[2] & 0x7F;
		if(saddr != sub_addr)
		{
			SetUsbErrno(BAD_SUBADDR_ERROR);
			USB_PurgeBuffers(id);
			return -1;
		}
	}

	memcpy(array, &InBuffer[3], usercount);

	return usercount;
}

/* =========================================================================== */
int UsbRd(int id, char sub_addr, void *array, int count)
/* =========================================================================== */
{
	return UsbRead(id, sub_addr, array, count);
}


/* =========================================================================== */
ULONG GetTotalByteCount(void)
/* =========================================================================== */
{
	return TotalByteCount;
}

/* =========================================================================== */
void ResetTotalByteCount(void)
/* =========================================================================== */
{
	TotalByteCount = 0;
}

/* =========================================================================== */
int UsbRdEx(int id, void *array, int maxcnt, int *frames)
/* =========================================================================== */
{
	return UsbReadEx(id, (void *) array, maxcnt, frames, NULL);
}


/* =========================================================================== */
int UsbReadEx(int id, void *array, int maxcnt, int *frames, int *frame_size)
/* =========================================================================== */
{
 unsigned char saddr = 0x7F, *buffer = InBuffer;
 unsigned char *user_buf = (char *) array;
 int count = -1, total_count = 0, nframes = 0, cnt;
// long save_curpos;
 FrameType ft;


	if(id > 0 && id < MAXDEVS)
	{
		count	= USB_GetStatus(id, RXSTATUS);
		if(count <= 0)
			return count;		

		if(count > maxcnt)
			count = maxcnt;

		// To be tried : TmpPos = TotalByteCount + NFrames*4, with TotalByteCount = Sigma(total_count)
		memset(buffer + TmpPos, 0, count);
		CurPos = 0;
		Header_Pos = 0;

		TotalByteCount += count;
		*frames = 0;

		if(!USB_Read(id, buffer + TmpPos, count, &cnt))
		{
			UsbPrintf("Lib info : chip read error\n");
			return -1;
		}
#ifdef _USRDLL
		
		if(Usblogfile)
			fwrite(buffer + TmpPos, cnt, 1, Usblogfile);
#endif
		count += TmpPos;
		do
		{
			
			ft = ChkFrame(buffer, count - CurPos, FALSE);
			switch(ft)
			{
				int nbytes;

				case ftINTERRUPT:
					total_count = ProcessInterrupt(id, saddr, buffer, DATA_READ, 0);
					if( total_count == - 5)
					{
						CurPos += 5;
						*frames = 1; 
					}
					buffer += CurPos;
					PrintFrameInfo();
					total_count = -1;
					
					//actual_count = readbytes;
					break;			
				case ftERROR:
					return -1;
				case ftDATA:
					nbytes = buffer[1] + 1; //  CurPos - Header_Pos - 4;
					if(frame_size)
						frame_size[nframes] = nbytes;
					*frames = ++nframes;
					memcpy(user_buf, &buffer[3], nbytes);
					user_buf += nbytes;
					total_count += nbytes;
					buffer = &InBuffer[CurPos];
					break;
				case ftINCOMPLETE:
				default:
//#ifdef _DEBUG
//					UsbPrintf("\rIncomplete cnt %d cpos%d\n", count, CurPos);
//#endif
//					buffer = &InBuffer[CurPos];
//					save_curpos = CurPos;
					CurPos = count;
//					go_on = FALSE;
					break;
			}

		}while((CurPos < (ULONG) count)); // || !go_out);

//		if(ft == ftINCOMPLETE)
//			CurPos = save_curpos;

	}
	else
	{
		SetErrorToInvalidHandle();
		return -1;
	}

	return total_count;
}

/* =========================================================================== */
void PrintFrameInfo(void)
/* =========================================================================== */
{
	UsbPrintf("\n****** Frame error report start\n");
	if(LastFrame.source == FE_DEVICE)
	{	
		UsbPrintf("Source : Interrupt\n");
		UsbPrintf("Type : %s\n",(LastFrame.type == FE_USER)? "User request" : "Hardware error");
	}
	else
		UsbPrintf("Source : %s\n", (LastFrame.source != FE_SOFTWARE)? "Unknown" : "(Host) Receive error");

	if(LastFrame.type != FE_USER)
		UsbPrintf("Error : %s missing\n", (LastFrame.error_type == FE_HEADER)? "Header":"Trailer");
	
	UsbPrintf("Access : %s\n", (LastFrame.io_dir == DATA_WRITE)? "Write":"Read");
	UsbPrintf("Current sub-address %02X\n", LastFrame.sub_addr);
	UsbPrintf("Last valid sub-adress : %02X\n",LastFrame.last_sub_addr);
	UsbPrintf("User status information :%02X\n",LastFrame.status);
	UsbPrintf("****** Frame error report end\n******\n");
}

/* =========================================================================== */
void ResetLostFrames(void)
/* =========================================================================== */
{
	nLostFrames = 0;
}

/* =========================================================================== */
ULONG GetLostFrames(void)
/* =========================================================================== */
{
	return nLostFrames;
}

/* =========================================================================== */
int GetLastFrameStatus(void)
/* =========================================================================== */
{
	return LastFrame.status;
}

/* =========================================================================== */
USB_Error GetLastFrameError(void)
/* =========================================================================== */
{
	return LastFrame.err_code;
}

/* =========================================================================== */
InterruptFrameInfoPtr GetLastFrame(void)
/* =========================================================================== */
{
	return &LastFrame;
}
