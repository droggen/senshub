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


#ifndef __BASEREADER_H
#define __BASEREADER_H

#include <vector>
#include <map>
#include <list>
#include <QThread>

#include "Base.h"


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// BaseReader   BaseReader   BaseReader   BaseReader   BaseReader   BaseReader   BaseReader   BaseR
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// 
//class BaseReader : public BlockOutput
class BaseReader : public BlockInputOutput
{
   Q_OBJECT

	protected:
      bool requeststop;

	public:
      BaseReader();
      virtual ~BaseReader(){};
		
      virtual bool Start()=0;
      virtual void Stop()=0;
      virtual bool IsOk()=0;


   private slots:
      virtual void InDataLost(double time,int num){};
      virtual void InDataReceived(double t,bool valid,std::vector<int> data){};
      virtual void InDataReceived(double t,std::vector<bool> valid,std::vector<int> data){};
      virtual void InState(double t,int state){};
      virtual void InConnected(double t){};
      virtual void InDisconnected(double t){};
      virtual void InConnectionAttempt(double t){};
      virtual void InNotify(double time,int message,int value) {};

   /*signals:
      virtual void OutDataLost(double time,int num);
      virtual void OutDataReceived(double time,bool valid,std::vector<int> data);
      virtual void OutConnected(double time);
      virtual void OutDisconnected(double time);
      virtual void OutConnectionAttempt(double time);
      virtual void OutState(double time,int s);
      virtual void InNotify(double time,int message,int value);*/
};

#endif

