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

#ifndef __WRITER_H
#define __WRITER_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QTcpSocket>
#include <QTcpServer>
#include <vector>
#include <map>
#include <list>



#include "Base.h"



class StatisticsReader : public BlockInput
{
   Q_OBJECT

   protected:
      // Statistics
      unsigned NumConnections;
      unsigned NumDisconnections;
      unsigned NumConnectionAttempts;
      unsigned PktRcv;
      unsigned PktLost;
      int State;

      double sr;
      double ReportedSampleRate;
      double lt;
      std::vector<double> dt;



   public:
      StatisticsReader();
      virtual ~StatisticsReader();

      virtual void Reset();

      virtual int GetState();
      virtual unsigned GetNumConnections();
      virtual unsigned GetNumDisconnections();
      virtual unsigned GetNumConnectionAttempts();
      virtual unsigned GetNumPktRcv();
      virtual unsigned GetNumPktLost();
      virtual double GetSampleRate();
      virtual double GetReportedSampleRate();
      virtual double GetJitter();
      virtual std::vector<double> GetJitterHistogram(int maxdt);

   private slots:
      virtual void InDataLost(double tine,int num);
      virtual void InDataReceived(double t,bool valid,std::vector<int> data);
      virtual void InDataReceived(double t,std::vector<bool> valid,std::vector<int> data);
      virtual void InState(double t,int state);
      virtual void InConnected(double t);
      virtual void InDisconnected(double t);
      virtual void InConnectionAttempt(double t);
      virtual void InNotify(double t,int message,int value);

   signals:
      void StatisticsChanged();
};


/*
  Writes data into a text file.
  Mode 0: writes data when the data is flagged as NaN
  Mode 1: writes NaNs when the data is flagged as NaN
*/

class TextFileWriter : public BlockInput
{
   Q_OBJECT

   protected:
      QString FileName;
      QFile *File;
      QTextStream *Stream;
      int Mode;

      void Reset();

   public:
      TextFileWriter(int mode=0);
      virtual ~TextFileWriter();

      bool Init(QString fn);

      virtual qint64 FileSize();

   private slots:
      virtual void InDataLost(double tine,int num) {};
      virtual void InDataReceived(double t,bool valid,std::vector<int> data);
      virtual void InDataReceived(double t,std::vector<bool> valid,std::vector<int> data);
      virtual void InState(double t,int state) {};
      virtual void InConnected(double t) {};
      virtual void InDisconnected(double t) {};
      virtual void InConnectionAttempt(double t) {};
      virtual void InNotify(double t,int message,int value) {};
};
/*
  Writes data into a text file.
  Mode 0: writes data when the data is flagged as NaN
  Mode 1: writes NaNs when the data is flagged as NaN
*/

class TextTCPWriter : public BlockInput
{
   Q_OBJECT

   protected:
      QTcpServer *TcpServer;
      std::vector<QTcpSocket*> TcpSockets;
      int Port;
      int Mode;

      void Reset();

   public:
      TextTCPWriter(int mode=0);
      virtual ~TextTCPWriter();

      bool Init(int p);

      virtual qint64 FileSize();

   private slots:
      virtual void InDataLost(double tine,int num) {};
      virtual void InDataReceived(double t,bool valid,std::vector<int> data);
      virtual void InDataReceived(double t,std::vector<bool> valid,std::vector<int> data);
      virtual void InState(double t,int state) {};
      virtual void InConnected(double t) {};
      virtual void InDisconnected(double t) {};
      virtual void InConnectionAttempt(double t) {};
      virtual void InNotify(double t,int message,int value) {};

      virtual void newConnection();
      virtual void disconnected();
};


class ConsoleWriter : public BlockInput
{
   Q_OBJECT

   public:
      ConsoleWriter() {};
      virtual ~ConsoleWriter() {};

   private slots:
      virtual void InDataLost(double tine,int num) {};
      virtual void InDataReceived(double t,bool valid,std::vector<int> data);
      virtual void InDataReceived(double t,std::vector<bool> valid,std::vector<int> data);
      virtual void InState(double t,int state);
      virtual void InConnected(double t);
      virtual void InDisconnected(double t);
      virtual void InConnectionAttempt(double t);
      virtual void InNotify(double t,int message,int value) {};

      };




#endif // __WRITER_H
