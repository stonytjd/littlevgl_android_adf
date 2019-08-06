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

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/cdefs.h>
#include <sys/mman.h>

#include "graphics_adf.h"

static adf_pdata *g_pdata;

static gr_surface adf_flip(adf_pdata *pdata);
static void adf_blank(adf_pdata *pdata, bool blank);

static int adf_surface_init(adf_pdata *pdata, struct drm_mode_modeinfo *mode, adf_surface_pdata *surf) {
    memset(surf, 0, sizeof(*surf));
    //fprintf(stderr, "adf_ui_init pdata->intf_fd = %d, mode->hdisplay = %d, mode->vdisplay = %d~\n", pdata->intf_fd, mode->hdisplay, mode->vdisplay);
    ALOGD("adf_ui_init pdata->intf_fd = %d, mode->hdisplay = %d, mode->vdisplay = %d~\n", pdata->intf_fd, mode->hdisplay, mode->vdisplay);
    surf->fd = adf_interface_simple_buffer_alloc(pdata->intf_fd, mode->hdisplay, mode->vdisplay, pdata->format, &surf->offset, &surf->pitch);
    //fprintf(stderr, "adf_ui_init surf->fd = %d, surf->offset = %d, surf->pitch = %d!\n", surf->fd, surf->offset, surf->pitch);
    ALOGD("adf_ui_init surf->fd = %d, surf->offset = %d, surf->pitch = %d!\n", surf->fd, surf->offset, surf->pitch);
    if (surf->fd < 0)
        return surf->fd;

    surf->base.width = mode->hdisplay;
    surf->base.height = mode->vdisplay;
    surf->base.row_bytes = surf->pitch;
    surf->base.pixel_bytes = (pdata->format == DRM_FORMAT_RGB565) ? 2 : 4;

    surf->base.data = mmap(NULL, surf->pitch * surf->base.height, PROT_WRITE, MAP_SHARED, surf->fd, surf->offset);
    if (surf->base.data == MAP_FAILED) {
        close(surf->fd);
        return -errno;
    }

    return 0;
}

static int adf_interface_init(adf_pdata *pdata)
{
    struct adf_interface_data intf_data;
    int ret = 0;
    int err;

    err = adf_get_interface_data(pdata->intf_fd, &intf_data);
    if (err < 0)
        return err;

    err = adf_surface_init(pdata, &intf_data.current_mode, &pdata->surfaces[0]);
    if (err < 0) {
        fprintf(stderr, "allocating surface 0 failed: %s\n", strerror(-err));
        ret = err;
        goto done;
    }

    err = adf_surface_init(pdata, &intf_data.current_mode, &pdata->surfaces[1]);
    if (err < 0) {
        fprintf(stderr, "allocating surface 1 failed: %s\n", strerror(-err));
        memset(&pdata->surfaces[1], 0, sizeof(pdata->surfaces[1]));
        pdata->n_surfaces = 1;

    } else {
        pdata->n_surfaces = 2;
    }

done:
    adf_free_interface_data(&intf_data);
    return ret;
}

static int adf_device_init(adf_pdata *pdata, struct adf_device *dev)
{
    adf_id_t intf_id;
    int intf_fd;
    int err;

    err = adf_find_simple_post_configuration(dev, &pdata->format, 1, &intf_id,
            &pdata->eng_id);
    if (err < 0)
        return err;

    err = adf_device_attach(dev, pdata->eng_id, intf_id);
    if (err < 0 && err != -EALREADY)
        return err;

    pdata->intf_fd = adf_interface_open(dev, intf_id, O_RDWR);
    if (pdata->intf_fd < 0)
        return pdata->intf_fd;

    err = adf_interface_init(pdata);
    if (err < 0) {
        close(pdata->intf_fd);
        pdata->intf_fd = -1;
    }

    return err;
}


gr_surface adf_init(void)
{
    adf_id_t *dev_ids = NULL;
    ssize_t n_dev_ids, i;
    gr_surface ret;

    g_pdata = calloc(1, sizeof(adf_pdata));
    if (!g_pdata) {
        perror("allocating adf_pdata failed");
        return NULL;
    }

    //g_pdata->format = DRM_FORMAT_ABGR8888;
    g_pdata->format = DRM_FORMAT_BGRA8888;
    //g_pdata->format = DRM_FORMAT_RGBX8888;
    //g_pdata->format = DRM_FORMAT_RGBX8888;

    n_dev_ids = adf_devices(&dev_ids);
    fprintf(stderr, "adf_ui_init n_dev_ids = %d\n", n_dev_ids);
    if (n_dev_ids == 0) {
        return NULL;
    } else if (n_dev_ids < 0) {
        fprintf(stderr, "enumerating adf devices failed: %s\n", strerror(-n_dev_ids));
        return NULL;
    }

    g_pdata->intf_fd = -1;

    for (i = 0; (i < n_dev_ids) && (g_pdata->intf_fd < 0); i++) {
        struct adf_device dev;

        int err = adf_device_open(dev_ids[i], O_RDWR, &dev);
        if (err < 0) {
            fprintf(stderr, "opening adf device %u failed: %s\n", dev_ids[i], strerror(-err));
            continue;
        }

        err = adf_device_init(g_pdata, &dev);
        if (err < 0){
            fprintf(stderr, "initializing adf device %u failed: %s\n", dev_ids[i], strerror(-err));
        }
        adf_device_close(&dev);
    }

    free(dev_ids);

    if (g_pdata->intf_fd < 0)
        return NULL;

//    ret = adf_flip(g_pdata);
	g_pdata->current_surface = 0;
    ret = &g_pdata->surfaces[g_pdata->current_surface].base;

    adf_blank(g_pdata, true);
    adf_blank(g_pdata, false);

    return ret;
}

static gr_surface adf_flip(adf_pdata *pdata)
{
    adf_surface_pdata *surf = &pdata->surfaces[pdata->current_surface];

    int fence_fd = adf_interface_simple_post(pdata->intf_fd, pdata->eng_id,
            surf->base.width, surf->base.height, pdata->format, surf->fd,
            surf->offset, surf->pitch, -1);
    if (fence_fd >= 0)
        close(fence_fd);
  //  if(!tp_flag1)
  //      pdata->current_surface = (pdata->current_surface + 1) % pdata->n_surfaces;
	
	pdata->current_surface = 0;
    return &pdata->surfaces[pdata->current_surface].base;

}

static void adf_blank(adf_pdata *pdata, bool blank)
{
    adf_interface_blank(pdata->intf_fd,
            blank ? DRM_MODE_DPMS_OFF : DRM_MODE_DPMS_ON);
}

static void adf_surface_destroy(adf_surface_pdata *surf)
{
    munmap(surf->base.data, surf->pitch * surf->base.height);
    close(surf->fd);
}

void adf_exit(void)
{
    unsigned int i;

    for (i = 0; i < g_pdata->n_surfaces; i++)
        adf_surface_destroy(&g_pdata->surfaces[i]);
    if (g_pdata->intf_fd >= 0)
        close(g_pdata->intf_fd);
    free(g_pdata);
}


//---------------------graphics driver api---------------------------------//
/* */
static gr_surface gr_get_surface(adf_pdata* pdata) 
{
    gr_surface surface;
    adf_surface_pdata *psurf = &pdata->surfaces[0];

    if(!psurf)
        return NULL;
    surface = &psurf->base;
    return surface;
}

static bool outside(int x, int y)
{
    gr_surface surface = gr_get_surface(g_pdata);
    if(!surface) return false;
    return x < 0 || x >= surface->width || y < 0 || y >= surface->height;
}

unsigned int gr_get_width(adf_pdata* pdata) 
{
    gr_surface surface = gr_get_surface(pdata);
    if (surface == NULL) {
        return 0;
    }

    return surface->width;
}

unsigned int gr_get_height(adf_pdata* pdata) 
{
    gr_surface surface = gr_get_surface(pdata);
    if (surface == NULL) {
        return 0;
    }

    return surface->height;
}

unsigned char* gr_get_surf_addr(adf_pdata* pdata) 
{
    gr_surface surface = gr_get_surface(pdata);
    if (surface == NULL) {
        return 0;
    }
    return surface->data;
}

void gr_draw_point(adf_pdata *pdata, int x, int y)
{
    unsigned char* base = gr_get_surf_addr(pdata);
    gr_surface surface = gr_get_surface(pdata);

	x += overscan_offset_x;
	y += overscan_offset_y;

	unsigned char *p = base + y * surface->row_bytes + x * surface->pixel_bytes;
	if (gr_current_a == 255)
	{
		*p++ = gr_current_r;
		*p++ = gr_current_g;
		*p++ = gr_current_b;
	}
	else if(gr_current_a > 0)
	{
		*p = ( (*p) * (255-gr_current_a) + gr_current_r * gr_current_a) / 255;
		++p;
		*p = ( (*p) * (255-gr_current_a) + gr_current_g * gr_current_a) / 255;
		++p;
		*p = ( (*p) * (255-gr_current_a) + gr_current_b * gr_current_a) / 255;
	}
}

/*fill buffer with one color*/
void gr_fill(adf_pdata *pdata, int x1, int y1, int x2, int y2) {
    unsigned char* base = gr_get_surf_addr(pdata);
    gr_surface surface = gr_get_surface(pdata);

    x1 += overscan_offset_x;
    y1 += overscan_offset_y;

    x2 += overscan_offset_x;
    y2 += overscan_offset_y;

    unsigned char* p = base + y1 * surface->row_bytes + x1 * surface->pixel_bytes;
    if (gr_current_a == 255) {
        int x, y;
        for (y = y1; y < y2; ++y) {
            unsigned char* px = p;
            for (x = x1; x < x2; ++x) {
                *px++ = gr_current_r;
                *px++ = gr_current_g;
                *px++ = gr_current_b;
                px++;
            }
            p += surface->row_bytes;
        }
    } else if (gr_current_a > 0) {
        int x, y;
        for (y = y1; y < y2; ++y) {
            unsigned char* px = p;
            for (x = x1; x < x2; ++x) {
                *px = (*px * (255-gr_current_a) + gr_current_r * gr_current_a) / 255;
                ++px;
                *px = (*px * (255-gr_current_a) + gr_current_g * gr_current_a) / 255;
                ++px;
                *px = (*px * (255-gr_current_a) + gr_current_b * gr_current_a) / 255;
                ++px;
                ++px;
            }
            p += surface->row_bytes;
        }
    }
}

void gr_blit(adf_pdata *pdata, GRSurface* source, int sx, int sy, int dx, int dy) {
    gr_surface surface = gr_get_surface(pdata);

    if (source == NULL) return;
    //fprintf(stderr, "surface pixel_bytes(%d) row_bytes(%d)\n", surface->pixel_bytes, surface->row_bytes);
    //fprintf(stderr, "source pixel_bytes(%d) row_bytes(%d)\n", source->pixel_bytes, source->row_bytes);
    if (surface->pixel_bytes != source->pixel_bytes) {
        printf("gr_blit: source has wrong format\n");
        return;
    }

    dx += overscan_offset_x;
    dy += overscan_offset_y;

    if (outside(dx, dy) || outside(dx + source->width - 1, dy + source->height - 1)) return;

    unsigned char* src_p = source->data + sy*source->row_bytes + sx*source->pixel_bytes;
    unsigned char* dst_p = surface->data + dy*surface->row_bytes + dx*surface->pixel_bytes;

    int i;
    for (i = 0; i < source->height; ++i) {
        memcpy(dst_p, src_p, source->width * source->pixel_bytes);
        src_p += source->row_bytes;
        dst_p += surface->row_bytes;
    }
}

/**
 * Flush a buffer to the marked area
 * @param drv pointer to driver where this function belongs
 * @param area an area where to copy `color_p`
 * @param color_p an array of pixel to copy to the `area` part of the screen
 */
void gr_flush(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
    int xoffset = 0;
    int yoffset = 0;
    int dx = 0;
    int dy = 0;
    int line_length = 0;

    ALOGD("%s: area (%d,%d) ---> (%d,%d)\n", __FUNCTION__, area->x1, area->y1, area->x2, area->y2);

    GRSurface source;
    source.width = area->x2 - area->x1 + 1;
    source.height = area->y2 - area->y1 + 1;
    source.row_bytes = source.width * sizeof(lv_color_t);  //Bytes per line
    source.pixel_bytes = sizeof(lv_color_t);   //Bytes per pixel
    source.data = (unsigned char*)color_p;



    gr_blit(g_pdata, &source, area->x1, area->y1, area->x1, area->y1);
    //gr_fill(g_pdata, 100,100, 200,200);

    adf_flip(g_pdata);

    lv_disp_flush_ready(drv);
}
