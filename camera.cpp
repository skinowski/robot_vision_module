/*
 Copyright (C) 2009 Giacomo Spigler
 2013 - George Jordanov - improve in performance for HSV conversion and improvements
 2016 - Tolga Ceylan - ported to robot code with limited mmap only features.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "camera.h"

#include <string.h>
#include <assert.h>

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <malloc.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>          /* for videodev2.h */

#include <linux/videodev2.h>

namespace robo {

static int g_lookup_done;
static unsigned char g_yv[256][256];
static unsigned char g_yu[256][256];
static int g_y2v[256][256];
static int g_y2u[256][256];

const size_t g_num_of_bufs = 4;

static void create_YUV_RGB_table()
{
    if (g_lookup_done)
        return;

    int iyv, iyu, iy2v, iy2u;
    int i, j;

    for(i = 0; i < 256; i++) {
        for(j = 0; j < 256; j++) {

            iyv = i + (1.370705 * (j-128));  //Red
            iyu = i + (1.732446 * (j-128)); //Blue
            iy2v = (i/2) - (0.698001 * (j-128));//Green 1/2
            iy2u = (i/2) - (0.337633 * (j-128));//Green 1/2

            if (iyv > 255) iyv = 255;
            if (iyu > 255) iyu = 255;
            if (iyv < 0) iyv = 0;
            if (iyu < 0) iyu = 0;
            
            g_yv[i][j] = (unsigned char) iyv;
            g_yu[i][j] = (unsigned char) iyu;

            g_y2v[i][j] = iy2v;
            g_y2u[i][j] = iy2u;
        }
    }

    g_lookup_done = 1;
}

static int query_device(const char *name, const char *tag, int fd, struct v4l2_queryctrl *queryctrl)
{
    assert(fd != -1);
    assert(name);
    assert(tag);
    assert(queryctrl);

    int res = HANDLE_EINTR(::ioctl(fd, VIDIOC_QUERYCTRL, queryctrl));
    if (res) {
        logger(LOG_WARN, "%s VIDIOC_QUERYCTRL (%s) res=%d errno=%d", name, tag, res, errno);
        return EFAULT;
    }

    if (queryctrl->flags & V4L2_CTRL_FLAG_DISABLED) {
        logger(LOG_WARN, "%s %s is not supported", name, tag);
        return EFAULT;
    }
    return 0;
}

static int control_device(const char *name, const char *tag, int fd, struct v4l2_control *control)
{
    int res = 0;
    res = HANDLE_EINTR(::ioctl(fd, VIDIOC_S_CTRL, control));
    if (res)
        logger(LOG_ERROR, "%s VIDIOC_S_CTRL (%s) res=%d errno=%d", name, tag, res, errno);
    return res;
}

Camera::Camera()
  :
  m_width_h(0),
  m_height(0),
  m_data(NULL),
  m_fd(-1),
  m_name(NULL),
  m_buffers(NULL)
{
    memset(m_settings, 0, sizeof(m_settings));

    m_settings[SETTING_BRIGHTNESS].vl_id  = V4L2_CID_BRIGHTNESS;
    m_settings[SETTING_CONTRAST].vl_id    = V4L2_CID_CONTRAST;
    m_settings[SETTING_SATURATION].vl_id  = V4L2_CID_SATURATION;
    m_settings[SETTING_HUE].vl_id         = V4L2_CID_HUE;
    m_settings[SETTING_HUE_AUTO].vl_id    = V4L2_CID_HUE_AUTO;
    m_settings[SETTING_SHARPNESS].vl_id   = V4L2_CID_SHARPNESS;

    m_settings[SETTING_BRIGHTNESS].tag  = "brightness";
    m_settings[SETTING_CONTRAST].tag    = "contrast";
    m_settings[SETTING_SATURATION].tag  = "saturation";
    m_settings[SETTING_HUE].tag         = "hue";
    m_settings[SETTING_HUE_AUTO].tag    = "hue auto";
    m_settings[SETTING_SHARPNESS].tag   = "sharpness";
}

int Camera::initialize(const char *name, int w, int h, int f) 
{
    assert(name);
    assert(w > 0);
    assert(h > 0);
    assert(f > 0);

    if (m_buffers)
        return EINVAL;

    int res = 0;

    create_YUV_RGB_table();

    m_width_h = w / 2;
    m_height = h;
    m_name = name;

    m_buffers = (Buffer *)::calloc(g_num_of_bufs, sizeof (*m_buffers));
    if (!m_buffers) {
        res = ENOMEM;
        goto fail;
    }

    m_data = (unsigned char *)::malloc(w * h * 4);
    if (!m_data)
      res = ENOMEM;
    res = res || open_cam_device();
    res = res || initialize_device(f);
    res = res || init_mmap();
    res = res || start_capture();
    if (res)
        goto fail;

    return 0;

fail:
    shutdown();
    return res;
}

Camera::~Camera() {
  shutdown();
}

void Camera::shutdown()
{
    int res = 0;

    const char *tag = m_name ? m_name : "N/A";

    if (m_fd != -1) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        res = HANDLE_EINTR(::ioctl(m_fd, VIDIOC_STREAMOFF, &type));
        if (res)
          logger(LOG_WARN, "%s VIDIOC_STREAMOFF res=%d errno=%d", tag, res, errno);
    }

    if (m_buffers) {
      
        for(size_t i = 0; i < g_num_of_bufs; ++i) {
            res = ::munmap(m_buffers[i].start, m_buffers[i].length);
            if (res)
                logger(LOG_WARN, "%s munmap i=%d res=%d errno=%d", tag, i, res, errno);
        }

        free(m_buffers);
        m_buffers = NULL;
    }

    if (m_fd != -1) {
        res = ::close(m_fd);
        if (res)
            logger(LOG_WARN, "%s ::close fd=%d res=%d errno=%d", tag, m_fd, res, errno);
        m_fd = -1;
    }

    if (m_data)
        free(m_data);
    m_data = NULL;
    m_name = NULL;
}

int Camera::initialize_device(int fps)
{
    assert(m_buffers);

    int res = 0;
    struct v4l2_capability cap;
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;
    struct v4l2_format fmt;
    unsigned int min;

    res = HANDLE_EINTR(::ioctl(m_fd, VIDIOC_QUERYCAP, &cap));
    if (res) {
        logger(LOG_ERROR, "%s VIDIOC_QUERYCAP res=%d errno=%d", m_name, res, errno);
        return res;
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        logger(LOG_ERROR, "%s no capture device", m_name);
        return EFAULT;
    }
    if(!(cap.capabilities & V4L2_CAP_STREAMING)) {
        logger(LOG_ERROR, "%s does not support streaming", m_name);
        return EFAULT;
    }

    memset(&cropcap, 0, sizeof(cropcap));

    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    res = HANDLE_EINTR(::ioctl(m_fd, VIDIOC_CROPCAP, &cropcap));
    if (res) {
        logger(LOG_ERROR, "%s VIDIOC_CROPCAP res=%d errno=%d (ignoring)", m_name, res, errno);
    }
    else {

      crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      crop.c = cropcap.defrect; /* reset to default */

      res = HANDLE_EINTR(::ioctl(m_fd, VIDIOC_S_CROP, &crop));
      if (res)
          logger(LOG_ERROR, "%s VIDIOC_S_CROP res=%d errno=%d (ignoring)", m_name, res, errno);
    }

    memset(&fmt, 0, sizeof(fmt));

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = m_width_h * 2;
    fmt.fmt.pix.height      = m_height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    res = HANDLE_EINTR(::ioctl(m_fd, VIDIOC_S_FMT, &fmt));
    if (res) {
        logger(LOG_ERROR, "%s VIDIOC_S_FMT res=%d errno=%d", m_name, res, errno);
        return EFAULT;
    }

    struct v4l2_streamparm p;
    memset(&p, 0, sizeof(p));

    p.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
    //p.parm.capture.capability=V4L2_CAP_TIMEPERFRAME;
    //p.parm.capture.capturemode=V4L2_MODE_HIGHQUALITY;
    p.parm.capture.timeperframe.numerator = 1;
    p.parm.capture.timeperframe.denominator = fps;
    p.parm.output.timeperframe.numerator = 1;
    p.parm.output.timeperframe.denominator = fps;
    //p.parm.output.outputmode=V4L2_MODE_HIGHQUALITY;
    //p.parm.capture.extendedmode=0;
    //p.parm.capture.readbuffers=n_buffers;

    res = HANDLE_EINTR(::ioctl(m_fd, VIDIOC_S_PARM, &p));
    if (res) {
        logger(LOG_ERROR, "%s VIDIOC_S_PARM res=%d errno=%d", m_name, res, errno);
        return EFAULT;
    }

    initialize_setting(SETTING_BRIGHTNESS);
    initialize_setting(SETTING_CONTRAST);
    initialize_setting(SETTING_SATURATION);
    initialize_setting(SETTING_HUE);
    initialize_setting(SETTING_HUE_AUTO);
    initialize_setting(SETTING_SHARPNESS);

    //TODO: TO ADD SETTINGS
    //here should go custom calls to xioctl
    //END TO ADD SETTINGS

    /* Buggy driver paranoia. */
    min = fmt.fmt.pix.width * 2;
    if(fmt.fmt.pix.bytesperline < min)
        fmt.fmt.pix.bytesperline = min;
    
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
    if(fmt.fmt.pix.sizeimage < min)
        fmt.fmt.pix.sizeimage = min;

    return 0;
}

int Camera::open_cam_device()
{
    assert(m_buffers);

    int tmp_fd = -1;
    struct stat st;

    if(-1 == ::stat(m_name, &st)) {
        logger(LOG_ERROR, "Cannot identify '%s' : %d, %s", m_name, errno, strerror(errno));
        return ENOENT;
    }

    if (!S_ISCHR(st.st_mode)) {
        logger(LOG_ERROR, "%s is no device", m_name);
        return EFAULT;
    }

    tmp_fd = ::open(m_name, O_RDWR | O_NONBLOCK, 0);
    if(-1 == tmp_fd) {
        logger(LOG_ERROR, "Cannot open '%s': %d, %s\n", m_name, errno, strerror(errno));
        return EFAULT;
    }

    m_fd = tmp_fd;
    return 0;
}

int Camera::init_mmap()
{
    assert(m_buffers);

    int res = 0;
    struct v4l2_requestbuffers req;

    memset(&req, 0, sizeof(req));

    req.count               = g_num_of_bufs;
    req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory              = V4L2_MEMORY_MMAP;

    res = HANDLE_EINTR(ioctl(m_fd, VIDIOC_REQBUFS, &req));
    if (res) {
        logger(LOG_ERROR, "%s VIDIOC_REQBUFS res=%d errno=%d", m_name, res, errno);
        return EFAULT;
    }

    for(size_t idx = 0; idx < g_num_of_bufs; ++idx) {

        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = idx;

        res = HANDLE_EINTR(ioctl(m_fd, VIDIOC_QUERYBUF, &buf));
        if (res) {
            logger(LOG_ERROR, "%s VIDIOC_QUERYBUF res=%d errno=%d", m_name, res, errno);
            return EFAULT;
        }

        m_buffers[idx].length = buf.length;
        m_buffers[idx].start = ::mmap(NULL /* start anywhere */,
                                  buf.length,
                                  PROT_READ | PROT_WRITE /* required */,
                                  MAP_SHARED /* recommended */,
                                  m_fd, buf.m.offset);

        if (MAP_FAILED == m_buffers[idx].start) {
            logger(LOG_ERROR, "%s MAP_FAILED res=%d errno=%d", m_name, res, errno);
            return EFAULT;
        }
    }

    return 0;
}

int Camera::start_capture()
{
    assert(m_buffers);
    assert(m_fd != -1);

    int res = 0;
    enum v4l2_buf_type type;

    for(size_t i = 0; i < g_num_of_bufs; ++i) {

        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = i;

        res = HANDLE_EINTR(::ioctl(m_fd, VIDIOC_QBUF, &buf));
        if (res) {
            logger(LOG_ERROR, "%s VIDIOC_QBUF res=%d errno=%d", m_name, res, errno);
            return res;
        }
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    res = HANDLE_EINTR(::ioctl(m_fd, VIDIOC_STREAMON, &type));
    if (res) {
        logger(LOG_ERROR, "%s VIDIOC_STREAMON res=%d errno=%d", m_name, res, errno);
        return res;
    }

    return res;
}

int Camera::capture()
{
    if (!m_buffers)
        return EINVAL;

    assert(m_data);
    assert(m_fd != -1);

    int res = 0;
    struct v4l2_buffer buf;

    memset(&buf, 0, sizeof(buf));

    buf.type    = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory  = V4L2_MEMORY_MMAP;

    res = HANDLE_EINTR(::ioctl(m_fd, VIDIOC_DQBUF, &buf));
    if (res) {
        logger(LOG_ERROR, "%s VIDIOC_DQBUF res=%d errno=%d", m_name, res, errno);
        return res;
    }

    assert(buf.index < g_num_of_bufs);
    memcpy(m_data, (unsigned char *)m_buffers[buf.index].start, m_buffers[buf.index].length);

    res = HANDLE_EINTR(::ioctl(m_fd, VIDIOC_QBUF, &buf));
    if (res) {
        logger(LOG_ERROR, "%s VIDIOC_QBUF res=%d errno=%d", m_name, res, errno);
        return res;
    }

    return res;
}

int Camera::update(uint64_t now)
{
    return capture();
}

int Camera::toIplImage(unsigned char *buf, int width) const
{
    assert(buf);
    assert(width > 0);

    if (!m_buffers)
        return EINVAL;

    const int w2 = m_width_h;
    const int h  = m_height;

    for(int x = 0; x < w2; ++x) {
        for(int y = 0; y < h; ++y) {

          int y0, y1, u, v; //y0 u y1 v

          int i=(y*w2+x)*4;
          y0 = m_data[i];
          u  = m_data[i+1];
          y1 = m_data[i+2];
          v  = m_data[i+3];

          int g;
          i=(y*width+2*x)*3;
          
          g = g_y2u[y0][u] + g_y2v[y0][v];
          if(g>255){g=255;}
          if(g<0){g=0;}
          buf[i] = g_yu[y0][u];
          buf[i+1] = g;
          buf[i+2] = g_yv[y0][v];
          
          g = g_y2u[y1][u] + g_y2v[y1][v];
          if(g>255){g=255;}
          if(g<0){g=0;}
          buf[i+3] = g_yu[y1][u];
          buf[i+4] = g;
          buf[i+5] = g_yv[y1][v];
        }
    }

    return 0;
}

int Camera::toGrayScaleIplImage(unsigned char *buf, int width) const
{
    assert(buf);
    assert(width > 0);

    if (!m_buffers)
        return EINVAL;

    const int w2 = m_width_h;
    const int h  = m_height;
    
    for (int x = 0; x < w2; ++x) {
        for (int y = 0; y < h; ++y) {

            int y0, y1;
            
            int i = (y * w2 + x)*4;
            y0 = m_data[i];
            y1 = m_data[i + 2];

            i = (y * width + 2 * x)*1;
            buf[i] = (unsigned char) (y0);
            buf[i + 1] = (unsigned char) (y1);
        }
    }

    return 0;
}

int Camera::toMat(unsigned char *buf, int channels, int cols) const
{
    assert(buf);
    assert(channels > 0);
    assert(cols > 0);

    if (!m_buffers)
        return EINVAL;

    const int w2 = m_width_h;
    const int h  = m_height;

    for(int x = 0; x < w2; ++x) {
        for(int y = 0; y < h; ++y) {

          int y0, y1, u, v; //y0 u y1 v

          int i=(y*w2+x)*4;
          y0 = m_data[i];
          u  = m_data[i+1];
          y1 = m_data[i+2];
          v  = m_data[i+3];
                
          int g;
          
          i = y*cols*channels + x*2*channels;  

          g = g_y2u[y0][u] + g_y2v[y0][v];
          if(g>255){g=255;}
          if(g<0){g=0;}
          buf[i] = g_yu[y0][u];
          buf[i+1] = (unsigned char)g;
          buf[i+2] = g_yv[y0][v];
          
          g = g_y2u[y1][u] + g_y2v[y1][v];
          if(g>255){g=255;}
          if(g<0){g=0;}
          buf[i+3] = g_yu[y1][u];
          buf[i+4] = g;
          buf[i+5] = g_yv[y1][v];
        }
    }

    return 0;
}   

int Camera::toGrayScaleMat(unsigned char *buf, int channels, int cols) const
{   
    assert(buf);
    assert(channels > 0);
    assert(cols > 0);

    if (!m_buffers)
        return EINVAL;

    const int w2 = m_width_h;
    const int h  = m_height;

    for (int x = 0; x < w2; ++x) {
        for (int y = 0; y < h; ++y) {

            int y0, y1;

            int i = (y * w2 + x)*4;
            y0 = m_data[i];
            y1 = m_data[i + 2];
            
            i = y*cols*channels + x*2*channels; 
            buf[i + 0] = (unsigned char) (y0);
            buf[i + 1] = (unsigned char) (y1);
            
            //More clear in logic but slower solution
            //m.at<uchar>(y, (2*x)) = (unsigned char) (y0);
            //m.at<uchar>(y, (2*x)+1) = (unsigned char) (y1);
        }
    }

    return 0;
}

void Camera::initialize_setting(SettingType set_type)
{
    assert(set_type >= 0 && set_type < SETTING_MAX);

    int res = 0;
    struct v4l2_queryctrl queryctrl;
    memset(&queryctrl, 0, sizeof(queryctrl));
    queryctrl.id = m_settings[set_type].vl_id;

    res = query_device(m_name, m_settings[set_type].tag, m_fd, &queryctrl);

    m_settings[set_type].err = res;

    if (!res) {
        m_settings[set_type].min  = queryctrl.minimum;
        m_settings[set_type].max  = queryctrl.maximum;
        m_settings[set_type].def  = queryctrl.default_value;
    }
}

int Camera::setSetting(SettingType set_type, int v)
{
    if (!m_buffers || set_type < 0 || set_type >= SETTING_MAX)
        return EINVAL;

    Setting &set = m_settings[set_type];

    if (set.err)
        return set.err;
    if (v < set.min || v > set.max)
        return EINVAL;

    struct v4l2_control control;

    memset(&control, 0, sizeof(control));

    control.id = set.vl_id;
    control.value = v;

    return control_device(m_name, set.tag, m_fd, &control);
}

Camera::Setting Camera::getSetting(SettingType set_type) const
{
    if (m_buffers && set_type >= 0 && set_type < SETTING_MAX)
        return m_settings[set_type];

    Setting tmp;
    memset(&tmp, 0, sizeof(tmp));
    tmp.err = EINVAL;
    return tmp;
}

} // namespace robo