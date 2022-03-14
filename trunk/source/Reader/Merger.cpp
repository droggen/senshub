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

#include <stdio.h>

#include "Merger.h"
#include "precisetimer.h"

/*
  Sleeps until the wakeuptime, using a combination of thread sleep for longer sleep time, and busy wait to achieve accurate wake up time.
*/
double SmartSleep(double wakeuptime)
{
   double t;
   double tnow = PreciseTimer::QueryTimer();
   double sleeplen;
   int nms;
   int busywaitwarn=4;

   sleeplen=wakeuptime-tnow;

   if(sleeplen<0)
      return tnow;

   // find number of milliseconds
   nms = (int)(sleeplen/0.001);
   // Go in busy wait mode before...
   nms-=busywaitwarn;

   // Thread sleep
   if(nms>=1)
      QThread::msleep(nms);

   while( (t=PreciseTimer::QueryTimer()) < wakeuptime );
   return t;
}

/******************************************************************************
*******************************************************************************
Merger   Merger   Merger   Merger   Merger   Merger   Merger   Merger   Merger
*******************************************************************************
******************************************************************************/
Merger::Merger()
{
   requeststop=false;
}
Merger::~Merger()
{
}
bool Merger::AddReader(BlockInputOutput *input)
{
   Readers.push_back(input);
   return true;
}
void Merger::ConnectReaders()
{
   for(unsigned i=0;i<Readers.size();i++)
      ConnectFrom(Readers[i]);
}
void Merger::DisconnectReaders()
{
   for(unsigned i=0;i<Readers.size();i++)
      DisconnectFrom(Readers[i]);
}
void Merger::ClearReaders()
{
   Stop();
   Readers.clear();
}
int Merger::GetActiveReader(QObject *s)
{
   for(unsigned i=0;i<Readers.size();i++)
   {
      if(Readers[i] == s)
         return i;
   }
   return -1;
}
int Merger::GetNumReaders()
{
   return Readers.size();
}
void Merger::StopThread()
{
   QMutexLocker locker(&mutex);
   requeststop=true;
   locker.unlock();
   bool ok=wait();
   printf("wait: %d\n",(int)ok);
}
/******************************************************************************
*******************************************************************************
LMerger   LMerger   LMerger   LMerger   LMerger   LMerger   LMerger   LMerger
*******************************************************************************
******************************************************************************/

LMerger::LMerger(double rout)
{
   DeltaT=1.0/rout;
}
LMerger::~LMerger()
{
   StopThread();
}

void LMerger::Start()
{
   requeststop=false;
   Buffer.InitBuffer(GetNumReaders());
   ConnectReaders();
   start();
};

void LMerger::Stop()
{
   DisconnectReaders();
   StopThread();
}


bool LMerger::AddReader(BlockInputOutput *reader)
{
   Merger::AddReader(reader);
   return true;
}


void LMerger::ClearReaders()
{
   Merger::ClearReaders();
}

/*!
   \brief Regular sampling
*/
void LMerger::run()
{
   double tstart = PreciseTimer::QueryTimer();
   double t;
   int i=0;
   bool firsttimeok=true;

   std::vector<bool> valid;
   std::vector<int> data;

   emit OutState(PreciseTimer::QueryTimer(),0);
   emit OutConnectionAttempt(PreciseTimer::QueryTimer());
   emit OutState(PreciseTimer::QueryTimer(),1);

   bool needstop=requeststop;
   while(!needstop)
   {
      double twakeup = tstart+i*DeltaT;
      t=SmartSleep(twakeup);

      QMutexLocker locker(&mutex);
      locker.relock();

      needstop=requeststop;

      bool ok=Buffer.GetMergedData(twakeup,valid,data);

      locker.unlock();

      if(ok)
      {
         // First time ok, then emit a connection
         if(firsttimeok)
         {
            firsttimeok=false;
            emit OutConnected(PreciseTimer::QueryTimer());
            emit OutState(PreciseTimer::QueryTimer(),2);
         }
         emit OutDataReceived(t,valid,data); // Data was merged at time t
      }
      i++;
   }
   emit OutDisconnected(PreciseTimer::QueryTimer());
   emit OutState(PreciseTimer::QueryTimer(),0);
}

/*
   Keep the last data in the internal buffers
*/
void LMerger::InDataReceived(double t,bool valid,std::vector<int> data)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)       // Data received from a known reader (upon disconnection old packets may still arrive)
   {
      QMutexLocker locker(&mutex);
      Buffer.AddData(sid,t,valid,data);
   }
}

void LMerger::InDisconnected(double t)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)
   {
      QMutexLocker locker(&mutex);
      Buffer.NotifyDisconnected(sid);
   }
}

/******************************************************************************
*******************************************************************************
RCMerger   RCMerger   RCMerger   RCMerger   RCMerger   RCMerger   RCMerger
*******************************************************************************
******************************************************************************/


RCMerger::RCMerger(double rin,double rout,double lat,double kp,double ki,double kd,double tc)
      : Buffer(rin,rout,lat,kp,ki,kd,tc)
{
   printf("RCMerger::RCMerger\n");
   SetPIDParameters(kp,ki,kd,tc);
   Rout = rout;
   DeltaT=1.0/(double)Rout;
}
RCMerger::~RCMerger()
{
   StopThread();
}

void RCMerger::SetPIDParameters(double kp,double ki,double kd,double tc)
{
   Buffer.SetPIDParameters(kp,ki,kd,tc);
}
void RCMerger::Start()
{
   printf("RCMerger Starting\n");
   requeststop=false;
   Buffer.InitBuffer(GetNumReaders());
   ConnectReaders();
   start();
};

void RCMerger::Stop()
{
   printf("RCMerger stopping\n");
   DisconnectReaders();
   StopThread();
   printf("RCMerger stopped\n");
}


/*!
   \brief Regular sampling
*/
void RCMerger::run()
{
   double tstart = PreciseTimer::QueryTimer();
   double t;
   int i=0;
   bool firsttimeok=true;

   std::vector<int> data;
   std::vector<bool> valid;

   emit OutState(PreciseTimer::QueryTimer(),0);
   emit OutConnectionAttempt(PreciseTimer::QueryTimer());
   emit OutState(PreciseTimer::QueryTimer(),1);


   bool needstop=requeststop;

   while(!needstop)
   {
      double twakeup = tstart+i*DeltaT;
      t=SmartSleep(twakeup);

      QMutexLocker locker(&mutex);
      locker.relock();

      needstop=requeststop;

      bool ok = Buffer.GetMergedData(twakeup,valid,data);

      locker.unlock();

      // Send data.
      if(ok)
      {
         // First time ok, then emit a connection
         if(firsttimeok)
         {
            firsttimeok=false;
            emit OutConnected(PreciseTimer::QueryTimer());
            emit OutState(PreciseTimer::QueryTimer(),2);
         }
         emit OutDataReceived(t,valid,data);    // Data was merged at time t
      }
      i++;
   }
   emit OutDisconnected(PreciseTimer::QueryTimer());
   emit OutState(PreciseTimer::QueryTimer(),0);
}

double RCMerger::BufferLevel(int reader)
{
   return Buffer.BufferLevel(reader);
}
QPixmap *RCMerger::GetBufferPixmap(int reader)
{
   QMutexLocker locker(&mutex);
   return Buffer.GetBufferPixmap(reader);
}


/*
   Keep the last data in the internal buffers
   Assumption:
   - no data loss (no packet counter used)
*/
void RCMerger::InDataReceived(double t,bool valid,std::vector<int> data)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)       // Data received from a known reader (upon disconnection old packets may still arrive)
   {
      QMutexLocker locker(&mutex);
      Buffer.AddData(sid,t,valid,data);
   }
}

void RCMerger::InDisconnected(double t)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)       // Data received from a known reader (upon disconnection old packets may still arrive)
   {
      QMutexLocker locker(&mutex);
      Buffer.NotifyDisconnected(sid);
   }
}
// Data lost (willingly not replaced by NAN) must be notified to the buffer to reset its logic (long disconnection)
void RCMerger::InDataLost(double,int)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)       // Data received from a known reader (upon disconnection old packets may still arrive)
   {
      QMutexLocker locker(&mutex);
      Buffer.NotifyDisconnected(sid);
   }
}
/*
  Capture the notification about the sample rate of the source sensor.
*/
void RCMerger::InNotify(double t,int message,int value)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)
   {
      printf("RCMerger::InNotify, message: %d=%d\n",message,value);
      if(message==0)
      {
         QMutexLocker locker(&mutex);
         Buffer.NotifySampleRate(sid,value/1000.0);
      }
   }
}

/******************************************************************************
*******************************************************************************
TSMerger   TSMerger   TSMerger   TSMerger   TSMerger   TSMerger   TSMerger
*******************************************************************************
******************************************************************************/



TSMerger::TSMerger(double rout,double lat)
      : Buffer(rout,lat)
{
   printf("TSMerger::TSMerger");
   Rout = rout;
   DeltaT=1.0/(double)Rout;

}
TSMerger::~TSMerger()
{
   StopThread();
}


void TSMerger::Start()
{
   printf("TSMerger Starting\n");
   requeststop=false;
   Buffer.InitBuffer(GetNumReaders());
   ConnectReaders();
   start();
};

void TSMerger::Stop()
{
   printf("TSMerger Stopping\n");
   // Set some variable - careful here....
   DisconnectReaders();
   StopThread();   
   printf("TSMerger Stopped\n");
}


/*!
   \brief Regular sampling
*/
void TSMerger::run()
{
   double tstart = PreciseTimer::QueryTimer();
   double t;
   int i=0;
   bool firsttimeok=true;

   emit OutState(PreciseTimer::QueryTimer(),0);
   emit OutConnectionAttempt(PreciseTimer::QueryTimer());
   emit OutState(PreciseTimer::QueryTimer(),1);


   bool needstop=requeststop;
   while(!needstop)
   {
      double twakeup = tstart+i*DeltaT;
      t=SmartSleep(twakeup);

      QMutexLocker locker(&mutex);
      locker.relock();

      needstop=requeststop;

      std::vector<int> data;
      std::vector<bool> valid;

      bool ok = Buffer.GetMergedData(twakeup,valid,data);

      locker.unlock();

      // Send data.
      if(ok)
      {
         // First time ok, then emit a connection
         if(firsttimeok)
         {
            firsttimeok=false;
            emit OutConnected(PreciseTimer::QueryTimer());
            emit OutState(PreciseTimer::QueryTimer(),2);
         }
         emit OutDataReceived(twakeup,valid,data);   // Data was merged at time twakeup
      }
      i++;
   }
   emit OutDisconnected(PreciseTimer::QueryTimer());
   emit OutState(PreciseTimer::QueryTimer(),0);
}

double TSMerger::BufferLevel(int reader)
{
   return Buffer.BufferLevel(reader);
}
QPixmap *TSMerger::GetBufferPixmap(int reader)
{
   QMutexLocker locker(&mutex);
   return Buffer.GetBufferPixmap(reader);
}

void TSMerger::InDataReceived(double t,bool valid,std::vector<int> data)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)
   {
      QMutexLocker locker(&mutex);
      Buffer.AddData(sid,t,valid,data);
   }
}

void TSMerger::InDisconnected(double t)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)
   {
      QMutexLocker locker(&mutex);
      Buffer.NotifyDisconnected(sid);
   }
}
// Data lots (willingly not replaced by NAN) must be notified to the buffer to reset its logic (long disconnection)
void TSMerger::InDataLost(double,int)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)
   {
      QMutexLocker locker(&mutex);
      Buffer.NotifyDisconnected(sid);
   }
}

