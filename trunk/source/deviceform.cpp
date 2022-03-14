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

#include "deviceform.h"
#include "ui_deviceform.h"

deviceform::deviceform(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::deviceform)
{
    m_ui->setupUi(this);
}

deviceform::~deviceform()
{
    delete m_ui;
}

void deviceform::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        m_ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

QString deviceform::GetDeviceSpecificationString()
{
   return m_ui->textEditDeviceSpecification->toPlainText();
}
void deviceform::SetDeviceSpecificationString(QString str)
{
   m_ui->textEditDeviceSpecification->setText(str);
}
