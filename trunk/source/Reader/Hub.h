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


#ifndef __HUB_H
#define __HUB_H

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <vector>
#include <map>
#include <list>

#include "Base.h"
#include "BaseReader.h"
#include "Merger.h"
#include "FBReader.h"
#include "Writer.h"
#include "LabelReader.h"

/*
  Helper to store device sources and file destinations.
*/
typedef struct
{
   // Input specification
   QString devtype,dev,spdport,format;

   // Output specification
   QString file;
   int port;

   int ctrchannel;      // packet counter channel (not used if no counter size)
   int ctrmod;         // packet counter modulo (0: none, 256: 8 bit; 65536: 16-bit)
   int maxburstregen;   // Number of packets that can be regenerated as NANs...
} DeviceSpec;

typedef struct _MergeSpec
{
   _MergeSpec(QString lf,int lp,QString rf,int rp,QString tf,int tp,int te=-1)
   {
      lmergerfile=lf;
      lmergerport=lp;
      rcmergerfile=rf;
      rcmergerport=rp;
      tsmergerfile=tf;
      tsmergerport=tp;
      EmitMerge=te;
   }
   QString lmergerfile,rcmergerfile,tsmergerfile;     // Output files
   int lmergerport,rcmergerport,tsmergerport;         // Output ports
   int EmitMerge;                                     // Emit merged data. 0=no, 1=l, 2=rc, 3=ts.
} MergeSpec;
typedef struct
{
   // L&RC&TS:
   double rout;
   // RC:
   double kp,ki,kd;
   // RC&TS:
   double lat;

} MergeParam;


typedef struct
{
   int ErrType;         // Error type
   int ErrSource;       // number of the device or merger that issued the error, depending on ErrType
   // 0: Device format string invalid
   // 1: Device output file creation error
   // 2: Device output port creation error
   // 3: Merger output file creation error
   // 4: Merger output port creation error
   // 5: Merger parameters
   // 6: Label file creation error
   // 7: other
} NTHubErr;



//-------------------------------------------------------------------------------------------------
// Hub    Hub    Hub    Hub    Hub    ReaderManag
//-------------------------------------------------------------------------------------------------
//
//class Hub : public QObject
class Hub : public BlockInput
{
   Q_OBJECT

   private:
      // We have a variable number of those
      std::vector<BaseReader*> Readers;
      std::vector<FBInterpreter*> FBInterpreters;
      std::vector<StatisticsReader*> StatisticsReaders;
      std::vector<TextFileWriter*> FileWriters;

      // We have 3 of those
      std::vector<Merger*> Mergers;
      std::vector<StatisticsReader*> MergerStatisticsReaders;
      std::vector<TextFileWriter*> MergerFileWriters;
      std::vector<TextTCPWriter*> MergerTCPWriters;

      // Labeler
      LabelReader *Labeler;
      QWidget *LabelWidget;
      TextFileWriter *LabelFileWriter;
      StatisticsReader *LabelStatisticsReader;

      bool EnableNanFile,EnableNanTcp;



      //ConsoleWriter consolewriter;

      int FindSender(QObject *s);


   public:
      Hub();
      virtual ~Hub();


      QString ErrorString(NTHubErr);
      int CheckMergeParam(MergeParam m);
      virtual bool Init(NTHubErr &err,QWidget *l,QString lfile,const std::vector<DeviceSpec> &DeviceSpecification, const MergeSpec &MergeSpecification,const MergeParam &MergeParameters,bool enablenanfile=false,bool enablenantcp=false);
      virtual bool SetMergeParameters(const MergeParam &MergeParameters);
      virtual void Start();
      virtual void Stop();


      virtual StatisticsReader *GetStatistics(int reader);
      virtual StatisticsReader *GetMergerStatistics(int merger);
      virtual StatisticsReader *GetLabelerStatistics();

      virtual qint64 GetFileSize(int reader);
      virtual qint64 GetMergerFileSize(int merger);
      virtual qint64 GetLabelerFileSize();

      virtual bool IsMerger(int merger);

      virtual void Reset();

      virtual QPixmap *GetBufferPixmapRC(int s);
      virtual QPixmap *GetBufferPixmapTS(int s);
      virtual QPixmap *GetLabelerBufferPixmapRC();
      virtual QPixmap *GetLabelerBufferPixmapTS();


   private slots:
      virtual void InDataLost(double,int) {};
      virtual void InDataReceived(double t,bool valid,std::vector<int> data);
      virtual void InDataReceived(double t,std::vector<bool> valid,std::vector<int> data);
      virtual void InState(double t,int state);
      virtual void InConnected(double t);
      virtual void InDisconnected(double t);
      virtual void InConnectionAttempt(double t);
      virtual void InNotify(double time,int message,int value);

      virtual void InStatisticsChanged();
      virtual void InMergerStatisticsChanged();

   signals:
      void StatisticsChanged(int type,int dm);      // type: 0=device, 1=merger. dm: device# or merger#
};



//-------------------------------------------------------------------------------------------------
// BaseData   BaseData   BaseData   BaseData   BaseData   BaseData   BaseData   BaseData   BaseData
//-------------------------------------------------------------------------------------------------
//



#endif // __HUB_H
