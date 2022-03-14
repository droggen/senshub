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
/*
	For generic use we need:
		- data format
		- input device (i.e. port now, but in future a generic stream)
		- device class

	Ideally opening the device would follow this syntax:
		USB:<port>
		RFCOMM:<port>
		FILE:<filename>
		USB¦RFCOMM:*
		USB¦RFCOMM:ID=<id>
*/

#include <stdio.h>
#include <assert.h>
#include <string>
#include <cfloat>
//#include "FrameParser.h"
#include "FBReader.h"

#include "precisetimer.h"


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// FBReader   FBReader   FBReader
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// 

FBReader::FBReader(std::string port,int baudrate, std::string format)
      : fp(format)
{
   Port = port;
   BaudRate=baudrate;

   //printf("FBReader::FBReader. Opening port %s. at Baudrate %d\n",port.c_str(),baudrate);

}
FBReader::~FBReader()
{
   Stop();
}
bool FBReader::IsOk()
{
   return fp.IsValid();
}
bool FBReader::Start()
{
   //printf("FBReader::Start %p. ok: %d\n",this,IsOk());
   if(!IsOk())
      return false;
   requeststop=false;
   start();
   return true;
};
void FBReader::Stop()
{
   //printf("FBReader::Stop %p\n",this);
   // Set some variable - careful here....
   requeststop=true;
   wait();
   //printf("Stopped\n");
};
void FBReader::run()
{
   int pkt=0;
   char ch;
   std::vector<std::vector<int> > frames;

   // Emit dummy data to notify the chain of the number of channels of this reader
   //std::vector<int> initv(fp.GetNumChannels()+1,0);      // +1 because we fake a packet counter in here
   std::vector<int> initv(fp.GetNumChannels(),0);
   emit OutDataReceived(0,false,initv);
   //printf("FBReader::run %p\n",this);

   while(!requeststop)
	{
      // Attempt to connect
      do
      {
         emit OutState(PreciseTimer::QueryTimer(),0);
         sleep(1);
         emit OutConnectionAttempt(0);
         emit OutState(PreciseTimer::QueryTimer(),1);
         fd = SerialOpen(Port,BaudRate,1);
      }
      while(fd==0 && !requeststop);
      emit OutConnected(PreciseTimer::QueryTimer());
      emit OutState(PreciseTimer::QueryTimer(),2);

      // Read data
      while(!requeststop)
		{
         int r = SerialReadChar(fd,&ch);
         if(r==0)
         {
            frames = fp.Parser(&ch,1);
            //printf("Got %d\n",frames.size());
            for(unsigned i=0;i<frames.size();i++)
            {
               std::vector<int> v = frames[i];
               //v.insert(v.begin(),pkt);       // Insert a dummy packet counter
               pkt++;
               emit OutDataReceived(PreciseTimer::QueryTimer(),true,v);
            }
         }
         if(r==-2)
            break;      // Disconnection
		}
      emit OutDisconnected(PreciseTimer::QueryTimer());
      emit OutState(PreciseTimer::QueryTimer(),1);
      SerialClose(fd);
      emit OutState(PreciseTimer::QueryTimer(),0);
	}	
}


/******************************************************************************
*******************************************************************************
FBInterpreter   FBInterpreter   FBInterpreter   FBInterpreter   FBInterpreter
*******************************************************************************
******************************************************************************/
/*
  Handles seamlessly the initialization packet by forwarding it but does not use it in the packet count logic
*/
FBInterpreter::FBInterpreter(int packetid,int packetmodulo,int maxpktloss)
{
   //printf("FBInterpreter::FBInterpreter id: %d modulo: %d maxpktloss: %d\n",packetid,packetmodulo,maxpktloss);
   PacketID = packetid;             // Channel containing packet (-1 if none - in which case it is passthrough)
   PacketModulo = packetmodulo;     // Counter (2^bitcount), or 0 if no packet counter
   MaxPacketLoss = maxpktloss;      // Maximum number of packets that are inserted as NAN

}
FBInterpreter::~FBInterpreter()
{
}
void FBInterpreter::Start()
{
   // Initialize the system
   first=true;

   SRETotDt=0;
   SRENumPacket=0;
   SRELocked=false;
}
void FBInterpreter::Stop()
{
   //OutState(t,0);
}
/*
  Heuristic to return number of packet lost.
*/
int FBInterpreter::PktLoss(int ido,int idn,double to,double tn,double sr,int packetmodulo)
{
   //printf("FBInterpreter::PktLoss ido %d idn %d\n",ido,idn);
   //printf("\tto %lf tn %lf\n",to,tn);
   // printf("\tsr: %lf mod: %d\n",sr,packetmodulo);

   // If we don't have a packet counter (modulo is 0) we can't determine any loss... so no loss.
   if(packetmodulo==0)
      return 0;

   // Test only on packet ID - not fool proof, time should be considered too.
   if( (ido+1)%packetmodulo == idn )
      return 0;

   //printf("pktloss: ido: %d, idn: %d, packetmodulo: %d\n",ido,idn,PacketModulo);
   // Lost data - assume we lost only 1 counter period, which is likely with 16-bit counters and moderate sample rates
   int l = (idn-(ido+1))%packetmodulo;
   if(l<0)
      l+=packetmodulo;
   //printf("l: %d. %d %d\n",l,-65536%65536,-65535%65536);
   //printf("Returning l: %d\n",l);
   return l;

}
int FBInterpreter::PktLoss(int ido,int idn,double to,double tn,double sr)
{
   return PktLoss(ido,idn,to,tn,sr,PacketModulo);
}
void FBInterpreter::InDataReceived(double t,bool valid,std::vector<int> data)
{
   // Normal data packets
   //printf("fbi before upd. locked %d tot %lf nump %d. t %lf. first: %d\n",(int)SRELocked,SRETotDt,SRENumPacket,t,(int)first);
   if(first)
   {
      LastPacket = data[PacketID];
      LastTime = t;
      first=false;
   }
   else
   {
      int l;
      if(PacketModulo!=0)     // Packet counter -> fault detection
      {
         // Do fault detection.
         //printf("data[packetid]: %d\n",data[PacketID]);
         l = PktLoss(LastPacket,data[PacketID],LastTime,t,0);
         if(l) printf("Lost %d\n",l);
         // Send the missing packets as invalid.
         if(l<=MaxPacketLoss)
            for(int i=0;i<l;i++)
               OutDataReceived(t,false,data);
         else
            emit OutDataLost(t,l);
      }
      else
         l=0;

      // Sample rate detection with packet counter
      if(SRELocked==false && l==0 && LastTime>DBL_EPSILON) // No packet lost, last packet wasnt an init packet -> cumulate consecutive packt inter time
      {
         SRETotDt += t-LastTime;
         SRENumPacket++;
      }
   }

   // Sample rate detection
   double locktime=4.0;
   if(SRELocked==false && SRETotDt>locktime && SRENumPacket!=0)
   {
      double sr = SRENumPacket/SRETotDt;
      SRELocked=true;
      emit OutState(t,2);
      emit OutNotify(t,0,(int)(sr*1000.0));    // Send the sample rate once we have a lock.
   }

   //printf("fbi locked %d tot %lf nump %d. t %lf. first: %d\n",(int)SRELocked,SRETotDt,SRENumPacket,t,(int)first);

   LastPacket=data[PacketID];
   LastTime=t;
   emit OutDataReceived(t,valid,data);

}
void FBInterpreter::InDataReceived(double t,std::vector<bool> valid,std::vector<int> data)
{
}

void FBInterpreter::InState(double t,int state)
{
   if(state==2)
   {
      // The reader notifies a connection.
      // Depending on whether we have a sample rate lock, we emit a state 3 or 2.
      if(SRELocked==false)
         emit OutState(t,3);           // Notify 'connected, waiting for sample rate lock'
      else
         emit OutState(t,2);           // Notify 'connected'
   }
   else
      emit OutState(t,state);
}
void FBInterpreter::InConnected(double t)
{
   first=true;
   emit OutConnected(t);
}
void FBInterpreter::InDisconnected(double t)
{
   emit OutDisconnected(t);
}
void FBInterpreter::InConnectionAttempt(double t)
{
   emit OutConnectionAttempt(t);
}



