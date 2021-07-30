import PyTango
from Lima import Core
from Lima import Dhyana as DhyanaAcq
from Lima.Server import AttrHelper


class Dhyana(PyTango.Device_4Impl):

    Core.DEB_CLASS(Core.DebModApplication, 'LimaCCDs')


#------------------------------------------------------------------
#    Device constructor
#------------------------------------------------------------------
    def __init__(self,*args) :
        PyTango.Device_4Impl.__init__(self,*args)

        # self.__Attribute2FunctionBase = {
        # }
        
        self.init_device()

#------------------------------------------------------------------
#    Device destructor
#------------------------------------------------------------------
    def delete_device(self):
        pass

#------------------------------------------------------------------
#    Device initialization
#------------------------------------------------------------------
    @Core.DEB_MEMBER_FUNCT
    def init_device(self):
        self.set_state(PyTango.DevState.ON)
        self.get_device_properties(self.get_device_class())

        if self.temperature_target:
            _DhyanaCam.setTemperatureTarget(self.temperature_target)

#------------------------------------------------------------------
#    getAttrStringValueList command:
#
#    Description: return a list of authorized values if any
#    argout: DevVarStringArray
#------------------------------------------------------------------
    @Core.DEB_MEMBER_FUNCT
    def getAttrStringValueList(self, attr_name):
        #use AttrHelper
        return AttrHelper.get_attr_string_value_list(self, attr_name)
#==================================================================
#
#    Dhyana read/write attribute methods
#
#==================================================================
    def __getattr__(self,name) :
        #use AttrHelper
        return AttrHelper.get_attr_4u(self,name,_DhyanaCam)


#==================================================================
#
#    DhyanaClass class definition
#
#==================================================================
class DhyanaClass(PyTango.DeviceClass):

    class_property_list = {}

    device_property_list = {
        'internal_trigger_timer':
        [PyTango.DevLong,
         "Internal Trigger Timer",999],
        'temperature_target':
        [PyTango.DevDouble,
         "Temperature set point", -10],
        }

    cmd_list = {
        'getAttrStringValueList':
        [[PyTango.DevString, "Attribute name"],
         [PyTango.DevVarStringArray, "Authorized String value list"]],
        }

    attr_list = {
        'temperature':
        [[PyTango.DevDouble,
          PyTango.SCALAR,
          PyTango.READ],
         {
             'unit': 'C',
             'format': '',
             'description': 'Camera temperature',
         }],        
        'tucam_version':
        [[PyTango.DevString,
          PyTango.SCALAR,
          PyTango.READ],
         {
             'unit': 'N/A',
             'format': '',
             'description': 'TuCam SDK version',
         }],
        'firmware_version':
        [[PyTango.DevString,
          PyTango.SCALAR,
          PyTango.READ],
         {
             'unit': 'N/A',
             'format': '',
             'description': 'Camera firmware version',
         }],        
        'temperature_target':
        [[PyTango.DevDouble,
          PyTango.SCALAR,
          PyTango.READ_WRITE],
         {
             'unit': 'C',
             'format': '',
             'description': 'Temperature target',
         }],
        'global_gain':
        [[PyTango.DevUShort,
          PyTango.SCALAR,
          PyTango.READ_WRITE],
         {
             'unit': 'C',
             'format': '',
             'description': 'Global gain setting',
         }],        
        'fan_speed':
        [[PyTango.DevUShort,
          PyTango.SCALAR,
          PyTango.READ_WRITE],
         {
             'unit': 'level',
             'format': '',
             'description': 'FAN speed',
         }],        

    }

    def __init__(self,name) :
        PyTango.DeviceClass.__init__(self,name)
        self.set_type(name)

#----------------------------------------------------------------------------
# Plugins
#----------------------------------------------------------------------------
_DhyanaCam = None
_DhyanaInterface = None

def get_control(**keys) :
    global _DhyanaCam
    global _DhyanaInterface

    internal_trigger_timer = int(keys.get('internal_trigger_timer', 999))

    # print ("Dhyana internal_trigger_timer:", internal_trigger_timer)
    
    # all properties are passed as string from LimaCCDs device get_control helper
    # so need to be converted to correct type

    if _DhyanaCam is None:
        _DhyanaCam = DhyanaAcq.Camera(internal_trigger_timer)
        _DhyanaInterface = DhyanaAcq.Interface(_DhyanaCam)
    return Core.CtControl(_DhyanaInterface)

def get_tango_specific_class_n_device():
    return DhyanaClass,Dhyana
