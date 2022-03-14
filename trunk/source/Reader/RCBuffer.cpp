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
#include "RCBuffer.h"

/******************************************************************************
*******************************************************************************
MergeBuffer   MergeBuffer   MergeBuffer   MergeBuffer   MergeBuffer   MergeBuffer
*******************************************************************************
******************************************************************************/


/******************************************************************************
*******************************************************************************
LBufferMulti   LBufferMulti   LBufferMulti   LBufferMulti   LBufferMulti   LBufferMulti   LBufferMulti   LBufferMulti
*******************************************************************************
******************************************************************************/
LBufferMulti::LBufferMulti()
{
   InitBuffer(0);
}
LBufferMulti::~LBufferMulti()
{
}
void LBufferMulti::AddData(unsigned reader,double time,bool valid,const std::vector<int> &data)
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
   Data[reader].time=time;
   Data[reader].nan=!valid;
   Data[reader].data=data;

}
bool LBufferMulti::GetMergedData(double attime,std::vector<bool> &valid,std::vector<int> &data)
{
   // Start outputting data only once we know each reader's channel number. This is indicated by MergedData
   if(MergedData.size()==0)
   {
      //printf("LBufferMulti::GetMergedData returning false\n");
      return false;
   }
   // We know the number of channels of each reader -> merge data.
   int startptr=0;
   for(unsigned i=0;i<Data.size();i++)
   {
      for(unsigned j=0;j<Data[i].data.size();j++)
      {
         MergedData[startptr+j] = Data[i].data[j];
         MergedDataValid[startptr+j] = !Data[i].nan;
      }
      startptr+=ReaderNumChannel[i];
   }
   data = MergedData;
   valid = MergedDataValid;
   return true;
}
void LBufferMulti::InitBuffer(unsigned numreader)
{
   Data.clear();
   Data.resize(numreader);
   ReaderNumChannel.clear();
   ReaderNumChannel.resize(numreader);
   MergedData.clear();
   MergedDataValid.clear();
}

/*
  Upon disconnection all the reader's channel become NANs
*/
void LBufferMulti::NotifyDisconnected(unsigned reader)
{
   Data[reader].nan=true;
}

/******************************************************************************
*******************************************************************************
Rate control buffer
*******************************************************************************
******************************************************************************/

RCBuffer::RCBuffer(double rin,double rout,double lat,double kp,double ki,double kd,double tc)
      //: BufferPixmap(100,40), LastPixmap(0,0,QImage::Format_RGB32)
      //: BufferPixmap(100,40,QImage::Format_RGB32), LastPixmap(0,0,QImage::Format_RGB32), TmpPixmap(100,40)
      : BufferPixmap(100,40,QImage::Format_RGB32), TmpPixmap(100,40)
{
   BufferPixmap.fill(Qt::black);
   Rin = rin;
   Rout = rout;
   Latency = lat;

   Disconnected=true;

   SetPIDParameters(kp,ki,kd,tc);
   Reset();

   //printf("Latency: %lf\n", lat);
   //printf("Rin: %lf\n", rin);

}
RCBuffer::~RCBuffer()
{
   //printf("RCBuffer::~RCBuffer\n");
}
void RCBuffer::Reset()
{
   // Nominal level of the buffer in samples
   Nominal = (int)(Latency * Rin);

   //printf("RCBuffer::Reset Nominal: %d\n",Nominal);

   // Create and initialize the buffer
   //printf("Before RCBuffer::Reset, resize\n");
   if(Nominal==0)
      Nominal=1;
   RCData.resize(2*Nominal);
   //printf("After RCBuffer::Reset, resize\n");
   for(unsigned i=0;i<RCData.size();i++)
   {
      RCData[i].counter=i;
      RCData[i].nan=true;
      RCData[i].time=0;
      //RCData[i].data.clear();     // Not needed
   }
   Top=Nominal;
   Pin=RCData[(int)Top].counter;
   Shift=0;

   // Reset the PID
   Integrator=0;
   LastE=0;
   AvgBufferLevel=Top;
}
/*
  Adds one data packet on the Top.
*/
void RCBuffer::AddData(double time,bool valid,const std::vector<int> &data)
{
   // Do nothing if the input sample rate isn't set yet
   if(Rin<DBL_EPSILON)
      return;

   // If formerly disconnected, AddData indicates we are reconnected.... however we need to fill the buffers
   // from the start position again, hence a reset.
   if(Disconnected)
   {
      //printf("Disconnected reset\n");
      Reset();
      Disconnected=false;
   }

   // Store the data in the buffer....

   if(Top<0)
   {
      //printf("Top <0 reset\n");
      Reset();
   }

   // Shift data out if too full
   //double t1=PreciseTimer::QueryTimer();
   while(Top>=RCData.size()-1)
      Shift1();
   //double t2=PreciseTimer::QueryTimer();

   Top++;
   RCData[Top].data=data;
   RCData[Top].time=time;
   RCData[Top].counter=Pin;
   RCData[Top].nan=!valid;
   Pin++;
   //double t3=PreciseTimer::QueryTimer();
   //printf("add: shift %lf store: %lf\n",t2-t1,t2-t2);

}
void RCBuffer::Shift1()
{

   /*
   // This version copies data in buffer...
   // Shift one out.
   for(unsigned i=1;i<RCData.size();i++)
      RCData[i-1]=RCData[i];
   // Mark top as NAN
   RCData[RCData.size()-1].nan=true;
   Top-=1;
   */

   // This version uses buffer add/erase
   // Shift one out.
   RCData.erase(RCData.begin());
   // Add one (same as top)
   RCData.push_back(RCData[RCData.size()-1]);
   // Mark top as NAN
   RCData[RCData.size()-1].nan=true;
   Top-=1;
}

double RCBuffer::BufferLevel()
{
   return (double)Top/(double)RCData.size();
}

bool RCBuffer::GetData(std::vector<int> &data)
{
   // Do nothing if the input sample rate isn't set yet
   if(Rin<DBL_EPSILON)
      return false;

   bool valid;
   valid=false;
   //if(Rin<=20)
   //printf("InOut:%d %d - %d N: %d T: %d B: %d\n",RCData[Top].counter,RCData[0].counter,RCData[Top].counter-RCData[0].counter,Nominal,Top,RCData.size());
   // Only do rate control when Top>=0... when Top<0: send NAN, give up on rate control
   if(Top>=0)
   {
      AvgBufferLevel = 0.9*AvgBufferLevel+0.1*Top;

      double a;
      // Error
      double e = (RCData[Top].counter-RCData[0].counter)-Nominal;
      //double e = AvgBufferLevel-Nominal;
      // Integral term
      Integrator += e/Rout;
      // Derivator
      double Derivator = (e-LastE)/Rout;
      // PI adjusment
      a = e*Kp + Integrator*Ki + Derivator*Kd;
      LastE=e;
      // Shift the data by Rin/Rout + PID-controlled adjustment factor
      double d;
      if(Disconnected)
         d = Rin/Rout;           // If we are disconnected, disable the rate control as anyway we don't get packets anymore to rate equalize reliably
      else
         d = Rin/Rout + a;
      if(d<0)
         d=0;
      Shift += d;


      /*if(Rin<=20)
      printf("e: %lf i: %lf d: %lf. disc %d\n",e,Integrator,Derivator,a,(int)Disconnected);
      if(Rin<=20)
      printf("RR: %lf adj: %lf d: %lf\n",Rin/Rout,a,d);
      if(Rin<=20)
      printf("Kp: %lf. Ki: %lf. Kd: %lf\n\n",Kp,Ki,Kd);*/


      //double t1=PreciseTimer::QueryTimer();
      //double s=Shift;
      while(Shift>=1.0)
      {
         Shift-=1.0;
         Shift1();
      }
      //double t2=PreciseTimer::QueryTimer();
      //printf("get shift (%lf): %lf\n",s,t2-t1);
   }
   data = RCData[0].data;      // Copy to emit outside of the lock
   valid = !RCData[0].nan;

   return valid;
}

void RCBuffer::SetPIDParameters(double kp,double ki,double kd,double tc)
{
   Kp=kp;
   Ki=ki;
   Kd=kd;
   Tc=tc;
   Reset();
}
void RCBuffer::SetRin(double r)
{
   Rin=r;
   Reset();
}

void RCBuffer::NotifyDisconnected()
{
   Disconnected=true;
}

QPixmap *RCBuffer::GetBufferPixmap()
{
/*
   // On the fly initialization
   if(LastPixmap.width()!=1 || LastPixmap.height()!=RCData.size())
      LastPixmap=QImage(1,RCData.size(),QImage::Format_RGB32);

   QPainter painter;
   painter.begin(&BufferPixmap);
   // Shift left
   painter.drawPixmap(0,0,BufferPixmap,1,0,0,0);
   // Clear last column
   painter.setPen(Qt::black);
   painter.drawLine(BufferPixmap.width()-1,0,BufferPixmap.width()-1,BufferPixmap.height()-1);


   // Draw points on a new pixmap for the last column
   LastPixmap.fill(Qt::black);   
   QRgb *pixel = (QRgb*)LastPixmap.scanLine(LastPixmap.height()-1);
   int bpl = LastPixmap.bytesPerLine()/4;
   std::deque<RCDATA>::iterator it = RCData.begin();
   unsigned rcs = RCData.size();
   for(unsigned i=0;i<rcs && i<Top && Top>=0;i++)
   {
      if((it++)->nan)
         *pixel = 0xFFff0000;
      else
         *pixel = 0xFF00ff00;
      pixel-=bpl;
   }
   // Scale & copy pixmap
   painter.drawImage(QRect(BufferPixmap.width()-1,0,1,BufferPixmap.height()),LastPixmap);

   painter.end();

   return &BufferPixmap;

*/

   // Shift left manually
   unsigned bytesperline = BufferPixmap.bytesPerLine();
   uchar *s=BufferPixmap.scanLine(0);
   for(unsigned i=0;i<BufferPixmap.height();i++)
   {
      memmove(s,s+4,bytesperline-4);
      s+=bytesperline;
   }

   // Draw directly in the last column of the pixmap
   QRgb *pixel = (QRgb*)BufferPixmap.scanLine(0);        // Top line
   pixel+=BufferPixmap.width()-1;                        // Last column
   int bpl = BufferPixmap.bytesPerLine()/4;
   unsigned rcs = RCData.size();
   std::deque<RCDATA>::iterator it = RCData.begin();
   unsigned h = BufferPixmap.height();

   // Clear last column
   QRgb *p=pixel;
   for(unsigned y=0;y<h;y++,p+=bpl)
      *p=0xFF000000;


   // Draw points on a new pixmap for the last column
   unsigned lastl=h-1;
   unsigned color;
   // Draw point at l = i*(h-1)/(rcs-1)   (if the rate control buffer is larger than the pixmap)
   // or a line from from lastline to l   (if the rate control buffer is smaller than the pixmap)
   if(rcs<h)
   {
      for(unsigned i=0;i<rcs && i<Top && Top>=0;i++)
      {
         unsigned l = (h-1) - i*(h-1)/(rcs-1);
         if((it++)->nan)
            color = 0xFFff0000;
         else
            color = 0xFF00ff00;

         for(unsigned tl=l;tl<=lastl;tl++)
            *(pixel + bpl*tl) = color;
         lastl=l;
      }
   }
   else
   {
      for(unsigned i=0;i<rcs && i<Top && Top>=0;i++)
      {
         unsigned l = (h-1) - i*(h-1)/(rcs-1);
         if((it++)->nan)
            *(pixel + bpl*l) = 0xFFff0000;
         else
            *(pixel + bpl*l) = 0xFF00ff00;
      }
   }
   TmpPixmap = QPixmap::fromImage(BufferPixmap);

   return &TmpPixmap;

}




/******************************************************************************
*******************************************************************************
RCBufferMulti   RCBufferMulti   RCBufferMulti   RCBufferMulti   RCBufferMulti
*******************************************************************************
******************************************************************************/

RCBufferMulti::RCBufferMulti(double rin,double rout,double lat,double kp,double ki,double kd,double tc)
{
   Rin=rin;
   Rout=rout;
   Latency=lat;
   kp=Kp;
   ki=Ki;
   Kd=kd;
   Tc=tc;
   AllSampleRateSet=false;
}
RCBufferMulti::~RCBufferMulti()
{
   DeleteBuffers();
}
void RCBufferMulti::DeleteBuffers()
{
   for(unsigned i=0;i<RCBuffers.size();i++)
      delete RCBuffers[i];
   RCBuffers.clear();
}
void RCBufferMulti::AddData(unsigned reader,double time,bool valid,const std::vector<int> &data)
{
   //printf("RCBufferMulti AddData reader %d %lf\n",reader,time);
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
   RCBuffers[reader]->AddData(time,valid,data);
}
bool RCBufferMulti::GetMergedData(double attime,std::vector<bool> &valid,std::vector<int> &data)
{
   // Start outputting data only once we know each reader's channel number. This is indicated by MergedData
   if(MergedData.size()==0)
      return false;

   // Start outputting data only once we know each reader input sample rate
   if(!AllSampleRateSet)
      return false;

   // We know the number of channels of each reader -> merge data.
   int startptr=0;
   std::vector<int> d;
   bool ok;
   for(unsigned i=0;i<RCBuffers.size();i++)
   {
      ok = RCBuffers[i]->GetData(d);
      for(unsigned j=0;j<d.size();j++)
         MergedData[startptr+j] = d[j];         // d is 0-sized in buffer underfill conditions
      for(unsigned j=0;j<ReaderNumChannel[i];j++)
         MergedDataValid[startptr+j] = ok;
      startptr+=ReaderNumChannel[i];
   }
   data = MergedData;
   valid = MergedDataValid;
   return true;
}
void RCBufferMulti::InitBuffer(unsigned numreader)
{
   DeleteBuffers();
   RCBuffers.resize(numreader,0);
   for(unsigned i=0;i<numreader;i++)
      RCBuffers[i] = new RCBuffer(Rin,Rout,Latency,Kp,Ki,Kd,Tc);

   ReaderNumChannel.clear();
   ReaderNumChannel.resize(numreader);

   SampleRateSet.clear();
   SampleRateSet.resize(numreader,false);
   AllSampleRateSet=false;

   MergedData.clear();
   MergedDataValid.clear();
}
void RCBufferMulti::NotifyDisconnected(unsigned reader)
{
   RCBuffers[reader]->NotifyDisconnected();
}
double RCBufferMulti::BufferLevel(unsigned reader)
{
   return RCBuffers[reader]->BufferLevel();
}
void RCBufferMulti::SetPIDParameters(double kp,double ki,double kd,double tc)
{
   Kp=kp;
   Ki=ki;
   Kd=kd;
   Tc=tc;
   for(unsigned i=0;i<RCBuffers.size();i++)
      RCBuffers[i]->SetPIDParameters(kp,ki,kd,tc);
}
QPixmap *RCBufferMulti::GetBufferPixmap(unsigned reader)
{
   return RCBuffers[reader]->GetBufferPixmap();
}
void RCBufferMulti::NotifySampleRate(unsigned reader,double samplerate)
{
   Q_ASSERT(reader<SampleRateSet.size());

   // Notify the buffer of the new input samplerate
   RCBuffers[reader]->SetRin(samplerate);
   SampleRateSet[reader]=true;

   // Update AllSampleRate, to output data only when this is true.
   AllSampleRateSet=true;
   for(unsigned i=0;i<SampleRateSet.size();i++)
   {
      if(!SampleRateSet[i])
      {
         AllSampleRateSet=false;
         break;
      }
   }
   //printf("AllSampleRateSet: %d\n",(int)AllSampleRateSet);
}
