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

#ifndef __LABELREADER_H
#define __LABELREADER_H

#include <QObject>
#include <QKeyEvent>
#include <QMutex>
#include "precisetimer.h"
#include "merger.h"

#include "BaseReader.h"

 class KeyPressEater : public QObject
 {
   Q_OBJECT
   protected:
      bool eventFilter(QObject *obj, QEvent *event);
   public:
      Q_INVOKABLE KeyPressEater ( QObject * parent = 0 );
      virtual ~KeyPressEater ();

   signals:
      void Key(int key);
 };



class LabelReader : public BaseReader
{
   Q_OBJECT
   friend double SmartSleep(double wakeuptime);

   private:
      QWidget *label;
      double Rout;
      double DeltaT;
      QMutex mutex;

      virtual void run();
      KeyPressEater *ke;
      std::vector<int> keypresses;


   public:
      LabelReader(QWidget *l);
      virtual ~LabelReader();

      virtual bool Start();
      virtual void Stop();
      virtual bool IsOk() {return true;};

   private slots:
      virtual void Key(int key);
};



#endif // __LABELREADER_H
