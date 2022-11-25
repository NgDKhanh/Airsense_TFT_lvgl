#include <lvgl.h>
#include <TFT_eSPI.h>
/*If you want to use the LVGL examples,
  make sure to install the lv_examples Arduino library
  and uncomment the following line.
#include <lv_examples.h>
*/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


/*Change to your screen resolution*/
static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 240;

/* Structure for holding display buffer information. */
static lv_disp_draw_buf_t draw_buffer;

static lv_color_t buffer[ screenWidth * 10 ];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */
Adafruit_BME280 bme; // I2C


#if LV_USE_LOG != 0
/* Serial debugging */
void my_print(const char * buffer)
{
    Serial.printf(buffer);
    Serial.flush();
}
#endif

/*
 * @brief Display flushing
 * @param disp      Display Driver structure to be registered by HAL
 * @param area      area of the screen that flush
 * @param color_p   color want to flush 
 */
void my_display_flush( lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p )
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );

    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( ( uint16_t * )&color_p->full, w * h, true );
    tft.endWrite();

    lv_disp_flush_ready( disp );
}

/*
 * @brief   Read the touchpad
 * @param   indev_driver  Initialized by the user and registered by 'lv_indev_add()'
 * @param   data          Data structure passed to an input driver to fill
 */
void my_touchpad_read( lv_indev_drv_t * indev_driver, lv_indev_data_t * data )
{
    uint16_t touchX, touchY;

    bool touched = tft.getTouch( &touchX, &touchY, 600 );

    if( !touched )
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

        Serial.print( "Data x " );
        Serial.println( touchX );

        Serial.print( "Data y " );
        Serial.println( touchY );
    }
}

static void event_cb(lv_event_t *e);

void lv_display_temperature();


void setup()
{
    Serial.begin( 115200 ); /* prepare for possible serial debug */

    lv_init();

    /*bool status = bme.begin(0x76);  
    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }*/

#if LV_USE_LOG != 0
    lv_log_register_print_cb( my_print ); /* register print function for debugging */
#endif

    tft.begin();          /* TFT init */
    tft.setRotation( 3 ); /* Landscape orientation, flipped */

    /*Set the touchscreen calibration data,
     the actual data for your display can be acquired using
     the Generic -> Touch_calibrate example from the TFT_eSPI library*/
    uint16_t calData[5] = { 275, 3620, 264, 3532, 1 };
    tft.setTouch( calData );

    lv_disp_draw_buf_init( &draw_buffer, buffer, NULL, screenWidth * 10 );

    /*Initialize the display*/
    static lv_disp_drv_t display_driver;
    lv_disp_drv_init( &display_driver );
    /*Change the following line to your display resolution*/
    display_driver.hor_res = screenWidth;
    display_driver.ver_res = screenHeight;
    display_driver.flush_cb = my_display_flush;
    display_driver.draw_buf = &draw_buffer;
    lv_disp_drv_register( &display_driver );

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t input_device_driver;
    lv_indev_drv_init( &input_device_driver );
    input_device_driver.type = LV_INDEV_TYPE_POINTER;
    input_device_driver.read_cb = my_touchpad_read;
    lv_indev_drv_register( &input_device_driver );
    bool status_of_bme;

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
    status_of_bme = bme.begin(0x76);  
    if (!status_of_bme) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }

    Serial.println( "Setup done" );
}


void loop()
{
    lv_display_temperature();
    
    lv_timer_handler(); /* let the GUI do its work */
    //delay( 1000 );
}

/* event that delete label every time unit*/
static void event_cb(lv_event_t *e){
    lv_obj_t *label = lv_event_get_target(e);
    lv_obj_del_delayed(label, 1);
}

/* 
 * create label text that display air data on TFT display,
 * then delete it and create new one to refresh data
 */
void lv_display_temperature(){
    lv_obj_t *label = lv_label_create(lv_scr_act());
    char message[100];
    sprintf(message, "Temperature:\t%.2fC\nHumidity:\t%.2f%\nPressure:\t%.2fPa", 
    bme.readTemperature(), bme.readHumidity(), bme.readPressure());
    lv_obj_add_event_cb(label, event_cb, LV_EVENT_DELETE, NULL);
    lv_label_set_text(label, message);
    lv_obj_align( label, LV_ALIGN_TOP_LEFT, 5, 5 );
    lv_event_send(label, LV_EVENT_DELETE, NULL);
}