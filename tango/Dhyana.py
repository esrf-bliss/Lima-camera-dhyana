###########################################################################
# This file is part of LImA, a Library for Image Acquisition
#
#  Copyright (C) : 2009-2026
#  European Synchrotron Radiation Facility
#  CS40220 38043 Grenoble Cedex 9
#  FRANCE
#
#  Contact: lima@esrf.fr
#
#  This is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 3 of the License, or
#  (at your option) any later version.
#
#  This software is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, see <http://www.gnu.org/licenses/>.
############################################################################
#=============================================================================
#
# file :        Dhyana.py
#
# description : Python source for the Basler and its commands.
#                The class is derived from Device. It represents the
#                CORBA servant object which will be accessed from the
#                network. All commands which can be executed on the
#                Dhyana are implemented in this file.
#
# project :     TANGO Device Server
#
#=============================================================================
from tango import AttrWriteType, DevState
from tango.server import Device, attribute, command, device_property

from lima import core
from lima import dhyana as DhyanaAcq
from lima.server import AttrHelper


class Dhyana(Device):

    core.DEB_CLASS(core.DebModule.DebModApplication, 'LimaCCDs')

    # ------------------------------------------------------------------
    #    Static properties
    # ------------------------------------------------------------------
    _DhyanaCam = None
    """Reference to Lima dhyana camera binding"""
    _DhyanaInterface = None
    """Reference to Lima dhyana interface binding"""

    _TriggerMode = {
        'STANDARD': DhyanaAcq.Camera.TucamTriggerMode.TriggerStandard,
        'GLOBAL': DhyanaAcq.Camera.TucamTriggerMode.TriggerGlobal,
        'SYNCHRONOUS': DhyanaAcq.Camera.TucamTriggerMode.TriggerSynchronous,
    }
    _TriggerEdge = {
        'RISING': DhyanaAcq.Camera.TucamTriggerEdge.EdgeRising,
        'FALLING': DhyanaAcq.Camera.TucamTriggerEdge.EdgeFalling,
    }
    _GlobalGain = {
        'HDR': DhyanaAcq.Camera.TucamGain.GainHDR,
        'HIGH': DhyanaAcq.Camera.TucamGain.GainHigh,
        'LOW': DhyanaAcq.Camera.TucamGain.GainLow,
    }

    # ------------------------------------------------------------------
    #    Device properties (initial hardware configuration only - the
    #    live attributes of the same name below read/write the camera
    #    directly, they don't share storage with these)
    # ------------------------------------------------------------------
    internal_trigger_timer = device_property(dtype=int, default_value=999)
    temperature_target = device_property(dtype=float, default_value=-10)
    trigger_mode = device_property(dtype=str, default_value="STANDARD")
    trigger_edge = device_property(dtype=str, default_value="RISING")

    # ------------------------------------------------------------------
    #    Device initialization
    # ------------------------------------------------------------------
    @core.DEB_MEMBER_FUNCT
    def init_device(self):
        Device.init_device(self)

        # Leagcy code used by AttrHelper.get_attr_string_value_list
        self.__TriggerMode = self._TriggerMode
        self.__TriggerEdge = self._TriggerEdge
        self.__GlobalGain = self._GlobalGain

        if self.temperature_target:
            self._DhyanaCam.setTemperatureTarget(self.temperature_target)
        if self.trigger_mode:
            self._DhyanaCam.setTriggerMode(self._TriggerMode[self.trigger_mode.upper()])
        if self.trigger_edge:
            self._DhyanaCam.setTriggerEdge(self._TriggerEdge[self.trigger_edge.upper()])

        self.set_state(DevState.ON)

    # ------------------------------------------------------------------
    #    Commands
    # ------------------------------------------------------------------
    @command(dtype_in=str, dtype_out=(str,))
    @core.DEB_MEMBER_FUNCT
    def getAttrStringValueList(self, attr_name):
        return AttrHelper.get_attr_string_value_list(self, attr_name)

    @command(
        dtype_in=int,
        doc_in="port(0-2)",
        dtype_out=(int,),
        doc_out="[signal(0-GROUND, 1-VCC, 2-IN, 3-EXPSTART, 4-EXPGLOBAL, 5-READEND), "
        "edge(0-RISING,1-FALLING), delay(in ms), width(in ms)",
    )
    @core.DEB_MEMBER_FUNCT
    def getOutputSignal(self, port):
        signal, edge, delay, width = self._DhyanaCam.getOutputSignal(port)
        return [signal.value, edge.value, delay, width]

    @command(
        dtype_in=(int,),
        doc_in="[port(0-2), signal(0-GROUND, 1-VCC, 2-IN, 3-EXPSTART, 4-EXPGLOBAL, "
        "5-READEND), edge(0-RISING,1-FALLING), delay(in ms), width(in ms)",
    )
    @core.DEB_MEMBER_FUNCT
    def setOutputSignal(self, signal_conf):
        port = signal_conf[0]
        signal = DhyanaAcq.Camera.TucamSignal(int(signal_conf[1]))
        edge = DhyanaAcq.Camera.TucamSignalEdge(int(signal_conf[2]))
        delay = signal_conf[3]
        width = signal_conf[4]

        self._DhyanaCam.setOutputSignal(port, signal, edge, delay, width)

    # ------------------------------------------------------------------
    #    Attributes with a name colliding with a device_property
    # ------------------------------------------------------------------
    _temperature_target_fget, _temperature_target_fset = AttrHelper.make_fget_fset(
        "temperature_target", lambda self: self._DhyanaCam
    )
    temperature_target_attr = attribute(
        name="temperature_target",
        dtype=float,
        access=AttrWriteType.READ_WRITE,
        unit="C",
        doc="Temperature target",
        fget=_temperature_target_fget,
        fset=_temperature_target_fset,
    )

    _trigger_mode_fget, _trigger_mode_fset = AttrHelper.make_fget_fset(
        "trigger_mode", lambda self: self._DhyanaCam, enum=_TriggerMode
    )
    trigger_mode_attr = attribute(
        name="trigger_mode",
        dtype=str,
        access=AttrWriteType.READ_WRITE,
        unit="mode",
        doc="Tucam trigger mode (standard, global or synchronize)",
        fget=_trigger_mode_fget,
        fset=_trigger_mode_fset,
    )

    _trigger_edge_fget, _trigger_edge_fset = AttrHelper.make_fget_fset(
        "trigger_edge", lambda self: self._DhyanaCam, enum=_TriggerEdge
    )
    trigger_edge_attr = attribute(
        name="trigger_edge",
        dtype=str,
        access=AttrWriteType.READ_WRITE,
        unit="edge",
        doc="Detection edge mode, rising or falling",
        fget=_trigger_edge_fget,
        fset=_trigger_edge_fset,
    )

    # ------------------------------------------------------------------
    #    Attributes fully dispatched by naming convention
    # ------------------------------------------------------------------
    _temperature_fget, _ = AttrHelper.make_fget_fset("temperature", lambda self: self._DhyanaCam)
    temperature = attribute(
        dtype=float,
        access=AttrWriteType.READ,
        unit="C",
        doc="Camera temperature",
        fget=_temperature_fget,
    )

    _tucam_version_fget, _ = AttrHelper.make_fget_fset(
        "tucam_version", lambda self: self._DhyanaCam
    )
    tucam_version = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        doc="TuCam SDK version",
        fget=_tucam_version_fget,
    )

    _firmware_version_fget, _ = AttrHelper.make_fget_fset(
        "firmware_version", lambda self: self._DhyanaCam
    )
    firmware_version = attribute(
        dtype=str,
        access=AttrWriteType.READ,
        doc="Camera firmware version",
        fget=_firmware_version_fget,
    )

    _global_gain_fget, _global_gain_fset = AttrHelper.make_fget_fset(
        "global_gain", lambda self: self._DhyanaCam, enum=_GlobalGain
    )
    global_gain = attribute(
        dtype=str,
        access=AttrWriteType.READ_WRITE,
        doc="Global gain setting",
        fget=_global_gain_fget,
        fset=_global_gain_fset,
    )

    _fan_speed_fget, _fan_speed_fset = AttrHelper.make_fget_fset(
        "fan_speed", lambda self: self._DhyanaCam
    )
    fan_speed = attribute(
        dtype=int,
        access=AttrWriteType.READ_WRITE,
        unit="level",
        doc="FAN speed",
        fget=_fan_speed_fget,
        fset=_fan_speed_fset,
    )


#----------------------------------------------------------------------------
# Plugins
#----------------------------------------------------------------------------

def get_control(_Dhyana=Dhyana, _Camera=DhyanaAcq.Camera, _Interface=DhyanaAcq.Interface, **keys):
    camera = _Dhyana._DhyanaCam
    interface = _Dhyana._DhyanaInterface

    internal_trigger_timer = int(keys.get('internal_trigger_timer', 999))

    if camera is None:
        camera = _Camera(internal_trigger_timer)
        interface = _Interface(camera)

    _Dhyana._DhyanaCam = camera
    _Dhyana._DhyanaInterface = interface

    return core.CtControl(interface)

def get_tango_specific_class_n_device():
    return Dhyana.TangoClassClass, Dhyana
