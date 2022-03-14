[![License](https://img.shields.io/github/license/esrf-bliss/lima.svg?style=flat)](https://opensource.org/licenses/GPL-3.0)
[![Gitter](https://img.shields.io/gitter/room/esrf-bliss/lima.svg?style=flat)](https://gitter.im/esrf-bliss/LImA)
[![Conda](https://img.shields.io/conda/dn/esrf-bcu/lima-camera-dhyana.svg?style=flat)](https://anaconda.org/esrf-bcu)
[![Version](https://img.shields.io/conda/vn/esrf-bcu/lima-camera-dhyana.svg?style=flat)](https://anaconda.org/esrf-bcu)
[![Platform](https://img.shields.io/conda/pn/esrf-bcu/lima-camera-dhyana.svg?style=flat)](https://anaconda.org/esrf-bcu)

# LImA Dhyana Camera Plugin

# Lima-camera-dhyana

Lima plugin for the Tucsen Dhyana camera controlled bu TUCam Api, Tested with Dhyana 95V1 (sdk 1.0*) and Dhyana 95V2 (sdk 2.0*)

## Install

### Camera python

conda install -c esrf-bcu lima-camera-dhyana

### Camera tango device server

conda install -c tango-controls -c esrf-bcu lima-camera-dhyana-tango

# LImA

Lima ( **L** ibrary for **Im** age **A** cquisition) is a project for the unified control of 2D detectors. The aim is to clearly separate hardware specific code from common software configuration and features, like setting standard acquisition parameters (exposure time, external trigger), file saving and image processing.

Lima is a C++ library which can be used with many different cameras. The library also comes with a [Python](http://python.org) binding and provides a [PyTango](http://pytango.readthedocs.io/en/stable/) device server for remote control.

## Documentation

The documentation is available [here](https://lima.blissgarden.org)