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


#include "Hub.h"



#include <QObject>
#include <QMetaType>
#include <cmath>
#include <stdio.h>
#include <algorithm>
#include <cfloat>

#include "Hub.h"
#include "FBReader.h"
#include "helper.h"






//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// Hub    Hub    Hub    Hub    Hub    ReaderManag
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

Hub::Hub()
{
   qRegisterMetaType< std::vector<int> >("std::vector<int>");
   qRegisterMetaType< std::vector<bool> >("std::vector<bool>");


   Labeler=0;
   LabelFileWriter=0;
   LabelStatisticsReader=0;

}

Hub::~Hub()
{
   Stop();
   Reset();
}



/**
  \brief Starts the hub
**/
void Hub::Start()
{
   for(unsigned i=0;i<MergerStatisticsReaders.size();i++)
   {
      if(MergerStatisticsReaders[i])
         MergerStatisticsReaders[i]->Reset();
   }
   for(unsigned i=0;i<StatisticsReaders.size();i++)
   {
      if(StatisticsReaders[i])
         StatisticsReaders[i]->Reset();
   }
   if(LabelStatisticsReader)
   {
      LabelStatisticsReader->Reset();
   }

   for(unsigned i=0;i<Mergers.size();i++)
   {
      if(Mergers[i])
         Mergers[i]->Start();
   }
   for(unsigned i=0;i<FBInterpreters.size();i++)
   {
      if(FBInterpreters[i])
         FBInterpreters[i]->Start();
   }
   for(unsigned i=0;i<Readers.size();i++)
   {
      if(Readers[i])
         Readers[i]->Start();
   }
   if(Labeler)
      Labeler->Start();
}

/**
  \brief Stop the hub
**/
void Hub::Stop()
{
   if(Labeler)
      Labeler->Stop();
   for(unsigned i=0;i<Readers.size();i++)
   {
      if(Readers[i])
         Readers[i]->Stop();
   }
   for(unsigned i=0;i<FBInterpreters.size();i++)
   {
      if(FBInterpreters[i])
         FBInterpreters[i]->Stop();
   }
   for(unsigned i=0;i<Mergers.size();i++)
   {
      if(Mergers[i])
         Mergers[i]->Stop();
   }

   Reset();

}


/*!
  \brief Rough sanity check of the merge parameters
*/
int Hub::CheckMergeParam(MergeParam m)
{
   printf("Hub::CheckMergeParam %lf %d\n",m.lat,(int)(m.lat<=DBL_EPSILON));
   if(m.rout<=DBL_EPSILON || m.kp<=DBL_EPSILON || m.ki<0 || m.kd<0 || m.lat<=DBL_EPSILON)
      return -1;
   if(m.rout>512)
      return -2;
   if(m.lat>60)
      return -3;
   return 0;
}

/*!
  \brief Initialize the hub.

      DeviceSpecification: single-device input and output parameters
      MergeSpecification: merge output parameters
      MergeParameters: parameters for the merge modules.

*/
bool Hub::Init(NTHubErr &err,QWidget *l,QString lfile,const std::vector<DeviceSpec> &DeviceSpecification, const MergeSpec &MergeSpecification,const MergeParam &MergeParameters,bool enablenanfile,bool enablenantcp)
{
   EnableNanFile = enablenanfile;
   EnableNanTcp = enablenantcp;

   LabelWidget = l;

   printf("Reader manager init.kp, ki, kd: %lf %lf %lf\n",MergeParameters.kp,MergeParameters.ki,MergeParameters.kd);


   Reset();

   // Some conversion to arrays to help
   QStringList mf;         // Merge file names
   mf.push_back(MergeSpecification.lmergerfile);
   mf.push_back(MergeSpecification.rcmergerfile);
   mf.push_back(MergeSpecification.tsmergerfile);
   std::vector<int> mp(3,0);    // Merge ports
   mp[0] = MergeSpecification.lmergerport;
   mp[1] = MergeSpecification.rcmergerport;
   mp[2] = MergeSpecification.tsmergerport;

   // Find the mergers that need to be instanciated: i.e. those for which is requested one of an output file, a port, or signal emission.
   int needm[3]={0,0,0};      // merger n is needed?
   int needanym=0;            // is any merger needed?
   for(int i=0;i<3;i++)
   {
      needm[i] = mf[i].size()!=0 || mp[i]!=0;
      if(MergeSpecification.EmitMerge==i)
         needm[i]++;
      needanym+=needm[i];
   }
   // Init the merger structures
   Mergers.resize(3,0);
   MergerStatisticsReaders.resize(3,0);
   MergerFileWriters.resize(3,0);
   MergerTCPWriters.resize(3,0);

   printf("Need any merger: %d\n",needanym);
   printf("Merger needed: %d %d %d\n",needm[0],needm[1],needm[2]);


   //
   // Check the merger parameters, only if mergers are needed
   //
   if(needanym!=0 && CheckMergeParam(MergeParameters)<0)
   {
      err.ErrType = 5;
      err.ErrSource = CheckMergeParam(MergeParameters);
      return false;
   }

   //
   // Create the mergers
   //
   printf("rout:%lf lat:%lf kp:%lf ki:%lf kd:%lf\n",MergeParameters.rout,MergeParameters.lat,MergeParameters.kp,MergeParameters.ki,MergeParameters.kd);
   if(needm[0])
      Mergers[0] = new LMerger(MergeParameters.rout);
   if(needm[1])
      Mergers[1] = new RCMerger(0,MergeParameters.rout,MergeParameters.lat,MergeParameters.kp,MergeParameters.ki,MergeParameters.kd,0);
   if(needm[2])
      Mergers[2] = new TSMerger(MergeParameters.rout,MergeParameters.lat);

   TextFileWriter *fw;
   TextTCPWriter *tw;
   StatisticsReader *sr;
   // For each existing merger, create the corresponding file writer if it is specified
   for(unsigned m=0;m<3;m++)
   {
      if(Mergers[m]==0)
         continue;
      if(mf[m].size())
      {
         fw = new TextFileWriter(EnableNanFile);
         printf("Init the reader %d with file: %s\n",m,mf[m].toStdString().c_str());
         if(!fw->Init(mf[m]))
         {
            Reset();
            err.ErrType=3;
            err.ErrSource=m;
            return false;
         }
         fw->ConnectFrom(Mergers[m]);
         MergerFileWriters[m]=fw;
      }
   }
   // Create the TCP writers, if specified
   for(unsigned m=0;m<3;m++)
   {
      if(Mergers[m]==0)
         continue;
      if(mp[m]!=0)
      {
         tw = new TextTCPWriter(EnableNanTcp);
         printf("Init the reader %d with tcp port: %d\n",m,mp[m]);
         if(!tw->Init(mp[m]))
         {
            Reset();
            err.ErrType=4;
            err.ErrSource=m;
            return false;
         }
         tw->ConnectFrom(Mergers[m]);
         MergerTCPWriters[m]=tw;
      }
   }
   // Create the statistics, always
   for(unsigned m=0;m<3;m++)
   {
      if(Mergers[m])
      {
         sr = new StatisticsReader();
         sr->ConnectFrom(Mergers[m]);
         connect(sr,SIGNAL(StatisticsChanged()),this,SLOT(InMergerStatisticsChanged()));
         MergerStatisticsReaders[m]=sr;
      }
   }


   //
   // Create the labeler
   //
   Labeler = new LabelReader(LabelWidget);
   // Connect it to the merger
   for(unsigned j=0;j<Mergers.size();j++)
   {
      if(Mergers[j])
         Mergers[j]->AddReader(Labeler);
   }
   // Connect it to a file writer
   if(lfile.size())
   {
      LabelFileWriter = new TextFileWriter(EnableNanFile);
      if(!LabelFileWriter->Init(lfile))
      {
         Reset();
         err.ErrType=6;
         err.ErrSource=0;
         return false;
      }
      LabelFileWriter->ConnectFrom(Labeler);
   }
   // Connect it to a statisticsreader
   LabelStatisticsReader = new StatisticsReader();
   LabelStatisticsReader->ConnectFrom(Labeler);
   connect(LabelStatisticsReader,SIGNAL(StatisticsChanged()),this,SLOT(InStatisticsChanged()));

   //
   // Create the readers
   //

   for(unsigned i=0;i<DeviceSpecification.size();i++)
   {
      FBReader *reader = new FBReader(DeviceSpecification[i].dev.toStdString(),Baud2Baud(DeviceSpecification[i].spdport),DeviceSpecification[i].format.toStdString());
      if(!reader->IsOk())
      {
         Reset();
         err.ErrType=0;
         err.ErrSource=i;
         return false;
      }
      Readers.push_back(reader);

      // Statistics reader
      /*StatisticsReader *sr;
      sr = new StatisticsReader();
      sr->ConnectFrom(reader);
      connect(sr,SIGNAL(StatisticsChanged()),this,SLOT(InStatisticsChanged()));
      StatisticsReaders.push_back(sr);*/

      // File writer
      TextFileWriter *fw;
      fw = new TextFileWriter(EnableNanFile);
      if(!fw->Init(DeviceSpecification[i].file))
      {
         Reset();
         err.ErrType=1;
         err.ErrSource=i;
         return false;
      }
      fw->ConnectFrom(reader);
      FileWriters.push_back(fw);

      // FB interpreter
      FBInterpreter *fbi;
      fbi = new FBInterpreter(DeviceSpecification[i].ctrchannel,DeviceSpecification[i].ctrmod,DeviceSpecification[i].maxburstregen);
      fbi->ConnectFrom(Readers[i]);
      FBInterpreters.push_back(fbi);

      // Statistics reader
      StatisticsReader *sr;
      sr = new StatisticsReader();
      sr->ConnectFrom(fbi);
      connect(sr,SIGNAL(StatisticsChanged()),this,SLOT(InStatisticsChanged()));
      StatisticsReaders.push_back(sr);


      for(unsigned j=0;j<Mergers.size();j++)
      {
         if(Mergers[j])
            Mergers[j]->AddReader(fbi);
      }
   }




   return true;
}
bool Hub::SetMergeParameters(const MergeParam &MergeParameters)
{
   return true;
}

/*!
  \brief Kills all the allocated objects.

*/
void Hub::Reset()
{
   if(Labeler)
   {
      delete Labeler;
      Labeler=0;
   }
   if(LabelFileWriter)
   {
      delete LabelFileWriter;
      LabelFileWriter=0;
   }
   if(LabelStatisticsReader)
   {
      delete LabelStatisticsReader;
      LabelStatisticsReader=0;
   }


   // Device stuff
   for(unsigned i=0;i<StatisticsReaders.size();i++)
   {
      if(StatisticsReaders[i])
         delete StatisticsReaders[i];
   }
   StatisticsReaders.clear();
   for(unsigned  i=0;i<Readers.size();i++)
   {
      if(Readers[i])
         delete Readers[i];
   }
   Readers.clear();
   for(unsigned  i=0;i<FBInterpreters.size();i++)
   {
      if(FBInterpreters[i])
         delete FBInterpreters[i];
   }
   FBInterpreters.clear();
   for(unsigned  i=0;i<FileWriters.size();i++)
   {
      if(FileWriters[i])
         delete FileWriters[i];
   }
   FileWriters.clear();

   // Merger stuff
   for(unsigned  i=0;i<Mergers.size();i++)
   {
      if(Mergers[i])
         delete Mergers[i];
   }
   Mergers.clear();
   for(unsigned  i=0;i<MergerStatisticsReaders.size();i++)
   {
      if(MergerStatisticsReaders[i])
         delete MergerStatisticsReaders[i];
   }
   MergerStatisticsReaders.clear();
   for(unsigned  i=0;i<MergerFileWriters.size();i++)
   {
      if(MergerFileWriters[i])
         delete MergerFileWriters[i];
   }
   MergerFileWriters.clear();
   for(unsigned  i=0;i<MergerTCPWriters.size();i++)
   {
      if(MergerTCPWriters[i])
         delete MergerTCPWriters[i];
   }
   MergerTCPWriters.clear();
}



int Hub::FindSender(QObject *s)
{
   for(unsigned i=0;i<Readers.size();i++)
   {
      if(Readers[i] == s)
         return i;
   }
   return -1;
}

StatisticsReader *Hub::GetStatistics(int reader)
{
   return StatisticsReaders[reader];
}
StatisticsReader *Hub::GetMergerStatistics(int merger)
{
   Q_ASSERT(merger>=0 && merger<=3);
   return MergerStatisticsReaders[merger];
}
StatisticsReader *Hub::GetLabelerStatistics()
{
   return LabelStatisticsReader;
}
qint64 Hub::GetFileSize(int reader)
{
   return FileWriters[reader]->FileSize();
}
qint64 Hub::GetMergerFileSize(int merger)
{
   Q_ASSERT(merger>=0 && merger<=3);
   if(MergerFileWriters[merger])
      return MergerFileWriters[merger]->FileSize();
   return -1;
}
qint64 Hub::GetLabelerFileSize()
{
   if(LabelFileWriter)
      return LabelFileWriter->FileSize();
   return -1;
}
QPixmap *Hub::GetBufferPixmapRC(int s)
{
   if(Mergers[1])
      return Mergers[1]->GetBufferPixmap(s);
  return 0;
}
QPixmap *Hub::GetBufferPixmapTS(int s)
{
   if(Mergers[2])
      return Mergers[2]->GetBufferPixmap(s);
  return 0;
}
QPixmap *Hub::GetLabelerBufferPixmapRC()
{
   if(Mergers[1])
      return Mergers[1]->GetBufferPixmap(0);
  return 0;
}
QPixmap *Hub::GetLabelerBufferPixmapTS()
{
   if(Mergers[2])
      return Mergers[2]->GetBufferPixmap(0);
   return 0;
}
bool Hub::IsMerger(int merger)
{
   if(Mergers[merger])
      return true;
   return false;
}
QString Hub::ErrorString(NTHubErr e)
{
   QString str;

   switch(e.ErrType)
   {
      case 0:
         str+="Device format string is invalid.\n";
         break;
      case 1:
         str+="Device output file creation error.\n";
         break;
      case 2:
         str+="Device output port creation error.\n";
         break;
      case 3:
         str+="Merger output file creation error.\n";
         break;
      case 4:
         str+="Merger output port creation error.\n";
         break;
      case 5:
         str+="Merger parameters error.\n";
         break;
      case 6:
         str+="Label file creation error.\n";
         break;
      default:
         str+="Other configuration error.\n";
         break;
   }
   if(e.ErrType>=0 && e.ErrType<=2)
      str+=QString("Device affected: %1").arg(e.ErrSource);
   if(e.ErrType>=3 && e.ErrType<=5)
      str+=QString("Merger affected: %1").arg(e.ErrSource);
   if(e.ErrSource>5)
      str+=QString("Error source: %1").arg(e.ErrSource);
   return str;
}


/******************************************************************************
*******************************************************************************
SERIALIZED NOTIFICATIONS   SERIALIZED NOTIFICATIONS   SERIALIZED NOTIFICATIONS
*******************************************************************************
******************************************************************************/
/*
  Emits a signal indicating the statistics have changed.
  Differentiates between devices and mergers when emitting the signal StatisticsChanged
*/
void Hub::InStatisticsChanged()
{
   QObject *s=sender();
   for(unsigned i=0;i<StatisticsReaders.size();i++)
   {
      if(StatisticsReaders[i] == s)
      {
         emit StatisticsChanged(0,i);
         return;
      }
   }
   if(s==LabelStatisticsReader)
   {
      emit StatisticsChanged(2,0);      // Statistics for label
   }
}
void Hub::InMergerStatisticsChanged()
{
   QObject *s=sender();
   for(unsigned i=0;i<MergerStatisticsReaders.size();i++)
   {
      if(MergerStatisticsReaders[i]==s)
      {
         emit StatisticsChanged(1,i);
         return;
      }
   }
}
/*!
  \brief Notification of received data
*/
void Hub::InDataReceived(double t,bool valid,std::vector<int> data)
{
   /*int sid = FindSender(sender());

   printf("%d: %lf - ",sid,t);
   for(int i=0;i<data.size();i++)
      printf("%d ",data[i]);
   printf("\n");*/
}
void Hub::InDataReceived(double t,std::vector<bool> valid,std::vector<int> data)
{
}
/*!
  \brief Notification of changed connection state
*/
void Hub::InState(double t,int state)
{
   //int sid = FindSender(sender());
   //printf("%d: %lf - state %d\n",sid,t,state);
}
/*!
  \brief Notification of changed connection state
*/
void Hub::InConnected(double t)
{
   //int sid = FindSender(sender());
   //printf("%d: %lf - connected\n",sid,t);
}
void Hub::InDisconnected(double t)
{
   //int sid = FindSender(sender());
   //printf("%d: %lf - disconnected\n",sid,t);
}
void Hub::InConnectionAttempt(double t)
{
   //int sid = FindSender(sender());
   //printf("%d: %lf - connection attempt\n",sid,t);
}

void Hub::InNotify(double time,int message,int value)
{
   printf("Notification %lf. Message %d=%d\n",time,message,value);
}





