/************************************************************************

*  Copyright (C) Xintu Photonics Co.,Ltd. 2012-2016. All rights reserved.

*  @file      TuCamApi.h

*  @brief     Tucsen Camera export functions header file	

*  @version	  1.0.0.0	    

*  @author    Zhang Ren

*  @date      2015-10-13 

************************************************************************/
#ifndef _TUCAM_API_H_
#define _TUCAM_API_H_

#include "TUDefine.h"

#if defined (TUCAM_TARGETOS_IS_WIN32)
    #ifdef TUCAM_EXPORTS
    #define TUCAM_API extern "C" __declspec(dllexport)
    #else
    #define TUCAM_API extern "C" __declspec(dllimport)
    #endif  // TUCAM_EXPORTS
#else
    #define  TUCAM_API
#endif      // TUCAM_TARGETOS_IS_WIN32

//
// Initialize uninitialize and misc.
//
TUCAM_API TUCAMRET TUCAM_Api_Init               (PTUCAM_INIT pInitParam);
TUCAM_API TUCAMRET TUCAM_Api_Uninit             ();

TUCAM_API TUCAMRET TUCAM_Dev_Open               (PTUCAM_OPEN pOpenParam);   
TUCAM_API TUCAMRET TUCAM_Dev_Close              (HDTUCAM hTUCam); 

// Get some device information (VID/PID/Version)
TUCAM_API TUCAMRET TUCAM_Dev_GetInfo            (HDTUCAM hTUCam, PTUCAM_VALUE_INFO pInfo);                          // PTUCAM_VALUE_INFO  call after TUCAM_Dev_Open()
TUCAM_API TUCAMRET TUCAM_Dev_GetInfoEx          (UINT32  uiICam, PTUCAM_VALUE_INFO pInfo);                          // PTUCAM_VALUE_INFO  call after TUCAM_Api_Init()            

//
// Capability control
//
TUCAM_API TUCAMRET TUCAM_Capa_GetAttr           (HDTUCAM hTUCam, PTUCAM_CAPA_ATTR pAttr);                           // PTUCAM_CAPA_ATTR
TUCAM_API TUCAMRET TUCAM_Capa_GetValue	        (HDTUCAM hTUCam, INT32 nCapa, INT32 *pnVal);                        // TUCAM_IDCAPA
TUCAM_API TUCAMRET TUCAM_Capa_SetValue	        (HDTUCAM hTUCam, INT32 nCapa, INT32 nVal);                          // TUCAM_IDCAPA
TUCAM_API TUCAMRET TUCAM_Capa_GetValueText      (HDTUCAM hTUCam, PTUCAM_VALUE_TEXT pVal);                           // PTUCAM_VALUE_TEXT

//
// Property control
//
TUCAM_API TUCAMRET TUCAM_Prop_GetAttr           (HDTUCAM hTUCam, PTUCAM_PROP_ATTR pAttr);                           // PTUCAM_PROP_ATTR
TUCAM_API TUCAMRET TUCAM_Prop_GetValue	        (HDTUCAM hTUCam, INT32 nProp, DOUBLE *pdbVal, INT32 nChn = 0);      // TUCAM_IDPROP
TUCAM_API TUCAMRET TUCAM_Prop_SetValue	        (HDTUCAM hTUCam, INT32 nProp, DOUBLE dbVal, INT32 nChn = 0);        // TUCAM_IDPROP
TUCAM_API TUCAMRET TUCAM_Prop_GetValueText      (HDTUCAM hTUCam, PTUCAM_VALUE_TEXT pVal, INT32 nChn = 0);           // PTUCAM_VALUE_TEXT

//
// Buffer control
//
TUCAM_API TUCAMRET TUCAM_Buf_Alloc              (HDTUCAM hTUCam, PTUCAM_FRAME pFrame);                              // call TUCAM_Buf_Release() to free.
TUCAM_API TUCAMRET TUCAM_Buf_Release            (HDTUCAM hTUCam);
TUCAM_API TUCAMRET TUCAM_Buf_AbortWait          (HDTUCAM hTUCam);                                                   // call after TUCAM_Buf_WaitForFrame()
TUCAM_API TUCAMRET TUCAM_Buf_WaitForFrame       (HDTUCAM hTUCam, PTUCAM_FRAME pFrame);                              // call after TUCAM_Cap_Start()
TUCAM_API TUCAMRET TUCAM_Buf_CopyFrame          (HDTUCAM hTUCam, PTUCAM_FRAME pFrame);                              // call after TUCAM_Buf_WaitForFrame()

//
// Capturing control
//
// ROI
TUCAM_API TUCAMRET TUCAM_Cap_SetROI             (HDTUCAM hTUCam, TUCAM_ROI_ATTR roiAttr);                           // call before TUCAM_Cap_Start()
TUCAM_API TUCAMRET TUCAM_Cap_GetROI             (HDTUCAM hTUCam, PTUCAM_ROI_ATTR pRoiAttr);         
// Trigger
TUCAM_API TUCAMRET TUCAM_Cap_SetTrigger         (HDTUCAM hTUCam, TUCAM_TRIGGER_ATTR tgrAttr);                       // call before TUCAM_Cap_Start()
TUCAM_API TUCAMRET TUCAM_Cap_GetTrigger         (HDTUCAM hTUCam, PTUCAM_TRIGGER_ATTR pTgrAttr);
TUCAM_API TUCAMRET TUCAM_Cap_DoSoftwareTrigger  (HDTUCAM hTUCam);                                                   // in trigger mode
// Capturing
TUCAM_API TUCAMRET TUCAM_Cap_Start              (HDTUCAM hTUCam, UINT32 uiMode);                                    // uiMode see enum TUCAM_CAPTURE_MODES
TUCAM_API TUCAMRET TUCAM_Cap_Stop               (HDTUCAM hTUCam);

//
// File control
//
// Image
TUCAM_API TUCAMRET TUCAM_File_SaveImage         (HDTUCAM hTUCam, TUCAM_FILE_SAVE fileSave);
// Profiles
TUCAM_API TUCAMRET TUCAM_File_LoadProfiles      (HDTUCAM hTUCam, PCHAR pPrfName);                                   // call before TUCAM_Cap_Start()  if after called TUCAM_Cap_Start(), must call TUCAM_Cap_Stop() first
TUCAM_API TUCAMRET TUCAM_File_SaveProfiles      (HDTUCAM hTUCam, PCHAR pPrfName);     
// Video
TUCAM_API TUCAMRET TUCAM_Rec_Start              (HDTUCAM hTUCam, TUCAM_REC_SAVE recSave);
TUCAM_API TUCAMRET TUCAM_Rec_AppendFrame        (HDTUCAM hTUCam, PTUCAM_FRAME pFrame);
TUCAM_API TUCAMRET TUCAM_Rec_Stop               (HDTUCAM hTUCam);

//
// Extened control
//
TUCAM_API TUCAMRET TUCAM_Reg_Read               (HDTUCAM hTUCam, TUCAM_REG_RW regRW);
TUCAM_API TUCAMRET TUCAM_Reg_Write              (HDTUCAM hTUCam, TUCAM_REG_RW regRW);

// Drawing contorl
TUCAM_API TUCAMRET TUCAM_Draw_Init              (HDTUCAM hTUCam, TUCAM_DRAW_INIT drawInit);
TUCAM_API TUCAMRET TUCAM_Draw_Frame             (HDTUCAM hTUCam, PTUCAM_DRAW pDrawing);
TUCAM_API TUCAMRET TUCAM_Draw_Uninit            (HDTUCAM hTUCam);

// Vendor control
TUCAM_API TUCAMRET TUCAM_Vendor_Config          (HDTUCAM hTUCam, UINT32 uiMode);
TUCAM_API TUCAMRET TUCAM_Vendor_Update          (HDTUCAM hTUCam, PTUCAM_FW_UPDATE updateFW);

#endif      // _TUCAM_API_H_