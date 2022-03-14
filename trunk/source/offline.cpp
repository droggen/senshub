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

#include <QFile>
#include <QTextStream>
#include <QProgressDialog>
#include <vector>
#include <cmath>

#include "offline.h"
#include "Reader/FBReader.h"

/**
  \brief Ordinary Least Square Regression

  Finds a and b so that: x2 = b*x1 + a

  Returns false if regression is not possible, true otherwise.
**/
bool OLSQRegress(std::vector<double> x1,std::vector<double> x2, double &a, double &b)
{
   // Mean
   double x1m,x2m;
   x1m=x2m=0;
   for(unsigned i=0;i<x1.size();i++)
   {
      x1m+=x1[i];
      x2m+=x2[i];
   }
   x1m/=(double)x1.size();
   x2m/=(double)x2.size();


   // b^
   b=0;
   double bn,bd;
   bn = 0;
   for(unsigned i=0;i<x1.size();i++)
      bn+=(x1[i]-x1m)*(x2[i]-x2m);
   bd = 0;
   for(unsigned i=0;i<x1.size();i++)
      bd+=pow(x1[i]-x1m,2.0);
   if(bd==0)
      return false;
   b = bn/bd;

   // a^
   a = x2m-b*x1m;
   return true;
}


/**
   \brief parses a null-terminated block of data for space/tab delimited values.

   Parses signed integers. Handles specially 'NaN': replace the value by zero.

**/
/*
std::vector<std::vector<int> > parse(char *data,bool &ok)
{
   // Find the first non
   int idx=0;
   bool gotdata;
   bool first=true;
   ok=false;
   gotdata=false;

   std::vector<std::vector<int> > alldata;
   std::vector<int> linedata;

   idx=-1;
   while(1)    // byte at data[len] is 0, guaranteed by the callee.
   {
      idx++;

      //printf("%d\n",alldata.size());
      // Skip any space
      if(data[idx]==32 || data[idx]=='\t')
         continue;

      //printf("idx: %d data[idx]: %d\n",idx,(int)data[idx]);

      // Non-space: either number, syntax problem, or newline
      if((data[idx]>='0' && data[idx]<='9') || data[idx]=='-'  || data[idx]=='.' || data[idx]=='N' || data[idx]=='a')
      {
         //printf("number start\n");
         // it is a number -> look for end of number
         int idx2=idx;
         while(data[idx2]!=0 && ((data[idx2]>='0' && data[idx2]<='9') || data[idx2]=='.' || data[idx2]=='-' || data[idx2]=='N' || data[idx2]=='a'))
            idx2++;
         //printf("idx: %d idx2: %d\n",idx,idx2);
         //if(idx2>=len)
         //   break;
         char t = data[idx2];    // Keep byte
         data[idx2]=0;           // Null terminate buffer to parse in place
         int v;
         // Check if the value is a NaN
         if(idx2-idx==3 && data[idx+0]=='N' && data[idx+1]=='a' && data[idx+2]=='N')
            v = 0;
         else
            v=atoi(data+idx);
         // If something is a . then should do a double parse...

         data[idx2]=t;           // Restore buffer to initial state
         linedata.push_back(v);
         gotdata=true;
         idx=idx2-1;

         //printf("v: %d \n",v);
         continue;
      }
      if(data[idx]==13 || data[idx]==10 || data[idx]==0)
      {
         //printf("got newline\n");

         // got a newline. If we got data, then we store it in the vector.
         if(gotdata==true)
         {
            //for(int i=0;i<linedata.size();i++)
               //printf("%d: %d\n ",i,linedata[i]);

            if(first)
            {
               alldata.resize(linedata.size());
               first=false;
            }
            if(linedata.size()!=alldata.size())
            {
               //printf("Size mismatch: %d %d at line %d\n",alldata.size(),linedata.size(),alldata[0].size());
               return alldata;
            }
            for(int i=0;i<linedata.size();i++)
            {
               alldata[i].push_back(linedata[i]);
            }
            linedata.clear();
            gotdata=false;
            //if((alldata[0].size()%10000)==0)
            //   printf("%d %d\n",alldata.size(),alldata[0].size());
         }
        // continue;
      }
      if(data[idx]==0)
         break;

      //printf("Invalid file\n");

      //return alldata;
   }
   ok=true;
   return alldata;
}
*/
std::vector<std::vector<double> > parse(char *data,bool &ok)
{
   // Find the first non
   int idx=0;
   bool gotdata;
   bool first=true;
   ok=false;
   gotdata=false;

   std::vector<std::vector<double> > alldata;
   std::vector<double> linedata;

   idx=-1;
   while(1)    // byte at data[len] is 0, guaranteed by the callee.
   {
      idx++;

      //printf("%d\n",alldata.size());
      // Skip any space
      if(data[idx]==32 || data[idx]=='\t')
         continue;

      //printf("idx: %d data[idx]: %d\n",idx,(int)data[idx]);

      // Non-space: either number, syntax problem, or newline
      if((data[idx]>='0' && data[idx]<='9') || data[idx]=='-'  || data[idx]=='.' || data[idx]=='N' || data[idx]=='a')
      {
         //printf("number start\n");
         // it is a number -> look for end of number
         int idx2=idx;
         while(data[idx2]!=0 && ((data[idx2]>='0' && data[idx2]<='9') || data[idx2]=='.' || data[idx2]=='-' || data[idx2]=='N' || data[idx2]=='a'))
            idx2++;
         //printf("idx: %d idx2: %d\n",idx,idx2);
         //if(idx2>=len)
         //   break;
         char t = data[idx2];    // Keep byte
         data[idx2]=0;           // Null terminate buffer to parse in place
         double v;
         // Check if the value is a NaN
         if(idx2-idx==3 && data[idx+0]=='N' && data[idx+1]=='a' && data[idx+2]=='N')
            v = 0;
         else
            v=atof(data+idx);
         // If something is a . then should do a double parse...

         data[idx2]=t;           // Restore buffer to initial state
         linedata.push_back(v);
         gotdata=true;
         idx=idx2-1;

         //printf("v: %d \n",v);
         continue;
      }
      if(data[idx]==13 || data[idx]==10 || data[idx]==0)
      {
         //printf("got newline\n");

         // got a newline. If we got data, then we store it in the vector.
         if(gotdata==true)
         {
            //for(int i=0;i<linedata.size();i++)
               //printf("%d: %d\n ",i,linedata[i]);

            if(first)
            {
               alldata.resize(linedata.size());
               first=false;
            }
            if(linedata.size()!=alldata.size())
            {
               //printf("Size mismatch: %d %d at line %d\n",alldata.size(),linedata.size(),alldata[0].size());
               return alldata;
            }
            for(unsigned i=0;i<linedata.size();i++)
            {
               alldata[i].push_back(linedata[i]);
            }
            linedata.clear();
            gotdata=false;
            /*if((alldata[0].size()%10000)==0)
               printf("%d %d\n",alldata.size(),alldata[0].size());*/
         }
        // continue;
      }
      if(data[idx]==0)
         break;

      //printf("Invalid file\n");

      //return alldata;
   }
   ok=true;
   return alldata;
}




/**
  \brief Load a data source from a file. Return format: data[channel][samples]

  rv: return value.
      0: success
      -1: file not found
      -2: invalid file content

   Parses signed integers space or tab separated. Handles special entries of the form 'NaN' and replaces them by zeroes.
**/
/*std::vector<std::vector<int> > LoadSignalFile(string file,int &rv, bool report)
{
   std::vector<std::vector<int> > data;

   rv=0;

   std::vector<int> d;
   bool first=true;

   QFile filein(QString(file.c_str()));
   int line=0;
   if (filein.open(QIODevice::ReadOnly))
   {
      QByteArray filedata = filein.readAll();
      bool ok;
      data = parse(filedata.data(),ok);
      if(!ok)
      {
         rv=-2;
         return data;
      }

   }
   else
   {
      rv=-1;
      return data;
   }

   return data;
}

*/

/*
std::vector<std::vector<int> > LoadSignalFile(std::string file,int &rv)
{
   std::vector<std::vector<int> > data;

   rv=0;

   std::vector<int> d;
   bool first=true;

   QFile filein(QString(file.c_str()));
   int line=0;
   if (filein.open(QIODevice::ReadOnly))
   {
      QByteArray filedata = filein.readAll();
      printf("Read in %d bytes\n",filedata.size());
      bool ok;
      data = parse(filedata.data(),ok);
      if(!ok)
      {
         rv=-2;
         return data;
      }

   }
   else
   {
      rv=-1;
      return data;
   }

   return data;
}
*/


std::vector<std::vector<double> > LoadSignalFile(std::string file,int &rv)
{
   std::vector<std::vector<double> > data;

   rv=0;


   QFile filein(QString(file.c_str()));
   if (filein.open(QIODevice::ReadOnly))
   {
      QByteArray filedata = filein.readAll();
      //printf("Read in %d bytes\n",filedata.size());
      bool ok;
      data = parse(filedata.data(),ok);
      if(!ok)
      {
         rv=-2;
         return data;
      }

   }
   else
   {
      rv=-1;
      return data;
   }

   return data;
}

/*
  Returns:
      -1    File can't be loaded

*/
int OfflineRegenerate(QString file,double sr,int packetmodulo,int packetid,std::vector<std::vector<double> > &datar,std::vector<bool> &validr,int &totpacket,int &pktlost)
{
   //printf("oregen sr %lf modulo %d channel %d\n",packetmodulo,packetid);

   totpacket=0;
   pktlost=0;


   std::vector<std::vector<double> > data;
   int rv;
   data = LoadSignalFile(file.toStdString(),rv);

   // Load error?
   if(rv!=0)
      return -1;

   // No data?
   if(data.size()==0)
      return -2;

   /*printf("Data.size: %d\n",data.size());
   if(data.size())
      printf("Other dimension: %d\n",data[0].size());*/
/*
   for(int i=0;i<10;i++)
   {
      for(int j=0;j<data.size();j++)
      {
         printf("%lf ",data[j][i]);
      }
      printf("\n");
   }
*/

   // Regenerate the new data
   datar.clear();
   datar.resize(data.size(),std::vector<double>(0,0));
   validr.clear();



   // Copy the first element
   // copy
   for(unsigned j=0;j<data.size();j++)
   {
      datar[j].push_back(data[j][0]);
   }
   validr.push_back(true);

   // Iterate all samples
   printf("Iterate sample\n");
   for(int i=1;i<data[0].size();i++)
   {
      // Check if we have packet lost
      //int FBInterpreter::PktLoss(int ido,int idn,double to,double tn,double sr,int packetmodulo)
      int l = FBInterpreter::PktLoss(data[packetid+1][i-1],data[packetid+1][i],data[0][i-1],data[0][i],sr,packetmodulo);   // packetid+1 because the time isn't a channel
      pktlost+=l;
      if(l)
      {
         printf("Between %d(sample %d) and %d(sample %d) lost %d samples\n",i-1,data[packetid+1][i-1],i,data[packetid+1][i],l);     // packetid+1 because the time isn't a channel
         // Add the missing shit
         for(int li=0;li<l;li++)
         {
            for(int j=0;j<data.size();j++)
            {
               datar[j].push_back(data[j][i-1]);
            }
            validr.push_back(false);
            totpacket++;
         }
      }

      // copy
      for(unsigned j=0;j<data.size();j++)
      {
         datar[j].push_back(data[j][i]);
      }
      validr.push_back(true);
      totpacket++;
   }
   //printf("\n");
   //printf("New size: %d (valid: %d)\n",datar[0].size(),validr.size());
   //for(int i=0;i<10;i++)
   //{
   //   for(int j=0;j<datar.size();j++)
   //   {
   //      printf("%lf ",datar[j][i]);
   //   }
   //   printf("\n");
   //}

   // At this stage we need to do the linear regression....
   // We take only the packets that are valid, and extract their
   std::vector<double> p,t;
   for(unsigned i=0;i<datar[0].size();i++)
   {
      if(validr[i])
      {
         p.push_back(i);
         t.push_back(datar[0][i]);
      }
   }
   //printf("Have %d %d sample points for regression\n",p.size(),t.size());

   // Let's do the shit regression
   double a,b;
   // a: offset
   // b: intersample time
   bool ok=OLSQRegress(p,t,a,b);
   //printf("Regression ok %d a: %lf b: %lf\n",(int)ok,a,b);

   // Touch the samples
   for(unsigned i=0;i<datar[0].size();i++)
   {
      datar[0][i] = a+b*i;
   }

   //printf("touched time\n");
   //for(int i=0;i<10;i++)
   //{
   //   for(int j=0;j<datar.size();j++)
   //   {
   //      printf("%lf ",datar[j][i]);
   //   }
   //   printf("\n");
   //}
   return 0;
}

/**
   Saves a signal back into a file.
   - First column is a double
   - Columns 2..end are integers (rounded to nearest)
   - enablenan: if true, stores a NAN if the signal is not valid
**/
int SaveSignalFile(QString fname, bool enablenan, const std::vector<std::vector<double> > &datar, const std::vector<bool> &validr)
{
   QFile file(fname);
   if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
      return -1;

   // Stream writer...
   QTextStream out(&file);
   out.setRealNumberNotation(QTextStream::FixedNotation);

   for(int i=0;i<datar[0].size();i++)     // Samples
   {
      for(int j=0;j<datar.size();j++)     // Channels
      {
         if(j==0)
            out.setRealNumberPrecision(12);

         if(enablenan && !validr[i])
            out << "NaN ";
         else
            out << datar[j][i] << " ";

         if(j==0)
            out.setRealNumberPrecision(0);
      }
      out << endl;
   }
  file.close();

   return 0;
}
int SaveSignalFile(QString fname, bool enablenan, const std::vector<std::vector<double> > &datar, const std::vector<std::vector<bool> > &validr)
{
   QFile file(fname);
   if(!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
      return -1;

   // Stream writer...
   QTextStream out(&file);
   out.setRealNumberNotation(QTextStream::FixedNotation);

   for(int i=0;i<datar[0].size();i++)     // Samples
   {
      for(int j=0;j<datar.size();j++)     // Channels
      {
         if(j==0)
            out.setRealNumberPrecision(12);

         if(enablenan && !validr[j][i])
            out << "NaN ";
         else
            out << datar[j][i] << " ";

         if(j==0)
            out.setRealNumberPrecision(0);
      }
      out << endl;
   }
   file.close();

   return 0;
}


/**
   Finds the closest sample within data to time t.
   Returns the -1 if time t is below/above the times covered by data.
   Returns the index of the closes sample if found.

   Start searching from index from.
**/
int FindClosest(double t,int from,const std::vector<std::vector<double> > &data)
{
   double mint,maxt;
   unsigned numsamples;
   unsigned numchannels;

   numsamples = data[0].size();
   numchannels = data.size();
   mint=data[0][0];
   maxt=data[0][numsamples-1];

   // Time is outside of the range covered by the dataset
   if(t<mint || t>maxt)
      return -1;


   // Here we should implement a smart search... currently not.
   for(unsigned i=from;i<numsamples;i++)
   {
      // Find the last sample of data, for which data_i.time is smaller than t (i.e. data_i+1.time is larger than t)
      if(data[0][i]>=t)
      {
         // The match was at i-1; if i==0 then we return the 1st one (this should not happen)
         unsigned fndidx;
         if(i>0)
            fndidx=i;
         else
            fndidx=0;
         // Return this
         return fndidx;
      }
   }
   // We never found a sample -> nothing found.
   return -1;
}



/**
  alldata[dataset][channel][sample]
  allvalid[dataset][sample]
  mergedata[channel][sample]
  mergevalid[channel][sample]
**/
void OfflineMergeData(const std::vector<std::vector<std::vector<double> > > &alldata,const std::vector<std::vector<bool> > &allvalid, double sr,std::vector<std::vector<double> > &mergedata,std::vector<std::vector<bool> > &mergevalid)
{
   //printf("CreateFilledFiles\n");
   //printf("Alldata dimension: %d\n",alldata.size());
   //for(int i=0;i<alldata.size();i++)
   //   printf("   x %d\n",alldata[i].size());
   //printf("Allvalid dimension: %d\n",allvalid.size());
   //for(int i=0;i<allvalid.size();i++)
   //   printf("   x %d\n",allvalid[i].size());

   // Find the minimum/maximum sample time

   double mint,maxt;
   bool first=true;

   // Iterate through all the entries
   for(int d=0;d<alldata.size();d++)
   {
      if(first)
      {
         mint = alldata[d][0][0];
         maxt = alldata[d][0][alldata[d][0].size()-1];
         first=false;
      }
      else
      {
         mint = min(mint,alldata[d][0][0]);
         maxt = max(maxt,alldata[d][0][alldata[d][0].size()-1]);
      }
   }


   // Prepare resampled dataset

   // Find total number of channels.
   int totchannels=0;
   for(int d=0;d<alldata.size();d++)
      totchannels+=alldata[d].size()-1;                           // Subtract 1 to get rid of the time channel of each dataset.

   mergedata.resize(totchannels+1,std::vector<double>(0,0));      // Add 1 for the new merged time channel
   mergevalid.resize(totchannels+1,std::vector<bool>(0,false));

   // Now do the resample
   double dt = 1.0/sr;
   for(double t=mint;t<=maxt;t+=dt)
   {
      // Store the time.
      mergedata[0].push_back(t);
      mergevalid[0].push_back(true);
   }

   unsigned channeloffset=1;
   for(unsigned d=0;d<alldata.size();d++)
   {
      unsigned numchannels = alldata[d].size();
      int lastfnd=0;
      for(double t=mint;t<=maxt;t+=dt)
      {
         int fnd = FindClosest(t,lastfnd,alldata[d]);
         if(fnd!=-1)
            lastfnd=fnd;

         //printf("dataset %d @ %lf->%d. Numchan: %d\n",d,t,fnd,numchannels);
         for(unsigned channel=0;channel<numchannels-1;channel++)    // Skip time channel
         {
            //printf("channel: %d channeloffset+channel: %d numchannels: %d\n",channel,channeloffset+channel,numchannels);
            //printf("mergevalid %d mergedata %d\n",mergevalid.size(),mergedata.size());
            if(fnd==-1)       // Not found -> invalid
            {
               mergevalid[channeloffset+channel].push_back(false);
               mergedata[channeloffset+channel].push_back(0);          // we could repeat the last data, alternatively
            }
            else              // Found -> copy the valid flag of the source data.
            {
               //printf("found... valid\n");
               //printf("allvalid: %d\n",allvalid.size());
               //printf("allvalid[0].size: %d\n",allvalid[0].size());
               //printf("read valid %d %d\n",d,fnd);
               bool v = allvalid[d][fnd];
               //printf("push back valid %d\n",channeloffset+channel);
               mergevalid[channeloffset+channel].push_back(v);
               //printf("found... data\n");
               //printf("Read data %d %d %d\n",d,channel,fnd);
               double x=alldata[d][channel+1][fnd];                  // Skip the time channel
               //printf("push back data %d\n",channeloffset+channel);
               mergedata[channeloffset+channel].push_back(x);
            }
         }
      }
      channeloffset+=numchannels-1;    // Skip time channel
   }

   /*
   // Dump the data
   printf("mergedata dimension: %d\n",mergedata.size());
   for(int i=0;i<mergedata.size();i++)
      printf("   x %d\n",mergedata[i].size());
   printf("mergevalid dimension: %d\n",mergevalid.size());
   for(int i=0;i<mergevalid.size();i++)
      printf("   x %d\n",mergevalid[i].size());
*/
}


// Create the filled files (fills in missing packets).
// Returns the complete dataset
// data[file][channel][sample]
// valid[file][sample]
int CreateFilledFiles(std::vector<std::string> file,std::vector<std::string> fileout,std::vector<int> modulo,std::vector<int> ctrchannel,bool enablenan,std::vector<std::vector<std::vector<double> > > &data,std::vector<std::vector<bool> > &valid,QString &err,std::vector<std::pair<int,int> > &PktStat)
{
   data.clear();
   valid.clear();

   QProgressDialog progress("Regenerate missing data","Abort",0,file.size()-1);
   progress.show();
   progress.setWindowModality(Qt::WindowModal);
   progress.setValue(0);
   // Iterate through all the entries
   for(int d=0;d<file.size();d++)
   {
      // Load data and do something
      std::vector<std::vector<double> > datar;
      std::vector<bool> validr;

      //printf("Regenerating %s\n",file[d].c_str());
      //printf("Paramters: modulo: %d, channel: %d\n",modulo[d],ctrchannel[d]);
      int totpacket,pktlost;
      int rv = OfflineRegenerate(QString(file[d].c_str()),0,modulo[d],ctrchannel[d],datar,validr,totpacket,pktlost);
      if(rv==-1)
      {
         err=QString("Can't open data file '%1'").arg(QString(file[d].c_str()));
         return -1;
      }
      if(rv==-2)
      {
         err=QString("No data in file '%1'").arg(QString(file[d].c_str()));
         return -1;
      }
      //printf("Offline regenerate: %d. totpacket: %d, lostpacket: %d\n",rv,totpacket,pktlost);
      //printf("Regenerated data: datar.size: %d validr.size: %d\n",datar.size(),validr.size());

      // Here we should save the individual file
      if(fileout[d].size())
      {
         SaveSignalFile(QString(fileout[d].c_str()),enablenan,datar,validr);
      }

      // Store data.
      valid.push_back(validr);
      data.push_back(datar);


      std::pair<int,int> p(totpacket,pktlost);
      PktStat.push_back(p);

      progress.setValue(d);
   }
   err="";
   return 0;
}
















