//###########################################################################
// This file is part of LImA, a Library for Image Acquisition
//
// Copyright (C) : 2009-2011
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
//
// DhyanaCamera.h
// Created on: October 24, 2018
// Author: Arafat NOUREDDINE

#ifndef DHYANATIMER_H_
#define DHYANATIMER_H_


#include <ostream>
#include <map>
// #include <process.h>
// #include <windows.h>
#include <stdio.h>
// #include <Mmsystem.h>
#pragma comment(lib, "Winmm.lib" )

#include "DhyanaCompatibility.h"
#include "lima/Debug.h"
#include "DhyanaCamera.h"

#include <errno.h>

using namespace std;

namespace lima
{

	namespace Dhyana
	{

		class Camera;

		/******************************************************************
		*
		*******************************************************************/
		class CBaseTimer
		{
			DEB_CLASS_NAMESPC(DebModCamera, "Camera", "CBaseTimer");
		public:
			// ctor
			//------------------------------------------------------------
			CBaseTimer(int period = 1000);

			// dtor
			//------------------------------------------------------------
			~CBaseTimer();

			//------------------------------------------------------------
			static void  base_timer_proc(union sigval dwUser)
			{
				std::cout << "** CBaseTimer::base_timer_proc \n";
				//std::cout<<" ----> base_timer_proc <------- [BEGIN]"<<std::endl;
				CBaseTimer* pThis = (CBaseTimer*)dwUser.sival_ptr;
				pThis->on_timer();
				//std::cout<<" ----> base_timer_proc <------- [END]"<<std::endl;
			}

			//------------------------------------------------------------
			void start();

			//------------------------------------------------------------
			void stop();

			//------------------------------------------------------------
			virtual void on_timer() = 0;

		protected:
			// int  m_timer_id;
			long m_period_ms;
			// UINT m_resolution;
			int  m_nb_triggers;
			struct sigevent m_se;
    		struct itimerspec m_ts;
			timer_t m_timer_id;
		};

		/******************************************************************
		*
		******************************************************************/
		class CSoftTriggerTimer : public CBaseTimer
		{
			DEB_CLASS_NAMESPC(DebModCamera, "Camera", "CSoftTriggerTimer");
		public:
			//ctor
			//------------------------------------------------------------
			CSoftTriggerTimer(int period, Camera& cam);

			//dtor
			//------------------------------------------------------------
			~CSoftTriggerTimer();

			//------------------------------------------------------------
			void on_timer();

		public:
			Camera& m_cam;
		};

	} // namespace Dhyana
} // namespace lima

#endif /* DHYANATIMER_H_ */
