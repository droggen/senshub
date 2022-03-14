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

#include <QWidget>
#include "LabelReader.h"

#include "precisetimer.h"



Q_INVOKABLE KeyPressEater::KeyPressEater (QObject * parent)
      : QObject(parent)
{
}
KeyPressEater::~KeyPressEater()
{
}

 bool KeyPressEater::eventFilter(QObject *obj, QEvent *event)
 {
     if (event->type() == QEvent::KeyPress) {
         QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
         //printf("Ate key press %d\n", keyEvent->key());
         emit Key(keyEvent->key());
         return true;
     } else {
         // standard event processing
         return QObject::eventFilter(obj, event);
     }
 }



LabelReader::LabelReader(QWidget *l)
{
   label=l;
   Rout=16;
   DeltaT=1.0/Rout;
   requeststop=false;
   ke = new KeyPressEater(0);
   connect(ke,SIGNAL(Key(int)),this,SLOT(Key(int)));
}
LabelReader::~LabelReader()
{
   if(ke) delete ke;
}

bool LabelReader::Start()
{
   //label->grab
   keypresses.clear();
   keypresses.resize(1,0);
   label->grabKeyboard();
   label->installEventFilter(ke);
   start();
   return true;
}
void LabelReader::Stop()
{

   requeststop=true;
   wait();
   label->removeEventFilter(ke);
   label->releaseKeyboard();
}

void LabelReader::run()
{
   double tstart = PreciseTimer::QueryTimer();
   double t;
   int i=0;

   std::vector<int> data;
   data.resize(1,0);

   emit OutState(PreciseTimer::QueryTimer(),0);
   emit OutConnectionAttempt(PreciseTimer::QueryTimer());
   emit OutState(PreciseTimer::QueryTimer(),1);
   emit OutConnected(PreciseTimer::QueryTimer());
   emit OutState(PreciseTimer::QueryTimer(),2);

   // Output channel width
   emit OutDataReceived(0,false,data);
   // Output the sample rate
   emit OutNotify(tstart,0,(int)(Rout*1000.0));

   while(!requeststop)
   {
      double twakeup = tstart+i*DeltaT;
      t=SmartSleep(twakeup);

      // Prepare the data
      QMutexLocker locker(&mutex);
      locker.relock();
      data[0] = keypresses[0];
      if(keypresses.size()>=2)
         keypresses.erase(keypresses.begin());
      locker.unlock();

      //printf("Key: %d\n",data[0]);
      emit OutDataReceived(t,true,data); // Data was merged at time t

      i++;
   }
   emit OutDisconnected(PreciseTimer::QueryTimer());
   emit OutState(PreciseTimer::QueryTimer(),0);
}

void LabelReader::Key(int key)
{
   QMutexLocker locker(&mutex);
   keypresses.push_back(key);
}
