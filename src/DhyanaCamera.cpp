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
#include <climits>
#include <iomanip>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include "lima/Exceptions.h"
#include "lima/Debug.h"
#include "lima/MiscUtils.h"
#include "DhyanaTimer.h"
#include "DhyanaCamera.h"

using namespace lima;
using namespace lima::Dhyana;
using namespace std;

//---------------------------
// @brief  Ctor
//---------------------------
Camera::Camera(unsigned short timer_period_ms):
m_depth(16),
m_trigger_mode(IntTrig),
m_status(Ready),
m_acq_frame_nb(0),
m_temperature_target(0),
m_timer_period_ms(timer_period_ms),
m_prepared(false),
m_tucam_trigger_mode(TriggerStandard),
m_tucam_trigger_edge_mode(EdgeRising),
m_cold_start(true)
{
	DEB_CONSTRUCTOR();	
	//Init TUCAM	
	init();		
	//create the acquisition thread
	DEB_TRACE() << "Create the acquisition thread";
	m_acq_thread = new AcqThread(*this);
	DEB_TRACE() <<"Create the Internal Trigger Timer";
	m_internal_trigger_timer = new CSoftTriggerTimer(m_timer_period_ms, *this);
	m_acq_thread->start();
	m_hThdLock = PTHREAD_MUTEX_INITIALIZER;
	m_hThdEvent = PTHREAD_COND_INITIALIZER;
	m_signalled = false;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
Camera::~Camera()
{
	DEB_DESTRUCTOR();
	// Close camera
	DEB_TRACE() << "Close TUCAM API ...";
	TUCAM_Dev_Close(m_opCam.hIdxTUCam);
	// Uninitialize SDK API environment
	DEB_TRACE() << "Uninitialize TUCAM API ...";
	TUCAM_Api_Uninit();
	//delete the acquisition thread
	DEB_TRACE() << "Delete the acquisition thread";
	delete m_acq_thread;
	//delete the Internal Trigger Timer
	DEB_TRACE() << "Delete the Internal Trigger Timer";
	delete m_internal_trigger_timer;
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
	Timestamp t0 = Timestamp::now();
	if (!m_prepared)
	  {
	    if (m_cold_start)
	      {
		//At cold start we must trig a fake capture, otherwise the camera will never capture frames
		m_cold_start = false;
		DEB_TRACE() << "Cold start";
		TUCAM_Cap_Start(m_opCam.hIdxTUCam, TUCCM_TRIGGER_SOFTWARE);
		TUCAM_Cap_Stop(m_opCam.hIdxTUCam);
	      }
	       m_frame.pBuffer = NULL;
	       m_frame.ucFormatGet = TUFRM_FMT_RAW;
	       m_frame.uiRsdSize = 1;// how many frames do you want
	       
	       // Alloc buffer after set resolution or set ROI attribute
	       DEB_TRACE() << "TUCAM_Buf_Alloc";
	       if(TUCAMRET_SUCCESS != TUCAM_Buf_Alloc(m_opCam.hIdxTUCam, &m_frame))
		 {
		   THROW_HW_ERROR(Error) << "Buff_Alloc failed";
		 }
	       m_prepared = true;
	       Timestamp t1 = Timestamp::now();
	       double delta_time = t1 - t0;
	       DEB_TRACE() << "Buff_Alloc = " << (int) (delta_time * 1000) << " (ms)";		
	       t0 = t1;
	  }
	TUCAM_TRIGGER_ATTR tgrAttr;
	if (TUCAMRET_SUCCESS != TUCAM_Cap_GetTrigger(m_opCam.hIdxTUCam, &tgrAttr))
	  {
	    THROW_HW_ERROR(Error) << "Cap_GetTrigger failed";
	  }
	tgrAttr.nTgrMode = -1;//NOT DEFINED (see below)
	tgrAttr.nFrames = 1;
	tgrAttr.nDelayTm = 0;
	tgrAttr.nExpMode = -1;//NOT DEFINED (see below)
	tgrAttr.nEdgeMode = m_tucam_trigger_edge_mode;
	
	switch(m_trigger_mode)
	  {
	  case IntTrig:
	    tgrAttr.nTgrMode = TUCCM_TRIGGER_SOFTWARE;
	    tgrAttr.nExpMode = TUCTE_EXPTM;
	    break;
	  case ExtTrigMult :
	    tgrAttr.nTgrMode = m_tucam_trigger_mode;
	    tgrAttr.nExpMode = TUCTE_EXPTM;
	    break;
	  case ExtGate:		
	    tgrAttr.nTgrMode = m_tucam_trigger_mode;
	    tgrAttr.nExpMode = TUCTE_WIDTH;
	    break;			
	  case ExtTrigSingle :
	    tgrAttr.nFrames = m_nb_frames;
	    tgrAttr.nTgrMode = m_tucam_trigger_mode;
	    tgrAttr.nExpMode = TUCTE_EXPTM;
	    break;
	  }
        if(TUCAMRET_SUCCESS != TUCAM_Cap_SetTrigger(m_opCam.hIdxTUCam, tgrAttr))
	  {
	    THROW_HW_ERROR(Error) << "Cap_SetTrigger failed";
	  }
	
	DEB_TRACE() << "TUCAM_Cap_SetTrigger : " << m_trigger_mode << ", " << tgrAttr.nTgrMode << ", " <<  tgrAttr.nExpMode;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Camera::startAcq()
{
	DEB_MEMBER_FUNCT();
	Timestamp t0 = Timestamp::now();
        Timestamp t1;
	DEB_TRACE() << "startAcq ...";
	
	//@BEGIN : trigger the acquisition
	DEB_TRACE() << "TUCAM_Cap_Start";
	if(m_trigger_mode == IntTrig)	
	{
	        // Start capture in software trigger
	  if(TUCAMRET_SUCCESS !=TUCAM_Cap_Start(m_opCam.hIdxTUCam, TUCCM_TRIGGER_SOFTWARE))
	    {
	      THROW_HW_ERROR(Error) << "Cap_SetTrigger failed";
	    }
	}
	else
	  {
	    if(TUCAMRET_SUCCESS !=TUCAM_Cap_Start(m_opCam.hIdxTUCam, m_tucam_trigger_mode))
	      {
		THROW_HW_ERROR(Error) << "Cap_SetTrigger failed";
	      }
	  }
	//  Cap_Start is not synchronous enough with the real camera status, so the camera can miss the trigger
	usleep(1e5);
	
	////DEB_TRACE() << "TUCAM CreateEvent";
	pthread_cond_init(&m_hThdEvent, NULL);
	
	//@BEGIN : trigger the acquisition
	if(m_trigger_mode == IntTrig)	
	{
		DEB_TRACE() <<"Start Internal Trigger Timer";
		m_internal_trigger_timer->start();
	}
	t1 = Timestamp::now();
	double delta_time = t1 - t0;
	DEB_TRACE() << "Cap_start = " << (int) (delta_time * 1000) << " (ms)";
	t0=t1;
	AutoMutex lock(m_cond.mutex());	

	m_acq_frame_nb = 0;
	StdBufferCbMgr& buffer_mgr = m_bufferCtrlObj.getBuffer();
	buffer_mgr.setStartTimestamp(Timestamp::now());
	
	DEB_TRACE() << "Ensure that Acquisition is Started  & wait thread to be started";
	setStatus(Camera::Exposure, false);		
	//Start acquisition thread & wait 
	{
		m_wait_flag = false;
		m_quit = false;
		m_cond.broadcast();
		m_cond.wait();
	}
	
	t1 = Timestamp::now();
	delta_time = t1 - t0;
	DEB_TRACE() << "elapsed time = " << (int) (delta_time * 1000) << " (ms)";
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
	Timestamp t0 = Timestamp::now();
	Timestamp t1;
	if(m_thread_running == true)
	{
		m_wait_flag = true;
		m_cond.broadcast();		
   
		//@BEGIN : Ensure that Acquisition is Stopped before return ...			
		DEB_TRACE() << "TUCAM_Buf_AbortWait";
		
		TUCAM_Buf_AbortWait(m_opCam.hIdxTUCam);
		t1 = Timestamp::now();
		double delta_time = t1 - t0;
		DEB_TRACE() << "AbortWait = " << (int) (delta_time * 1000) << " (ms)";		
		t0 = t1;
		pthread_mutex_lock(&m_hThdLock);
		while (!m_signalled) {
		  pthread_cond_wait(&m_hThdEvent, &m_hThdLock);
		}
		m_signalled = false;
		pthread_mutex_unlock(&m_hThdLock);
		pthread_cond_destroy(&m_hThdEvent);
		t1 = Timestamp::now();
		delta_time = t1 - t0;
		DEB_TRACE() << "bordel mutex = " << (int) (delta_time * 1000) << " (ms)";		
		t0 = t1;

		// Stop capture   
		DEB_TRACE() << "TUCAM_Cap_Stop";
		TUCAM_Cap_Stop(m_opCam.hIdxTUCam);
		t1 = Timestamp::now();
		delta_time = t1 - t0;
		DEB_TRACE() << "Cap_Stop = " << (int) (delta_time * 1000) << " (ms)";		
		t0 = t1;
		//Release alloc buffer after stop capture
		DEB_TRACE() << "TUCAM_Buf_Release";
		TUCAM_Buf_Release(m_opCam.hIdxTUCam);
		m_prepared = false;
		t1 = Timestamp::now();
		delta_time = t1 - t0;
		DEB_TRACE() << "Buf_Release = " << (int) (delta_time * 1000) << " (ms)";		
		t0 = t1;
	
		//@BEGIN : trigger the acquisition
		if(m_trigger_mode == IntTrig)	
		  {
		    DEB_TRACE() <<"Stop Internal Trigger Timer";
		    m_internal_trigger_timer->stop();
		  }
		//@END
	}	
	//@BEGIN
	//now detector is ready
	DEB_TRACE() << "Ensure that Acquisition is Stopped";
	setStatus(Camera::Ready, false);
	//@END	
	
	t1 = Timestamp::now();
	double delta_time = t1 - t0;
	DEB_TRACE() << "elapsed time = " << (int) (delta_time * 1000) << " (ms)";		
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

	Timestamp t1 = Timestamp::now();
	double delta_time = t1 - t0;
	DEB_TRACE() << "readFrame : elapsed time = " << (int) (delta_time * 1000) << " (ms)";
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

		Timestamp t0_capture = Timestamp::now();

		//@BEGIN 
		DEB_TRACE() << "Capture all frames ...";
		bool continueFlag = true;
		while(continueFlag && (!m_cam.m_nb_frames || m_cam.m_acq_frame_nb < m_cam.m_nb_frames))
		{
			// Check first if acq. has been stopped
			if(m_cam.m_wait_flag)
			{
				DEB_TRACE() << "AcqThread has been stopped from user";
				continueFlag = false;
				continue;
			}

			//set status to exposure
			m_cam.setStatus(Camera::Exposure, false);
			
			//wait frame from TUCAM API ...
			if(m_cam.m_acq_frame_nb == 0)//display TRACE only once ...
			{				
				DEB_TRACE() << "TUCAM_Buf_WaitForFrame ...";
			}
			
			if(TUCAMRET_SUCCESS == TUCAM_Buf_WaitForFrame(m_cam.m_opCam.hIdxTUCam, &m_cam.m_frame))
			{
				// Grabbing was successful, process image
				m_cam.setStatus(Camera::Readout, false);

				//Prepare Lima Frame Ptr 
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
				
				//wait latency after each frame , except for the last image 
				if((!m_cam.m_nb_frames) || (m_cam.m_acq_frame_nb < m_cam.m_nb_frames) && (m_cam.m_lat_time))
				{
					////DEB_TRACE() << "Wait latency time : " << m_cam.m_lat_time * 1000 << " (ms) ...";
					usleep((DWORD) (m_cam.m_lat_time * 1000000));
				}				
			}
			else
			{
				DEB_TRACE() << "Unable to get the frame from the camera !";
			}
		}

		//
		////DEB_TRACE() << "TUCAM SetEvent";
		pthread_mutex_lock(&m_cam.m_hThdLock);
		m_cam.m_signalled = true;
		pthread_cond_signal(&m_cam.m_hThdEvent);
		pthread_mutex_unlock(&m_cam.m_hThdLock);
		//@END
		
		//stopAcq only if this is not already done		
		DEB_TRACE() << "stopAcq only if this is not already done";
		if(!m_cam.m_wait_flag)
		{
			////DEB_TRACE() << "stopAcq";
			m_cam.stopAcq();
		}

		//now detector is ready
		m_cam.setStatus(Camera::Ready, false);
		DEB_TRACE() << "AcqThread is no more running";		
		
		Timestamp t1_capture = Timestamp::now();
		double delta_time_capture = t1_capture - t0_capture;
		DEB_TRACE() << "Capture all frames elapsed time = " << (int) (delta_time_capture * 1000) << " (ms)";			

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
		case ExtTrigSingle:
		case ExtGate:
			valid_mode = true;
			break;
		case ExtTrigReadout:
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
void Camera::setGlobalGain(TucamGain gain)
{
	DEB_MEMBER_FUNCT();

	double dbVal = (double) gain;
	if(TUCAMRET_SUCCESS != TUCAM_Prop_SetValue(m_opCam.hIdxTUCam, TUIDP_GLOBALGAIN, dbVal))
	{
		THROW_HW_ERROR(Error) << "Unable to Write TUIDP_GLOBALGAIN to the camera !";
	}
}

//-----------------------------------------------------
//
//-----------------------------------------------------  
void Camera::getGlobalGain(TucamGain& gain)
{
	DEB_MEMBER_FUNCT();

	double dbVal;
	if(TUCAMRET_SUCCESS != TUCAM_Prop_GetValue(m_opCam.hIdxTUCam, TUIDP_GLOBALGAIN, &dbVal))
	{
		THROW_HW_ERROR(Error) << "Unable to Read TUIDP_GLOBALGAIN from the camera !";
	}
	gain = (TucamGain)dbVal;
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
void Camera::setOutputSignal(int port, TucamSignal signal, TucamSignalEdge edge, int delay, int width)
{
  DEB_MEMBER_FUNCT();
  TUCAM_TRGOUT_ATTR tgroutAttr;

  if (port <0 || port >2)
    {
      THROW_HW_ERROR(Error) << "Invalid output port number range is [0-2]";

    }
  tgroutAttr.nTgrOutPort = port;
  tgroutAttr.nTgrOutMode = (int)signal;
  tgroutAttr.nEdgeMode = (int)edge;
  tgroutAttr.nDelayTm = delay;
  tgroutAttr.nWidth = width;
    
  if(TUCAMRET_SUCCESS != TUCAM_Cap_SetTriggerOut (m_opCam.hIdxTUCam, tgroutAttr))
    {
      THROW_HW_ERROR(Error) << "Unable to set Output signal port "<< port;
    }
  
}

void Camera::getOutputSignal(int port, TucamSignal& signal, TucamSignalEdge& edge, int& delay, int& width)
{
  DEB_MEMBER_FUNCT();
  TUCAM_TRGOUT_ATTR tgroutAttr;

  tgroutAttr.nTgrOutPort = port;

  if(TUCAMRET_SUCCESS != TUCAM_Cap_GetTriggerOut (m_opCam.hIdxTUCam, &tgroutAttr))
    {
      THROW_HW_ERROR(Error) << "Unable to get Output signal port "<< port;
    }  
  signal = (TucamSignal) tgroutAttr.nTgrOutMode;
  edge = (TucamSignalEdge)tgroutAttr.nEdgeMode;
  delay = tgroutAttr.nDelayTm;
  width =tgroutAttr.nWidth;
}
    
