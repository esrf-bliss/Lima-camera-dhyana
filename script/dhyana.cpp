#include <TUCamApi.h>
#include <TUDefine.h>

void SetCurrentExposureTime(double dbVal) {
    // dbVal returns the current exposure time in ms
    TUCAM_Prop_SetValue(opCam.hIdxTUCam, TUIDP_EXPOSURETM, dbVal);
}

int main (int argc, char** argv) {
    TUCAM_INIT itApi; // Initializing SDK environment parameters 
    TUCAM_OPEN opCam; // Open camera parameters
    itApi.pstrConfigPath = NULL;
    itApi.uiCamCount = 0;
    if (TUCAMRET_SUCCESS != TUCAM_Api_Init(&itApi)) {
        // Initializing SDK API environment failed
        return 0;
    }
    if (0 == itApi.uiCamCount) {
        // No camera
        return 0;
    }
    opCam.hIdxTUCam = 0;
    opCam.uiIdxOpen = 0;
    if (TUCAMRET_SUCCESS != TUCAM_Dev_Open(&opCam)) {
        // Failed to open camera
        return 0;
    }
    TUCAM_Capa_SetValue(m_opCam.hIdxTUCam, TUIDC_PIXELCLOCK, 0);
    



    // Application can use the handle of opCam.hIdxTUCam
    TUCAM_Dev_Close(opCam.hIdxTUCam); // Close camera 
    TUCAM_Api_Uninit(); // Initializing SDK API environment
    return 0;
}