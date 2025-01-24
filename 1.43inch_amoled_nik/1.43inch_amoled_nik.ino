/*

nikthefix 23.01.25

Lilygo T-Display-S3-Amoled-1.75"



Dependencies:

ESP32_Arduino boards package version 3.1.1
LVGL 8.3.11



Notes:

This build uses a lean CO5300 display driver rather than the Arduino_GFX framework used in the Lilygo supplied examples.
The driver files are supplied in the sketch directory so no installation is required.
The touch driver is also included in the sketch directory so no installation is required.
I recommend a clean LVGL install via the Arduino library manager and perhaps a clean lv_conf.h with the following modifications:

Please set in lv_conf.h:  --->   #if 1                                  (line 15)
                          --->   #define LV_COLOR_16_SWAP 1             (line 30)
                          --->   #define LV_MEM_CUSTOM 1                (line 49)
                          --->   #define LV_TICK_CUSTOM 1               (line 88)
                          --->   #define LV_FONT_MONTSERRAT_14 1        (line 367)

                          A copy of my lv_conf.h file is provided.

Set display brightness in setup() ---> lcd_brightness(200); // 0-255



Build options:

Select board ESP32S3 Dev Module
Select USB CDC On Boot "Enabled"
Select Flash Size 16M
Select Partition Scheme "custom" - partitions.csv in sketch folder will be used
Select PSRAM "OPI PSRAM"



Troubleshooting:

In case of build failure you can try deleting the cached sketch object files located here -
"C:\Users\your_user_name\AppData\Local\arduino\sketches"

*/


#include <Arduino.h>
#include "pins_config.h"
#include "src/display/sh8601.h"
#include "src/touch/FT3168.h"
#include "lvgl.h"
#include "ui.h"

bool  isPressed = false;
 
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *buf;


void lv_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    lcd_PushColors(area->x1, area->y1, w, h, (uint16_t *)&color_p->full); 
    lv_disp_flush_ready(disp);
}


void my_rounder(lv_disp_drv_t * disp_drv, lv_area_t * area)
{
   area->x1 = area->x1 & 0xFFFE;     // round down the refresh area x-axis start point to next even number - required for this display
   area->x2 = (area->x2 & 0xFFFE)+1; // round down the refresh area x-axis end point to next even number - required for this display

   area->y1 = area->y1 & 0xFFFE;     // round down the refresh area y-axis start point to next even number - required for this display
   area->y2 = (area->y2 & 0xFFFE)+1; // round down the refresh area y-axis end point to next even number - required for this display
}


//Alternative rounder code1:

// void my_rounder(struct _lv_disp_drv_t *disp_drv, lv_area_t *area)
// {
//   uint16_t x1 = area->x1;
//   uint16_t x2 = area->x2;

//   uint16_t y1 = area->y1;
//   uint16_t y2 = area->y2;

//   // round the start of coordinate down to the nearest 2n number
//   area->x1 = (x1 >> 1) << 1;
//   area->y1 = (y1 >> 1) << 1;
//   // round the end of coordinate up to the nearest 2n+1 number
//   area->x2 = ((x2 >> 1) << 1) + 1;
//   area->y2 = ((y2 >> 1) << 1) + 1;
// }


//Alternative rounder code2:

// void my_rounder(lv_disp_drv_t *disp_drv, lv_area_t *area)
// {
//     if (area->x1 % 2 != 0)
//         area->x1 += 1;
//     if (area->y1 % 2 != 0)
//         area->y1 += 1;

//     uint32_t w = (area->x2 - area->x1 + 1);
//     uint32_t h = (area->y2 - area->y1 + 1);
//     if (w % 2 != 0)
//         area->x2 -= 1;
//     if (h % 2 != 0)
//         area->y2 -= 1;
// }



static void lv_indev_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  if (isPressed)
  {
     isPressed = false;  
     uint16_t tp_x,tp_y;
     uint8_t touchpad = getTouch(&tp_x, &tp_y);
       if (touchpad > 0)
       {
          data->state = LV_INDEV_STATE_PR;
          data->point.x = tp_x;
          data->point.y = tp_y;
          //Serial.printf("X: %d   Y: %d\n", tp_x, tp_y); //for testing
       }
    else
       {
        data->state = LV_INDEV_STATE_REL;
       }
  }
}


void setup()
{
    Serial.begin(115200);
    Serial.printf("psram size : %d MB\r\nflash size : %d MB\r\n", ESP.getPsramSize() / 1024 / 1024, ESP.getFlashChipSize() / 1024 / 1024);


    pinMode(LCD_VCI_EN, OUTPUT);
    digitalWrite(LCD_VCI_EN, HIGH);
    attachInterrupt(TOUCH_INT, []() {isPressed = true;}, FALLING); //Attach interrupt
    Touch_Init(); 
    sh8601_init();
    //lcd_setRotation(2); 
    lcd_brightness(200); // 0-255
    lv_init();
    buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * LVGL_LCD_BUF_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, LVGL_LCD_BUF_SIZE);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = EXAMPLE_LCD_H_RES;
    disp_drv.ver_res = EXAMPLE_LCD_V_RES;
    disp_drv.rounder_cb = my_rounder; // required for partial refresh
    disp_drv.flush_cb = lv_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    //disp_drv.full_refresh = 1; // useful for testing but slower
    lv_disp_drv_register(&disp_drv);
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lv_indev_read;
    lv_indev_drv_register(&indev_drv);     

    ui_init();   
}



void loop()
{
    lv_timer_handler();
    delay(1);  
}




void check_for_memory_leaks() 
{
    Serial.print(F("Total heap  ")); Serial.println(ESP.getHeapSize());
    Serial.print(F("Free heap   ")); Serial.println(ESP.getFreeHeap());
    Serial.print(F("Total psram ")); Serial.println(ESP.getPsramSize());
    Serial.print(F("Free psram  ")); Serial.println(ESP.getFreePsram());
    Serial.println(F(" "));
}
