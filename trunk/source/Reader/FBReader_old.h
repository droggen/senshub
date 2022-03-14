/*
	Here we should have a universal reader for frame-based binary data stream coming from serial lines (uart, ttyusb, ttyrfcomm).
*/
#ifndef __GENERICFRAMEBASEDBINARYREADER_H
#define __GENERICFRAMEBASEDBINARYREADER_H


#include <vector>
#include <map>
#include <list>

#include "DThread.h"
#include "serialio.h"
#include "Devices.h"
#include "BaseReader.h"
#include "RateCalculator.h"




using namespace std;
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// GenericFrameBasedBinaryReader   GenericFrameBasedBinaryReader   GenericFrameBasedBinaryReader   
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// 
class GenericFrameBasedBinaryReader : public BaseReader, public DThread
{
	private:
		
		//-----------------------------------------------------------------------
		// Class variables
		//-----------------------------------------------------------------------
		
		RateCalculator  InRateCalc;
		
		bool					ischange;
		bool					available;
		
		SERIALIO_HANDLE fd;
		unsigned 		port;
		unsigned 		DesiredPort;
		unsigned 		opmode;			// opmode: ID or Port
		unsigned int p_ID;				// ID publicly available
		unsigned int DesiredID;			// Desired ID (target) for InitByID
	
	// Generic parameters.
		unsigned FrameSize;
		unsigned NumChannels;
		string FrameFormat;
		int BaudRate;
		int DeviceClass;
	
	
		vector<unsigned> p_data;					// Public, latched/acquired non-thread-accessed data
		

		//-----------------------------------------------------------------------
		// Thread variables
		//-----------------------------------------------------------------------
		vector<unsigned> t_tmpdata;					// Temporary buffer holding NumChannels used for channel parsing
		vector<vector<unsigned> > t_rcdata;			// Buffer for rate control
		unsigned int t_ID;								// ID as decoded by the thread.
		int maxDataSize;									// Rate control variable
		bool bufferFillup;								// Rate control variable
		char *buffer;										// Incoming data buffer
		bool t_ischange;									// indicate whether there has been a change
		bool t_available;									// indicate if the sensor is available

		

		bool OpenPort(unsigned port,bool blocking);
		bool OpenID(int id,bool blocking);
		//static void ProbeByPort();
		//static void ProbeByID();
		void Close();

		int Process();

			
		int t_Run();
		void t_CancelHandler();
		
	public:
		GenericFrameBasedBinaryReader(string Format,int cls,int br);
		virtual ~GenericFrameBasedBinaryReader();
		
		virtual void Start();
		virtual void Stop();
		
		virtual int GetDeviceClass();
		virtual int GetDeviceID();
		
		virtual volatile bool IsChange();
		virtual bool IsAvailable();
		
		virtual int AcquireData(double OutRate);

		virtual void *GetData();				// Data transfer
		virtual unsigned int GetDataSize();

		virtual void WriteDeviceDescription(FILE *pFile);
		virtual void WriteDeviceDescriptionText(FILE *pFile);
		virtual void WriteData(FILE *pFile);
		virtual bool WriteTextData(FILE *pFile);
		virtual bool WriteDataOnConsole();
		
		virtual void WriteBufferData(FILE *pFile);
		
		virtual void InitByPort(unsigned port);
		virtual int ProbePort(int port);
		virtual void InitByID(unsigned id,bool plugandplay=false);
		virtual void InitPlugAndPlay();
};



#endif
