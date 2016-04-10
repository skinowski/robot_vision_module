/*
 * Copyright (C) 2009 Giacomo Spigler
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
 */
#include "common.h"
#include "camera.h"
#include "server.h"

#include <cv.h>
#include <highgui.h>

#include <unistd.h>

#ifdef __arm__
#define RASPBERRY
#endif

using namespace robo;

// these should goto config.json/yaml
const char *UDS_PATH = "/tmp/robo.vision.s";

const char *VIDEO_0 = "/dev/video0";
const char *VIDEO_1 = "/dev/video1";

const char *VIDEO_0_IMG = "img1.png";
const char *VIDEO_1_IMG = "img2.png";

/*
int ww = 800;
int hh = 600;
*/
int ww = 640;
int hh = 480;

int max_attempts = 100;
int attemp_sleep_usec = 1000;

int main() {

    int res = 0;
    uint64_t iterations = 0;

    Camera c1;
    Camera c2;
    Server srv;

    res = srv.initialize(UDS_PATH);
    if (res)
        return res;

    /* POC Code Below, pulls two images and saved them. Or if not
    on raspberry, then displays them. */

    c1.initialize(VIDEO_0, ww, hh, 15);
    c2.initialize(VIDEO_1, ww, hh, 15);

    #ifndef RASPBERRY
    cvNamedWindow(VIDEO_0, CV_WINDOW_AUTOSIZE);
    cvNamedWindow(VIDEO_1, CV_WINDOW_AUTOSIZE);
    #endif

    IplImage *l1 = cvCreateImage(cvSize(ww, hh), 8, 3);
    IplImage *l2 = cvCreateImage(cvSize(ww, hh), 8, 3);

    while (1) {

        ++iterations;

        proto::Request  request;
        proto::Response response;

        // srv operations can block forever
        res = srv.get_request(request);
        if (res)
            break;

        response.trx_id = request.trx_id;
        response.cmd    = request.cmd;
        response.data   = 0;

        if (request.cmd == proto::CMD_PING) {
            // ignore failure, show must go on  
            srv.send_response(response);
            continue;
        }

        if (request.cmd == proto::CMD_EXIT) {
            logger(LOG_INFO, "Exit cmd received");
            break;
        }
      
        if (request.cmd != proto::CMD_GET_MAP) {
            logger(LOG_ERROR, "Invalid cmd=%u trx_id=%u", request.cmd, request.trx_id);
            continue;
        }

        logger(LOG_TRACE, "Loop");

        // Below is super flawed. Time difference between two captures is important, but
        // for now (in our case where things are not "moving", we are OK). Remember
        // this is an indoor toy robot and we do not expect zipping objects or basketball
        // players, runners, cats, dogs, bees, etc.

        int attempts = max_attempts;

        while (--attempts > 0) {
            if (!c1.update(0))
                break;
            usleep(attemp_sleep_usec);
        }
        while (--attempts > 0) {
            if (!c2.update(0))
                break;
            usleep(attemp_sleep_usec);
        }

        if (attempts <= 0) {
            logger(LOG_ERROR, "Failed capturing images in %d attempts", max_attempts);
            break;
        }

        c1.toIplImage((unsigned char *)l1->imageData, l1->width);
        c2.toIplImage((unsigned char *)l2->imageData, l2->width);

        #ifndef RASPBERRY
        cvShowImage(VIDEO_0, l1);
        cvShowImage(VIDEO_1, l2);
        if((cvWaitKey(10) & 255) == 27)
            break;
        #endif

        response.data = iterations; /* dummy, TODO pass actual data here when impl is ready */
        // ignore res, show must go on...
        srv.send_response(response);
    }

    logger(LOG_INFO, "Exiting");

    #ifndef RASPBERRY
    cvDestroyWindow(VIDEO_0);
    cvDestroyWindow(VIDEO_1);
    #else
    /* DEMO CODE, remove this when impl is ready to send data via srv */
    cvSaveImage(VIDEO_0_IMG, l1);
    cvSaveImage(VIDEO_1_IMG, l2);
    #endif

    cvReleaseImage(&l1);
    cvReleaseImage(&l2);

    c1.shutdown();
    c2.shutdown();
    srv.shutdown();

    return 0;
}



