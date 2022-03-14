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

#ifndef __TSBUFFER_H
#define __TSBUFFER_H

#include <QPixmap>
#include <vector>
#include <deque>
#include "RCBuffer.h"

class TSBuffer
{
   private:
      double Rout;
      double Latency;
      //std::vector<RCDATA> RCData;
      // deque is prefered over vector: erasing old entries in the buffer is more than 100-1000 slower for larger buffers (60 sec latency) with a vector.
      std::deque<RCDATA> RCData;
      RCDATA LastData;
      bool LastDataValid;
      bool newdata;
      bool Disconnected;
      double LastAttime;
      QPixmap TmpPixmap;
      QImage BufferPixmap;

      virtual void Reset();

   public:
      TSBuffer(double rout,double lat);
      virtual ~TSBuffer ();

      virtual void AddData(double time,bool valid,const std::vector<int> &data);
      virtual double BufferLevel();
      virtual bool GetData(double attime,std::vector<int> &data);
      virtual void NotifyDisconnected();
      virtual QPixmap *GetBufferPixmap();
};

class TSBufferMulti : public MergeBuffer
{
   private:
      std::vector<TSBuffer*> TSBuffers;
      double Rin,Rout,Latency;
      double Kp,Ki,Kd,Tc;
      std::vector<int> MergedData;
      std::vector<bool> MergedDataValid;
      std::vector<int> ReaderNumChannel;

      virtual void DeleteBuffers();

   public:
      TSBufferMulti(double rout,double lat);
      virtual ~TSBufferMulti();

      virtual void AddData(unsigned reader,double time,bool valid,const std::vector<int> &data);
      virtual bool GetMergedData(double attime,std::vector<bool> &valid,std::vector<int> &data);
      virtual void InitBuffer(unsigned numreader);
      virtual void NotifyDisconnected(unsigned reader);
      virtual double BufferLevel(unsigned reader);
      virtual QPixmap *GetBufferPixmap(unsigned reader);

};


#endif // __TSBUFFER_H
