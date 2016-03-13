/*
 * Copyright (C) 2009 Giacomo Spigler
 * 2013 - George Jordanov - improve in performance for HSV conversion and improvements
 *
 * CopyPolicy: Released under the terms of the GNU GPL v3.0.
 */

#ifndef __LIBCAM__H__
#define __LIBCAM__H__

#include "test.h"

class Camera
{
public:

  enum SettingType {
    SETTING_BRIGHTNESS,
    SETTING_CONTRAST,
    SETTING_SATURATION,
    SETTING_HUE,
    SETTING_HUE_AUTO,
    SETTING_SHARPNESS,
    SETTING_MAX
  };

  struct Buffer {
      void    *start;
      size_t  length;
  };

  struct Setting {
    const char *tag;
    int vl_id;
    int min;
    int max;
    int def;
    int init;
  };

  int             m_width_h;
  int             m_height;
  unsigned char   *m_data;
  int             m_fd;
  Buffer          *m_buffers;
  const char      *m_name;

  Setting         m_settings[SETTING_MAX];

  Camera();
  ~Camera();

  int initialize(const char *name, int w, int h, int fps = 30);
  void shutdown();
  int update(uint64_t now);

  int toIplImage(unsigned char *buf, int width) const;
  int toGrayScaleIplImage(unsigned char *buf, int width) const;
  int toGrayScaleMat(unsigned char *buf, int channels, int cols) const;
  int toMat(unsigned char *buf, int channels, int cols) const;

  Setting getSetting(SettingType set_type) const;
  int setSetting(SettingType set_type, int v);

private:
  int start_capture();
  int init_mmap();
  int open_cam_device();
  int initialize_device(int fps);
  int capture();
  int initialize_setting(SettingType set_type);
};

#endif
