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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QThread>
#include <QFileDialog>
#include <QPainter>
#include <QMessageBox>
#include <QIntValidator>
#include <QRegExpValidator>
#include <QXmlStreamWriter>
#include <QKeyEvent>


#include <stdio.h>
#include <utility>

#include "precisetimer.h"
#include "cio.h"
#include "helper.h"
#include "Reader/SimSensReader.h"
#include "Reader/Hub.h"
#include "Reader/FBReader.h"
#include "deviceform.h"
#include "helpdialog.h"
#include "offline.h"



/*
*/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)

{
#ifdef DEVELMODE
   ConsoleInit();
   printf("Starting up\n");
#endif

   ui->setupUi(this);

   ui->lineEditWorkingDirectory->setText(QDir::currentPath());

   connect(&MyHub,SIGNAL(StatisticsChanged(int,int)),this,SLOT(StatisticsChanged(int,int)));



   // Create the state/statistics display
   QStringList hdr;
   hdr << "Dev" << "State" << "Rcv" << "Lost" << "#Dis" << "#Con" << "#ConTry" << "SR inst" << "SR lock" << "dt histogram" << "File size" << "RC Buffer" << "TS Buffer";
   StatTable = new QTableDisplayWidget(hdr,this);
   ui->horizontalLayoutStat->addWidget(StatTable);

   hdr.clear();
   hdr << "Merger" << "State" << "Rcv" << "SR inst" << "dt histogram" << "File size";
   StatTableM = new QTableDisplayWidget(hdr,this);
   ui->horizontalLayoutStatM->addWidget(StatTableM);


   // Set-up the edit fields for the mergers
   ui->lineEdit_Merger_L_Port->setValidator(new QIntValidator(0,65535,this));
   ui->lineEdit_Merger_RC_Port->setValidator(new QIntValidator(0,65535,this));
   ui->lineEdit_Merger_TS_Port->setValidator(new QIntValidator(0,65535,this));

   // Set-up the edit fields for the devices
   ui->lineEdit_DSDev->setValidator(new QRegExpValidator(QRegExp("\\s*([\\w]+)\\s*:\\s*([\\d]+)\\s*"),this));
   ui->lineEdit_DSFormat->setValidator(new QRegExpValidator(QRegExp("\\s*([\\w]+)\\s*;\\s*([csSiIbB\\d-]+)\\s*(;\\s*([x|f|F])\\s*)?"),this));

   //
   on_listWidget_DSList_currentRowChanged(-1);


   // Load the pixmap for the connection states
   PixmapConnected=QPixmap(":/i_cnc.bmp");
   PixmapConnectedSRE=QPixmap(":/i_cncsre.bmp");
   PixmapDisconnected=QPixmap(":/i_disc.bmp");
   PixmapInProgress=QPixmap(":/i_int.bmp");


   // Hub parameters
   HubStarted=false;

   // Status bar
   SBStart=new QLabel(statusBar());
   statusBar()->addWidget(SBStart);
   SBDuration=new QLabel(statusBar());
   statusBar()->addWidget(SBDuration);



}

MainWindow::~MainWindow()
{
   delete StatTable;
   delete StatTableM;

   delete SBStart;
   delete SBDuration;

   delete ui;
}








void MainWindow::timerEvent(QTimerEvent *event)
{
   static double dt=0;
   static double lt=0;

   double t=PreciseTimer::QueryTimer();
   double idt = t-lt;
   dt = 0.9*dt+0.1*idt;
   lt=t;

   printf("Timer ID: %d. %lf %lf %lf\n",event->timerId(),t,idt,dt);
}









void MainWindow::on_toolButtonWorkingDirectory_clicked()
{
   QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),"",QFileDialog::ShowDirsOnly|QFileDialog::DontResolveSymlinks);
   if(!dir.isNull())
   {
      ui->lineEditWorkingDirectory->setText(dir);
      path.setCurrent(dir);
   }
}



/**
  \brief Convert a textual description of the sensors to connect from, into a device specification.
  The string has the following format:
  <ser:/path/to/device : speed : frameformat : file : ctrsize : ctrchannel : burstregen>...
**/

int MainWindow::ParseDeviceString(QString devicestring,std::vector<DeviceSpec> &devspec,QString &err)
{
   err="";

   std::vector<DeviceSpec> ds;
   devspec.clear();

   //printf("Parsing: '%s'\n",devicestring.toStdString().c_str());

   // Expression for a single device, of the type <ser:comport:speed:frameformat:fileout>
   // Ensures no spaces are provided as format strings, file, etc (with [...^\\s])
   //string expression = string("\\s*<\\s*(ser)\\s*:\\s*([^:^>^<^\\s]+)\\s*:\\s*([^:^>^<^\\s]+)\\s*:\\s*([^:^>^<^\\s]*)\\s*:\\s*([^:^>^<^\\s]+)\\s*>\\s*");
   //                              <     ser     :  com         :    spd       :         fmt      :    file      :   ctrsiz         :    ctrchannel  :    burstregen
   string expression = string("\\s*<\\s*(ser)\\s*:\\s*(\\w+)\\s*:\\s*(\\d+)\\s*:\\s*([\\w-;]+)\\s*:\\s*(\\w+)\\s*:\\s*([nN\\d]+)\\s*:\\s*([\\d]+)\\s*:\\s*(\\d+)\\s*>\\s*");

   QRegExp e(QString(expression.c_str()));
   e.setCaseSensitivity(Qt::CaseInsensitive);

   //printf("e.isValid: %d\n",(int)e.isValid());
   //printf("e.isValid: %s\n",e.errorString().toStdString().c_str());

   // Expression for multiple devices, of the type <.....><....>...
   string expression2=string("(")+expression+string(")+");
   QRegExp validation(QString(expression2.c_str()));
   validation.setCaseSensitivity(Qt::CaseInsensitive);

   //printf("validation.isValid: %d\n",(int)validation.isValid());
   //printf("validation.isValid: %s\n",validation.errorString().toStdString().c_str());


   // Global validation
   if(validation.exactMatch(devicestring)==0)
   {
      err = "Invalid device string - check syntax";
      return -5;
   }
   //printf("Matched length: %d\n",validation.matchedLength());

   // Validation is okay - we extract the content

   int pos =0;
   unsigned midx=0;
   while ((pos = e.indexIn(devicestring, pos)) != -1) {
      pos += e.matchedLength();

      /*printf("Capture group %d\n",midx);
      printf("NumCaptures: %d\n",e.numCaptures());
      for(int i=0;i<e.numCaptures()+1;i++)
         printf("Capture %d: %s\n",i,e.cap(i).toStdString().c_str());*/

      if(e.captureCount()!=8)
      {
         err = "Invalid device string - check syntax";
         return -4;
      }

      // Check baudrate
      int baud = Baud2Baud(e.cap(3).trimmed());
      if(baud==-1)
      {
         err = "Invalid baud rate";
         return -3;
      }

      //printf("Baud rate: %s\n",e.cap(3).trimmed().toStdString().c_str());
      // Extract the components
      DeviceSpec spec;
      spec.devtype=e.cap(1).trimmed();
      spec.dev=e.cap(2).trimmed();
      spec.spdport=e.cap(3).trimmed();
      spec.format=e.cap(4).trimmed();
      spec.file=e.cap(5).trimmed();

      // counter size: n,N,8,or 16
      bool ok;
      QString cs = e.cap(6).trimmed();
      if(cs.compare("n")==0 || cs.compare("N")==0)
      {
         spec.ctrmod=0;
      }
      else
      {
         int csi = cs.toInt(&ok);
         if( (ok==false) || (csi!=8 && csi!=16))
         {
            err = "Invalid counter size";
            return -6;
         }
         spec.ctrmod = 1 << csi;
      }
      spec.ctrchannel=e.cap(7).toInt(&ok);
      if(ok==false || spec.ctrchannel<0)
      {
         err = "Invalid counter channel";
         return -7;
      }
      spec.maxburstregen=e.cap(8).toInt(&ok);
      if(ok==false || spec.maxburstregen<0)
      {
         err = "Invalid maximum packet regeneration";
         return -8;
      }

      ds.push_back(spec);

      midx++;

   }

   // Return ok.
   devspec = ds;
   return 0;
}

void MainWindow::DeviceSpecification2DeviceSpecificationAbs()
{
   // Relative to absolute path for devices
   DeviceSpecificationAbs = DeviceSpecification;
   for(unsigned i=0;i<DeviceSpecificationAbs.size();i++)
   {
      QString abs=ui->lineEditWorkingDirectory->text()+"/"+DeviceSpecificationAbs[i].file;
      //printf("%d. %s -> %s\n",i,ds[i].file.toStdString().c_str(),abs.toStdString().c_str());
      QString clean = QDir::cleanPath(abs);
      //printf("\t%s\n",clean.toStdString().c_str());
      DeviceSpecificationAbs[i].file = clean;
   }
}
/**
**/
QString MainWindow::GetLabelFile()
{
   return ui->lineEdit_Label_File->text();
}
QString MainWindow::GetLabelFileAbs()
{
   return ui->lineEdit_Label_File->text().size()?QDir::cleanPath(ui->lineEditWorkingDirectory->text()+"/"+ui->lineEdit_Label_File->text()):"";
}
bool MainWindow::ApplyConfiguration()
{
   NTHubErr err;
   QString errtitle = "Configuration of data aquisition hub failed";

   // Check that the merger and label path aren't absolute, also for offline
   if(
      QDir::isAbsolutePath(ui->lineEdit_Merger_L_File->text()) ||
      QDir::isAbsolutePath(ui->lineEdit_Merger_RC_File->text()) ||
      QDir::isAbsolutePath(ui->lineEdit_Merger_TS_File->text()) ||
      QDir::isAbsolutePath(ui->lineEdit_Merger_O_File->text()) ||
      QDir::isAbsolutePath(GetLabelFile())
      )
   {
      QMessageBox::information(this,errtitle,"Invalid merger or label file: only relative path are allowed.");
      return false;
   }


   // Merge specification
   bool ok1,ok2,ok3;
   int p1,p2,p3;
   p1=ui->lineEdit_Merger_L_Port->text().toInt(&ok1);
   p2=ui->lineEdit_Merger_RC_Port->text().toInt(&ok2);
   p3=ui->lineEdit_Merger_TS_Port->text().toInt(&ok3);
   printf("%d-%d %d-%d %d-%d\n",(int)ok1,p1,(int)ok2,p2,(int)ok3,p3);
   MergeSpec MergeSpecification(
                                 ui->lineEdit_Merger_L_File->text().size()?QDir::cleanPath(ui->lineEditWorkingDirectory->text()+"/"+ui->lineEdit_Merger_L_File->text()):"",
                                 ok1?p1:0,
                                 ui->lineEdit_Merger_RC_File->text().size()?QDir::cleanPath(ui->lineEditWorkingDirectory->text()+"/"+ui->lineEdit_Merger_RC_File->text()):"",
                                 ok2?p2:0,
                                 ui->lineEdit_Merger_TS_File->text().size()?QDir::cleanPath(ui->lineEditWorkingDirectory->text()+"/"+ui->lineEdit_Merger_TS_File->text()):"",
                                 ok3?p3:0
                                 );

   printf("%s\n",MergeSpecification.lmergerfile.toStdString().c_str());
   printf("%s\n",MergeSpecification.rcmergerfile.toStdString().c_str());
   printf("%s\n",MergeSpecification.tsmergerfile.toStdString().c_str());

   // Merge parameters
   MergeParam MergeParameters;
   MergeParameters.rout=ui->spinBox_Merger_SampleRate->value();
   MergeParameters.lat=ui->spinBox_Merger_Latency->value();
   MergeParameters.kp=ui->doubleSpinBox_Merger_Kp->value();
   MergeParameters.ki=ui->doubleSpinBox_Merger_Ki->value();
   MergeParameters.kd=ui->doubleSpinBox_Merger_Kd->value();

   DeviceSpecification2DeviceSpecificationAbs();

   printf("Label file abs: %s\n",GetLabelFileAbs().toStdString().c_str());
   //bool ok = MyHub.Init(err,ui->frame_Label,ui->lineEdit_Label_File->text(),DeviceSpecification,MergeSpecification,MergeParameters,ui->checkBox_NANFile->isChecked(),ui->checkBox_NANTCP->isChecked());
   //bool ok = MyHub.Init(err,this,ui->lineEdit_Label_File->text(),DeviceSpecificationAbs,MergeSpecification,MergeParameters,ui->checkBox_NANFile->isChecked(),ui->checkBox_NANTCP->isChecked());
   bool ok = MyHub.Init(err,this,GetLabelFileAbs(),DeviceSpecificationAbs,MergeSpecification,MergeParameters,ui->checkBox_NANFile->isChecked(),ui->checkBox_NANTCP->isChecked());
   if(ok==false)
   {
      QString str=MyHub.ErrorString(err);
      QMessageBox::information(this,errtitle,str);
      return false;
   }

   InitTable();
   return true;
}
void MainWindow::InitTable()
{

   // Init the table with the device specifications
   StatTable->Clear();
   StatTable->setText(0,0,"Keyboard");
   for(unsigned i=0;i<DeviceSpecification.size();i++)
   {
      StatTable->setText(i+1,0,QString("%1").arg(i));
   }
   StatTableM->Clear();
   unsigned r=0;
   if(MyHub.IsMerger(0))
      StatTableM->setText(r++,0,"L-Merge");
   if(MyHub.IsMerger(1))
      StatTableM->setText(r++,0,"RC-Merge");
   if(MyHub.IsMerger(2))
      StatTableM->setText(r++,0,"TS-Merge");
}


void MainWindow::StatisticsChanged(int type,int i)
{
   UpdateStatusBar();
   // If device
   if(type==0)
   {
      StatUpdateCounter0++;
      if(StatUpdateCounter0>=StatUpdatePeriod)
      {
         StatUpdateCounter0=0;

         int r=i+1;
         // "Dev" << "State" << "Rcv" << "Lost" << "%lost" << "#Disc" << "#Con" << "#ConAttempt" << "Est. SR" << "SR jitter" << "SR jitter histogram" << "File size" << "RC Buffer" << "TS Buffer";
         int state = MyHub.GetStatistics(i)->GetState();
         switch(state)
         {
            case 0:
               StatTable->setPixmap(r,1,PixmapDisconnected);
               break;
            case 1:
               StatTable->setPixmap(r,1,PixmapInProgress);
               break;
            case 2:
               StatTable->setPixmap(r,1,PixmapConnected);
               break;
            default:
               StatTable->setPixmap(r,1,PixmapConnectedSRE);
               break;
         }

         StatTable->setText(r,2,QString("%1").arg(MyHub.GetStatistics(i)->GetNumPktRcv()));
         StatTable->setText(r,3,QString("%1").arg(MyHub.GetStatistics(i)->GetNumPktLost()));
         StatTable->setText(r,4,QString("%1").arg(MyHub.GetStatistics(i)->GetNumDisconnections()));
         StatTable->setText(r,5,QString("%1").arg(MyHub.GetStatistics(i)->GetNumConnections()));
         StatTable->setText(r,6,QString("%1").arg(MyHub.GetStatistics(i)->GetNumConnectionAttempts()));
         StatTable->setText(r,7,QString("%1").arg(MyHub.GetStatistics(i)->GetSampleRate()));
         //StatTable->setText(r,8,QString("%1").arg(MyHub.GetStatistics(i)->GetJitter()));
         StatTable->setText(r,8,QString("%1").arg(MyHub.GetStatistics(i)->GetReportedSampleRate()));
         // Histogram, converted to icon.
         std::vector<double> hist = MyHub.GetStatistics(i)->GetJitterHistogram(100);
         StatTable->setPixmap(r,9,Hist2Pixmap(hist));
         StatTable->setText(r,10,QString("%1").arg(MyHub.GetFileSize(i)));
      }
   }
   // If merger
   if(type==1)
   {
      StatUpdateCounter1++;
      if(StatUpdateCounter1>=StatUpdatePeriod)
      {
         StatUpdateCounter1=0;

         //return;
         // "Merger" << "State" << "Rcv" << "Est. SR" << "SR jitter" << "SR jitter histogram" << "File size";

         // find the row of the merger
         unsigned r=0;
         for(int j=0;j<i;j++)
            r+=MyHub.IsMerger(j);

         int state = MyHub.GetMergerStatistics(i)->GetState();
         switch(state)
         {
            case 0:
               StatTableM->setPixmap(r,1,PixmapDisconnected);
               break;
            case 1:
               StatTableM->setPixmap(r,1,PixmapInProgress);
               break;
            default:
               StatTableM->setPixmap(r,1,PixmapConnected);
               break;
         }
         StatTableM->setText(r,2,QString("%1").arg(MyHub.GetMergerStatistics(i)->GetNumPktRcv()));
         StatTableM->setText(r,3,QString("%1").arg(MyHub.GetMergerStatistics(i)->GetSampleRate()));
         //StatTableM->setText(r,4,QString("%1").arg(MyHub.GetMergerStatistics(i)->GetJitter()));
         // Histogram, converted to icon.
         std::vector<double> hist = MyHub.GetMergerStatistics(i)->GetJitterHistogram(100);
         StatTableM->setPixmap(r,4,Hist2Pixmap(hist));
         StatTableM->setText(r,5,QString("%1").arg(MyHub.GetMergerFileSize(i)));

         // Get the buffer levels
         for(unsigned d=0;d<DeviceSpecification.size();d++)
         {
            if(i==1)
            {
               //double t1=PreciseTimer::QueryTimer();
               StatTable->setPixmap(d+1,11,*MyHub.GetBufferPixmapRC(d+1));
               //double t2=PreciseTimer::QueryTimer();
               //printf("Get buffer RC: %lf\n",t2-t1);
            }
            if(i==2)
            {
               //double t1=PreciseTimer::QueryTimer();
               StatTable->setPixmap(d+1,12,*MyHub.GetBufferPixmapTS(d+1));
               //double t2=PreciseTimer::QueryTimer();
               //printf("Get buffer TS: %lf\n",t2-t1);
            }
         }
         // For the keyboard
        if(i==1)
           StatTable->setPixmap(0,11,*MyHub.GetBufferPixmapRC(0));
            //StatTable->setPixmap(0,11,*MyHub.GetLabelerBufferPixmapRC());
         if(i==2)
            StatTable->setPixmap(0,12,*MyHub.GetBufferPixmapTS(0));
            //StatTable->setPixmap(0,12,*MyHub.GetLabelerBufferPixmapTS());
      }
   }
   if(type==2)
   {
      StatUpdateCounter2++;
      if(StatUpdateCounter2>=StatUpdatePeriod)
      {
         StatUpdateCounter2=0;

         //return;
         int r=0;
         // Keyboard statistics changed
         int state = MyHub.GetLabelerStatistics()->GetState();
         switch(state)
         {
            case 0:
               StatTable->setPixmap(r,1,PixmapDisconnected);
               break;
            case 1:
               StatTable->setPixmap(r,1,PixmapInProgress);
               break;
            case 2:
               StatTable->setPixmap(r,1,PixmapConnected);
               break;
            default:
               StatTable->setPixmap(r,1,PixmapConnectedSRE);
               break;
         }

         StatTable->setText(r,2,QString("%1").arg(MyHub.GetLabelerStatistics()->GetNumPktRcv()));
         StatTable->setText(r,3,QString("%1").arg(MyHub.GetLabelerStatistics()->GetNumPktLost()));
         StatTable->setText(r,4,QString("%1").arg(MyHub.GetLabelerStatistics()->GetNumDisconnections()));
         StatTable->setText(r,5,QString("%1").arg(MyHub.GetLabelerStatistics()->GetNumConnections()));
         StatTable->setText(r,6,QString("%1").arg(MyHub.GetLabelerStatistics()->GetNumConnectionAttempts()));
         StatTable->setText(r,7,QString("%1").arg(MyHub.GetLabelerStatistics()->GetSampleRate()));
         //StatTable->setText(r,8,QString("%1").arg(MyHub.GetStatistics(i)->GetJitter()));
         StatTable->setText(r,8,QString("%1").arg(MyHub.GetLabelerStatistics()->GetReportedSampleRate()));
         // Histogram, converted to icon.

         //double t1=PreciseTimer::QueryTimer();
         std::vector<double> hist = MyHub.GetLabelerStatistics()->GetJitterHistogram(100);
         //double t2=PreciseTimer::QueryTimer();
         StatTable->setPixmap(r,9,Hist2Pixmap(hist));
         //double t3=PreciseTimer::QueryTimer();
         //printf("Jitter hist: %lf \t%lf\n",t2-t1,t3-t2);


         StatTable->setText(r,10,QString("%1").arg(MyHub.GetLabelerFileSize()));
      }
   }
}


QPixmap MainWindow::Hist2Pixmap(const std::vector<double> &hist)
{


   //double t1 = PreciseTimer::QueryTimer();
   //QImage *p;
   QPainter painter;
   int height;
   int wmult;

   wmult=1;
   height=40;

   QPixmap p(hist.size()*wmult,height);
   //p.fill(Qt::lightGray);
   p.fill(Qt::black);

   painter.begin(&p);
   //painter.setPen(Qt::darkBlue);
   painter.setPen(Qt::green);

   for(unsigned i=0;i<hist.size();i++)
   {
      int h = (int)(hist[i]*height);

      for(int w=0;w<wmult;w++)
         painter.drawLine(i*wmult+w,height-h,i*wmult+w,height-1);

   }

   painter.end();
   //double t2 = PreciseTimer::QueryTimer();

   //printf("hist: %lf\n",t2-t1);
   return p;
}













/******************************************************************************
* Device specification related items
******************************************************************************/

DeviceSpec MainWindow::CheckDSValid(bool &ok,QString &err)
{
   DeviceSpec ds;
   QRegExp regexp("\\s*([\\w^:]+)\\s*:\\s*([\\d^:]+)\\s*");
   regexp.setCaseSensitivity(Qt::CaseInsensitive);
   if(regexp.exactMatch(ui->lineEdit_DSDev->text())==0)
   {
      err="Invalid device specification. It must be of the format: <i>device:speed</i>.\n";
      ok=false;
      return ds;
   }

   QStringList cap = regexp.capturedTexts();
   Q_ASSERT(cap.size()==3);

   ds.devtype="ser";
   ds.dev=cap[1];
   ds.spdport=cap[2];
   printf("CheckDSValid. dev: %s, spd: %s\n",ds.dev.toStdString().c_str(),ds.spdport.toStdString().c_str());

   if(ui->lineEdit_DSFormat->text().size()==0)
   {
      err="Invalid format specification.";
      ok=false;
      return ds;
   }
   ds.format = ui->lineEdit_DSFormat->text();
   if(ui->lineEdit_DSFile->text().size()==0)
   {
      err="Invalid file.";
      ok=false;
      return ds;
   }
   ds.file = ui->lineEdit_DSFile->text();
   if(QDir::isAbsolutePath(ds.file))
   {
      err="Invalid file: only relative path are allowed.";
      ok=false;
      return ds;
   }


   switch(ui->listWidget_DSCounter->currentRow())
   {
      case 0:
         ds.ctrmod=0;
         break;
      case 1:
         ds.ctrmod=256;
         break;
      default:
         ds.ctrmod=65536;
   }
   ds.ctrchannel=ui->spinBox_DSPktChannel->value();
   ds.maxburstregen=ui->spinBox_DSPktRegen->value();


   ok=true;
   return ds;
}
/*
  \brief Clicked on add button. Adds the device specification in the list widget
*/
void MainWindow::on_pushButton_DSAdd_clicked()
{
   QString str;
   DeviceSpec ds;
   bool ok;

   ds=CheckDSValid(ok,str);
   if(!ok)
   {
      str+="\nCan't add this to the device specifications.";
      QMessageBox::information(this,"Error",str);
      return;
   }

   DeviceSpecification.push_back(ds);

   SetDSList();
   ui->listWidget_DSList->setCurrentRow(ui->listWidget_DSList->count()-1);
}

QString MainWindow::CtrSize2String(unsigned v)
{
   switch(v)
   {
      case 0:
         return QString("N");
         break;
      case 256:
         return QString("8");
         break;
      default:
         break;
   }
   return QString("16");
}

/*
  Reflects the content of DeviceSpecification in the device specification list.
*/
void MainWindow::SetDSList()
{
   QString str;
   DeviceSpec ds;

   ui->listWidget_DSList->clear();

   for(unsigned i=0;i<DeviceSpecification.size();i++)
   {
      ds=DeviceSpecification[i];

      QString s=QString(i);
      //str = QString("%1 ").arg(s,-10,QChar(32));//,QChar(' '));
      str = QString("%1 ").arg(i,5,10);//,QChar('x'));
      //nt a, int fieldWidth = 0, int base = 10, const QChar & fillChar = QLatin1Char( ' ' ) ) const

      str += DeviceSpecification2String(ds);
      ui->listWidget_DSList->addItem(str);
   }


}

/*
  Delete the selected element from the device specifications
*/
void MainWindow::on_pushButton_DSDelete_clicked()
{
   DeviceSpecification.erase(DeviceSpecification.begin()+ui->listWidget_DSList->currentRow());
   SetDSList();
}

void MainWindow::on_listWidget_DSList_currentRowChanged(int currentRow)
{
   if(ui->listWidget_DSList->count()==0)
   {
      ui->pushButton_DSDelete->setEnabled(false);
      ui->pushButton_DSModify->setEnabled(false);
      ui->pushButton_DSUp->setEnabled(false);
      ui->pushButton_DSDown->setEnabled(false);
      return;
   }
   if(currentRow==-1)
   {
      ui->pushButton_DSDelete->setEnabled(false);
      ui->pushButton_DSModify->setEnabled(false);
      ui->pushButton_DSUp->setEnabled(false);
      ui->pushButton_DSDown->setEnabled(false);
      return;
   }
   ui->pushButton_DSDelete->setEnabled(true);
   ui->pushButton_DSModify->setEnabled(true);
   ui->pushButton_DSUp->setEnabled(true);
   ui->pushButton_DSDown->setEnabled(true);

   Q_ASSERT(ui->listWidget_DSList->currentRow()<DeviceSpecification.size());

   DeviceSpec ds;
   ds=DeviceSpecification[ui->listWidget_DSList->currentRow()];

   ui->lineEdit_DSDev->setText(ds.dev+":"+ds.spdport);
   ui->lineEdit_DSFormat->setText(ds.format);
   ui->lineEdit_DSFile->setText(ds.file);
   switch(ds.ctrmod)
   {
      case 0:
         ui->listWidget_DSCounter->setCurrentRow(0);
         break;
      case 256:
         ui->listWidget_DSCounter->setCurrentRow(1);
         break;
      default:
         ui->listWidget_DSCounter->setCurrentRow(2);
         break;
   }
   ui->spinBox_DSPktChannel->setValue(ds.ctrchannel);
   ui->spinBox_DSPktRegen->setValue(ds.maxburstregen);
}

/*
   Take the content of the edit fields and replace the current selected entry
*/
void MainWindow::on_pushButton_DSModify_clicked()
{
   int row = ui->listWidget_DSList->currentRow();
   if(row==-1)
      return;
   Q_ASSERT(row<DeviceSpecification.size());
   bool ok;
   QString str;
   DeviceSpec ds=CheckDSValid(ok,str);
   if(!ok)
   {
      str+="\nCan't modify the device specification.";
      QMessageBox::information(this,"Error",str);
      return;
   }
   DeviceSpecification[row]=ds;

   SetDSList();
   ui->listWidget_DSList->setCurrentRow(row);
}

/*
  Move entry up
*/
void MainWindow::on_pushButton_DSUp_clicked()
{
   int row = ui->listWidget_DSList->currentRow();
   if(row==-1)
      return;
   if(row==0)
      return;

   Q_ASSERT(row<DeviceSpecification.size());

   DeviceSpec d = DeviceSpecification[row];
   DeviceSpecification[row] = DeviceSpecification[row-1];
   DeviceSpecification[row-1] = d;

   SetDSList();

   ui->listWidget_DSList->setCurrentRow(row-1);

}

/*
  Move entry down
*/
void MainWindow::on_pushButton_DSDown_clicked()
{
   int row = ui->listWidget_DSList->currentRow();
   if(row==-1)
      return;
   if(row>=ui->listWidget_DSList->count()-1)
      return;

   Q_ASSERT(row<DeviceSpecification.size());

   DeviceSpec d = DeviceSpecification[row];
   DeviceSpecification[row] = DeviceSpecification[row+1];
   DeviceSpecification[row+1] = d;

   SetDSList();

   ui->listWidget_DSList->setCurrentRow(row+1);
}

QString MainWindow::DeviceSpecification2String(DeviceSpec ds)
{
   QString fmt;
   QTextStream stream(&fmt);
   stream << "ser:";
   stream << ds.dev << ":";
   stream << ds.spdport << ":";
   stream << ds.format << ":";
   stream << ds.file << ":";
   stream << CtrSize2String(ds.ctrmod) << ":";
   stream << ds.ctrchannel << ":";
   stream << ds.maxburstregen;
   return fmt;
}

void MainWindow::on_pushButton_DSDefine_clicked()
{
   deviceform *dlg = new deviceform(this);
   QString fmt;
   QTextStream stream(&fmt);

   for(unsigned i=0;i<DeviceSpecification.size();i++)
   {
      stream << "<";
      stream << DeviceSpecification2String(DeviceSpecification[i]);
      stream << ">" << endl;
   }
   //dlg->SetDeviceSpecificationString("<ser:com37:19200:DX3;ccsss-s-s-s:file1><ser:com38:19200:DX3;ccsss-s-s-s:file2>");
   dlg->SetDeviceSpecificationString(fmt);
   int rv = dlg->exec();
   if(rv)
   {
      QString str = dlg->GetDeviceSpecificationString();
      SetDSListFromString(str);
   }

   delete dlg;



}
/*
  Set the device specifications from the parsed string format.
  Example format: <ser:com37:19200:DX3;ccsss-s-s-s:file1:N:0:100><ser:com38:19200:DX3;ccsss-s-s-s:file2:16:0:100>
*/
void MainWindow::SetDSListFromString(QString str)
{
   QString err;


   std::vector<DeviceSpec> ds;
   int rv;
   rv=ParseDeviceString(str,ds,err);
   if(rv!=0)
   {
      QMessageBox::information(this,"Invalid device specification",QString("%1\nError number %2").arg(err).arg(rv));
      return;
   }
   DeviceSpecification = ds;
   SetDSList();



}




/******************************************************************************
* Save/Load configuration
******************************************************************************/





/**
  \brief Loads the program configuration from the specified file
**/
void MainWindow::loadConfiguration(QString fileName)
{
   QFile file(fileName);
   if (file.open(QIODevice::ReadOnly | QIODevice::Text))
   {
      DeviceSpecification.clear();

      QXmlStreamReader xml(&file);
      while (!xml.atEnd())
      {
         QXmlStreamReader::TokenType type = xml.readNext();
         //printf("readNext type: %d\n",type);
         if(type==QXmlStreamReader::StartElement)
         {
            //printf("name: %s\n",xml.name().toString().toStdString().c_str());
            //printf("namespaceUri: %s\n",xml.namespaceUri().toString().toStdString().c_str());
            if(xml.name().toString().compare("Main")==0)
            {
               QXmlStreamAttributes at = xml.attributes();
               QString path = at.value("ProjectPath").toString();
               ui->lineEditWorkingDirectory->setText(path);
               QString en;
               en = at.value("NANFile").toString();
               if(en.compare("1")==0)
                  ui->checkBox_NANFile->setChecked(true);
               else
                  ui->checkBox_NANFile->setChecked(false);
               en = at.value("NANTCP").toString();
               if(en.compare("1")==0)
                  ui->checkBox_NANTCP->setChecked(true);
               else
                  ui->checkBox_NANTCP->setChecked(false);

               bool ok;
               int su = at.value("StatUpdate").toString().toInt(&ok);
               if(ok)
                  ui->spinBox_StatUpdate->setValue(su);
            }
            if(xml.name().toString().compare("Device")==0)
            {
               QXmlStreamAttributes at = xml.attributes();
               DeviceSpec ds;
               bool ok;
               ds.devtype = at.value("DevType").toString();
               ds.dev = at.value("Dev").toString();
               ds.spdport = at.value("SpdPort").toString();
               ds.format = at.value("Format").toString();
               ds.file = at.value("File").toString();
               ds.port = at.value("Port").toString().toInt(&ok);
               ds.ctrmod = at.value("PktMod").toString().toInt(&ok);
               ds.ctrchannel = at.value("PktChan").toString().toInt(&ok);
               ds.maxburstregen = at.value("PktRegen").toString().toInt(&ok);

               DeviceSpecification.push_back(ds);
            }
            if(xml.name().toString().compare("LMerger")==0)
            {
               QXmlStreamAttributes at = xml.attributes();
               ui->lineEdit_Merger_L_File->setText(at.value("File").toString());
               ui->lineEdit_Merger_L_Port->setText(at.value("Port").toString());
            }
            if(xml.name().toString().compare("RCMerger")==0)
            {
               QXmlStreamAttributes at = xml.attributes();
               ui->lineEdit_Merger_RC_File->setText(at.value("File").toString());
               ui->lineEdit_Merger_RC_Port->setText(at.value("Port").toString());
            }
            if(xml.name().toString().compare("TSMerger")==0)
            {
               QXmlStreamAttributes at = xml.attributes();
               ui->lineEdit_Merger_TS_File->setText(at.value("File").toString());
               ui->lineEdit_Merger_TS_Port->setText(at.value("Port").toString());
            }
            if(xml.name().toString().compare("OMerger")==0)
            {
               QXmlStreamAttributes at = xml.attributes();
               ui->lineEdit_Merger_O_File->setText(at.value("File").toString());
               ui->lineEdit_Merger_O_Extension->setText(at.value("Extension").toString());
            }
            if(xml.name().toString().compare("MergerConfiguration")==0)
            {
               QXmlStreamAttributes at = xml.attributes();
               bool ok;
               ui->spinBox_Merger_SampleRate->setValue(at.value("SampleRate").toString().toInt(&ok));
               ui->spinBox_Merger_Latency->setValue(at.value("Latency").toString().toDouble(&ok));
               ui->doubleSpinBox_Merger_Kp->setValue(at.value("Kp").toString().toDouble(&ok));
               ui->doubleSpinBox_Merger_Ki->setValue(at.value("Ki").toString().toDouble(&ok));
               ui->doubleSpinBox_Merger_Kd->setValue(at.value("Kd").toString().toDouble(&ok));
            }
            if(xml.name().toString().compare("Label")==0)
            {
               QXmlStreamAttributes at = xml.attributes();
               QString path = at.value("File").toString();
               ui->lineEdit_Label_File->setText(path);
            }
         }
      }
      file.close();

      // Effect the loaded configuration
      SetDSList();
   }
   else
   {
      QMessageBox::critical(this, "Load configuration error", "Cannot read from file");
   }
}

/**
  \brief Save program configuration
**/
void MainWindow::on_action_Save_configuration_triggered()
{
   QString fileName = QFileDialog::getSaveFileName(this, "Save Configuration",QString(),"Configuration (*.xml)");
   if(!fileName.isNull())
   {
      QFile file(fileName);
      if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
      {
         QXmlStreamWriter xml(&file);
         xml.setAutoFormatting(true);
         xml.writeStartDocument();
         // Start configuration
         xml.writeStartElement("Configuration");

         // Main-related itms
         xml.writeStartElement("Main");
         xml.writeAttribute("ProjectPath",ui->lineEditWorkingDirectory->text());
         xml.writeAttribute("NANFile",ui->checkBox_NANFile->checkState()==Qt::Checked?"1":"0");
         xml.writeAttribute("NANTCP",ui->checkBox_NANTCP->checkState()==Qt::Checked?"1":"0");
         xml.writeAttribute("StatUpdate",QString("%1").arg(ui->spinBox_StatUpdate->value()));
         xml.writeEndElement();

         // Device configuration
         xml.writeStartElement("Devices");
         for(unsigned i=0;i<DeviceSpecification.size();i++)
         {
            QString str;
            str.sprintf("relation%d",i);
            xml.writeStartElement("Device");
            xml.writeAttribute("DevType",DeviceSpecification[i].devtype);
            xml.writeAttribute("Dev",DeviceSpecification[i].dev);
            xml.writeAttribute("SpdPort",DeviceSpecification[i].spdport);
            xml.writeAttribute("Format",DeviceSpecification[i].format);
            xml.writeAttribute("File",DeviceSpecification[i].file);
            xml.writeAttribute("Port",QString("%1").arg(DeviceSpecification[i].port));
            xml.writeAttribute("PktMod",QString("%1").arg(DeviceSpecification[i].ctrmod));
            xml.writeAttribute("PktChan",QString("%1").arg(DeviceSpecification[i].ctrchannel));
            xml.writeAttribute("PktRegen",QString("%1").arg(DeviceSpecification[i].maxburstregen));
            xml.writeEndElement();
         }
         xml.writeEndElement();  // Device

         // L merger
         xml.writeStartElement("LMerger");
         xml.writeAttribute("File",ui->lineEdit_Merger_L_File->text());
         xml.writeAttribute("Port",ui->lineEdit_Merger_L_Port->text());
         xml.writeEndElement();  // L merger

         // RC merger
         xml.writeStartElement("RCMerger");
         xml.writeAttribute("File",ui->lineEdit_Merger_RC_File->text());
         xml.writeAttribute("Port",ui->lineEdit_Merger_RC_Port->text());
         xml.writeEndElement();  // RC merger

         // TS merger
         xml.writeStartElement("TSMerger");
         xml.writeAttribute("File",ui->lineEdit_Merger_TS_File->text());
         xml.writeAttribute("Port",ui->lineEdit_Merger_TS_Port->text());
         xml.writeEndElement();  // TS merger


         // O merger
         xml.writeStartElement("OMerger");
         xml.writeAttribute("File",ui->lineEdit_Merger_O_File->text());
         xml.writeAttribute("Extension",ui->lineEdit_Merger_O_Extension->text());
         xml.writeEndElement();  // O merger

         // Configuration of the mergers
         xml.writeStartElement("MergerConfiguration");
         xml.writeAttribute("SampleRate",QString("%1").arg(ui->spinBox_Merger_SampleRate->value()));
         xml.writeAttribute("Latency",QString("%1").arg(ui->spinBox_Merger_Latency->value()));
         xml.writeAttribute("Kp",QString("%1").arg(ui->doubleSpinBox_Merger_Kp->value()));
         xml.writeAttribute("Ki",QString("%1").arg(ui->doubleSpinBox_Merger_Ki->value()));
         xml.writeAttribute("Kd",QString("%1").arg(ui->doubleSpinBox_Merger_Kd->value()));
         xml.writeEndElement();  // Configuration of the mergers

         // Label
         xml.writeStartElement("Label");
         xml.writeAttribute("File",ui->lineEdit_Label_File->text());
         xml.writeEndElement();  // Label


         xml.writeEndElement();  // Configuration

         xml.writeEndDocument();
         file.close();
      }
      else
      {
         QMessageBox::critical(this, "Save configuration error", "Cannot write to file");
      }
   }
}

/**
   \brief Load program configuration
**/


void MainWindow::on_action_Load_configuration_triggered()
{
   QString fileName = QFileDialog::getOpenFileName(this, "Load Configuration",QString(),"Configuration (*.xml)");
   if(!fileName.isNull())
   {
      QFileInfo fi(fileName);
      loadConfiguration(fileName);
   }
}


/**
  \brief Start/Stop the hub
**/
void MainWindow::on_pushButton_StartStop_clicked()
{
   if(HubStarted==false)
   {
      if(!ApplyConfiguration())
         return;
      StatUpdateCounter0=0;
      StatUpdateCounter1=0;
      StatUpdateCounter2=0;
      StatUpdatePeriod=ui->spinBox_StatUpdate->value();
      MyHub.Start();
      HubStarted=true;
      HubStartTime=QTime();
      HubStartTime.start();
      SBStart->setText(QString("Recording started at ")+HubStartTime.toString());
   }
   else
   {
      MyHub.Stop();
      HubStarted=false;
      SBStart->setText(QString("Recording ended at ")+QTime::currentTime().toString());
      OfflineMerge();
   }
}



void MainWindow::UpdateStatusBar()
{
   //SBDuration->setText((QTime()-HubStartTime).toString());
   //SBDuration->setText(QTime::currentTime().toString());
   SBDuration->setText(QString("Recording duration: %1 s").arg(HubStartTime.elapsed()/1000));

}

void MainWindow::on_action_About_triggered()
{
    QMessageBox::about(this, "About",
   "<p><b>SensHub</b></p>\n"
   "<p>Version 1.01</p>"
   "<p>(c) 2009-2011 Daniel Roggen</p>");
}



void MainWindow::on_pushButton_clicked()
{
   OfflineMerge();
}


void MainWindow::OfflineMerge()
{
   QString fout = ui->lineEdit_Merger_O_File->text().size()?QDir::cleanPath(ui->lineEditWorkingDirectory->text()+"/"+ui->lineEdit_Merger_O_File->text()):"";
   if(fout.size()==0)
      return;

   std::vector<std::vector<std::vector<double> > > alldata;
   std::vector<std::vector<bool> > allvalid;
   std::vector<std::vector<double> > mergedata;
   std::vector<std::vector<bool> > mergevalid;


   std::vector<string> flistin,flistout;
   std::vector<int> modulo,ctrchannel;

   QString err;
   QString errtitle="Offline merge error";
   bool enablenan=true;

   DeviceSpecification2DeviceSpecificationAbs();

   // Build the file list to process
   QString ext = ui->lineEdit_Merger_O_Extension->text();

   // keyboard
   if(GetLabelFileAbs().size())
   {
      flistin.push_back(GetLabelFileAbs().toStdString());
      if(ext.size()==0)
         flistout.push_back("");
      else
      {
         QString f = GetLabelFileAbs()+ext;
         flistout.push_back(f.toStdString());
      }
      modulo.push_back(0);
      ctrchannel.push_back(0);
   }

   // Devices
   for(unsigned i=0;i<DeviceSpecificationAbs.size();i++)
   {
      flistin.push_back(DeviceSpecificationAbs[i].file.toStdString());
      if(ext.size()==0)
         flistout.push_back("");
      else
      {
         QString f = DeviceSpecificationAbs[i].file+ext;
         flistout.push_back(f.toStdString());
      }
      modulo.push_back(DeviceSpecificationAbs[i].ctrmod);
      ctrchannel.push_back(DeviceSpecificationAbs[i].ctrchannel);

   }

   std::vector<std::pair<int,int> > PktStat;

   if(CreateFilledFiles(flistin,flistout,modulo,ctrchannel,enablenan,alldata,allvalid,err,PktStat)<0)
   {
      QMessageBox::information(this,errtitle,err);
      return;
   }
   OfflineMergeData(alldata,allvalid,(double)ui->spinBox_Merger_SampleRate->value(),mergedata,mergevalid);
   if(SaveSignalFile(fout,true,mergedata,mergevalid)<0)
   {
      QMessageBox::information(this,errtitle,err);
      return;
   }

   // Prepare a report about the offline merge activity?

   QString report;
   QTextStream stream(&report);

   printf("Pktstat size: %d\n",PktStat.size());
   stream << "Summary of offline data metge:" << endl;
   for(unsigned i=0;i<PktStat.size();i++)
   {
      printf("i %d\n",i);
      if(GetLabelFileAbs().size())
      {
         if(i==0)
            stream << "Labels" << ": \t Total packets: " << PktStat[i].first << " of which lost: " << PktStat[i].second << endl;
         else
            stream << DeviceSpecification[i-1].file << ": \t Total packets: " << PktStat[i].first << " of which lost: " << PktStat[i].second << endl;
      }
      else
         stream << DeviceSpecification[i].file << ": \t Total packets: " << PktStat[i].first << " of which lost: " << PktStat[i].second << endl;
   }


   QMessageBox::information(this,errtitle,report);



}



/**
  The logic for the PID auto set is the following:
  - The buffer should be allowed to empty updon disconnections / long losses
  - The buffer shit at each read out cycle is d=Rin/Rout-a (a is the PID adjustment factor)
  - When the buffer level lowers, d decreases, corresponding to effectively a lowering of the perceived input frequencies
  - Thus: i) for the buffer to empty, d > 0; to minimize changes in the perceived frequencies, d>alpha*Rin/Rout
  - Typically alpha=0.9, meaning that the frequency changes should not be more than 10%

  Excluding the derivative and integral term, the maximum error occurs when the buffer is 'almost' empty (i.e. error=Nominal)
  Thus:
  - d = Rin/Rout-Nominal*Kp
  - d > alpha * Rin/Rout

  Rin/Rout-Nominal*Kp > alpha * Rin/Rout
  Rin/Rout(1-alpha)/Nominal>Kp

  With Nominal = Latency*Rin

  Kp<(1-alpha)(L*Rout)

  This expression is not valid with the integrator term, that further tends to slow down the buffer emptying

**/
void MainWindow::on_pushButton_PIDAutoSet_clicked()
{
   double alpha = 0.8;
   double Kp,Ki,Kd;

   double L = ui->spinBox_Merger_Latency->value();
   double Rout = ui->spinBox_Merger_SampleRate->value();

   Kp = (1.0-alpha)/(L*Rout);
   Ki = Kp/10.0;
   Kd = 0;

   ui->doubleSpinBox_Merger_Kp->setValue(Kp);
   ui->doubleSpinBox_Merger_Ki->setValue(Ki);
   ui->doubleSpinBox_Merger_Kd->setValue(Kd);


}


/**
  \brief Show a short program usage howto
**/
void MainWindow::on_actionHowto_triggered()
{
   HelpDialog *dlg = new HelpDialog(this);
   dlg->exec();
   delete dlg;
}



