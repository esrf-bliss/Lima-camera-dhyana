namespace Dhyana
{
  class Interface : HwInterface
  {
%TypeHeaderCode
#include <DhyanaInterface.h>
%End
  public:
    Interface(Dhyana::Camera& cam /KeepReference/);
    virtual void getCapList(std::vector<HwCap> &cap_list /Out/) const;
    virtual void reset(ResetLevel reset_level);
    virtual void prepareAcq();
    virtual void startAcq();
    virtual void stopAcq();
    virtual void getStatus(StatusType& status /Out/);
    virtual int getNbHwAcquiredFrames();
  };
};