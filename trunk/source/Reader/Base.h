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


#ifndef __BASE_H
#define __BASE_H

#include <QThread>
#include <vector>

class BlockInput : public QObject
{
   Q_OBJECT
   private:

   protected:


   public:
      BlockInput() {};
      virtual ~BlockInput() {};

      virtual void ConnectFrom(QObject *);
      virtual void DisconnectFrom(QObject *);

   private slots:
      virtual void InDataLost(double time,int num)=0;
      virtual void InDataReceived(double t,bool valid,std::vector<int> data)=0;
      virtual void InDataReceived(double t,std::vector<bool> valid,std::vector<int> data)=0;
      virtual void InState(double t,int state)=0;
      virtual void InConnected(double t)=0;
      virtual void InDisconnected(double t)=0;
      virtual void InConnectionAttempt(double t)=0;
      virtual void InNotify(double time,int message,int value)=0;
};

class BlockInputOutput : public QThread
{
   Q_OBJECT


   public:
      BlockInputOutput() {};
      virtual ~BlockInputOutput() {};

      virtual void ConnectFrom(QObject *);
      virtual void DisconnectFrom(QObject *);

   private:
      virtual void run()=0;

   protected:

   private slots:
      virtual void InDataLost(double time,int num)=0;
      virtual void InDataReceived(double t,bool valid,std::vector<int> data)=0;
      virtual void InDataReceived(double t,std::vector<bool> valid,std::vector<int> data)=0;
      virtual void InState(double t,int state)=0;
      virtual void InConnected(double t)=0;
      virtual void InDisconnected(double t)=0;
      virtual void InConnectionAttempt(double t)=0;
      virtual void InNotify(double time,int message,int value)=0;

   signals:
      virtual void OutDataLost(double time,int num);
      virtual void OutDataReceived(double time,bool valid,std::vector<int> data);
      virtual void OutDataReceived(double time,std::vector<bool> valid,std::vector<int> data);
      virtual void OutConnected(double time);
      virtual void OutDisconnected(double time);
      virtual void OutConnectionAttempt(double time);
      virtual void OutState(double time,int s);
      virtual void OutNotify(double time,int message,int value);
};
/*
class BlockOutput : public QThread
{
   Q_OBJECT

   public:
      BlockOutput() {};
      virtual ~BlockOutput() {};

   private:
      virtual void run()=0;

   protected:


   signals:
      virtual void OutDataLost(double time,int num);
      virtual void OutDataReceived(double time,bool valid,std::vector<int> data);
      virtual void OutConnected(double time);
      virtual void OutDisconnected(double time);
      virtual void OutConnectionAttempt(double time);
      virtual void OutState(double time,int s);
};
*/


#endif
