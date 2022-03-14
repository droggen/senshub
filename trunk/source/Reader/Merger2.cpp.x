#include "Merger2.h"
#include "precisetimer.h"



/*
  Sleeps until the wakeuptime, using a combination of thread sleep for longer sleep time, and busy wait to achieve accurate wake up time.
*/
double SmartSleep(double wakeuptime)
{
   double t;
   double tnow = PreciseTimer::QueryTimer();
   double sleeplen;
   int nms;
   int busywaitwarn=4;

   sleeplen=wakeuptime-tnow;

   if(sleeplen<0)
      return tnow;

   // find number of milliseconds
   nms = (int)(sleeplen/0.001);
   // Go in busy wait mode before...
   nms-=busywaitwarn;

   // Thread sleep
   if(nms>=1)
      QThread::msleep(nms);

   while( (t=PreciseTimer::QueryTimer()) < wakeuptime );
   return t;
}


/******************************************************************************
*******************************************************************************
Merger   Merger   Merger   Merger   Merger   Merger   Merger   Merger   Merger
*******************************************************************************
******************************************************************************/
Merger::Merger()
{
}
Merger::~Merger()
{
}
bool Merger::AddReader(BlockInputOutput *input)
{
   Readers.push_back(input);
   return true;
}
void Merger::ConnectReaders()
{
   for(unsigned i=0;i<Readers.size();i++)
      ConnectFrom(Readers[i]);
}
void Merger::DisconnectReaders()
{
   for(unsigned i=0;i<Readers.size();i++)
      DisconnectFrom(Readers[i]);
}
void Merger::ClearReaders()
{
   Stop();
   Readers.clear();
}
int Merger::GetActiveReader(QObject *s)
{
   for(unsigned i=0;i<Readers.size();i++)
   {
      if(Readers[i] == s)
         return i;
   }
   return -1;
}
int Merger::GetNumReaders()
{
   return Readers.size();
}
void Merger::Start()
{

}
void Merger::Stop()
{

}

/******************************************************************************
*******************************************************************************
TemplateMerger   TemplateMerger   TemplateMerger   TemplateMerger   TemplateMerger
*******************************************************************************
******************************************************************************/

TemplateMerger::TemplateMerger(MergeBuffer *buffer,MERGERPARAM p)
{
   Param = p;
   DeltaT=1.0/Param.rout;
   requeststop=false;
   Buffer=buffer;
}
TemplateMerger::~TemplateMerger()
{
   StopThread();
   delete Buffer;
}

void TemplateMerger::Start()
{
   Buffer->InitBuffer(GetNumReaders());
   ConnectReaders();
   start();
}

void TemplateMerger::Stop()
{
   DisconnectReaders();
   StopThread();
}
void TemplateMerger::StopThread()
{
   requeststop=true;
   wait();
}

bool TemplateMerger::AddReader(BlockInputOutput *reader)
{
   Merger::AddReader(reader);
   return true;
}


void TemplateMerger::ClearReaders()
{
   Merger::ClearReaders();
}

/*!
   \brief Regular sampling
*/
void TemplateMerger::run()
{
   double tstart = PreciseTimer::QueryTimer();
   double t;
   int i=0;

   std::vector<bool> valid;
   std::vector<int> data;

   while(!requeststop)
   {
      // Wait until time elapsed -  busy loop, not nice.
      double twakeup = tstart+i*DeltaT;
      t=SmartSleep(twakeup);

      QMutexLocker locker(&mutex);
      locker.relock();

      bool ok=Buffer->GetMergedData(twakeup,valid,data);

      locker.unlock();

      if(ok)
         emit OutDataReceived(t,valid,data);
      i++;
   }
}

/*
   Keep the last data in the internal buffers
*/
void TemplateMerger::InDataReceived(double t,bool valid,std::vector<int> data)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)       // Data received from a known reader (upon disconnection old packets may still arrive)
   {
      QMutexLocker locker(&mutex);
      Buffer->AddData(sid,t,valid,data);
   }
}
void TemplateMerger::InDisconnected(double t)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)
   {
      QMutexLocker locker(&mutex);
      Buffer->NotifyDisconnected(sid);
   }
}

/******************************************************************************
*******************************************************************************
Merger factory   Merger factory   Merger factory   Merger factory   Merger factory
*******************************************************************************
******************************************************************************/

TemplateMerger *CreateLMerger(double rout)
{
   MERGERPARAM p;
   p.rout=rout;
   MergeBuffer *b = new LBuffer();
   TemplateMerger *t = new TemplateMerger(b,p);
   return t;
}
TemplateMerger *CreateRCMerger(double rin,double rout,double lat,double kp,double ki,double kd,double tc)
{
   MERGERPARAM p;
   p.rout=rout;
   p.rin=rin;
   p.lat=lat;
   p.kp=kp;
   p.ki=ki;
   p.kd=kd;
   p.tc=tc;
   MergeBuffer *b = new RCBufferMulti(rin,rout,lat,kp,ki,kd,tc);
   TemplateMerger *t = new TemplateMerger(b,p);
   return t;
}
TemplateMerger *CreateTSMerger(double rout,double lat)
{
   MERGERPARAM p;
   p.rout=rout;
   p.lat=lat;
   MergeBuffer *b = new TSBufferMulti(rout,lat);
   TemplateMerger *t = new TemplateMerger(b,p);
   return t;
}



/******************************************************************************
*******************************************************************************
RCMerger   RCMerger   RCMerger   RCMerger   RCMerger   RCMerger   RCMerger
*******************************************************************************
******************************************************************************/

/*

RCMerger::RCMerger(double rin,double rout,double lat,double kp,double ki,double kd,double tc)
      : Buffer(rin,rout,lat,kp,ki,kd,tc)
{
   printf("RCMerger::RCMerger\n");
   SetPIDParameters(kp,ki,kd,tc);
   Rout = rout;
   requeststop=false;
   DeltaT=1.0/(double)Rout;
}
RCMerger::~RCMerger()
{
}

void RCMerger::SetPIDParameters(double kp,double ki,double kd,double tc)
{
   Buffer.SetPIDParameters(kp,ki,kd,tc);
}
void RCMerger::Start()
{
   printf("RCMerger Starting\n");
   Buffer.InitBuffer(GetNumReaders());
   Merger::Start();
   start();
};

void RCMerger::Stop()
{
   printf("RCMerger Stopping\n");
   // Set some variable - careful here....
   requeststop=true;
   wait();
   Merger::Stop();
   printf("RCMerger Stopped\n");
}


void RCMerger::run()
{
   double tstart = PreciseTimer::QueryTimer();
   double t;
   int i=0;

   std::vector<int> data;
   std::vector<bool> valid;

   while(!requeststop)
   {
      // Wait until time elapsed -  busy loop, not nice.
      t=SmartSleep(tstart+i*DeltaT);


      QMutexLocker locker(&mutex);
      locker.relock();

      bool ok = Buffer.GetMergedData(valid,data);

      locker.unlock();

      // Send data.
      if(ok)
         emit OutDataReceived(t,valid,data);
      i++;
   }
}

double RCMerger::BufferLevel(int reader)
{
   return Buffer.BufferLevel(reader);
}
void RCMerger::GetBufferPixmap(int reader,QPixmap &pixmap)
{
   QMutexLocker locker(&mutex);
   Buffer.GetBufferPixmap(reader,pixmap);
}



void RCMerger::InDataReceived(double t,bool valid,std::vector<int> data)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)       // Data received from a known reader (upon disconnection old packets may still arrive)
   {
      QMutexLocker locker(&mutex);
      Buffer.AddData(sid,t,valid,data);
   }
}

void RCMerger::InDisconnected(double t)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)       // Data received from a known reader (upon disconnection old packets may still arrive)
   {
      QMutexLocker locker(&mutex);
      Buffer.NotifyDisconnected(sid);
   }
}
// Data lost (willingly not replaced by NAN) must be notified to the buffer to reset its logic (long disconnection)
void RCMerger::InDataLost(double,int)
{
   int sid = GetActiveReader(sender());
   if(sid!=-1)       // Data received from a known reader (upon disconnection old packets may still arrive)
   {
      QMutexLocker locker(&mutex);
      Buffer.NotifyDisconnected(sid);
   }
}

*/
/******************************************************************************
*******************************************************************************
TSMerger   TSMerger   TSMerger   TSMerger   TSMerger   TSMerger   TSMerger
*******************************************************************************
******************************************************************************/

/*

TSMerger::TSMerger(double rout,double lat)
      : Buffer(rout,lat)
{
   printf("TSMerger::TSMerger");
   Rout = rout;
   requeststop=false;
   DeltaT=1.0/(double)Rout;

}
TSMerger::~TSMerger()
{
}


void TSMerger::Start()
{
   printf("TSMerger Starting\n");
   Buffer.InitBuffer(GetNumReaders());
   Merger::Start();
   start();
};

void TSMerger::Stop()
{
   printf("TSMerger Stopping\n");
   // Set some variable - careful here....
   requeststop=true;
   wait();
   Merger::Stop();
   printf("TSMerger Stopped\n");
}


//
//   \brief Regular sampling
//
void TSMerger::run()
{
   double tstart = PreciseTimer::QueryTimer();
   double t;
   int i=0;
   while(!requeststop)
   {
      // Wait until time elapsed -  busy loop, not nice.
      t=SmartSleep(tstart+i*DeltaT);


      QMutexLocker locker(&mutex);
      locker.relock();

      std::vector<int> data;
      std::vector<bool> valid;

      bool ok = Buffer.GetMergedData(t,valid,data);

      locker.unlock();

      // Send data.
      if(ok) emit OutDataReceived(t,true,data);
      i++;
   }
}

double TSMerger::BufferLevel(int reader)
{
   return Buffer.BufferLevel(reader);
}

void TSMerger::InDataReceived(double t,bool valid,std::vector<int> data)
{
   QMutexLocker locker(&mutex);
   Buffer.AddData(t,valid,data);
}

void TSMerger::InDisconnected(double t)
{
   QMutexLocker locker(&mutex);
   Buffer.NotifyDisconnected();
}
// Data lots (willingly not replaced by NAN) must be notified to the buffer to reset its logic (long disconnection)
void TSMerger::InDataLost(double,int)
{
   QMutexLocker locker(&mutex);
   Buffer.NotifyDisconnected();
}
void TSMerger::GetBufferPixmap(QPixmap &pixmap)
{
   QMutexLocker locker(&mutex);
   Buffer.GetBufferPixmap(pixmap);
}
*/
