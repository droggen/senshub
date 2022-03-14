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

#include <QTime>

#include <stdio.h>
#include <stdlib.h>
#include "SimSensReader.h"

SimSensReader::SimSensReader(double rout)
{
   Rout=rout;
   requeststop=false;
}
SimSensReader::~SimSensReader()
{
   Stop();
}
bool SimSensReader::Start()
{
   printf("Starting\n");
   start();
   return true; // Success
};
void SimSensReader::Stop()
{
   printf("Stopping\n");
   // Set some variable - careful here....
   requeststop=true;
   wait();
   printf("Stopped\n");
};


void SimSensReader::run()
{
   std::vector<int> data;
   data.resize(5);
   int packet=0;
   int r;

   // Emit dummy data to notify the chain of the number of channels of this reader
   emit OutDataReceived(0,false,data);

   while(!requeststop)
   {
      // Attempt to connect
      r=rand()%3;
      for(int i=0;i<r;i++)
      {
         emit OutConnectionAttempt(0);
      }
      emit OutConnected(0);
      emit OutState(0,1);

      r=rand()%100;
      for(int i=0;i<r;i++)
      {
         // Create a packet,
         data[0]=packet;
         emit OutDataReceived(0,true,data);
         msleep(1000/Rout);
         packet++;
      }

      // Disconnected!
      emit OutDisconnected(0);
      emit OutState(0,0);

   }
}


