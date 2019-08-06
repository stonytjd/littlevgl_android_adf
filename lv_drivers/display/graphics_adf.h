/*
 * Copyright (C) 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _GRAPHICS_H_
#define _GRAPHICS_H_
#include <utils/Log.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/


#ifndef LV_DRV_NO_CONF
#ifdef LV_CONF_INCLUDE_SIMPLE
#include "lv_drv_conf.h"
#else
#include "../../lv_drv_conf.h"
#endif
#endif

#if USE_FBDEV

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif
#endif
#include <adf/adf.h>

#define    ROTATE_0               0
#define    ROTATE_90              1
#define    ROTATE_180             2
#define    ROTATE_270             3
#define    ROTATE_UNKNOWN         -1

typedef struct {
    int width;
    int height;
    int row_bytes;
    int pixel_bytes;
    unsigned char* data;
} GRSurface;

typedef struct adf_surface_pdata {
    GRSurface base;
    GRSurface swap1;
    GRSurface swap2;
    int fd;
    unsigned int offset;
    unsigned int pitch;
}adf_surface_pdata;

typedef struct adf_pdata {
    int intf_fd;
    adf_id_t eng_id;
    unsigned int format;

    unsigned int current_surface;
    unsigned int n_surfaces;
    adf_surface_pdata surfaces[2];
}adf_pdata;

typedef GRSurface* gr_surface;

static int overscan_offset_x = 0;
static int overscan_offset_y = 0;

static unsigned char gr_current_r = 255;
static unsigned char gr_current_g = 255;
static unsigned char gr_current_b = 255;
static unsigned char gr_current_a = 255;

gr_surface adf_init(void);
void gr_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p);
#ifdef __cplusplus
}
#endif

#endif
