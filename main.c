//#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/display/graphics_adf.h"
#include "lvgl/lvgl.h"
#include "lv_examples/lv_apps/demo/demo.h"
#include "lv_examples/lv_apps/benchmark/benchmark.h"
#include "lv_examples/lv_tests/lv_test.h"
#include "lv_examples/lv_tutorial/6_images/lv_tutorial_images.h"

#include "lvgl/src/lv_hal/lv_hal_indev.h"
#include "lv_drivers/indev/evdev.h"
#include <utils/Log.h>
#include <stdio.h>
#include <unistd.h>

void revo_log(lv_log_level_t level, const char * file, int line, const char * dsc)
{
        static const char * lvl_prefix[] = {"Trace", "Info", "Warn", "Error"};
        ALOGD("%s: %s \t(%s #%d)\n", lvl_prefix[level], dsc, file, line);
}

int main(void)
{
    static int surface_width = 720;
    static int surface_height = 960;
    /*LittlevGL init*/
    lv_init();

    /*Linux frame buffer device init*/
    //fbdev_init();
    adf_init();
    evdev_init();

    lv_log_print_g_cb_t log_callback = (lv_log_print_g_cb_t)revo_log;
    lv_log_register_print_cb(log_callback);

    /*A small buffer for LittlevGL to draw the screen's content*/
    lv_color_t *buf = (lv_color_t *)calloc(surface_width * surface_height, sizeof(lv_color_t));
    if(!buf) {
        fprintf(stderr, "request memory failed!!!!\n");
        return -1;
    }

    lv_color_t *buf1 = (lv_color_t *)calloc(surface_width * surface_height, sizeof(lv_color_t));
    if(!buf1) {
        fprintf(stderr, "request memory failed!!!!\n");
        return -1;
    }

    // /*Initialize a descriptor for the buffer*/
    static lv_disp_buf_t disp_buf;
    lv_disp_buf_init(&disp_buf, buf, buf1, surface_width * surface_height);
   // lv_disp_buf_init(&disp_buf, buf, NULL, surface_width * surface_height);

    /*Initialize and register a display driver*/
    lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.buffer = &disp_buf;
    //disp_drv.flush_cb = fbdev_flush;
    disp_drv.flush_cb = gr_flush;
    disp_drv.hor_res = surface_width;
    disp_drv.ver_res = surface_height;
    lv_disp_drv_register(&disp_drv);

    /*Create a Demo*/
    //demo_create();
    //benchmark_create();
    //terminal_create();
    //sysmon_create();
    //tpcal_create();

    lv_theme_t * th = lv_theme_default_init(0, NULL);
    lv_test_theme_1(th);

    //lv_test_theme_2();

    //lv_tutorial_image();
    /*Initialize and register a input driver*/
    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);      /*Basic initialization*/
    indev_drv.type = LV_INDEV_TYPE_POINTER;                  /*See below.*/
    indev_drv.read_cb = evdev_read;              /*See below.*/
    /*Register the driver in LittlevGL and save the created input device object*/
    lv_indev_t * my_indev = lv_indev_drv_register(&indev_drv);


    /*Handle LitlevGL tasks (tickless mode)*/
    while(1) {
        lv_tick_inc(5);
        lv_task_handler();
        usleep(5000);
    }

    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}