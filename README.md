# vision_module

Vision module is a separate process that will provide point cloud, stereo
correspondence, disparity map, etc. to the controller module.

The communication will be via network sockets for easier synchronization.
Initially I've considered shared memory (higher performance), but
this is much more difficult to manage robustly.

Incorporated libv4l2cam as robo::Camera, which is now decoupled from OpenCV.

Currently using two Logitech C920 HD Pro cameras on a powered USB 2.0 hub (Manhattan 7 port.)

## Credits

Original libv4l2cam code is based on Giacomo Spigler and George Jordanov.

## Dependencies

* OpenCV

## TODO

* Implement the following settings in robo::Camera to avoid different focus
values in two cameras:
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

* Implement camera calibration support

* Implement stereo processing

* Connect to controller module and wait for commands.

* Determine good resolution / frame rate settings for 2 x USB cameras (and
a wifi USB eth card) to avoid saturating the USB bus.

## Links / References

* [Google code Minoru Webcam](https://code.google.com/archive/p/sentience/wikis/MinoruWebcam.wiki)
* [Google code libv4l2cam](https://code.google.com/archive/p/libv4l2cam/)
* [Related OpenCV API](http://docs.opencv.org/2.4.12/modules/calib3d/doc/calib3d.html)
* [Minoru 3D webcam for real-time stereo imaging](http://robocv.blogspot.com/2012/01/minoru-3d-webcam-for-real-time-stereo.html)
* [Computing a disparity map in OpenCV](http://glowingpython.blogspot.de/2011/11/computing-disparity-map-in-opencv.html)
* [Fundamental Guide for Stereo Vision Cameras in Robotics – Tutorials and Resources](http://www.intorobotics.com/fundamental-guide-for-stereo-vision-cameras-in-robotics-tutorials-and-resources/)