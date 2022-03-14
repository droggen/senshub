/*
	Here we should have a universal reader for frame-based binary data stream coming from serial lines (uart, ttyusb, ttyrfcomm).
*/
/*
	For generic use we need:
		- data format
		- input device (i.e. port now, but in future a generic stream)
		- device class

	Ideally opening the device would follow this syntax:
		USB:<port>
		RFCOMM:<port>
		FILE:<filename>
		USB¦RFCOMM:*
		USB¦RFCOMM:ID=<id>
*/

#include <stdio.h>
#include <assert.h>
#include "FrameParser.h"
#include "GenericFrameBasedBinaryReader.h"

#include "precisetimer.h"

#define DELAY_TIME	1


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// GenericFrameBasedBinaryReader   GenericFrameBasedBinaryReader   GenericFrameBasedBinaryReader
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// 
/*
	Taxonomy:
*/

GenericFrameBasedBinaryReader::GenericFrameBasedBinaryReader(string Format,int cls,int br)
	: InRateCalc(64, 128)	//: InRateCalc( startInRate, buflength)
	, DThread()
{
	// Rate control
	FactorCount			= 0;
	outrate 			= 1;
	nSoll				= InRateCalc.GetRate()/4;
	maxDataSize 	= 128;
	bufferFillup = true;
	
	// Other stuff
	fd = 0;
	port = 0;
	opmode = SERIAL_MODE_PORT;
	t_ischange=ischange = false;
	t_available=available = false;

	FrameFormat = Format;
	DeviceClass = cls;
	BaudRate = br;
	if(FrameParser_ValidateFormat(Format.c_str())!=FRAMEPARSER_NOERROR)
	{
		assert(0);
		printf("GenericFrameBasedBinaryReader: Invalid format\n");
		exit(1);
	}
	FrameSize=FrameParser_GetFrameSize(Format.c_str());
	NumChannels=FrameParser_GetNumChannels(Format.c_str());
	
	// Memory initializations
	buffer = new char[FrameSize];
	t_tmpdata.resize(NumChannels,0);
	p_data.resize(NumChannels,0);
}
GenericFrameBasedBinaryReader::~GenericFrameBasedBinaryReader()
{
	printf("GenericFrameBasedBinaryReader destructor - Cancelling and joining thread");
	m_Cancel();
	int rv=m_Join();
	printf("GenericFrameBasedBinaryReader destructor - Joined thread returned %d\n",rv);
	delete[] buffer;
}
/*-------------------------------------------------------------------------------------------------
	::t_CancelHandler   ::t_CancelHandler   ::t_CancelHandler   ::t_CancelHandler   
---------------------------------------------------------------------------------------------------
Called when the thread is cancelled. Frees thread allocated resources
-------------------------------------------------------------------------------------------------*/
void GenericFrameBasedBinaryReader::t_CancelHandler()
{
	cout << "GenericFrameBasedBinaryReader::t_CancelHandler\n";
	Close();
}
/*-------------------------------------------------------------------------------------------------
::Start				::Start  			::Start   	   
---------------------------------------------------------------------------------------------------
// Start reading thread
-------------------------------------------------------------------------------------------------*/
void GenericFrameBasedBinaryReader::Start()
{
	// Initialize t_data;
	 
	printf("GenericFrameBasedBinaryReader Start. opmode: %d\n",opmode);
	m_Start();
}
/*-------------------------------------------------------------------------------------------------
::Stop				::Stop  			::Stop   	   
---------------------------------------------------------------------------------------------------
// Stop reading thread
-------------------------------------------------------------------------------------------------*/
void GenericFrameBasedBinaryReader::Stop()
{
	// Close is called by the cancellation handler
	m_Cancel();
	int rv=m_Join();
	printf("DX3Reader Stop - Joined thread returned %d\n",rv);
}
/*-------------------------------------------------------------------------------------------------
::GetData		   	::GetData	  		::GetData   	   
---------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------*/
void *GenericFrameBasedBinaryReader::GetData()
{
	return (void*)&p_data;
}
/*-------------------------------------------------------------------------------------------------
::GetDataSize   	::GetDataSize  		::GetDataSize      
---------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------*/
unsigned int GenericFrameBasedBinaryReader::GetDataSize()
{
	// tofix
	// Depending on the not yet defined implementation of the data storage, the size of the payload
	// wouldn't be the sizeof of the structure when a memory pointer (or a vector) would be used.
	//return sizeof(DAReaderData); 
	return NumChannels;
}


/*-------------------------------------------------------------------------------------------------
::GetDeviceClass		::GetDeviceClass  		::GetDeviceClass   	   
---------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------*/
int GenericFrameBasedBinaryReader::GetDeviceClass()
{
	return DeviceClass;
}
/*-------------------------------------------------------------------------------------------------
::GetDeviceID		::GetDeviceID  		::GetDeviceID   	   
---------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------*/
int GenericFrameBasedBinaryReader::GetDeviceID()
{
	return p_ID;		// Should return hardware ID
}

/*-------------------------------------------------------------------------------------------------
::IsChange   ::IsChange   ::IsChange    
---------------------------------------------------------------------------------------------------
Is there a change since the last call to the function? IsChange is synchronous
-------------------------------------------------------------------------------------------------*/
volatile bool GenericFrameBasedBinaryReader::IsChange()
{
	bool b = ischange;
	//if (ischange)
		//printf("GenericFrameBasedBinaryReader Detected a change %d\n",p_data.ID);
	ischange = false;
	return b;
}

/*-------------------------------------------------------------------------------------------------
::IsAvailable		::IsAvailable  		::IsAvailable   	   
---------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------*/
bool GenericFrameBasedBinaryReader::IsAvailable()
{
	return available;
}


/*-------------------------------------------------------------------------------------------------
::AcquireData		::AcquireData  		::AcquireData   	   
---------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------*/
int GenericFrameBasedBinaryReader::AcquireData(double OutRate)
{
	outrate = OutRate;
	//printf("Acquire Data of ReadThread(%p). port: %d    available %d \n",t_data.tid,t_data.port,t_data.available);

/*
	SDL_mutexP(mutex);
	if(!t_data.data.empty())
	{
		memcpy(&data,&(t_data.data[0]),sizeof(DAReaderData));
	
		ischange = ischange|t_data.ischange;
		t_data.ischange=false;
		available=t_data.available;
	}
	else
		available=false;
	SDL_mutexV(mutex);
*/

	if(!t_rcdata.empty())
	{
		// Lock the mutex
		s_Lock();
		
		while(FactorCount < 1)						// running variable sum of getFactor and rest
		{
			if(t_rcdata.size()>1)
				t_rcdata.erase( t_rcdata.begin() );

			double InRate = InRateCalc.GetRate();
			double getFactor = (OutRate / InRate);
			FactorCount += getFactor;
		}
		
		//memcpy(&p_data,&(t_data.t_rcdata[0]),sizeof(DAReaderData));	// old
		p_data = t_rcdata[0];
		FactorCount = FactorCount-1;
		FactorCount += (nSoll-t_rcdata.size())*0.03;		// I-Regler
		
		ischange = ischange|t_ischange;
		t_ischange = false;
		available = t_available;
		//printf("ischange: %d\n",ischange);
		
		// Unlock the mutex
		s_Unlock();
	}
	else
		available=false;	
	
	return 0;
}
/*-------------------------------------------------------------------------------------------------
::WriteDeviceDescription   ::WriteDeviceDescription   
---------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------*/
void GenericFrameBasedBinaryReader::WriteDeviceDescription(FILE *pFile)
{
	//deviceID
	fprintf(pFile,"%c",BDF_TYPE_3DACC);
	//ID
	unsigned char ch = (unsigned char) t_ID;
	fprintf(pFile,"%c",ch);
	
	return;
}
/*-------------------------------------------------------------------------------------------------
::WriteDeviceDescriptionText   ::WriteDeviceDescriptionText   
---------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------*/
void GenericFrameBasedBinaryReader::WriteDeviceDescriptionText(FILE *pFile)
{
	//deviceID
	fprintf(pFile,"Available device is the Bluetooth Acceleration Sensor with DevID %d\n",p_ID);
	printf("Available device is the Bluetooth Acceleration Sensor with DevID %d\n",p_ID);
	
	return;
}

/*-------------------------------------------------------------------------------------------------
::WriteData   ::WriteData   
---------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------*/
void GenericFrameBasedBinaryReader::WriteData(FILE *pFile)
{
	if (available)
	{
		//TODO
// To update to the generic version.
/*		unsigned char  ch1,ch2;
		
		for(int j=0;j<3;j++)
		{
			ch1 = (p_data.accc[j] >>  8 ) & 0xff;
			ch2 = (p_data.accc[j]       ) & 0xff;
			fprintf(pFile,"%c%c",ch1,ch2);
		}*/
	}
	return;
}
/*-------------------------------------------------------------------------------------------------
::WriteTextData   ::WriteTextData   
---------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------*/
bool GenericFrameBasedBinaryReader::WriteTextData(FILE *pFile)
{
	if (available)
	{
		//TODO
/* 
	        fprintf(pFile, "Blue:  %d ",p_data.ID);
		fprintf(pFile, "%u ",(unsigned char)p_data.keystate);
		for(int j=0;j<3;j++)
			fprintf(pFile,"%5d ",p_data.accc[j]);
		*/
		return true;
	}
	/*
	fprintf(pFile, "Blue:  %d ",p_data.ID);
	fprintf(pFile, "%u ",(unsigned char)p_data.keystate);
	int x = 0;
	for(int j=0;j<3;j++)
		fprintf(pFile,"%5d ",x);
*/
	return false;
}
/*-------------------------------------------------------------------------------------------------
::WriteDataOnConsole   ::WriteDataOnConsole   
---------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------*/
bool GenericFrameBasedBinaryReader::WriteDataOnConsole()
{
	if (available)
	{
	/*	printf("Blue:  %d ",p_data.ID);
		printf("%u ",(unsigned char)p_data.keystate);
		
		for(int j=0;j<3;j++)
			printf("%5d ",p_data.accc[j]);*/
		for(int i=0;i<NumChannels;i++)
		{
			if(FrameParser_IsSigned(FrameFormat.c_str(),i))
				printf("%d ",p_data[i]);
			else
				printf("%u ",p_data[i]);
		}
		
		//printf("%u ",t_data.data.size());
		//printf("%f ",InRateCalc.GetRate());
		//printf("%f ",outrate);
		
		return true;
	}
	
	return false;
}

/*-------------------------------------------------------------------------------------------------
::WriteBufferData   ::WriteBufferData   
---------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------*/
void GenericFrameBasedBinaryReader::WriteBufferData(FILE *pFile)
{
	/*
	fprintf(pFile, "%d %.2f %.2f ", t_data.data.size(), InRateCalc.GetRate(), outrate);
*/
	return;
}

/*-------------------------------------------------------------------------------------------------
::Process   ::Process   ::Process  
---------------------------------------------------------------------------------------------------
Read and process a single character from the input stream (passed by DAData). 
Callable from thread or standalone.
Input:
	d:			internal data
	
Return:
	1: 	one packet has been received and processed
	0:		no packets have yet been received
	-1:	error
-------------------------------------------------------------------------------------------------*/

int GenericFrameBasedBinaryReader::Process()
{
	char ch;
	int r = SerialReadChar(fd,&ch);
	//printf("c: %d\n",ch);
	if(r==-2)
		return -2;		// Error -> return error
	
	if(r==-1)		// !!! not used so far!!!		
		return 0;		// No character (e.g. non-blocking read) -> return no packet

	// Shift memory.
	memmove(buffer,buffer+1,FrameSize-1);
	buffer[FrameSize-1] = ch;
	
	if(FrameParser_IsFrame(FrameFormat.c_str(),buffer)==1)
	{
		FrameParserV(FrameFormat.c_str(),buffer,&t_tmpdata[0]);
		
		t_ID=t_tmpdata[0];
		
		// Minimize locking time / number of locks by doing a time-efficient copy.
		// Do this only if sdl is included... 
		s_Lock();
		
		if(t_rcdata.size() < maxDataSize)
			t_rcdata.push_back(t_tmpdata);
		
		p_ID=t_ID;
		
		s_Unlock();
		
		return true;
	}
	return 0;
}

/*-------------------------------------------------------------------------------------------------
::ProbePort   ::ProbePort   ::ProbePort   ::ProbePort   ::ProbePort   ::ProbePort   ::ProbePort   
---------------------------------------------------------------------------------------------------
Opens a connection to port and looks for a sensor.
Input:
	Port:		port number (0 based)
	
Return:
	0: 		error (port not present, etc)
	!=0		sensor number
-------------------------------------------------------------------------------------------------*/
int GenericFrameBasedBinaryReader::ProbePort(int port)
{
	double tt1,tt2;
	bool answered;


	if(!OpenPort(port,false))		// Non blocking, we check until some timeout occurs
		return 0;
	// Port has successfully been open. Now we have to probe the device ID. Wait for packets, and return the ID.
	tt1=PreciseTimer::QueryTimer();
	answered=false;
	do
	{
		if(Process()==1)
		{
			answered=true;
			break;
		}
					
	}
	while( (tt2=PreciseTimer::QueryTimer()) - tt1 < 0.1);
	Close();
	
	
	if(answered)
	{
		printf("ID: %d\n",p_ID);
		return ((int)p_ID)&0xff;
	}
	return 0;
}

/*-------------------------------------------------------------------------------------------------
	::t_Run   ::t_Run   ::t_Run   ::t_Run   ::t_Run   ::t_Run   ::t_Run   ::t_Run   ::t_Run   
---------------------------------------------------------------------------------------------------
Thread entry point: reads data and manages serial port connection
-------------------------------------------------------------------------------------------------*/
int GenericFrameBasedBinaryReader::t_Run()
{
	bool initsuccess;
	bool lookforchange;
	
	printf("GenericFrameBasedBinaryReader::ReadThread(%p). change: %d  port: %d\n", this, t_ischange, port);
	
	memset(buffer,0,FrameSize);
	
	t_available = false;
	t_ischange = false;
	
	int rv;
	
	while(1)
	{
		initsuccess		= false;
		lookforchange	= true;
		printf("GenericFrameBasedBinaryReader::ReadThread(%p) Initialize (opmode %d)\n",this,opmode);
		do
		{	// Initialization
			switch(opmode)
			{
				case SERIAL_MODE_PORT:
					initsuccess = OpenPort(DesiredPort,true);
					break;
				case SERIAL_MODE_ID:
					//printf("opening id %d\n",reader->t_data.ID);
					initsuccess = OpenID(DesiredID,true);
					break;
				default:
					initsuccess = OpenID(-1,true);
			}
			//printf("GenericFrameBasedBinaryReader::ReadThread(%p) Initsuccess %d\n",this,initsuccess);
			if(initsuccess)
				break;
			sleep(DELAY_TIME);
		}		
		while(initsuccess==false);
		
		printf("GenericFrameBasedBinaryReader::ReadThread(%p)  Init successfull port %d \n",this,port);
		
		while(1)	//MAIN LOOP
		{
			//printf("ReadThread(%p) Process\n",this);
			
			rv = Process();
			if(rv==1)
			{
				InRateCalc.AddBufferTime(PreciseTimer::QueryTimer());
				//printf("rv %d\n",rv);
				// change detection: very crude: for now we only look at the detection of a sensor. If it's the same sensor we don't indicate.
				if(lookforchange)
				{
					t_available = true;
					t_ischange  = true;
					lookforchange			 = false;
				}
			}
			if(rv == -2)
			{
				break;
			}
		}
		t_available = false;
		t_ischange  = true;
		
		Close();
		printf("__________________________________________________________________________\n");
		printf("GenericFrameBasedBinaryReader::ReadThread    Error on serial port -> close serial port!!\n");
		printf("__________________________________________________________________________\n");
	}	
	
	return 0;
}

/*-------------------------------------------------------------------------------------------------
::InitByPort   ::InitByPort   ::InitByPort   ::InitByPort   ::InitByPort   ::InitByPort   
---------------------------------------------------------------------------------------------------
Initalize the class to use the port mode, i.e. reading data from the specified port.
Must be called before ::Start();
-------------------------------------------------------------------------------------------------*/
void GenericFrameBasedBinaryReader::InitByPort(unsigned port)
{
	opmode	= SERIAL_MODE_PORT;
	//t_data.plugandplay=false;
	DesiredPort 	= port;
}
/*-------------------------------------------------------------------------------------------------
::InitByID   ::InitByID   ::InitByID   ::InitByID   ::InitByID   ::InitByID   ::InitByID   
---------------------------------------------------------------------------------------------------
Initalize the class to use the ID mode, i.e. will scan all the ports to find a device with the 
corresponding ID.
Must be called before ::Start();
-------------------------------------------------------------------------------------------------*/
void GenericFrameBasedBinaryReader::InitByID(unsigned id,bool plugandplay)
{
	opmode	= SERIAL_MODE_ID;
	//t_data.plugandplay=plugandplay;
	DesiredID 		= id;
	//printf("InitbyID: opmode %d (%p)\n",t_data.opmode,&t_data);
}
/*-------------------------------------------------------------------------------------------------
::InitPlugAndPlay   ::InitPlugAndPlay   ::InitPlugAndPlay   ::InitPlugAndPlay   ::InitPlugAndPlay   
---------------------------------------------------------------------------------------------------
Initalize the class to use the first sensor found on any port.

Should not be mixed with InitByID or InitByPort: risk of having race conditions making some sensors
undiscoverable, e.g. when plug and play threads open ports, ID or Port threads can't access the 
port, and no guarantee that this does not occur continuously (although unlikely).
-------------------------------------------------------------------------------------------------*/
void GenericFrameBasedBinaryReader::InitPlugAndPlay()
{
	opmode=SERIAL_MODE_PNP;
	DesiredPort	= 0;
	DesiredID		= 0;
}

/*-------------------------------------------------------------------------------------------------
::OpenPort   ::OpenPort   ::OpenPort   ::OpenPort   ::OpenPort   ::OpenPort   ::OpenPort   
---------------------------------------------------------------------------------------------------
Opens the specified port.
Return:
	false:		Not successful
	true:			Successful
-------------------------------------------------------------------------------------------------*/
bool GenericFrameBasedBinaryReader::OpenPort(unsigned port,bool blocking)
{
	//printf("(%p) DAReader::OpenPort\n",d.tid);
	memset(buffer,0,FrameSize);
	
	//printf("###########BlueReader::OpenPort \n");
	fd = SerialOpen(port,BaudRate,blocking?0:1);
	
	if(fd == 0)
	{
	    //printf("###########BlueReader::OpenPort Cannot create the serial device\n");
		return false;
	}
    //printf("###########BlueReader::OpenPort Create serial device\n");
	return true;
	
}
/*-------------------------------------------------------------------------------------------------
::OpenID   ::OpenID   ::OpenID   ::OpenID   ::OpenID   ::OpenID   ::OpenID   ::OpenID   ::OpenID   
---------------------------------------------------------------------------------------------------
Scans the ports to find device ID and opens the corresponding port.
When ID is -1, any device ID is matched (first match).


Return:
	false:		Not successful
	true:			Successful
	d.fd:			Port handle
-------------------------------------------------------------------------------------------------*/
bool GenericFrameBasedBinaryReader::OpenID(int id,bool blocking)
{
//	printf("(%p) DAReader::OpenID (id=%d)\n",d.tid,id);
	int foundport=-1;
	int fndid;
	
	for(int p=0;p<32;p++)
	{
		fndid=ProbePort(p);
		if(fndid==0)
			continue;	// No device found
		if(id == -1)	// Looking for any ID (plug and play)
		{
			foundport=p;
			break;
		}
		if(fndid==id)	// Looking for a specific ID
		{
			foundport=p;
			break;
		}
	}
	if(foundport==-1)
		return false;
	
	//xxx to be changed
	fd = SerialOpen(foundport,BaudRate,blocking?0:1);
		
	if(fd == 0)
		return false;
	port = foundport;
	return true;
}

/*-------------------------------------------------------------------------------------------------
::Close   ::Close   ::Close   ::Close   ::Close   ::Close   ::Close   ::Close   ::Close   ::Close   
---------------------------------------------------------------------------------------------------
Closes the port which has been opened
-------------------------------------------------------------------------------------------------*/
void GenericFrameBasedBinaryReader::Close()
{	
	if(fd)
		SerialClose(fd);
	fd=0;
}
