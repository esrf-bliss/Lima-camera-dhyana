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

        # self.__TestImageSelector = {'TESTIMAGE_OFF': DhyanaAcq.Camera.TestImage_Off,
        #                             'TESTIMAGE_1': DhyanaAcq.Camera.TestImage_1,
        #                             'TESTIMAGE_2': DhyanaAcq.Camera.TestImage_2,
        #                             'TESTIMAGE_3': DhyanaAcq.Camera.TestImage_3,
        #                             'TESTIMAGE_4': DhyanaAcq.Camera.TestImage_4,
        #                             'TESTIMAGE_5': DhyanaAcq.Camera.TestImage_5,
        #                             'TESTIMAGE_6': DhyanaAcq.Camera.TestImage_6,
        #                             'TESTIMAGE_7': DhyanaAcq.Camera.TestImage_7,
        # }
        
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
        # define one and only one of the following 4 properties:
        'internal_trigger_timer':
        [PyTango.DevLong,
         "Internal Trigger Timer",0],
        }

    cmd_list = {
        'getAttrStringValueList':
        [[PyTango.DevString, "Attribute name"],
         [PyTango.DevVarStringArray, "Authorized String value list"]],
        }

    attr_list = {
        'statistics_total_buffer_count':
        [[PyTango.DevLong,
          PyTango.SCALAR,
          PyTango.READ],
         {
             'unit': 'N/A',
             'format': '',
             'description': 'total number of frame requested',
         }],        
        'statistics_failed_buffer_count':
        [[PyTango.DevLong,
          PyTango.SCALAR,
          PyTango.READ],
         {
             'unit': 'N/A',
             'format': '',
             'description': 'total number of failed frame',
         }],        
        'test_image_selector':
        [[PyTango.DevString,
          PyTango.SCALAR,
          PyTango.READ_WRITE],
         {
             'unit': 'N/A',
             'format': '',
             'description': 'select a test image image_off/image_1/.../image_7',
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

# packet_size = 8000 suppose the eth MTU is set at least to 8192 (Jumbo mode !)
# otherwise frame transfer can failed, the package size must but
# correspond to the MTU, see README file under Pylon-3.2.2 installation
# directory for for details about network optimization.

def get_control(frame_transmission_delay = 0, inter_packet_delay = 0,
                packet_size = 8000,force_video_mode= 'false', **keys) :
    global _DhyanaCam
    global _DhyanaInterface

    # if 'internal_trigger_timer' in keys:
    #     internal_trigger_timer = keys['internal_trigger_timer']

    # print ("Dhyana internal_trigger_timer:", internal_trigger_timer)
    
    # all properties are passed as string from LimaCCDs device get_control helper
    # so need to be converted to correct type

    if _DhyanaCam is None:
        _DhyanaCam = DhyanaAcq.Camera(999)
        _DhyanaInterface = DhyanaAcq.Interface(_DhyanaCam)
    return Core.CtControl(_DhyanaInterface)

def get_tango_specific_class_n_device():
    return DhyanaClass,Dhyana