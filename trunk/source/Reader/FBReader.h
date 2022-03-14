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


/*
	Here we should have a universal reader for frame-based binary data stream coming from serial lines (uart, ttyusb, ttyrfcomm).
*/
#ifndef __FBReader_H
#define __FBReader_H


#include <vector>
#include <map>
#include <list>

//#include "DThread.h"
//#include "RateCalculator.h"

#include "serialio/serialio.h"
#include "BaseReader.h"
#include "FrameParser3.h"




using namespace std;
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// FBReader   FBReader   FBReader
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// 
class FBReader : public BaseReader //, public DThread
{
	private:
      SERIALIO_HANDLE fd;
      FrameParser3 fp;
      string Port;
      int BaudRate;

      virtual void run();

	public:
      FBReader(std::string port,int baudrate,std::string format);
      virtual ~FBReader();
		
      virtual bool Start();
		virtual void Stop();
      virtual bool IsOk();
};


class FBInterpreter : public BlockInputOutput
{
   Q_OBJECT

   friend double SmartSleep(double wakeuptime);

   private:
      int PacketID,PacketModulo,MaxPacketLoss;
      int LastPacket;
      double LastTime;
      bool first;

      // Sample rate estimation
      double SRETotDt;
      unsigned SRENumPacket;
      bool SRELocked;

      int PktLoss(int ido,int idn,double to,double tn,double sr);

      virtual void run() {};

   public:
      FBInterpreter(int packetid,int packetmodulo,int maxpktloss);
      virtual ~FBInterpreter();

      virtual void Start();
      virtual void Stop();


      static int PktLoss(int ido,int idn,double to,double tn,double sr,int packetmodulo);

   private slots:
      virtual void InDataLost(double,int) {};
      virtual void InDataReceived(double t,bool valid,std::vector<int> data);
      virtual void InDataReceived(double t,std::vector<bool> valid,std::vector<int> data);
      virtual void InState(double t,int state);
      virtual void InConnected(double t);
      virtual void InDisconnected(double t);
      virtual void InConnectionAttempt(double t);
      virtual void InNotify(double time,int message,int value) {};

};




#endif
