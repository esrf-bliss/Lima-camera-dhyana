//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2014
// European Synchrotron Radiation Facility
// BP 220, Grenoble 38043
// FRANCE
//
// This is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This software is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//###########################################################################

#include <sstream>
#include <iostream>
#include <string>
#include <math.h>
//#include <chrono>
#include <climits>
#include <iomanip>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "lima/Exceptions.h"
#include "lima/Debug.h"
#include "lima/MiscUtils.h"
#include "DhyanaCamera.h"

using namespace lima;
using namespace lima::Dhyana;
using namespace std;

//---------------------------
// @brief  Ctor
//---------------------------
Camera::Camera():
m_depth(16),
m_trigger_mode(IntTrig),
m_status(Ready),
m_acq_frame_nb(0),
m_temperature_target(0)
{

	DEB_CONSTRUCTOR();	
	//Init TUCAM	
	init();		
	//create the acquisition thread
	DEB_TRACE() << "Create the acquisition thread ...";
	m_acq_thread = new AcqThread(*this);
	m_acq_thread->start();
	
}

//-----------------------------------------------------
//
//-----------------------------------------------------
Camera::~Camera()
{
	DEB_DESTRUCTOR();
	DEB_TRACE() << "Close TUCAM API ...";
	TUCAM_Dev_Close(m_opCam.hIdxTUCam);// Close camera
	DEB_TRACE() << "Uninitialize TUCAM API ...";
	TUCAM_Api_Uninit();// Uninitialize SDK API environment
	DEB_TRACE() << "Delete the acquisition thread ...";
	delete m_acq_thread;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::init()
{
	DEB_MEMBER_FUNCT();

	DEB_TRACE() << "Initialize TUCAM API ...";
	m_itApi.pstrConfigPath = NULL;//Camera parameters input saving path is not defined
	m_itApi.uiCamCount = 0;

	if(TUCAMRET_SUCCESS != TUCAM_Api_Init(&m_itApi))
	{
		// Initializing SDK API environment failed
		THROW_HW_ERROR(Error) << "Unable to initialize TUCAM_Api !";
	}

	if(0 == m_itApi.uiCamCount)
	{
		// No camera
		THROW_HW_ERROR(Error) << "Unable to locate the camera !";
	}

	DEB_TRACE() << "Open TUCAM API ...";
	m_opCam.hIdxTUCam = NULL;
	m_opCam.uiIdxOpen = 0;	
	if(TUCAMRET_SUCCESS != TUCAM_Dev_Open(&m_opCam))
	{
		// Failed to open camera
		THROW_HW_ERROR(Error) << "Unable to Open the camera !";
	}

	if(NULL == m_opCam.hIdxTUCam)
	{
		THROW_HW_ERROR(Error) << "Unable to open the camera !";
	}
	
	//initialize TUCAM Event used when Waiting for Frame
	m_hThdEvent = NULL;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::reset()
{
	DEB_MEMBER_FUNCT();
	stopAcq();	
	//@BEGIN : other stuff on Driver/API
	//...
	//@END
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::prepareAcq()
{
	DEB_MEMBER_FUNCT();
	AutoMutex lock(m_cond.mutex());
	Timestamp t0 = Timestamp::now();

	//@BEGIN : Ensure that Acquisition is Started before return ...
	DEB_TRACE() << "Ensure that Acquisition is Started  ";
	setStatus(Camera::Exposure, false);
	if(NULL == m_hThdEvent)
	{
		m_frame.pBuffer = NULL;
		m_frame.ucFormatGet = TUFRM_FMT_RAW;
		m_frame.uiRsdSize = 1;// how many frames do you want

		DEB_TRACE() << "TUCAM_Buf_Alloc";
		TUCAM_Buf_Alloc(m_opCam.hIdxTUCam, &m_frame);// Alloc buffer after set resolution or set ROI attribute

		DEB_TRACE() << "TUCAM_Cap_Start";
		if(m_trigger_mode == IntTrig)
			TUCAM_Cap_Start(m_opCam.hIdxTUCam, TUCCM_TRIGGER_SOFTWARE);// Start capture in software trigger
		else if(m_trigger_mode == ExtTrigMult)
			TUCAM_Cap_Start(m_opCam.hIdxTUCam, TUCCM_TRIGGER_STANDARD);// Start capture in external trigger STANDARD (EXPOSURE SOFT)
		else if(m_trigger_mode == ExtGate)
			TUCAM_Cap_Start(m_opCam.hIdxTUCam, TUCCM_TRIGGER_STANDARD);// Start capture in external trigger STANDARD (EXPOSURE WIDTH)
		
		DEB_TRACE() << "CreateEvent";
		m_hThdEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	}
	
	Timestamp t1 = Timestamp::now();
	double delta_time = t1 - t0;
	DEB_TRACE() << "prepareAcq : elapsed time = " << (int) (delta_time * 1000) << " (ms)";
	//@END
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::startAcq()
{
	DEB_MEMBER_FUNCT();
	AutoMutex lock(m_cond.mutex());
	
	Timestamp t0 = Timestamp::now();

	DEB_TRACE() << "startAcq ...";
	m_acq_frame_nb = 0;
	StdBufferCbMgr& buffer_mgr = m_bufferCtrlObj.getBuffer();
	buffer_mgr.setStartTimestamp(Timestamp::now());
	
	DEB_TRACE() << "Ensure that Acquisition is Started  & wait thread to be started ...";
	setStatus(Camera::Exposure, false);		
	//Start acquisition thread & wait 
	{
		m_wait_flag = false;
		m_quit = false;
		m_cond.broadcast();
		m_cond.wait();
	}
	
	//@BEGIN : tigger the acquisition
	if(m_trigger_mode == IntTrig)	
	{
		DEB_TRACE() << "DoSoftwareTrigger ";
		TUCAM_Cap_DoSoftwareTrigger(m_opCam.hIdxTUCam);
	}
	//@END
	
	Timestamp t1 = Timestamp::now();
	double delta_time = t1 - t0;
	DEB_TRACE() << "startAcq : elapsed time = " << (int) (delta_time * 1000) << " (ms)";
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::stopAcq()
{
	DEB_MEMBER_FUNCT();
	AutoMutex aLock(m_cond.mutex());
	DEB_TRACE() << "stopAcq ...";
	// Don't do anything if acquisition is idle.
	if(m_thread_running == true)
	{
		m_wait_flag = true;
		m_cond.broadcast();
		DEB_TRACE() << "stop requested ";
	}

	//@BEGIN : Ensure that Acquisition is Stopped before return ...			
	Timestamp t0 = Timestamp::now();
	if(NULL != m_hThdEvent)
	{
		TUCAM_Buf_AbortWait(m_opCam.hIdxTUCam);// If you called TUCAM_Buf_WaitForFrames()
		WaitForSingleObject(m_hThdEvent, INFINITE);
		CloseHandle(m_hThdEvent);
		m_hThdEvent = NULL;
		TUCAM_Cap_Stop(m_opCam.hIdxTUCam);// Stop capture   
		TUCAM_Buf_Release(m_opCam.hIdxTUCam);// Release alloc buffer after stop capture
	}

	//now detector is ready
	setStatus(Camera::Ready, false);
	//@END	
	
	Timestamp t1 = Timestamp::now();
	double delta_time = t1 - t0;
	DEB_TRACE() << "stopAcq : elapsed time = " << (int) (delta_time * 1000) << " (ms)";
	
	DEB_TRACE() << "Ensure that Acquisition is Stopped";
}

//-----------------------------------------------------
// @brief set the new camera status
//-----------------------------------------------------
void Camera::setStatus(Camera::Status status, bool force)
{
	DEB_MEMBER_FUNCT();
	//AutoMutex aLock(m_cond.mutex());
	if(force || m_status != Camera::Fault)
		m_status = status;
	//m_cond.broadcast();
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::getStatus(Camera::Status& status)
{
	DEB_MEMBER_FUNCT();
	AutoMutex aLock(m_cond.mutex());
	status = m_status;

	DEB_RETURN() << DEB_VAR1(status);
}

//-----------------------------------------------------
//
//-----------------------------------------------------
bool Camera::readFrame(void *bptr, int& frame_nb)
{
	DEB_MEMBER_FUNCT();
	Timestamp t0 = Timestamp::now();

	//@BEGIN : Get frame from Driver/API & copy it into bptr already allocated 
//	DEB_TRACE() << "Copy Buffer image into Lima Frame Ptr";
	memcpy((unsigned short *) bptr, (unsigned short *) (m_frame.pBuffer + m_frame.usOffset), m_frame.uiImgSize);//we need a nb of BYTES .		
	frame_nb = m_frame.uiIndex;
	//@END	

//	Timestamp t1 = Timestamp::now();
//	double delta_time = t1 - t0;
//	DEB_TRACE() << "readFrame : elapsed time = " << (int) (delta_time * 1000) << " (ms)";
	return false;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::AcqThread::threadFunction()
{
	DEB_MEMBER_FUNCT();
	AutoMutex aLock(m_cam.m_cond.mutex());
	StdBufferCbMgr& buffer_mgr = m_cam.m_bufferCtrlObj.getBuffer();

	while(!m_cam.m_quit)
	{
		while(m_cam.m_wait_flag && !m_cam.m_quit)
		{
			DEB_TRACE() << "Wait for start acquisition ...";
			m_cam.m_thread_running = false;
			m_cam.m_cond.broadcast();
			m_cam.m_cond.wait();
		}

		//if quit is requested (requested only by destructor)
		if(m_cam.m_quit)
			return;

		DEB_TRACE() << "Running ...";
		m_cam.m_thread_running = true;
		m_cam.m_cond.broadcast();
		aLock.unlock();		

		//@BEGIN 
		bool continueFlag = true;
		while(continueFlag && (!m_cam.m_nb_frames || m_cam.m_acq_frame_nb < m_cam.m_nb_frames))
		{
			// Check first if acq. has been stopped
			////DEB_TRACE() << "Check first if acq. has been stopped ";
			if(m_cam.m_wait_flag)
			{
				DEB_TRACE() << "AcqThread has been stopped from user";
				continueFlag = false;
				continue;
			}

			//set status to exposure
			m_cam.setStatus(Camera::Exposure, false);

			//wait frame from TUCAM API ...
			DEB_TRACE() << "Wait frame from TUCAM API ...";
			Timestamp t0_capture = Timestamp::now();
			if(TUCAMRET_SUCCESS == TUCAM_Buf_WaitForFrame(m_cam.m_opCam.hIdxTUCam, &m_cam.m_frame))
			{
				Timestamp t1_capture = Timestamp::now();
				double delta_time_capture = t1_capture - t0_capture;
				DEB_TRACE() << "Capture elapsed time = " << (int) (delta_time_capture * 1000) << " (ms)";		
				
				//Generate software trigger for each frame, except for the first image becaus eit is already done in startAcq()
				if((!m_cam.m_nb_frames || m_cam.m_acq_frame_nb < m_cam.m_nb_frames) && (m_cam.m_trigger_mode == IntTrig))
				{
					DEB_TRACE() << "DoSoftwareTrigger ";
					TUCAM_Cap_DoSoftwareTrigger(m_cam.m_opCam.hIdxTUCam);
				}			
				
				//The based information
				//DEB_TRACE() << "m_cam.m_frame.szSignature = "	<< m_cam.m_frame.szSignature<<std::endl;		// [out]Copyright+Version: TU+1.0 ['T', 'U', '1', '\0']		
				//DEB_TRACE() << "m_cam.m_frame.usHeader = "	<< m_cam.m_frame.usHeader<<std::endl;			// [out] The frame header size
				//DEB_TRACE() << "m_cam.m_frame.usOffset = "	<< m_cam.m_frame.usOffset<<std::endl;			// [out] The frame data offset
				//DEB_TRACE() << "m_cam.m_frame.usWidth = "		<< m_cam.m_frame.usWidth;						// [out] The frame width
				//DEB_TRACE() << "m_cam.m_frame.usHeight = "	<< m_cam.m_frame.usHeight;						// [out] The frame height
				//DEB_TRACE() << "m_cam.m_frame.uiWidthStep = "	<< m_cam.m_frame.uiWidthStep<<std::endl;		// [out] The frame width step
				//DEB_TRACE() << "m_cam.m_frame.ucDepth = "		<< m_cam.m_frame.ucDepth<<std::endl;			// [out] The frame data depth 
				//DEB_TRACE() << "m_cam.m_frame.ucFormat = "	<< m_cam.m_frame.ucFormat<<std::endl;			// [out] The frame data format                  
				//DEB_TRACE() << "m_cam.m_frame.ucChannels = "	<< m_cam.m_frame.ucChannels<<std::endl;			// [out] The frame data channels
				//DEB_TRACE() << "m_cam.m_frame.ucElemBytes = "	<< m_cam.m_frame.ucElemBytes<<std::endl;		// [out] The frame data bytes per element
				//DEB_TRACE() << "m_cam.m_frame.ucFormatGet = "	<< m_cam.m_frame.ucFormatGet<<std::endl;		// [in]  Which frame data format do you want    see TUFRM_FORMATS
				//DEB_TRACE() << "m_cam.m_frame.uiIndex = "		<< m_cam.m_frame.uiIndex;						// [in/out] The frame index number
				//DEB_TRACE() << "m_cam.m_frame.uiImgSize = "	<< m_cam.m_frame.uiImgSize;						// [out] The frame size
				//DEB_TRACE() << "m_cam.m_frame.uiRsdSize = "	<< m_cam.m_frame.uiRsdSize;						// [in]  The frame reserved size    (how many frames do you want)
				//DEB_TRACE() << "m_cam.m_frame.uiHstSize = "	<< m_cam.m_frame.uiHstSize<<std::endl;			// [out] The frame histogram size	

				// Grabbing was successful, process image
				////DEB_TRACE() << "Frame from TUCAM API has arrived";
				m_cam.setStatus(Camera::Readout, false);

				//Prepare Lima Frame Ptr 
				////DEB_TRACE() << "Prepare  Lima Frame Ptr";
				void* bptr = buffer_mgr.getFrameBufferPtr(m_cam.m_acq_frame_nb);

				//Copy Frame into Lima Frame Ptr
				int frame_nb = 0;
				m_cam.readFrame(bptr, frame_nb);
		
				//Push the image buffer through Lima 
				Timestamp t0 = Timestamp::now();
				////DEB_TRACE() << "Declare a Lima new Frame Ready (" << m_cam.m_acq_frame_nb << ")";
				HwFrameInfoType frame_info;
				frame_info.acq_frame_nb = m_cam.m_acq_frame_nb;
				continueFlag = buffer_mgr.newFrameReady(frame_info);
				m_cam.m_acq_frame_nb++;
				Timestamp t1 = Timestamp::now();
				double delta_time = t1 - t0;
				////DEB_TRACE() << "newFrameReady elapsed time = " << (int) (delta_time * 1000) << " (ms)";				
				//wait latency after each frame , except for the last image
				if(!m_cam.m_nb_frames || m_cam.m_acq_frame_nb < m_cam.m_nb_frames)
				{
					DEB_TRACE() << "Wait latency time : " << m_cam.m_lat_time * 1000 << " (ms) ...";
					usleep((DWORD) (m_cam.m_lat_time * 1000000));
				}
			}
			else
			{
				DEB_TRACE() << "Unable to get the frame from the camera !";
				//m_cam.setStatus(Camera::Fault,false);
			}
		}

		//
		DEB_TRACE() << "SetEvent";
		SetEvent(m_cam.m_hThdEvent);
		//@END
		
		//stopAcq only if this is not already done		
		DEB_TRACE() << "stopAcq only if this is not already done";
		if(!m_cam.m_wait_flag)
		{
			DEB_TRACE() << "stopAcq";
			m_cam.stopAcq();
		}

		//now detector is ready
		m_cam.setStatus(Camera::Ready, false);
		DEB_TRACE() << "AcqThread is no more running";		
		aLock.lock();
		m_cam.m_thread_running = false;
		m_cam.m_wait_flag = true;
	}
}

//-----------------------------------------------------
//
//-----------------------------------------------------
Camera::AcqThread::AcqThread(Camera& cam):
m_cam(cam)
{
	AutoMutex aLock(m_cam.m_cond.mutex());
	m_cam.m_wait_flag = true;
	m_cam.m_quit = false;
	aLock.unlock();
	pthread_attr_setscope(&m_thread_attr, PTHREAD_SCOPE_PROCESS);
}

//-----------------------------------------------------
//
//-----------------------------------------------------
Camera::AcqThread::~AcqThread()
{
	AutoMutex aLock(m_cam.m_cond.mutex());
	m_cam.m_wait_flag = true;
	m_cam.m_quit = true;
	m_cam.m_cond.broadcast();
	aLock.unlock();
	join();
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::getImageType(ImageType& type)
{
	DEB_MEMBER_FUNCT();
	//@BEGIN : Fix the image type (pixel depth) into Driver/API		
	switch(m_depth)
	{
		case 16: type = Bpp16;
			break;
		default:
			THROW_HW_ERROR(Error) << "This pixel format of the camera is not managed, only 16 bits cameras are already managed!";
			break;
	}
	//@END	
	return;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::setImageType(ImageType type)
{
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "setImageType - " << DEB_VAR1(type);
	//@BEGIN : Fix the image type (pixel depth) into Driver/API	
	switch(type)
	{
		case Bpp16:
			m_depth = 16;
			break;
		default:
			THROW_HW_ERROR(Error) << "This pixel format of the camera is not managed, only 16 bits cameras are already managed!";
			break;
	}
	//@END	
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::getDetectorType(std::string& type)
{
	DEB_MEMBER_FUNCT();
	//@BEGIN : Get Detector type from Driver/API
	type = "Tucsen - Dhyana";
	//@END	

}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::getDetectorModel(std::string& model)
{
	DEB_MEMBER_FUNCT();
	//@BEGIN : Get Detector model/type from Driver/API
	stringstream ss;
	TUCAM_VALUE_INFO valInfo;
	valInfo.nID = TUIDI_CAMERA_MODEL;
	if(TUCAMRET_SUCCESS != TUCAM_Dev_GetInfo(m_opCam.hIdxTUCam, &valInfo))
	{
		THROW_HW_ERROR(Error) << "Unable to Read TUIDI_CAMERA_MODEL from the camera !";
	}
	ss << valInfo.pText;
	model = ss.str();
	//@END		
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::getDetectorImageSize(Size& size)
{
	DEB_MEMBER_FUNCT();

	//@BEGIN : Get Detector size in pixels from Driver/API
	size = Size(PIXEL_NB_WIDTH, PIXEL_NB_HEIGHT);
	//@END
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::getPixelSize(double& sizex, double& sizey)
{
	DEB_MEMBER_FUNCT();
	//@BEGIN : Get Pixels size in micron from Driver/API			
	sizex = PIXEL_SIZE_WIDTH_MICRON;
	sizey = PIXEL_SIZE_HEIGHT_MICRON;
	//@END
}

//-----------------------------------------------------
//
//-----------------------------------------------------
HwBufferCtrlObj* Camera::getBufferCtrlObj()
{
	return &m_bufferCtrlObj;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
bool Camera::checkTrigMode(TrigMode mode)
{
	DEB_MEMBER_FUNCT();
	bool valid_mode;
	//@BEGIN
	switch(mode)
	{
		case IntTrig:
		case ExtTrigMult:
		case ExtGate:
			valid_mode = true;
			break;
		case ExtTrigReadout:
		case ExtTrigSingle:
		case IntTrigMult:
		default:
			valid_mode = false;
			break;
	}
	//@END
	return valid_mode;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::setTrigMode(TrigMode mode)
{
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "setTrigMode() " << DEB_VAR1(mode);
	DEB_PARAM() << DEB_VAR1(mode);
	//@BEGIN
	TUCAM_TRIGGER_ATTR tgrAttr;
	tgrAttr.nTgrMode = -1;//NOT DEFINED (see below)
	tgrAttr.nFrames = 1;
	tgrAttr.nDelayTm = 0;
	tgrAttr.nExpMode = -1;//NOT DEFINED (see below)
	tgrAttr.nEdgeMode = TUCTD_RISING;

	switch(mode)
	{
		case IntTrig:
			tgrAttr.nTgrMode = TUCCM_TRIGGER_SOFTWARE;
			tgrAttr.nExpMode = TUCTE_EXPTM;
			TUCAM_Cap_SetTrigger(m_opCam.hIdxTUCam, tgrAttr);
			DEB_TRACE() << "TUCAM_Cap_SetTrigger : TUCCM_TRIGGER_SOFTWARE (EXPOSURE SOFTWARE)";
			break;
		case ExtTrigMult :
			tgrAttr.nTgrMode = TUCCM_TRIGGER_STANDARD;
			tgrAttr.nExpMode = TUCTE_EXPTM;
			TUCAM_Cap_SetTrigger(m_opCam.hIdxTUCam, tgrAttr);
			DEB_TRACE() << "TUCAM_Cap_SetTrigger : TUCCM_TRIGGER_STANDARD (EXPOSURE SOFTWARE: "<<tgrAttr.nExpMode<<")";
			break;
		case ExtGate:		
			tgrAttr.nTgrMode = TUCCM_TRIGGER_STANDARD;
			tgrAttr.nExpMode = TUCTE_WIDTH;
			TUCAM_Cap_SetTrigger(m_opCam.hIdxTUCam, tgrAttr);
			DEB_TRACE() << "TUCAM_Cap_SetTrigger : TUCCM_TRIGGER_STANDARD (EXPOSURE TRIGGER WIDTH: "<<tgrAttr.nExpMode<<")";
			break;			
		case ExtTrigSingle :		
		case IntTrigMult:
		case ExtTrigReadout:
		default:
			THROW_HW_ERROR(NotSupported) << DEB_VAR1(mode);
	}
	m_trigger_mode = mode;
	//@END

}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::getTrigMode(TrigMode& mode)
{
	DEB_MEMBER_FUNCT();
	mode = m_trigger_mode;
	DEB_RETURN() << DEB_VAR1(mode);
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::getExpTime(double& exp_time)
{
	DEB_MEMBER_FUNCT();
	//@BEGIN
	double dbVal;
	if(TUCAMRET_SUCCESS != TUCAM_Prop_GetValue(m_opCam.hIdxTUCam, TUIDP_EXPOSURETM, &dbVal))
	{
		THROW_HW_ERROR(Error) << "Unable to Read TUIDP_EXPOSURETM from the camera !";
	}
	m_exp_time = dbVal / 1000;//TUCAM use (ms), but lima use (second) as unit 
	//@END
	exp_time = m_exp_time;
	DEB_RETURN() << DEB_VAR1(exp_time);
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::setExpTime(double exp_time)
{
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "setExpTime() " << DEB_VAR1(exp_time);
	//@BEGIN
	if(TUCAMRET_SUCCESS != TUCAM_Prop_SetValue(m_opCam.hIdxTUCam, TUIDP_EXPOSURETM, exp_time * 1000))//TUCAM use (ms), but lima use (second) as unit 
	{
		THROW_HW_ERROR(Error) << "Unable to Write TUIDP_EXPOSURETM to the camera !";
	}
	//@END
	m_exp_time = exp_time;
}


//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::setLatTime(double lat_time)
{
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "setLatTime() " << DEB_VAR1(lat_time);
	m_lat_time = lat_time;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::getLatTime(double& lat_time)
{
	DEB_MEMBER_FUNCT();
	//@BEGIN
	//@END
	m_lat_time = lat_time;
	DEB_RETURN() << DEB_VAR1(lat_time);
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::getExposureTimeRange(double& min_expo, double& max_expo) const
{
	DEB_MEMBER_FUNCT();
	//@BEGIN
	min_expo = 0.;
	max_expo = 10;//10s
	//@END
	DEB_RETURN() << DEB_VAR2(min_expo, max_expo);
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::getLatTimeRange(double& min_lat, double& max_lat) const
{
	DEB_MEMBER_FUNCT();
	//@BEGIN
	// --- no info on min latency
	min_lat = 0.;
	// --- do not know how to get the max_lat, fix it as the max exposure time
	max_lat = 10;//10s
	//@END
	DEB_RETURN() << DEB_VAR2(min_lat, max_lat);
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::setNbFrames(int nb_frames)
{
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "setNbFrames() " << DEB_VAR1(nb_frames);
	//@BEGIN
	if(nb_frames < 0)
	{
		THROW_HW_ERROR(Error) << "Number of frames to acquire has not been set";
	}
	if(nb_frames > 1)
	{
		//THROW_HW_ERROR(Error) << "Unable to acquire more than 1 frame ! ";	//T@D@ allowed only for test 
	}
	//@END
	m_nb_frames = nb_frames;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::getNbFrames(int& nb_frames)
{
	DEB_MEMBER_FUNCT();
	DEB_RETURN() << DEB_VAR1(m_nb_frames);
	//@BEGIN
	//@END
	nb_frames = m_nb_frames;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
int Camera::getNbHwAcquiredFrames()
{
	DEB_MEMBER_FUNCT();
	return m_acq_frame_nb;
}

//-----------------------------------------------------
// @brief range the binning to the maximum allowed
//-----------------------------------------------------
void Camera::checkBin(Bin &hw_bin)
{
	DEB_MEMBER_FUNCT();

	//@BEGIN : check available values of binning H/V
	int x = hw_bin.getX();
	int y = hw_bin.getY();
	if(x != 1 || y != 1)
	{
		DEB_ERROR() << "Binning values not supported";
		THROW_HW_ERROR(Error) << "Binning values not supported = " << DEB_VAR1(hw_bin);
	}
	//@END

	hw_bin = Bin(x, y);
	DEB_RETURN() << DEB_VAR1(hw_bin);
}
//-----------------------------------------------------
// @brief set the new binning mode
//-----------------------------------------------------
void Camera::setBin(const Bin &set_bin)
{
	DEB_MEMBER_FUNCT();

	//@BEGIN : set binning H/V to the Driver/API
	//...
	//@END
	m_bin = set_bin;

	DEB_RETURN() << DEB_VAR1(set_bin);
}

//-----------------------------------------------------
// @brief return the current binning mode
//-----------------------------------------------------
void Camera::getBin(Bin &hw_bin)
{
	DEB_MEMBER_FUNCT();

	//@BEGIN : get binning from Driver/API
	int bin_x = 1;
	int bin_y = 1;
	//@END
	Bin tmp_bin(bin_x, bin_y);

	hw_bin = tmp_bin;
	m_bin = tmp_bin;

	DEB_RETURN() << DEB_VAR1(hw_bin);
}

//-----------------------------------------------------
//! Camera::checkRoi()
//-----------------------------------------------------
void Camera::checkRoi(const Roi& set_roi, Roi& hw_roi)
{
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "checkRoi";
	DEB_PARAM() << DEB_VAR1(set_roi);
	//@BEGIN : check available values of Roi
	if(set_roi.isActive())
	{
		// Taking care of the value increment, roi parameters must be power of 2 and > 32 . 
		if(	!IS_POWER_OF_2(set_roi.getTopLeft().x)			|| (set_roi.getTopLeft().x) < 32		||
			!IS_POWER_OF_2(set_roi.getTopLeft().y)			|| (set_roi.getTopLeft().y) < 32		||
			!IS_POWER_OF_2(set_roi.getSize().getWidth())	|| (set_roi.getSize().getWidth()) < 32	||
			!IS_POWER_OF_2(set_roi.getSize().getHeight())	|| (set_roi.getSize().getHeight()) < 32)
		{
			THROW_HW_ERROR(Error) << "Roi coordinates (x, y, width, height) must be a power of 2 AND must be at least 32 !";
		}
		hw_roi = set_roi;
	}
	else
	{
		hw_roi = set_roi;
	}
	//@END


	DEB_RETURN() << DEB_VAR1(hw_roi);
}

//---------------------------------------------------------------------------------------
//! Camera::getRoi()
//---------------------------------------------------------------------------------------
void Camera::getRoi(Roi& hw_roi)
{
	DEB_MEMBER_FUNCT();
	//@BEGIN : get Roi from the Driver/API
	TUCAM_ROI_ATTR roiAttr;
	if(TUCAMRET_SUCCESS != TUCAM_Cap_GetROI(m_opCam.hIdxTUCam, &roiAttr))
	{
		THROW_HW_ERROR(Error) << "Unable to GetRoi from  the camera !";
	}
	hw_roi = Roi(roiAttr.nHOffset,
				roiAttr.nVOffset,
				roiAttr.nWidth,
				roiAttr.nHeight);
	//@END

	DEB_RETURN() << DEB_VAR1(hw_roi);
}

//---------------------------------------------------------------------------------------
//! Camera::setRoi()
//---------------------------------------------------------------------------------------
void Camera::setRoi(const Roi& set_roi)
{
	DEB_MEMBER_FUNCT();
	DEB_TRACE() << "setRoi";
	DEB_PARAM() << DEB_VAR1(set_roi);
	//@BEGIN : set Roi from the Driver/API	
	if(!set_roi.isActive())
	{
		DEB_TRACE() << "Roi is not Enabled : so set full frame";

		//set Roi to Driver/API
		Size size;
		getDetectorImageSize(size);
		TUCAM_ROI_ATTR roiAttr;
		roiAttr.bEnable = TRUE;
		roiAttr.nHOffset = 0;
		roiAttr.nVOffset = 0;
		roiAttr.nWidth = size.getWidth();
		roiAttr.nHeight = size.getHeight();

		if(TUCAMRET_SUCCESS != TUCAM_Cap_SetROI(m_opCam.hIdxTUCam, roiAttr))
		{
			THROW_HW_ERROR(Error) << "Unable to SetRoi to the camera !";
		}

	}
	else
	{
		DEB_TRACE() << "Roi is Enabled";
		//set Roi to Driver/API
		TUCAM_ROI_ATTR roiAttr;
		roiAttr.bEnable = TRUE;
		roiAttr.nHOffset = set_roi.getTopLeft().x;
		roiAttr.nVOffset = set_roi.getTopLeft().y;
		roiAttr.nWidth = set_roi.getSize().getWidth();
		roiAttr.nHeight = set_roi.getSize().getHeight();

		if(TUCAMRET_SUCCESS != TUCAM_Cap_SetROI(m_opCam.hIdxTUCam, roiAttr))
		{
			THROW_HW_ERROR(Error) << "Unable to SetRoi to the camera !";
		}
	}
	//@END	
}

//-----------------------------------------------------
//
//-----------------------------------------------------
bool Camera::isAcqRunning() const
{
	DEB_MEMBER_FUNCT();
	//	AutoMutex aLock(m_cond.mutex());
	DEB_TRACE() << "isAcqRunning - " << DEB_VAR1(m_thread_running) << "---------------------------";
	return m_thread_running;
}

///////////////////////////////////////////////////////
// Dhyana specific stuff now
///////////////////////////////////////////////////////

//-----------------------------------------------------
//
//-----------------------------------------------------  
void Camera::setTemperatureTarget(double temp)
{
	DEB_MEMBER_FUNCT();
	TUCAM_PROP_ATTR attrProp;
	attrProp.nIdxChn = 0;// Current channel (camera monochrome = 0) . VERY IMPORTANT, doesn't work otherwise !!!!!
	attrProp.idProp = TUIDP_TEMPERATURE;
	if(TUCAMRET_SUCCESS != TUCAM_Prop_GetAttr(m_opCam.hIdxTUCam, &attrProp))
	{
		THROW_HW_ERROR(Error) << "Unable to Read TUIDP_TEMPERATURE range from the camera !";
	}
	DEB_TRACE() << "Temperature range [" << (int) attrProp.dbValMin << " , " << (int) attrProp.dbValMax << "]";

	int temp_middle = (int) attrProp.dbValMax / 2;
	if(((int) temp + temp_middle)<((int) attrProp.dbValMin) || ((int) temp + temp_middle)>((int) attrProp.dbValMax))
	{
		THROW_HW_ERROR(Error) << "Unable to set the Temperature Target !\n"
		 << "It is out of range : "
		 << "["
		 << (int) attrProp.dbValMin - temp_middle
		 << ","
		 << (int) attrProp.dbValMax - temp_middle
		 << "]";
	}

	int nVal = (int) temp + temp_middle;//temperatureTarget is a delta according to the middle temperature !!
	if(TUCAMRET_SUCCESS != TUCAM_Prop_SetValue(m_opCam.hIdxTUCam, TUIDP_TEMPERATURE, nVal))
	{
		THROW_HW_ERROR(Error) << "Unable to Write TUIDP_TEMPERATURE to the camera !";
	}
	m_temperature_target = (double) temp;
}

//-----------------------------------------------------
//
//-----------------------------------------------------  
void Camera::getTemperatureTarget(double& temp)
{
	DEB_MEMBER_FUNCT();

	temp = m_temperature_target;
}

//-----------------------------------------------------
//
//-----------------------------------------------------  
void Camera::getTemperature(double& temp)
{
	DEB_MEMBER_FUNCT();

	double dbVal = 0.0f;
	if(TUCAMRET_SUCCESS != TUCAM_Prop_GetValue(m_opCam.hIdxTUCam, TUIDP_TEMPERATURE, &dbVal))
	{
		THROW_HW_ERROR(Error) << "Unable to Read TUIDP_TEMPERATURE from the camera !";
	}
	temp = dbVal;
}

//-----------------------------------------------------
//
//-----------------------------------------------------  
void Camera::setFanSpeed(unsigned speed)
{
	DEB_MEMBER_FUNCT();

	int nVal = (int) speed;

	if(TUCAMRET_SUCCESS != TUCAM_Capa_SetValue(m_opCam.hIdxTUCam, TUIDC_FAN_GEAR, nVal))
	{
		THROW_HW_ERROR(Error) << "Unable to Write TUIDC_FAN_GEAR to the camera !";
	}
}

//-----------------------------------------------------
//
//-----------------------------------------------------  
void Camera::getFanSpeed(unsigned& speed)
{
	DEB_MEMBER_FUNCT();

	int nVal;
	if(TUCAMRET_SUCCESS != TUCAM_Capa_GetValue(m_opCam.hIdxTUCam, TUIDC_FAN_GEAR, &nVal))
	{
		THROW_HW_ERROR(Error) << "Unable to Read TUIDC_FAN_GEAR from the camera !";
	}
	speed = (unsigned) nVal;
}

//-----------------------------------------------------
//
//-----------------------------------------------------  
void Camera::setGlobalGain(unsigned gain)
{
	DEB_MEMBER_FUNCT();

	if(gain != 0 && gain != 1 && gain != 2)
	{
		THROW_HW_ERROR(Error) << "Available gain values are : 0:HDR\n1:HIGH\n2: LOW !";
	}

	double dbVal = (double) gain;
	if(TUCAMRET_SUCCESS != TUCAM_Prop_SetValue(m_opCam.hIdxTUCam, TUIDP_GLOBALGAIN, dbVal))
	{
		THROW_HW_ERROR(Error) << "Unable to Write TUIDP_GLOBALGAIN to the camera !";
	}
}

//-----------------------------------------------------
//
//-----------------------------------------------------  
void Camera::getGlobalGain(unsigned& gain)
{
	DEB_MEMBER_FUNCT();

	double dbVal;
	if(TUCAMRET_SUCCESS != TUCAM_Prop_GetValue(m_opCam.hIdxTUCam, TUIDP_GLOBALGAIN, &dbVal))
	{
		THROW_HW_ERROR(Error) << "Unable to Read TUIDP_GLOBALGAIN from the camera !";
	}
	gain = (unsigned) dbVal;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::getTucamVersion(std::string& version)
{
	DEB_MEMBER_FUNCT();
	TUCAM_VALUE_INFO valInfo;
	valInfo.nID = TUIDI_VERSION_API;
	if(TUCAMRET_SUCCESS != TUCAM_Dev_GetInfo(m_opCam.hIdxTUCam, &valInfo))
	{
		THROW_HW_ERROR(Error) << "Unable to Read TUIDI_VERSION_API from the camera !";
	}
	version = valInfo.pText;
}

//-----------------------------------------------------
//
//-----------------------------------------------------  
void Camera::getFirmwareVersion(std::string& version)
{
	DEB_MEMBER_FUNCT();
	TUCAM_VALUE_INFO valInfo;
	valInfo.nID = TUIDI_VERSION_FRMW;
	if(TUCAMRET_SUCCESS != TUCAM_Dev_GetInfo(m_opCam.hIdxTUCam, &valInfo))
	{
		THROW_HW_ERROR(Error) << "Unable to Read TUIDI_VERSION_FRMW from the camera !";
	}
	version = valInfo.nValue;
}

//-----------------------------------------------------
//
//-----------------------------------------------------  