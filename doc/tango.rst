Dhyana Tango device
========================

This is the reference documentation of the Dhyana Tango device.

you can also find some useful information about the camera models/prerequisite/installation/configuration/compilation in the :ref:`Dhyana camera plugin <camera-dhyana>` section.


Properties
----------
======================== =============== ================================= =====================================
Property name	         Mandatory	 Default value	                   Description
======================== =============== ================================= =====================================
internal_trigger_timer   No              999                               Soft timer to generate software
                                                                           trigger in millisecond.
======================== =============== ================================= =====================================


Attributes
----------
======================= ======= ======================= ======================================================================
Attribute name		RW	Type			Description
======================= ======= ======================= ======================================================================
global_gain		rw	DevUShort	 	Global gain setting on the sensor, from 0 to 10
fan_speed               rw      DevUShort               FAN speed for cooling, from 0 to 10
temperature             ro      Devdouble               Temperature of the sensor
temperature_target      rw      Devdouble               Temperature target
firmware_version        ro      DevString               Firmware version
tucam_version           ro      DevString               TUCAM SDK version
======================= ======= ======================= ======================================================================

Commands
--------
=======================	======================== ======================= ===========================================
Command name		Arg. in		         Arg. out		 Description
=======================	======================== ======================= ===========================================
Init			DevVoid 	         DevVoid		 Do not use
State			DevVoid		         DevLong		 Return the device state
Status			DevVoid		         DevString		 Return the device state as a string
getAttrStringValueList	DevString:	         DevVarStringArray:	 Return the authorized string value list for
			Attribute name	         String value list	 a given attribute name
=======================	======================== ======================= ===========================================
