#include <iostream>
#include <DhyanaCamera.h>
#include <DhyanaInterface.h>
#include <lima/CtControl.h>
#include <lima/CtSaving.h>
#include <lima/CtImage.h>
#include <lima/CtAcquisition.h>
#include <chrono>

using namespace lima::Dhyana;
using namespace lima;


int main (void){
    // A camera instance and its hardware interface
    unsigned short int_trigger_tim = 999;
    int expo = 0.5;
    int nframe = 10;
    Camera simu(int_trigger_tim);
    Interface hw(simu);

    // // The control object
    CtControl ct = CtControl(&hw);

    // // Get the saving control and set some properties
    CtSaving *save = ct.saving();
    save->setDirectory("./data");
    save->setPrefix("test_");
    save->setSuffix(".edf");
    save->setNextNumber(1);
    save->setFormat(CtSaving::EDF);
    save->setSavingMode(CtSaving::AutoFrame);
    save->setFramesPerFile(1);

    // // Set the binning or any other processing
    // Bin bin(2, 2);
    // CtImage *image = ct.image();
    // image->setBin(bin);

    // // Get the acquisition control and set some properties
    CtAcquisition *acq = ct.acquisition();
    acq->setAcqMode(Single);
    acq->setAcqExpoTime(expo);
    acq->setAcqNbFrames(nframe);
    // acq->setLatencyTime(1);

    // // Prepare acquisition (transfer properties to the camera)
    ct.prepareAcq();

    // // // Start acquisition
    ct.startAcq();
    std::cout << "SIMUTEST: acq started" << std::endl;

    //
    long frame = -1;
    while (frame < (nframe - 1))
    {
        using namespace std::chrono;

        high_resolution_clock::time_point begin = high_resolution_clock::now();

        usleep(1000000);

        CtControl::ImageStatus img_status;
        ct.getImageStatus(img_status);

        high_resolution_clock::time_point end = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(end - begin).count();

        std::cout << "SIMUTEST: acq frame nr " << img_status.LastImageAcquired
                << " - saving frame nr " << img_status.LastImageSaved << std::endl;

        if (frame != img_status.LastImageAcquired) {
                unsigned int nb_frames = img_status.LastImageAcquired - frame;

                std::cout << "  " << 1e6 * nb_frames / duration << " fps" << std::endl;

                frame = img_status.LastImageAcquired;
        }
    }
    std::cout << "SIMUTEST: acq finished" << std::endl;

    // Stop acquisition ( not really necessary since all frames where acquired)
    ct.stopAcq();

    std::cout << "SIMUTEST: acq stopped" << std::endl;
}