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

#include <cmath>
#include <QHostAddress>
#include <cfloat>
#include "Writer.h"

double stddev(std::vector<double> d)
{
   double s;
   double m;

   m=0;
   for(unsigned i=0;i<d.size();i++)
      m+=d[i];
   m/=(double)d.size();
   s=0;
   for(unsigned i=0;i<d.size();i++)
      s+=pow(d[i]-m,2.0);
   s/=(double)d.size();
   s=sqrt(s);
   return s;
}



/******************************************************************************
*******************************************************************************
StatisticsReader   StatisticsReader   StatisticsReader   StatisticsReader
*******************************************************************************
******************************************************************************/
StatisticsReader::StatisticsReader()
{
   Reset();
}
StatisticsReader::~StatisticsReader()
{
}
void StatisticsReader::Reset()
{
   NumConnections=0;
   NumDisconnections=0;
   NumConnectionAttempts=0;
   PktRcv=0;
   PktLost=0;
   lt=0;
   dt.resize(128,0);
   ReportedSampleRate=0;

}
void StatisticsReader::InConnected(double time)
{
   NumConnections++;
   emit StatisticsChanged();
}
void StatisticsReader::InDisconnected(double time)
{
   NumDisconnections++;
   emit StatisticsChanged();
}
void StatisticsReader::InConnectionAttempt(double time)
{
   NumConnectionAttempts++;
   emit StatisticsChanged();
}
void StatisticsReader::InDataLost(double tine,int num)
{
   PktLost+=num;
   emit StatisticsChanged();
}
void StatisticsReader::InDataReceived(double time,bool valid,std::vector<int> data)
{
   if(time<=DBL_EPSILON)
      return;
   if(valid)
   {
      PktRcv++;
      dt.push_back(time-lt);
      dt.erase(dt.begin());
      lt = time;
   }
   else
      PktLost++;
   emit StatisticsChanged();
}
void StatisticsReader::InDataReceived(double time,std::vector<bool> valid,std::vector<int> data)
{
   PktRcv++;
   dt.push_back(time-lt);
   dt.erase(dt.begin());
   lt = time;
   emit StatisticsChanged();
}
void StatisticsReader::InState(double t,int state)
{
   State = state;
   emit StatisticsChanged();
}
void StatisticsReader::InNotify(double t,int message,int value)
{
   switch(message)
   {
      case 0:
         ReportedSampleRate = value/1000.0;
         break;
      default:
         break;
   }
   emit StatisticsChanged();
}


/*
  public
*/
int StatisticsReader::GetState()
{
   return State;
}
unsigned StatisticsReader::GetNumConnections()
{
   return NumConnections;
}
unsigned StatisticsReader::GetNumDisconnections()
{
   return NumDisconnections;
}
unsigned StatisticsReader::GetNumConnectionAttempts()
{
   return NumConnectionAttempts;
}
unsigned StatisticsReader::GetNumPktRcv()
{
   return PktRcv;
}
unsigned StatisticsReader::GetNumPktLost()
{
   return PktLost;
}
double StatisticsReader::GetSampleRate()
{
   //return 1.0/ *dt.end();
   double s;
   s=0;
   for(unsigned i=0;i<dt.size();i++)
      s+=dt[i];
   s/=(double)dt.size();
   return 1.0/s;
}
double StatisticsReader::GetReportedSampleRate()
{
   return ReportedSampleRate;
}
double StatisticsReader::GetJitter()
{
   // compute the std dev.
   return stddev(dt);
}
std::vector<double> StatisticsReader::GetJitterHistogram(int maxdt)
{
   std::vector<double> hist;


   hist.resize(maxdt,0);
   for(unsigned i=0;i<dt.size();i++)
   {
      //printf("%.1lf\n",dt[i]*1000.0);
      int idx = (int)(dt[i]*1000.0);
      if(idx<maxdt)
         hist[idx]+=1.0/(double)dt.size();
   }
   //printf("\n");
   return hist;
}

/******************************************************************************
*******************************************************************************
TextFileWriter   TextFileWriter   TextFileWriter   TextFileWriter
*******************************************************************************
******************************************************************************/

TextFileWriter::TextFileWriter(int mode)
{
   Mode=mode;
   Stream=0;
   File=0;

   Reset();

   

}
TextFileWriter::~TextFileWriter()
{
   Reset();
}
void TextFileWriter::Reset()
{
   if(Stream)
   {
      delete Stream;
      Stream=0;
   }
   if(File)
   {
      File->close();
      delete File;
      File=0;
   }
}
bool TextFileWriter::Init(QString fn)
{
   File = new QFile(fn);
   if(!File->open(QIODevice::WriteOnly|QIODevice::Truncate|QIODevice::Text))
   {
      Reset();
      return false;
   }
   Stream = new QTextStream(File);
   Stream->setRealNumberNotation(QTextStream::FixedNotation);
   Stream->setRealNumberPrecision(12);
   return true;
}
void TextFileWriter::InDataReceived(double t,bool valid,std::vector<int> data)
{
   if(t<=DBL_EPSILON)
      return;
   /*if(data.size()<5)
   {
      printf("TextFileWriter::InDataReceived single,mode: %d\n",Mode);
      for(int i=0;i<data.size();i++)
         printf("%d ",data[i]);
      printf("\n");
   }*/
   (*Stream) << t << " " ;
   if(Mode == 0)
      for(int i=0;i<data.size();i++)
         (*Stream) << data[i] << " ";
   else
      for(int i=0;i<data.size();i++)
         if(valid)
            (*Stream) << data[i] << " ";
         else
            (*Stream) << "NaN ";

   (*Stream) << endl;
}
void TextFileWriter::InDataReceived(double t,std::vector<bool> valid,std::vector<int> data)
{
   if(t<=DBL_EPSILON)
      return;
   /*
   printf("TextFileWriter::InDataReceived vector (mode: %d)\n",Mode);
   printf("Received %lf\n",t);
   for(int i=0;i<valid.size();i++)
      printf("%d ",(int)valid[i]);
   printf("\n");*/
   (*Stream) << t << " " ;
   if(Mode == 0)
   {
      for(int i=0;i<data.size();i++)
         (*Stream) << data[i] << " ";
   }
   else
   {
      for(int i=0;i<data.size();i++)
      {
         if(valid[i])
            (*Stream) << data[i] << " ";
         else
            (*Stream) << "NaN ";
      }
   }

   (*Stream) << endl;
}

qint64 TextFileWriter::FileSize()
{
   return File->size();
}



/******************************************************************************
*******************************************************************************
TextTCPWriter   TextTCPWriter   TextTCPWriter   TextTCPWriter   TextTCPWriter
*******************************************************************************
******************************************************************************/

TextTCPWriter::TextTCPWriter(int mode)
{
   Mode=mode;
   Port=0;
   TcpServer=0;

   Reset();

}
TextTCPWriter::~TextTCPWriter()
{
   Reset();
}
void TextTCPWriter::Reset()
{
   for(int i=0;i<TcpSockets.size();i++)
      delete TcpSockets[i];
   TcpSockets.clear();
   if(TcpServer)
   {
      TcpServer->close();
      delete TcpServer;
      TcpServer=0;
   }
}
bool TextTCPWriter::Init(int port)
{
   printf("open port %d\n",port);
   Port=port;
   TcpServer = new QTcpServer(this);
   //QHostAddress ha(QHostAddress::LocalHost);
   QHostAddress ha(QHostAddress::Any);
   bool ok;
   ok = TcpServer->listen(ha,Port);
   if(!ok)
      return false;
   connect(TcpServer,SIGNAL(newConnection()),this,SLOT(newConnection()));
   return true;
}

qint64 TextTCPWriter::FileSize()
{

   return 0;
}


void TextTCPWriter::InDataReceived(double t,bool valid,std::vector<int> data)
{
   if(t<=DBL_EPSILON)
      return;
   QString s;
   QTextStream stream(&s);

   // Build the string

   stream << t << " ";
   if(Mode == 0)
      for(int i=0;i<data.size();i++)
         stream << data[i] << " ";
   else
      for(int i=0;i<data.size();i++)
         stream << "NaN ";

   stream << endl;

   // Send to all connected devices
   for(unsigned i=0;i<TcpSockets.size();i++)
      //TcpSockets[i]->write(s.toAscii());
       TcpSockets[i]->write(s.toLatin1());
}
void TextTCPWriter::InDataReceived(double t,std::vector<bool> valid,std::vector<int> data)
{
   if(t<=DBL_EPSILON)
      return;
   QString s;
   QTextStream stream(&s);

   // Build the string
   stream << t << " " ;
   if(Mode == 0)
   {
      for(unsigned  i=0;i<data.size();i++)
         stream << data[i] << " ";
   }
   else
   {
      for(unsigned i=0;i<data.size();i++)
      {
         if(valid[i])
            stream << data[i] << " ";
         else
            stream << "NaN ";
      }
   }
   stream << endl;

   // Send to all connected devices
   for(unsigned i=0;i<TcpSockets.size();i++)
      //TcpSockets[i]->write(s.toAscii());
       TcpSockets[i]->write(s.toLatin1());
}
void TextTCPWriter::newConnection()
{
   //printf("New connection!\n");
   QTcpSocket * s = TcpServer->nextPendingConnection();
   TcpSockets.push_back(s);

   // Set-up a disconnection handler
   connect(s,SIGNAL(disconnected()),this,SLOT(disconnected()));

   /*QHostAddress ha;
   printf("Peer name: %s\n",s->peerName().toStdString().c_str());
   ha = s->peerAddress();
   printf("Peer address: %s\n",ha.toString().toStdString().c_str());
   ha = s->localAddress();
   printf("Local address: %s\n",ha.toString().toStdString().c_str());*/
}
void TextTCPWriter::disconnected()
{  
   //printf("Disconnection\n");
   QObject *s = sender();
   for(unsigned i=0;i<TcpSockets.size();i++)
   {
      if(s == TcpSockets[i])
      {
         //printf("Disconnection of entry %d\n",i);
         TcpSockets[i]->deleteLater();
         TcpSockets.erase(TcpSockets.begin()+i);
         break;
      }
   }

}

/******************************************************************************
ConsoleWriter   ConsoleWriter   ConsoleWriter   ConsoleWriter   ConsoleWriter
******************************************************************************/
void ConsoleWriter::InDataReceived(double t,bool valid,std::vector<int> data)
{
   printf("%lf - %s",t,valid?"V":"NAN");
   for(int i=0;i<data.size();i++)
      printf("%d ",data[i]);
   printf("\n");
}
void ConsoleWriter::InDataReceived(double t,std::vector<bool> valid,std::vector<int> data)
{

}
void ConsoleWriter::InState(double t,int state)
{
}
void ConsoleWriter::InConnected(double t)
{
}
void ConsoleWriter::InDisconnected(double t)
{
}
void ConsoleWriter::InConnectionAttempt(double t)
{
}



