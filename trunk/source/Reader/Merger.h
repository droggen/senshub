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

#ifndef __MERGER_H
#define __MERGER_H

#include <QMutex>
#include "Base.h"
#include "BaseReader.h"
#include "RCBuffer.h"
#include "TSBuffer.h"


double SmartSleep(double wakeuptime);

/******************************************************************************
*******************************************************************************
Merger   Merger   Merger   Merger   Merger   Merger   Merger   Merger   Merger
*******************************************************************************
******************************************************************************/

class Merger : public BlockInputOutput
{

   protected:
      QMutex mutex;
      std::vector<BlockInputOutput*> Readers;
      bool requeststop;

      virtual int GetActiveReader(QObject *s);
      virtual void ConnectReaders();
      virtual void DisconnectReaders();

   public:
      Merger();
      virtual ~Merger();

      virtual void Start()=0;
      virtual void Stop()=0;
      virtual void StopThread();

      virtual bool AddReader(BlockInputOutput *input);
      virtual void ClearReaders();
      virtual int GetNumReaders();
      virtual QPixmap *GetBufferPixmap(int reader)=0;
};

/******************************************************************************
*******************************************************************************
LMerger   LMerger   LMerger   LMerger   LMerger   LMerger   LMerger   LMerger
*******************************************************************************
******************************************************************************/

class LMerger : public Merger
{
   Q_OBJECT

   friend double SmartSleep(double wakeuptime);

   private:
      double DeltaT;
      virtual void run();
      LBufferMulti Buffer;


   protected:


   public:
      LMerger(double rout);
      virtual ~LMerger();

      virtual void Start();
      virtual void Stop();
      virtual void ClearReaders();
      virtual bool AddReader(BlockInputOutput *input);
      virtual QPixmap *GetBufferPixmap(int reader) {return 0;}

   private slots:
      virtual void InDataLost(double,int) {};
      virtual void InDataReceived(double t,bool valid,std::vector<int> data);
      virtual void InDataReceived(double t,std::vector<bool> valid,std::vector<int> data) {};
      virtual void InState(double t,int state) {};
      virtual void InConnected(double t) {};
      virtual void InDisconnected(double t);
      virtual void InConnectionAttempt(double t) {};
      virtual void InNotify(double t,int message,int value) {};

};

/******************************************************************************
*******************************************************************************
RCMerger   RCMerger   RCMerger   RCMerger   RCMerger   RCMerger   RCMerger
*******************************************************************************
******************************************************************************/

class RCMerger : public Merger
{
   Q_OBJECT

   friend double SmartSleep(double wakeuptime);

   private:
      double Rout;
      double DeltaT;
      RCBufferMulti Buffer;

      virtual void run();



   protected:

   public:
      RCMerger(double rin,double rout,double lat,double kp,double ki,double kd,double tc);
      virtual ~RCMerger();

      virtual void Start();
      virtual void Stop();

      virtual double BufferLevel(int reader);
      virtual void SetPIDParameters(double kp,double ki,double kd,double tc);
      virtual QPixmap *GetBufferPixmap(int reader);


   private slots:
      virtual void InDataLost(double,int);
      virtual void InDataReceived(double t,bool valid,std::vector<int> data);
      virtual void InDataReceived(double t,std::vector<bool> valid,std::vector<int> data){};
      virtual void InState(double t,int state) {};
      virtual void InConnected(double t) {};
      virtual void InDisconnected(double t);
      virtual void InConnectionAttempt(double t) {};
      virtual void InNotify(double t,int message,int value);

};

/******************************************************************************
*******************************************************************************
TSMerger   TSMerger   TSMerger   TSMerger   TSMerger   TSMerger   TSMerger
*******************************************************************************
******************************************************************************/

class TSMerger : public Merger
{
   Q_OBJECT

   friend double SmartSleep(double wakeuptime);

   private:
      double Rout;
      double DeltaT;
      TSBufferMulti Buffer;

      virtual void run();

   protected:

   public:
      TSMerger (double rout,double lat);
      virtual ~TSMerger ();

      virtual void Start();
      virtual void Stop();

      virtual double BufferLevel(int reader);
      virtual QPixmap *GetBufferPixmap(int reader);


   private slots:
      virtual void InDataLost(double,int);
      virtual void InDataReceived(double t,bool valid,std::vector<int> data);
      virtual void InDataReceived(double t,std::vector<bool> valid,std::vector<int> data){};
      virtual void InState(double t,int state) {};
      virtual void InConnected(double t) {};
      virtual void InDisconnected(double t);
      virtual void InConnectionAttempt(double t) {};
      virtual void InNotify(double t,int message,int value) {};

};

#endif // __MERGER_H
