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

#ifndef __TABLEDISPLAYWIDGET_H
#define __TABLEDISPLAYWIDGET_H


#include <QFrame>
#include <QGridLayout>

class QTableDisplayWidget : public QFrame
{
   Q_OBJECT

   public:

      QTableDisplayWidget(QStringList hdr,QWidget *parent = 0,Qt::WindowFlags f = 0);
      virtual ~QTableDisplayWidget ();

      virtual void setText(int row,int column,const QString &text);
      virtual void setPixmap(int row,int column,const QPixmap &pixmap);
      virtual void Clear();

   private:
      QGridLayout *gl;
};



#endif
