/*
 * Copyright (C) 2009 Giacomo Spigler
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
 */
#include <unistd.h>
#include <iostream>
#include "libcam.h"
#include <cv.h>
#include <highgui.h>

//#define RASP

using namespace std;

int main() {

  int res = 0;

/*
  int ww = 800;
  int hh = 600;
*/
  int ww = 640;
  int hh = 480;

  Camera c1;
  Camera c2;

  c1.initialize("/dev/video0", ww, hh, 15);
  c2.initialize("/dev/video1", ww, hh, 15);

//cout<<c.setSharpness(3)<<"   "<<c.minSharpness()<<"  "<<c.maxSharpness()<<" "<<c.defaultSharpness()<<endl;

#ifndef RASP
  cvNamedWindow("1", CV_WINDOW_AUTOSIZE);
  cvNamedWindow("2", CV_WINDOW_AUTOSIZE);
#endif

  IplImage *l1 = cvCreateImage(cvSize(ww, hh), 8, 3);
  IplImage *l2 = cvCreateImage(cvSize(ww, hh), 8, 3);

  int i1 = 0;
  int i2 = 0;

  while(1){

	fprintf(stderr, "loop\n");

	if (!i1) {
		fprintf(stderr, "trying 1\n");
	    if (!c1.update(0)) {
			fprintf(stderr, "got 1\n");
			i1 = 1;
		    c1.toIplImage((unsigned char *)l1->imageData, l1->width);
		}
	}

	usleep(10000);

	if (!i2) {
		fprintf(stderr, "trying 2\n");
		if (!c2.update(0)) {
			fprintf(stderr, "got 2\n");
			i2 = 1;
		    c2.toIplImage((unsigned char *)l2->imageData, l2->width);
		}
	}

	usleep(10000);

#ifndef RASP
    cvShowImage("1", l1);
    cvShowImage("2", l2);

    if( (cvWaitKey(10) & 255) == 27 ) break;
#else
	if (i1 && i2)
		break;
#endif
  }

#ifndef RASP
  cvDestroyWindow("1");
  cvDestroyWindow("2");
#else
  cvSaveImage("1.png", l1);
  cvSaveImage("2.png", l2);
#endif

  cvReleaseImage(&l1);
  cvReleaseImage(&l2);

  c1.shutdown();
  c2.shutdown();

  return 0;
}



