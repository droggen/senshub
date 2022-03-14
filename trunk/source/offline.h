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

#ifndef OFFLINE_H
#define OFFLINE_H

#include <vector>
#include <string>
#include <utility>


//std::vector<std::vector<int> > parse(char *data,bool &ok);
std::vector<std::vector<double> > parse(char *data,bool &ok);

//std::vector<std::vector<int> > LoadSignalFile(std::string file,int &rv);
std::vector<std::vector<double> > LoadSignalFile(std::string file,int &rv);
int SaveSignalFile(QString file, bool enablenan, const std::vector<std::vector<double> > &datar, const std::vector<bool> &validr);
int SaveSignalFile(QString file, bool enablenan, const std::vector<std::vector<double> > &datar, const std::vector<std::vector<bool> > &validr);

bool OLSQRegress(std::vector<double> x1,std::vector<double> x2, double &a, double &b);

int OfflineRegenerate(QString file,double sr,int packetmodulo,int packetid,std::vector<std::vector<double> > &datar,std::vector<bool> &validr,int &totpacket,int &pktlost);

int FindClosest(double t,int from,const std::vector<std::vector<double> > &data);


void OfflineMergeData(const std::vector<std::vector<std::vector<double> > > &alldata,const std::vector<std::vector<bool> > &allvalid, double sr,std::vector<std::vector<double> > &mergedata,std::vector<std::vector<bool> > &mergevalid);
int CreateFilledFiles(std::vector<std::string> file,std::vector<std::string> fileout,std::vector<int> modulo,std::vector<int> ctrchannel,bool enablenan,std::vector<std::vector<std::vector<double> > > &data,std::vector<std::vector<bool> > &valid,QString &err,std::vector<std::pair<int,int> > &PktStat);



#endif // OFFLINE_H
