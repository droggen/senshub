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

#include <QLabel>

#include "TableDisplayWidget.h"

/************************************************************************
QTableDisplayWidget   QTableDisplayWidget   QTableDisplayWidget
************************************************************************/

QTableDisplayWidget::QTableDisplayWidget(QStringList hdr,QWidget *parent,Qt::WindowFlags f)
      : QFrame(parent,f)
{
   gl = new QGridLayout(this);
   setLayout(gl);
   setFrameShape(QFrame::Panel);
   setAutoFillBackground(true);

   QPalette palette;
   palette.setColor(backgroundRole(),QColor(255,255,255));
   setPalette(palette);


   for(int i=0;i<hdr.size();i++)
   {
      QLabel *label;
      label = new QLabel(hdr[i]);
      QFont font;
      font.setBold(true);
      label->setFont(font);
      label->setWordWrap(true);
      gl->addWidget(label,0,i,1,1,Qt::AlignCenter);
   }
}
QTableDisplayWidget::~QTableDisplayWidget()
{
   // Delete the items of the grid layout.
   // ?
   /*
   for(unsigned y=0;y<gl->rowCount();y++)
   {
      for(unsigned x=0;x<gl->columnCount();x++)
      {
         QLayoutItem *item = gl->itemAtPosition(y,x);
         if(item)
         {
            gl->removeWidget(item->widget());
            delete item->widget();
         }
      }
   }
   */
   delete gl;
}

void QTableDisplayWidget::Clear()
{
   // Clear the table.
   if(gl->rowCount()<=1)
      return;
   for(int y=1;y<gl->rowCount();y++)
   {
      for(int x=0;x<gl->columnCount();x++)
      {
         QLayoutItem *item = gl->itemAtPosition(y,x);
         QWidget *widget;
         if(item)
         {
            widget = item->widget();
         }
         else
            widget=0;
         if(widget)
         {
            gl->removeWidget(widget);
            delete widget;
         }
      }
   }
}

void QTableDisplayWidget::setText(int row,int column,const QString &text)
{
   int irow,icolumn;
   irow=1+row;
   icolumn=column;
   QLabel *label;
   QLayoutItem *item = gl->itemAtPosition(irow,icolumn);

   if(item)
   {
      label = dynamic_cast<QLabel*>(item->widget());
   }
   else
   {
      label = new QLabel();
      label->setWordWrap(true);
      gl->addWidget(label,irow,icolumn,1,1,Qt::AlignCenter);
   }

   label->setText(text);
}
void QTableDisplayWidget::setPixmap(int row,int column,const QPixmap &pixmap)
{
   int irow,icolumn;
   irow=1+row;
   icolumn=column;
   QLabel *label;
   QLayoutItem *item = gl->itemAtPosition(irow,icolumn);

   if(item)
   {
      label = dynamic_cast<QLabel*>(item->widget());
   }
   else
   {
      label = new QLabel();
      gl->addWidget(label,irow,icolumn,1,1,Qt::AlignCenter);
   }

   label->setPixmap(pixmap);

}
