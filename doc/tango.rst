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
temperature_target       No              n/a                               To start cooling the detector (C)
trigger_mode             No              STANDARD                          Tucam trigger mode:
                                                                            * STANDARD
									    * GLOBAL
									    * SYNCHRONOUS
trigger_edge             No              RISING                            To set the trigger level:
                                                                            * RISING
									    * FALLING
======================== =============== ================================= =====================================


Attributes
----------
======================= ======= ======================= ======================================================================
Attribute name		RW	Type			Description
======================= ======= ======================= ======================================================================
global_gain		rw	DevString	 	Global gain setting on the sensor, HDR, HIGH or LOW
fan_speed               rw      DevUShort               FAN speed for cooling, from 0 to 10
temperature             ro      Devdouble               Temperature of the sensor
temperature_target      rw      Devdouble               Temperature target
firmware_version        ro      DevString               Firmware version
tucam_version           ro      DevString               TUCAM SDK version
trigger_mode            rw      DevString               Tucam trigger mode: STANDARD, GLOBAL or SYNCHRONOUS
trigger_edge            rw      DevString               To set the input trigger level: RISING or FALLING
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
