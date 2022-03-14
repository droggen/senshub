#ifndef __MERGER_H
#define __MERGER_H

#include <QMutex>
#include "Base.h"
#include "BaseReader.h"
#include "RCBuffer.h"


/******************************************************************************
*******************************************************************************
Merger   Merger   Merger   Merger   Merger   Merger   Merger   Merger   Merger
*******************************************************************************
******************************************************************************/

class Merger : public BlockInputOutput
{

   protected:
      QMutex mutex;
      std::vector<BlockInputOutput*> Readers;

      virtual int GetActiveReader(QObject *s);
      virtual void ConnectReaders();
      virtual void DisconnectReaders();
      //virtual double SmartSleep(double wakeuptime);

   public:
      Merger();
      virtual ~Merger();

      virtual void Start();
      virtual void Stop();
      virtual void StopThread()=0;

      virtual bool AddReader(BlockInputOutput *input);
      virtual void ClearReaders();
      virtual int GetNumReaders();
};

// Structure holding the parameters of any merger
typedef struct
{
   // General
   double rout;
   // RC-merger
   double rin,kp,ki,kd,tc;
   // TS-merger
   double lat;

} MERGERPARAM;

/******************************************************************************
*******************************************************************************
Templatized merger   Templatized merger   Templatized merger   Templatized merger
*******************************************************************************
******************************************************************************/


class TemplateMerger : public Merger
{
   Q_OBJECT

   friend double SmartSleep(double wakeuptime);

   private:
      double DeltaT;
      virtual void run();
      MergeBuffer *Buffer;
      MERGERPARAM Param;

   protected:
      bool requeststop;

   public:
      TemplateMerger(MergeBuffer *buffer,MERGERPARAM param);
      virtual ~TemplateMerger();

      virtual void Start();
      virtual void Stop();
      virtual void StopThread();
      virtual void ClearReaders();
      virtual bool AddReader(BlockInputOutput *input);

   private slots:
      virtual void InDataLost(double,int) {};
      virtual void InDataReceived(double t,bool valid,std::vector<int> data);
      virtual void InDataReceived(double t,std::vector<bool> valid,std::vector<int> data) {};
      virtual void InState(double t,int state) {};
      virtual void InConnected(double t) {};
      virtual void InDisconnected(double t);
      virtual void InConnectionAttempt(double t) {};
};


/******************************************************************************
*******************************************************************************
Merger factory   Merger factory   Merger factory   Merger factory   Merger factory
*******************************************************************************
******************************************************************************/

TemplateMerger *CreateLMerger(double rout);
TemplateMerger *CreateRCMerger(double rin,double rout,double lat,double kp,double ki,double kd,double tc);
TemplateMerger *CreateTSMerger(double rout,double lat);

#endif // __MERGER_H
