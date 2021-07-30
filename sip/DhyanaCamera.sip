namespace Dhyana
{
  class Camera
  {
%TypeHeaderCode
#include <DhyanaCamera.h>
%End

  public:
    enum Status
    {
        Ready, Exposure, Readout, Latency, Fault
    } ;

    enum TucamTriggerMode
    {
      TriggerStandard = TUCCM_TRIGGER_STANDARD,
      TriggerSynchronous = TUCCM_TRIGGER_SYNCHRONOUS,
      TriggerGlobal = TUCCM_TRIGGER_GLOBAL,
      //TriggerSoftware = TUCCM_TRIGGER_SOFTWARE, do not map, this mode is used for Lima IntTrigSingle and Timer to retrig
    };
    
    enum TucamTriggerEdge
    {
      EdgeRising = TUCTD_RISING,
      EdgeFalling = TUCTD_FAILING,
    };

    Camera(unsigned short timer_period_ms);
    virtual ~Camera();

    void prepareAcq();
    void startAcq();
    void stopAcq();
    void getStatus(Dhyana::Camera::Status& status /Out/);
    int  getNbHwAcquiredFrames();

    // -- detector info object
    void getImageType(ImageType& type /Out/);
    void setImageType(ImageType type);

    void getDetectorType(std::string& type /Out/);
    void getDetectorModel(std::string& model /Out/);
    void getDetectorImageSize(Size& size /Out/);
    void getPixelSize(double& sizex  /Out/, double& sizey /Out/);

    // -- Buffer control object
    HwBufferCtrlObj* getBufferCtrlObj();

    //-- Synch control object
    void setTrigMode(TrigMode mode);
    void getTrigMode(TrigMode& mode /Out/);
    bool checkTrigMode(TrigMode mode);

    void setExpTime(double exp_time);
    void getExpTime(double& exp_time /Out/);

    void setLatTime(double lat_time);
    void getLatTime(double& lat_time /Out/);

    void getExposureTimeRange(double& min_expo /Out/, double& max_expo /Out/) const;
    void getLatTimeRange(double& min_lat /Out/, double& max_lat /Out/) const;

    void setNbFrames(int nb_frames);
    void getNbFrames(int& nb_frames /Out/);

    //-- Related to Bin control object
    void setBin(const Bin& bin);
    void getBin(Bin& bin /Out/);
    void checkBin(Bin& bin);

    //-- Related to Roi control object
    void checkRoi(const Roi& set_roi, Roi& hw_roi);
    void setRoi(const Roi& set_roi);
    void getRoi(Roi& hw_roi /Out/);

    ///////////////////////////////
    // -- dhyana specific functions
    ///////////////////////////////

    void setTemperatureTarget(double temp);
    void getTemperatureTarget(double& temp /Out/);
    void getTemperature(double& temp /Out/);
    void setFanSpeed(unsigned speed);
    void getFanSpeed(unsigned& speed /Out/);
    void setGlobalGain(unsigned gain);
    void getGlobalGain(unsigned& gain /Out/);
    void getTucamVersion(std::string& version /Out/);
    void getFirmwareVersion(std::string& version /Out/);
    
    void getTriggerMode(TucamTriggerMode& mode /Out/);
    void setTriggerMode(TucamTriggerMode mode);
    void getTriggerEdge(TucamTriggerEdge& edge /Out/);
    void setTriggerEdge(TucamTriggerEdge edge);
    bool isAcqRunning() const;
  };
};