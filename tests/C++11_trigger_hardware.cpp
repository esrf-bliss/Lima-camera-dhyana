// C++11_trigger_hardware.cpp : 定义控制台应用程序的入口点。
//

#include <cstdio>
#include <chrono>
#include <iostream>

using namespace std::chrono;
using Clock = std::chrono::steady_clock;

#include <TUCamApi.h>                         // 导入SDK头文件

static TUCAM_INIT m_itApi;       // SDK API initialized object
static TUCAM_OPEN m_opCam;       // Open camera object
static TUCAM_FRAME m_frame;      // The frame object
static TUCAM_TRIGGER_ATTR m_tgr; // The trigger object

/* Init the TUCAM API */
TUCAMRET InitApi()
{
	/* Get the current directory */
	m_itApi.uiCamCount = 0;
	m_itApi.pstrConfigPath = "./";

	TUCAM_Api_Init(&m_itApi);

	printf("Connect %d camera\r\n",m_itApi.uiCamCount);

	if (0 == m_itApi.uiCamCount)
	{
		return TUCAMRET_NO_CAMERA;
	}

	return TUCAMRET_SUCCESS;
}

/* UnInit the TUCAM API */
TUCAMRET UnInitApi()
{
	return TUCAM_Api_Uninit();
}

/* Open the camera by index number */
TUCAMRET OpenCamera(UINT uiIdx)
{
	if (uiIdx >= m_itApi.uiCamCount)
	{
		return TUCAMRET_OUT_OF_RANGE;
	}

	m_opCam.uiIdxOpen = uiIdx;

	return TUCAM_Dev_Open(&m_opCam);
}

/* Close the current camera */
TUCAMRET CloseCamera()
{
	if (NULL != m_opCam.hIdxTUCam)
	{
		TUCAM_Dev_Close(m_opCam.hIdxTUCam);
	}

	printf("Close the camera success\r\n");

	return TUCAMRET_SUCCESS;
}

/* Do hardware trigger */
static void DoHardwareTrigger()
{
	int nTimes = 10;

	m_frame.pBuffer = NULL;
	m_frame.ucFormatGet = TUFRM_FMT_USUAl;
	m_frame.uiRsdSize = 1;

	// set 100ms exposure 
	if(TUCAMRET_SUCCESS != TUCAM_Prop_SetValue(m_opCam.hIdxTUCam, TUIDP_EXPOSURETM, 100))
	{
	  printf("Unable to Write TUIDP_EXPOSURETM to the camera !");
	}
	TUCAM_Cap_GetTrigger(m_opCam.hIdxTUCam, &m_tgr);
	m_tgr.nTgrMode = (INT32)TUCCM_TRIGGER_STANDARD;
	/* how many frames do you want to capture to RAM(the frames less than 0, use maximum frames ) */
	//m_tgr.nfFrames = 1;
	m_tgr.nFrames = 1;  
	//m_tgr.nBufFrames = 1;  
	m_tgr.nDelayTm = 0;
	m_tgr.nExpMode = (INT32)TUCTE_EXPTM;
	m_tgr.nEdgeMode = (INT32)TUCTD_RISING;
	TUCAM_Cap_SetTrigger(m_opCam.hIdxTUCam, m_tgr);

	// will take 3 times 10 frames
	for (int j = 0; j < 10; j++)
	{
	  TUCAM_Buf_Alloc(m_opCam.hIdxTUCam, &m_frame);
	
	  if (TUCAMRET_SUCCESS == TUCAM_Cap_Start(m_opCam.hIdxTUCam, (UINT32)TUCCM_TRIGGER_STANDARD))
	  {
		for (int i = 0; i < nTimes; ++i)
		{
			if (TUCAMRET_SUCCESS == TUCAM_Buf_WaitForFrame(m_opCam.hIdxTUCam, &m_frame,10000))
			{
				/* After call TUCAM_Buf_WaitForFrame your hardware can send the trigger signal */
				/* if m_tgr.nFrames == 1, the SDK create 1 frame memory to receive one trigger signal and the TUCAM_Buf_WaitForFrame interface return once */
				/* if m_tgr.nFrames == N, the SDK create N frames memory to receive N times trigger signals, the TUCAM_Buf_WaitForFrame interface return, and you call TUCAM_Buf_WaitForFrame N-1 times to grab the frame */
				/* if m_tgr.nFrames < 0, the SDK create Max frames memory to receive trigger signals */

				printf("Grab the hardware trigger frame success, index number is %d, width:%d, height:%d, channel:%d, depth:%d, image size:%d\r\n", i, m_frame.usWidth, m_frame.usHeight, m_frame.ucChannels, (2 == m_frame.ucElemBytes) ? 16 : 8, m_frame.uiImgSize);
			}
			else
			{
				printf("Grab the hardware trigger frame failure, index number is %d\r\n", i);
			}
		}

		TUCAM_Buf_AbortWait(m_opCam.hIdxTUCam);

		auto tic = Clock::now();	      
		TUCAM_Cap_Stop(m_opCam.hIdxTUCam);
		auto toc = Clock::now();
		std::cout << "TUCAM_Cap_Stop() took " << duration_cast<milliseconds>(toc-tic).count() /1000.0 << " seconds" << std::endl;
	  }

	  TUCAM_Buf_Release(m_opCam.hIdxTUCam);
	}
}

int main(int argc, char* argv[])
{
	if (TUCAMRET_SUCCESS == InitApi())
	{
		if (TUCAMRET_SUCCESS == OpenCamera(0))
		{
			printf("Open the camera success\r\n");
			printf("\r\n");
			DoHardwareTrigger();
			printf("\r\n");
			CloseCamera();
		}
		else
		{
			printf("Open the camera failure\r\n");
		}

		UnInitApi();
	}

	printf("Press any key to exit...\r\n");
	getchar();

	return 0;
}

