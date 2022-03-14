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


#include "Base.h"

#include <cstdio>

/******************************************************************************
BlockInput   BlockInput   BlockInput   BlockInput   BlockInput   BlockInput
******************************************************************************/
void BlockInput::ConnectFrom(QObject *obj)
{
   //printf("BlockInput::ConnectFrom %p\n",obj);
   connect(obj,SIGNAL(OutDataLost(double,int)),this,SLOT(InDataLost(double,int)));
   connect(obj,SIGNAL(OutDataReceived(double,bool,std::vector<int>)),this,SLOT(InDataReceived(double,bool,std::vector<int>)));
   connect(obj,SIGNAL(OutDataReceived(double,std::vector<bool>,std::vector<int>)),this,SLOT(InDataReceived(double,std::vector<bool>,std::vector<int>)));
   connect(obj,SIGNAL(OutState(double,int)),this,SLOT(InState(double,int)));
   connect(obj,SIGNAL(OutConnected(double)),this,SLOT(InConnected(double)));
   connect(obj,SIGNAL(OutDisconnected(double)),this,SLOT(InDisconnected(double)));
   connect(obj,SIGNAL(OutConnectionAttempt(double)),this,SLOT(InConnectionAttempt(double)));
   connect(obj,SIGNAL(OutNotify(double,int,int)),this,SLOT(InNotify(double,int,int)));
}
void BlockInput::DisconnectFrom(QObject *obj)
{
   //printf("BlockInput::DisconnectFrom %p\n",obj);
   disconnect(obj,SIGNAL(OutDataLost(double,int)),this,SLOT(InDataLost(double,int)));
   disconnect(obj,SIGNAL(OutDataReceived(double,bool,std::vector<int>)),this,SLOT(InDataReceived(double,bool,std::vector<int>)));
   disconnect(obj,SIGNAL(OutDataReceived(double,std::vector<bool>,std::vector<int>)),this,SLOT(InDataReceived(double,std::vector<bool>,std::vector<int>)));
   disconnect(obj,SIGNAL(OutState(double,int)),this,SLOT(InState(double,int)));
   disconnect(obj,SIGNAL(OutConnected(double)),this,SLOT(InConnected(double)));
   disconnect(obj,SIGNAL(OutDisconnected(double)),this,SLOT(InDisconnected(double)));
   disconnect(obj,SIGNAL(OutConnectionAttempt(double)),this,SLOT(InConnectionAttempt(double)));
   disconnect(obj,SIGNAL(OutNotify(double,int,int)),this,SLOT(InNotify(double,int,int)));
}

/******************************************************************************
BlockInputOutput   BlockInputOutput   BlockInputOutput   BlockInputOutput
******************************************************************************/
void BlockInputOutput::ConnectFrom(QObject *obj)
{
   //printf("BlockInputOutput::ConnectFrom %p\n",obj);
   connect(obj,SIGNAL(OutDataLost(double,int)),this,SLOT(InDataLost(double,int)));
   connect(obj,SIGNAL(OutDataReceived(double,bool,std::vector<int>)),this,SLOT(InDataReceived(double,bool,std::vector<int>)));
   connect(obj,SIGNAL(OutDataReceived(double,std::vector<bool>,std::vector<int>)),this,SLOT(InDataReceived(double,std::vector<bool>,std::vector<int>)));
   connect(obj,SIGNAL(OutState(double,int)),this,SLOT(InState(double,int)));
   connect(obj,SIGNAL(OutConnected(double)),this,SLOT(InConnected(double)));
   connect(obj,SIGNAL(OutDisconnected(double)),this,SLOT(InDisconnected(double)));
   connect(obj,SIGNAL(OutConnectionAttempt(double)),this,SLOT(InConnectionAttempt(double)));
   connect(obj,SIGNAL(OutNotify(double,int,int)),this,SLOT(InNotify(double,int,int)));
}
void BlockInputOutput::DisconnectFrom(QObject *obj)
{
   //printf("BlockInputOutput::DisconnectFrom %p\n",obj);
   disconnect(obj,SIGNAL(OutDataLost(double,int)),this,SLOT(InDataLost(double,int)));
   disconnect(obj,SIGNAL(OutDataReceived(double,bool,std::vector<int>)),this,SLOT(InDataReceived(double,bool,std::vector<int>)));
   disconnect(obj,SIGNAL(OutDataReceived(double,std::vector<bool>,std::vector<int>)),this,SLOT(InDataReceived(double,std::vector<bool>,std::vector<int>)));
   disconnect(obj,SIGNAL(OutState(double,int)),this,SLOT(InState(double,int)));
   disconnect(obj,SIGNAL(OutConnected(double)),this,SLOT(InConnected(double)));
   disconnect(obj,SIGNAL(OutDisconnected(double)),this,SLOT(InDisconnected(double)));
   disconnect(obj,SIGNAL(OutConnectionAttempt(double)),this,SLOT(InConnectionAttempt(double)));
   disconnect(obj,SIGNAL(OutNotify(double,int,int)),this,SLOT(InNotify(double,int,int)));
}


