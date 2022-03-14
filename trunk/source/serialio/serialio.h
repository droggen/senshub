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
/******************************************************************************
*  Compiler must define:
*		PLATFORM_MINGW		For a mingw compile (or cygwin with mingw target) (maybe also BC++ and VC++)
*		PLATFORM_LINUX		For a cygwin or linux compile
*
******************************************************************************/

#ifndef __SERIALIO_H
#define __SERIALIO_H


#ifdef WIN32
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>
#include <vector>
#include <string>
#endif

#ifdef PLATFORM_LINUX
//xxx#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

#include <cmdline.h>
#include <stddef.h>
#include <dirent.h>
#include <iostream>
#include <malloc.h>
#include <Devices.h>
#endif


// Define some bitrates
#ifdef WIN32
#define SERIALIO_BITRATE_1200		CBR_1200
#define SERIALIO_BITRATE_2400		CBR_2400
#define SERIALIO_BITRATE_9600		CBR_9600
#define SERIALIO_BITRATE_19200		CBR_19200
#define SERIALIO_BITRATE_38400		CBR_38400
#define SERIALIO_BITRATE_57600		CBR_57600
#define SERIALIO_BITRATE_115200		CBR_115200
#endif

#ifdef PLATFORM_LINUX
#define SERIALIO_BITRATE_1200		B1200
#define SERIALIO_BITRATE_2400		B2400
#define SERIALIO_BITRATE_9600		B9600
#define SERIALIO_BITRATE_19200		B19200
#define SERIALIO_BITRATE_38400		B38400
#define SERIALIO_BITRATE_57600		B57600
#define SERIALIO_BITRATE_115200		B115200
#endif

// Define platform independent types
#ifdef WIN32
   #define SERIALIO_HANDLE HANDLE
#endif

#ifdef PLATFORM_LINUX
   #define SERIALIO_HANDLE int
#endif

// Define platform independent types
#define SERIAL_MODE_PORT 	0
#define SERIAL_MODE_ID 		1
#define SERIAL_MODE_PNP 	2
/*
	device: com port, starting at 0 (i.e. 0 is /dev/ttyS0 on Linux or COM1: on Windows)
	bitrate: see above
	timeout: timeout in millisecond before reading a character returns with a failure code (i.e. non-blocking read). When timeout is 0, the function blocks until a character is read.
	
	SerialOpen: returns 0 for error.
*/

	//TODO to be changed - adapt for linux
   SERIALIO_HANDLE SerialOpen(std::string device,int bitrate,int timeout=0);

	
	void SerialClose(SERIALIO_HANDLE hCom);
	bool SerialIsDataAvailable(int fd);
	int  SerialReadChar(SERIALIO_HANDLE hCom, char *ch);


#endif

