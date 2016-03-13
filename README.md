# vision_module

Porting libv4l2cam for my robot code base to be used as a dual/stereo
camera setup.

Original libv4l2cam code is based on Giacomo Spigler and George Jordanov.

## Warning

OpenCV is compiled with rtti and exceptions while robot code base
is not. Here libcam is modified so that it no longer depends
on OpenCV. The test code (test.cpp) is OK for now, but be warned
that test.cpp relies on rtti/exceptions enabled (due to OpenCV)
while libcam expects rtti/exceptions disabled as per the robot code base.

## TODO

* Implement the following settings in libcam:
```
	#
	# disable auto-focus
	#
	v4l2-ctl -d 0 -c focus_auto=0
	v4l2-ctl -d 1 -c focus_auto=0

	#
	# actual focus value to be determined empirically?
	#
	v4l2-ctl -d 0 -c focus_absolute=0
	v4l2-ctl -d 1 -c focus_absolute=0
```

* This process upon startup should try to connect to controller
module and wait for commands.
