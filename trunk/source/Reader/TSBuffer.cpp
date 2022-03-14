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

#include <cfloat>
#include <QPainter>
#include "precisetimer.h"
#include "TSBuffer.h"



/******************************************************************************
*******************************************************************************
TSBuffer   TSBuffer   TSBuffer   TSBuffer   TSBuffer   TSBuffer   TSBuffer
*******************************************************************************
******************************************************************************/





TSBuffer::TSBuffer(double rout,double lat)
      //: BufferPixmap(100,40), LastPixmap(1,100,QImage::Format_RGB32)
      : BufferPixmap(100,40,QImage::Format_RGB32), TmpPixmap(100,40)
{
   BufferPixmap.fill(Qt::black);
   Rout=rout;
   Latency=lat;
   Disconnected=true;
   Reset();
}
TSBuffer::~TSBuffer()
{
}
void TSBuffer::Reset()
{
   LastDataValid=false;
   newdata=false;
   RCData.clear();
}

double TSBuffer::BufferLevel()
{
}
void TSBuffer::AddData(double time,bool valid,const std::vector<int> &data)
{
   // If formerly disconnected, AddData indicates we are reconnected.... however we need to reinitialize our logic
   if(Disconnected)
   {
      Reset();
      Disconnected=false;
   }

   // If the buffer was empty ->? do nothing: we assume the packet counting logic is at work?
   if(RCData.size()==0)     // We do a reset
      Reset();

   RCDATA d;
   d.counter=0;
   d.data=data;
   d.time=time;
   d.nan=!valid;
   //double t1=PreciseTimer::QueryTimer();
   RCData.push_back(d);
   //double t2=PreciseTimer::QueryTimer();
   //printf("Add data: %lf\n",t2-t1);
   newdata=true;
   //printf("newdata: %d time: %lf\n",newdata,time);
}
/*
  Returns true if the data is valid, false if NAN.
*/
bool TSBuffer::GetData(double attime,std::vector<int> &data)
{
   double mint,maxt;
   LastAttime=attime;

   if(newdata)       // Received new data since the last call... space it.
   {
      //printf("TS Buffer size: %d capacity: %d\n",RCData.size(),0);//,RCData.capacity());

      // Respace the data if: at least 3 packets, and spacing is non null (to avoid divide by 0)
      // We use here 'LastPacket' (that can be outside of the buffered window) as well as two packet within the buffer window
      if(RCData.size()>=3)     // For resampling only with packet within the window
      //if(LastDataValid && RCData.size()>=2)
      {
         double mint = RCData[0].time;     // For resampling only with packet within the window
         //mint = LastData.time;
         maxt = RCData[RCData.size()-1].time;
         double dt = (maxt-mint)/(double)(RCData.size()-1);
         if(dt>=0.000001)    // Avoid divide by zero, resize if dt>epsilon
         {
            for(unsigned i=0;i<RCData.size();i++)
            {
               RCData[i].time = mint + ((double)i)*dt;               // For resampling only with packet within the window
               //RCData[i].time = mint + ((double)i+1)*dt;          // i+1 because we use LastData as the base
            }
         }
      }
   }


   // The output logic is as follows:
   // - When no buffered data is more recent than latency output NAN
   // - When there is buffered data more recent than latency, output one of:
   // -- The data that was more recent than latency at the previous GetData, and is now older than Latency (i.e. the data crossing the Latency age)
   // -- If no data crosses the latency age, repeat the last outputted data

   // Look for elements that cross the latency line... i.e. at the previous time step datatime>currenttime-latency, now datatime<=currenttime-latency

   bool fnd=false;
   double st = 1.0/Rout;
   //if(RCData.size())
   //   printf("0t:%lf et:%lf dl:%lf dh:%lf N:%lf\n",RCData[0].time,RCData[RCData.size()-1].time,attime-Latency,attime-Latency-st,attime);
   for(int i=RCData.size()-1;i>=0;i--)
   {

      if( (RCData[i].time<=attime-Latency) && (RCData[i].time>attime-Latency-st))
      {
         //printf("---------------------------FOUND!\n");
         // found data to output
         fnd=true;
         LastData=RCData[i];
         LastDataValid=true;
         break;
      }
   }

   // Erase all the entries older than Latency.
   int lasttoerase=-1;
   for(unsigned i=0;i<RCData.size();i++)
   {
      if(RCData[i].time<=attime-Latency)
         lasttoerase=i;
      else
         break;
   }
   if(lasttoerase!=-1)
   {
      //double t1=PreciseTimer::QueryTimer();
      RCData.erase(RCData.begin(),RCData.begin()+lasttoerase+1);     // +1 because erase removes entries in [first;last[
      //double t2=PreciseTimer::QueryTimer();
      //printf("Erase %d. %lf\n",lasttoerase,t2-t1);
   }
   //printf("After erase: RCData.size: %d\n",RCData.size());

   //printf("RCSiz %d Fnd %d LastDataValid %d\n",RCData.size(),fnd,LastDataValid);
   //printf("   min/max: %lf %lf\n",mint,maxt);



   // Output the data, according to the logic described above
   if(fnd)        // If we found data crossing the latency age, we return it...
   {
      data = LastData.data;
      if(LastData.nan)
         return false;
      return true;
   }
   // No data crossed the latency age... either repeat last or NAN
   if(RCData.size()==0)    // No more data in buffer->NAN
      return false;

   if(LastDataValid)
   {
      data = LastData.data;   // Repeat last data, if it is available
      return true;
   }
   return false;              // Wanted to repeat the last data, but we don't have yet last data -> NAN
}
void TSBuffer::NotifyDisconnected()
{
   Disconnected=true;
}

QPixmap *TSBuffer::GetBufferPixmap()
{
   //double t1=PreciseTimer::QueryTimer();

   // Shift left manually
   unsigned bytesperline = BufferPixmap.bytesPerLine();
   uchar *s=BufferPixmap.scanLine(0);
   for(unsigned i=0;i<BufferPixmap.height();i++)
   {
      memmove(s,s+4,bytesperline-4);
      s+=bytesperline;
   }

   //double t1b=PreciseTimer::QueryTimer();



   // Draw directly in the last column of the pixmap
   QRgb *pixel = (QRgb*)BufferPixmap.scanLine(0);     // First line
   pixel+=BufferPixmap.width()-1;                     // Last column
   int bpl = BufferPixmap.bytesPerLine()/4;
   unsigned rcs = RCData.size();
   std::deque<RCDATA>::iterator it = RCData.begin();
   unsigned h = BufferPixmap.height();

   // Clear last column
   QRgb *p=pixel;
   for(unsigned y=0;y<h;y++,p+=bpl)
      *p=0xFF000000;

   // Draw last column
   for(unsigned i=0;i<rcs;i++)
   {
      int y = h-1- ((it->time-(LastAttime-Latency))/Latency*(h-1));
      if(y<0) y=0;
      if(y>=h) y=h-1;
      if(it->nan)
         *(pixel+bpl*y) = 0xFFff0000;
      else
         *(pixel+bpl*y) = 0xFF00ff00;
      it++;
   }



   //double t1d=PreciseTimer::QueryTimer();

   TmpPixmap = QPixmap::fromImage(BufferPixmap);

   //double t2=PreciseTimer::QueryTimer();

   //printf("TSBuffer: %lf (%lf, %lf, %lf)\n",t2-t1,t1b-t1,t1d-t1b,t2-t1d);

   return &TmpPixmap;
}

/******************************************************************************
*******************************************************************************
TSBufferMulti   TSBufferMulti   TSBufferMulti   TSBufferMulti   TSBufferMulti
*******************************************************************************
******************************************************************************/


TSBufferMulti::TSBufferMulti(double rout,double lat)
{
   Rout=rout;
   Latency=lat;
}
TSBufferMulti::~TSBufferMulti()
{
   DeleteBuffers();
}
void TSBufferMulti::DeleteBuffers()
{
   for(unsigned i=0;i<TSBuffers.size();i++)
      delete TSBuffers[i];
   TSBuffers.clear();
}
void TSBufferMulti::AddData(unsigned reader,double time,bool valid,const std::vector<int> &data)
{
   if(MergedData.size()==0)      // The number of channels of each reader hasn't been yet identified
   {
      ReaderNumChannel[reader]=data.size();
      // Check if we have received data from each reader
      bool allin=true;
      int mergedchannelsize=0;
      for(unsigned i=0;i<ReaderNumChannel.size();i++)
      {
         mergedchannelsize+=ReaderNumChannel[i];
         if(ReaderNumChannel[i]==0)
         {
            allin=false;
            break;
         }
      }
      if(allin)
      {
         // Allocate the merged data buffer
         MergedData.resize(mergedchannelsize,0);
         MergedDataValid.resize(mergedchannelsize,false);
      }
   }
   TSBuffers[reader]->AddData(time,valid,data);
}
bool TSBufferMulti::GetMergedData(double attime,std::vector<bool> &valid,std::vector<int> &data)
{
   // Start outputting data only once we know each reader's channel number. This is indicated by MergedData
   if(MergedData.size()==0)
      return false;

   /*printf("TSBufferMulti::GetMergedData. MergedData: %d TSBuffers: %d\n",MergedData.size(),TSBuffers.size());
   printf("Channel size: ");
   for(int i=0;i<ReaderNumChannel.size();i++)
      printf("%d ",ReaderNumChannel[i]);
   printf("\n");*/


   // We know the number of channels of each reader -> merge data.
   int startptr=0;
   std::vector<int> d;
   bool ok;
   for(unsigned i=0;i<TSBuffers.size();i++)
   {
      ok = TSBuffers[i]->GetData(attime,d);
      if(ok)
         for(unsigned j=0;j<d.size();j++)
            MergedData[startptr+j] = d[j];         // copy data only if it is valid.... to keep the last data in the buffer
      for(unsigned j=0;j<ReaderNumChannel[i];j++)
         MergedDataValid[startptr+j] = ok;
      startptr+=ReaderNumChannel[i];
   }
   data = MergedData;
   valid = MergedDataValid;
   return true;
}
void TSBufferMulti::InitBuffer(unsigned numreader)
{
   DeleteBuffers();
   TSBuffers.resize(numreader,0);
   for(unsigned i=0;i<numreader;i++)
      TSBuffers[i] = new TSBuffer(Rout,Latency);

   ReaderNumChannel.clear();
   ReaderNumChannel.resize(numreader);

   MergedData.clear();
   MergedDataValid.clear();
}
void TSBufferMulti::NotifyDisconnected(unsigned reader)
{
   TSBuffers[reader]->NotifyDisconnected();
}
double TSBufferMulti::BufferLevel(unsigned reader)
{
   return TSBuffers[reader]->BufferLevel();
}
QPixmap *TSBufferMulti::GetBufferPixmap(unsigned reader)
{
   return TSBuffers[reader]->GetBufferPixmap();
}


