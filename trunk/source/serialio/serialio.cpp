/*
   SerialIO - Basic cross-platform library for raw serial port access.

   Copyright (C) 2006,2009:
         Daniel Roggen, droggen@gmail.com

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/


#include "serialio.h"

//#include <sys/ioctl.h>

/******************************************************************************
* PLATFORM_MINGW   PLATFORM_MINGW   PLATFORM_MINGW   PLATFORM_MINGW   PLATFORM_
******************************************************************************/
#ifdef WIN32
HANDLE SerialOpen(std::string device,int bitrate,int timeout)
{
	DCB dcb;
	HANDLE hCom;
	BOOL fSuccess;

   device = std::string("\\\\.\\")+device;

	
   hCom = CreateFileA(device.c_str(),
                    GENERIC_READ | GENERIC_WRITE,
                    0,    // must be opened with exclusive-access
                    NULL, // no security attributes
                    OPEN_EXISTING, // must use OPEN_EXISTING
                    0,    // not overlapped I/O
                    NULL  // hTemplate must be NULL for comm devices
                    );

	if (hCom == INVALID_HANDLE_VALUE) 
	{
       // Handle the error.
/*       printf ("CreateFile failed with error %d.\n", GetLastError());
       
         LPVOID lpMsgBuf;
			    DWORD dw = GetLastError(); 
			
			    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
			        FORMAT_MESSAGE_FROM_SYSTEM,
			        NULL,
			        dw,
			        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			        (LPTSTR) &lpMsgBuf,
			        0, NULL );
			
			    printf("%s\n",lpMsgBuf);
			
			    LocalFree(lpMsgBuf);*/
       
       return 0;
   }
	
	// Build on the current configuration, and skip setting the size
	// of the input and output buffers with SetupComm.
	
	fSuccess = GetCommState(hCom, &dcb);
	
	if (!fSuccess) 
	{
      // Handle the error.
      printf ("GetCommState failed with error %ld.\n", GetLastError());
      return 0;
	}
	
	// Fill in DCB: 57,600 bps, 8 data bits, no parity, and 1 stop bit.
	
   dcb.BaudRate = (DWORD)bitrate;       // set the baud rate
	dcb.ByteSize = 8;             // data size, xmit, and rcv
	dcb.Parity = NOPARITY;        // no parity bit
	dcb.StopBits = ONESTOPBIT;    // one stop bit
	
	fSuccess = SetCommState(hCom, &dcb);
	
	if (!fSuccess) 
	{
		// Handle the error.
      printf ("SetCommState failed with error %ld.\n", GetLastError());
		return 0;
	}
	
	COMMTIMEOUTS CommTimeout;
	
	GetCommTimeouts(hCom,&CommTimeout);
	/*printf("Default timeouts\n");
	printf("ReadIntervalTimeout: %d\n",CommTimeout.ReadIntervalTimeout);
	printf("ReadTotalTimeoutMultiplier: %d\n",CommTimeout.ReadTotalTimeoutMultiplier);
	printf("ReadTotalTimeoutConstant: %d\n",CommTimeout.ReadTotalTimeoutConstant);
	printf("WriteTotalTimeoutMultiplier: %d\n",CommTimeout.WriteTotalTimeoutMultiplier);
	printf("WriteTotalTimeoutConstant: %d\n",CommTimeout.WriteTotalTimeoutConstant);*/
	
	
	// Infinite timeout   
	/*
	CommTimeout.ReadIntervalTimeout			= 0;
	CommTimeout.ReadTotalTimeoutMultiplier	= 0;
	CommTimeout.ReadTotalTimeoutConstant	= 0;
	*/
	
	// Finite timeout. (1ms is useful until ~115200bps)
   CommTimeout.ReadIntervalTimeout			= timeout;
	CommTimeout.ReadTotalTimeoutMultiplier	= timeout;
   CommTimeout.ReadTotalTimeoutConstant	= timeout;

   /*CommTimeout.ReadIntervalTimeout			= MAXDWORD;
   CommTimeout.ReadTotalTimeoutMultiplier	= MAXDWORD;
   CommTimeout.ReadTotalTimeoutConstant	= timeout;*/

			
	// For write we never use the timeout
	CommTimeout.WriteTotalTimeoutMultiplier	= 0;
	CommTimeout.WriteTotalTimeoutConstant	= 0;
	
	SetCommTimeouts(hCom,&CommTimeout);
	
	//printf ("Serial port %s successfully reconfigured.\n", pcCommPort);
	
	return hCom;
}
void SerialClose(HANDLE hCom)
{
	CloseHandle(hCom);
}

bool SerialIsDataAvailable(int fd)
{
	/*
		NOT IMPLEMENTED.
	*/
	return 0;
}

/*
	Return:
   0: 	character
	-1:	no character
	-2:	error
	
*/
int SerialReadChar(HANDLE hCom, char *c)
{
	//char c;
	int r;
	DWORD nr;
	
	r=ReadFile(hCom,c,1,&nr,NULL);
	//printf("read: %d %d\n",r,nr);
	if(r==1 && nr==1)
      return 0;
	else
	{
		//printf("r: %d\n",r);
		if(r==1)
			return -1;
		return -2;
	}
}



#endif	// Q_OS_WIN32


/******************************************************************************
* PLATFORM_LINUX   PLATFORM_LINUX   PLATFORM_LINUX   PLATFORM_LINUX   PLATFORM_
******************************************************************************/
#ifdef PLATFORM_LINUX

int isDevicesAvailable(char *strDir, char *strKey)
{
	DIR 			*dp;
	struct dirent 	*ep;
	int 			 num = 0;
	
	//printf ("strKey = %s is %u characters long.\n",strKey, strlen(strKey));	
	
	dp = opendir (strDir);
	
	if (dp != NULL)
	{
		 while (ep = readdir (dp))
		 {	 
			   if (strncmp (strKey,ep->d_name,strlen(strKey)) == 0)
			   {
				   printf ("Found  %s \n",ep->d_name);
				   (void) closedir (dp);
	        	   return 1;
			   }
		 }	
		 (void) closedir (dp);
	}
	else
	{
		return -1;
		perror ("Couldn't open the directory");
	}

	//printf ("serialIO::isDevicesAvailable Device %s not found \n",strKey);
	return 0;
}

int SerialOpen(int port,SERIALIO_HANDLE baudrate,int timeout) 
{
	char strDir[] = "/dev/";
	char strKey[10];
	
	sprintf(strKey,"ttyUSB%d",port);

	if (port>=10 && port<100)	// TODO: change port selection / creation!!
	{
		unsigned int ui_port = port -10;
		sprintf(strKey,"rfcomm%d",ui_port);
		printf("serialIO::SerialOpen Try to open  %s\n",strKey);
	}	
	
	if (port>=100 )	// TODO: change port selection / creation!!
	{
		unsigned int ui_port = port -100;
		sprintf(strKey,"ttyS%d",ui_port);
		printf("serialIO::SerialOpen Try to open  %s\n",strKey);
	}	
	
	if (isDevicesAvailable(strDir, strKey) == 0)
		return 0;
	else
		printf("serialIO::SerialOpen   Found %s%s!!\n",strDir, strKey); 
	
	int fd;
	struct termios oldtio,newtio;
	
	char device[20];
	strcpy(device, strDir);
	strcat(device, strKey);
		//printf("device = %s\n",device); 
	
	
//	fd = open("/dev/ttyS5", O_RDWR | O_NOCTTY | O_NDELAY); O_RDONLY
	//fd = open(device, O_RDWR );
	fd = open(device, O_RDWR | O_NOCTTY);
	if (fd <0) 
	{
		printf("serialIO: Can't open serial device port %d \n",port);
		return 0;
	}

	printf("serialIO::SerialOpen  port %d: Opening serial port returned: %d\n",port,fd);
	
	
	tcgetattr(fd,&oldtio); // save current port settings 
	
	// Start with old settings, then we'll modify 
	newtio = oldtio;
	
	//    Now do the port setup to match what the EPLD is expecting 
	
	
	// Set baudrate 
	cfsetispeed(&newtio,baudrate);
	cfsetospeed(&newtio,baudrate);
	
	// Enable the receiver and set local. 
	newtio.c_cflag |= (CLOCAL | CREAD);
	
	// Set 8 bit characters, no parity, 1 stop (8N1) 
	newtio.c_cflag &= ~PARENB;	// Clear previous parity bits values 
	newtio.c_cflag &= ~CSTOPB;	// Clear previous stop bits values 
	newtio.c_cflag &= ~CSIZE;	// Clear previous charsize bits values
	newtio.c_cflag |= CS8;	// Set for 8 bits. 
	
	// I think we want to disable hardware flow control.  FIXME  
	newtio.c_cflag &= ~CRTSCTS;
	
	// When we read, we want completely raw access  
	//newtio.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	newtio.c_lflag = 0;
	
	// I think we want to disable software flow control.  FIXME  
	//newtio.c_iflag &= ~(IXON | IXOFF | IXANY);
 	   newtio.c_iflag &= ~(IGNBRK | IGNCR | INLCR | ICRNL | IUCLC |
		    IXANY | IXON | IXOFF | INPCK | ISTRIP);
    	newtio.c_iflag |= (BRKINT | IGNPAR);
	
	// Set output filtering to raw. 
	//newtio.c_oflag &= ~OPOST;
	newtio.c_oflag = 0;
	
	// Set readsize and timeout to some reasonable values, just to be safe
	newtio.c_cc[VTIME]    = 0;   // inter-character timer unused 
	if (timeout > 0 )
		newtio.c_cc[VMIN]     = 0;   	/* non-blocking mode */
	else
		newtio.c_cc[VMIN]     = 1;     	/* blocking mode: read until 1 character arrives */	
	
	
	//    Now we can flush, and then make the control changes!
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);
	
	return fd;
}

/** ##########################################################################
 * Close serial device
 */
void SerialClose(int fd)
{
	close(fd);
}

/** ##########################################################################
 * Check whether data from the serial device is available. 
 * @return True = data available, false = no data available.
 */
bool SerialIsDataAvailable(int fd)
{
	int bytes;
	
	ioctl( fd,  FIONREAD, &bytes);
	
	return (bytes?true:false);
}


/** ##########################################################################
 * Reads one character from device fd 
 * @return 1 = data available, -2 = cannot read from device, -1 = no character (e.g. non-blocking read)
 */
int SerialReadChar(int fd, char *c)
{
	int r;
	r = read(fd,c,1);

	if(r==1)
		return r;
	else 
		return -2;
/*
	return -1;  // No character (e.g. non-blocking read)
	not implementend yet!!!
*/
}

int SerialReadUnsignedChar(int fd, unsigned char *c)
{
	int r;
	char ch;
	r = read(fd,&ch,1);
	c[0] = (unsigned char) ch;
	
	return r;
	
	// for SuuntoReader need also feedback information of r = 0!!
/*	if(r==1)
		return r;
	else 
		return -2;
*/
}


/**
 * Writes \p size bytes to the serial device.
 *
 * @param[in] buf The buffer to be sent. Must be at least \p size bytes long.
 * @param[in] size Number of bytes to write.
 * @return Actual number of bytes written.
 */
int SerialWriteBuf( int fd, const unsigned char *buf, int size )
{
  int pos = 0;
  int ret = 0;

  bool readonly = false;
  
  while( !readonly && (pos < size) )
  {
    ret = write( fd, &buf[0], size - pos );
    if( ret > 0 )
    {
      pos += ret;
    }
    else
    {
		printf("SerialDevice::writeBuf    ERROR   throw IOError \n");
		return 0; 
    }
  }

  return pos;
}

/**
 * Reads all characters until a newline character, but
 * maximal \p size-1 bytes. The newline character will be
 * omitted. A string termination character (0x0) will
 * be inserted after the last character.
 * The internal buffer is used to reduce read operations on
 * the serial device.
 *
 * @param[out] buf The buffer for the received characters.
 * Must be at least \p size bytes long.
 * @param[in] size Number of bytes to read.
 * @param[in] sep Line separator character.
 * @return 1 = data available, -2 = cannot read from device, -1 = no character (e.g. non-blocking read)
 */
int SerialReadLine( int fd, unsigned char *buf, int size, char sep )
{
	int pos = 0;
	char ch;
	int rv;
	
	rv = SerialReadChar(fd,&ch);
	while( (rv!=-2) && (pos < size-1) && ( ch != sep) ) 
	{
		buf[pos++] = (unsigned char) ch;
		rv = SerialReadChar(fd,&ch);
	}
	buf[pos] = '\0';
	
	return rv;
}

#endif	// PLATFORM_LINUX

