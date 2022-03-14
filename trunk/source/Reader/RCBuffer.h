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

#ifndef __RCBUFFER_H
#define __RCBUFFER_H

#include <QPixmap>
#include <vector>
#include <deque>


typedef struct
{
   double time;
   long counter;
   std::vector<int> data;
   bool nan;
} RCDATA;


/******************************************************************************
*******************************************************************************
MergeBuffer   MergeBuffer   MergeBuffer   MergeBuffer   MergeBuffer   MergeBuffer
Generic interface   Generic interface   Generic interface   Generic interface
*******************************************************************************
******************************************************************************/

class MergeBuffer
{
   public:
      virtual ~MergeBuffer() {};
      virtual void AddData(unsigned reader,double time,bool valid,const std::vector<int> &data)=0;
      virtual bool GetMergedData(double attime,std::vector<bool> &valid,std::vector<int> &data)=0;
      virtual void InitBuffer(unsigned numreader)=0;
      virtual void NotifyDisconnected(unsigned reader)=0;
      virtual QPixmap *GetBufferPixmap(unsigned reader)=0;
};

/******************************************************************************
*******************************************************************************
LBufferMulti   LBufferMulti   LBufferMulti   LBufferMulti   LBufferMulti   LBufferMulti   LBufferMulti   LBufferMulti
*******************************************************************************
******************************************************************************/

class LBufferMulti : public MergeBuffer
{
   private:
      std::vector<RCDATA> Data;              // Data buffer Data[reader]
      std::vector<int> MergedData;
      std::vector<bool> MergedDataValid;
      std::vector<int> ReaderNumChannel;
      QPixmap BufferPixmap;

   public:
      LBufferMulti();
      virtual ~LBufferMulti();

      virtual void AddData(unsigned reader,double time,bool valid,const std::vector<int> &data);
      virtual bool GetMergedData(double attime,std::vector<bool> &valid,std::vector<int> &data);
      virtual void InitBuffer(unsigned numreader);
      virtual void NotifyDisconnected(unsigned reader);
      virtual QPixmap *GetBufferPixmap(unsigned reader) {return 0;};
};

/******************************************************************************
*******************************************************************************
RCBuffer   RCBuffer   RCBuffer   RCBuffer   RCBuffer   RCBuffer   RCBuffer
*******************************************************************************
******************************************************************************/
class RCBuffer
{
   private:
      double Rin,Rout,Latency;
      double Kp,Ki,Kd,Tc;
      double Integrator;
      double LastE;
      double AvgBufferLevel;
      //QPixmap BufferPixmap;
      QImage BufferPixmap;
      QPixmap TmpPixmap;
      //QImage LastPixmap;

      //std::vector<RCDATA> RCData;         // Rate-control data
      // deque prefered over vector: Shift1 erases data at head which is much faster (100-1000x) with deque
      std::deque<RCDATA> RCData;         // Rate-control data
      long Pin;
      //long Pout;
      int Nominal;
      int Top;
      double Shift;
      bool Disconnected;

      virtual void Shift1();
      virtual void Reset();

   public:
      RCBuffer(double rin,double rout,double lat,double kp,double ki,double kd,double tc);
      virtual ~RCBuffer();

      virtual void AddData(double time,bool valid,const std::vector<int> &data);
      virtual double BufferLevel();
      virtual bool GetData(std::vector<int> &data);

      virtual void SetPIDParameters(double kp,double ki,double kd,double tc);
      virtual void SetRin(double r);
      virtual void NotifyDisconnected();


      virtual QPixmap *GetBufferPixmap();
};

class RCBufferMulti : public MergeBuffer
{
   private:
      std::vector<RCBuffer*> RCBuffers;
      double Rin,Rout,Latency;
      double Kp,Ki,Kd,Tc;
      std::vector<int> MergedData;
      std::vector<bool> MergedDataValid;
      std::vector<int> ReaderNumChannel;
      std::vector<bool> SampleRateSet;
      bool AllSampleRateSet;

      virtual void DeleteBuffers();

   public:
      RCBufferMulti(double rin,double rout,double lat,double kp,double ki,double kd,double tc=0);
      virtual ~RCBufferMulti();

      virtual void AddData(unsigned reader,double time,bool valid,const std::vector<int> &data);
      virtual bool GetMergedData(double attime,std::vector<bool> &valid,std::vector<int> &data);
      virtual void InitBuffer(unsigned numreader);
      virtual void NotifyDisconnected(unsigned reader);
      virtual void NotifySampleRate(unsigned reader,double samplerate);
      virtual double BufferLevel(unsigned reader);
      virtual void SetPIDParameters(double kp,double ki,double kd,double tc);
      virtual QPixmap *GetBufferPixmap(unsigned reader);

};


#endif

