/*
   SensHub

   Copyright (C) 2009:
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


#include "helper.h"

#include "serialio/serialio.h"

int Baud2Baud(QString spd)
{
   if(spd==QString("115200"))
      return SERIALIO_BITRATE_115200;
   if(spd==QString("57600"))
      return SERIALIO_BITRATE_57600;
   if(spd==QString("38400"))
      return SERIALIO_BITRATE_38400;
   if(spd==QString("19200"))
      return SERIALIO_BITRATE_19200;
   if(spd==QString("9600"))
      return SERIALIO_BITRATE_9600;
   if(spd==QString("2400"))
      return SERIALIO_BITRATE_2400;
   if(spd==QString("1200"))
      return SERIALIO_BITRATE_1200;
   return -1;
}


