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

#include "DhyanaInterface.h"

using namespace lima;
using namespace lima::Dhyana;

/*******************************************************************
 * \brief Hw Interface constructor
 *******************************************************************/
Interface::Interface(Camera& cam):
m_cam(cam),
m_det_info(cam),
m_sync(cam),
m_bin(cam), 
m_roi(cam)
{
	DEB_CONSTRUCTOR();
	HwDetInfoCtrlObj *det_info = &m_det_info;
	m_cap_list.push_back(HwCap(det_info));

	HwSyncCtrlObj *sync = &m_sync;
	m_cap_list.push_back(HwCap(sync));
    
	HwBufferCtrlObj *buffer = cam.getBufferCtrlObj();
	m_cap_list.push_back(HwCap(buffer));

	HwRoiCtrlObj *roi = &m_roi;
	m_cap_list.push_back(HwCap(roi));
	
	HwBinCtrlObj *bin = &m_bin;
	m_cap_list.push_back(HwCap(bin));
}

//-----------------------------------------------------
//
//-----------------------------------------------------
Interface::~Interface()
{
	DEB_DESTRUCTOR();
	delete &m_det_info;
	delete &m_sync;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Interface::getCapList(CapList &cap_list) const
{
	DEB_MEMBER_FUNCT();
	cap_list = m_cap_list;
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Interface::reset(ResetLevel reset_level)
{
	DEB_MEMBER_FUNCT();
	DEB_PARAM() << DEB_VAR1(reset_level);

	m_cam.reset();

	Size image_size;
	m_det_info.getMaxImageSize(image_size);
	ImageType image_type;
	m_det_info.getDefImageType(image_type);
	FrameDim frame_dim(image_size, image_type);

	HwBufferCtrlObj *buffer = m_cam.getBufferCtrlObj();
	buffer->setFrameDim(frame_dim);

	buffer->setNbConcatFrames(1);
	buffer->setNbBuffers(1);
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Interface::prepareAcq()
{
	DEB_MEMBER_FUNCT();

	m_cam.prepareAcq();

}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Interface::startAcq()
{
	DEB_MEMBER_FUNCT();
	m_cam.startAcq();
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Interface::stopAcq()
{
	DEB_MEMBER_FUNCT();
	m_cam.stopAcq();
}

//-----------------------------------------------------
//
//-----------------------------------------------------
void Interface::getStatus(StatusType& status)
{

    Camera::Status camera_status = Camera::Ready;
    m_cam.getStatus(camera_status);
	
	switch (camera_status)
    {
    case Camera::Ready:
      status.set(HwInterface::StatusType::Ready);
      break;
    case Camera::Exposure:
      status.set(HwInterface::StatusType::Exposure);
      break;
    case Camera::Readout:
      status.set(HwInterface::StatusType::Readout);
      break;
    case Camera::Latency:
      status.set(HwInterface::StatusType::Latency);
      break;
    case Camera::Fault:
      status.set(HwInterface::StatusType::Fault);
    }
}

//-----------------------------------------------------
//
//-----------------------------------------------------
int Interface::getNbHwAcquiredFrames()
{
	DEB_MEMBER_FUNCT();
	int NbHwAcquiredFrames;

	NbHwAcquiredFrames = m_cam.getNbHwAcquiredFrames();

	DEB_RETURN() << DEB_VAR1(NbHwAcquiredFrames);
	return NbHwAcquiredFrames;
}
//-----------------------------------------------------
