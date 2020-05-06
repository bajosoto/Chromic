#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <timer.h>
#include "icons.h"

// SCL GPIO5
// SDA GPIO4
#define OLED_RESET 0  // GPIO0
Adafruit_SSD1306 display(OLED_RESET);

#define FONT_SIZE_X_PIXELS  5
#define FONT_SIZE_Y_PIXELS  8

#define LED_PIN             D4
#define MODE_BTN_PIN        D6
#define START_STOP_BTN_PIN  D5
#define MOTOR_PIN           D7
#define UV_LEDS_PIN         D0

typedef enum { 
  IMG_WASH,
  IMG_CURE,
  IMG_WASHCURE,
  IMG_LOGO,
} image;

typedef enum {
  MODE_WASH_01,
  MODE_WASH_05,
  MODE_WASH_10,
  MODE_WASH_15,
  MODE_WASH_20,
  MODE_CURE_01,
  MODE_CURE_05,
  MODE_CURE_10,
  MODE_CURE_15,
  MODE_CURE_20,
  MODE_WASHCURE_01,
  MODE_WASHCURE_05,
  MODE_WASHCURE_10,
  MODE_WASHCURE_15,
  MODE_WASHCURE_20,
  NUM_MODES
} mode;

#if (SSD1306_LCDHEIGHT != 48)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

auto timer = timer_create_default(); // create a timer with default settings
unsigned long time_ms = 60000; // 60 sec
bool colon = true;
bool time_running = false;

unsigned long now = millis();
unsigned long lastTrigger = 0;

bool btn_mode_pressed = false;
bool btn_start_stop_pressed = false;
mode curr_mode = MODE_WASH_01;

void setup() {

  digitalWrite(MOTOR_PIN, HIGH);    // LOW active
  digitalWrite(UV_LEDS_PIN, HIGH);  // LOW active
  digitalWrite(LED_PIN, HIGH);      // LOW active
  pinMode(LED_PIN, OUTPUT);
  pinMode(MOTOR_PIN, OUTPUT);
  pinMode(UV_LEDS_PIN, OUTPUT);

  pinMode(MODE_BTN_PIN, INPUT_PULLUP);
  pinMode(START_STOP_BTN_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(MODE_BTN_PIN), on_btn_mode_pressed, RISING);
  attachInterrupt(digitalPinToInterrupt(START_STOP_BTN_PIN), on_btn_start_stop_pressed, RISING);
  // attachInterrupt(digitalPinToInterrupt(START_STOP_BTN_PIN), on_btn_mode_pressed, RISING);


  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 64x48)
  
  // Show image buffer on the display hardware.
  display.display();
  delay(1000);

  // Clear the buffer.
  display.clearDisplay();

  // Set time
  update_icon_and_time();
  // update_signals();

  // Set timer to every half second
   timer.every(500, timer_tick_cb);

}


void loop() {
  timer.tick(); // tick the timer

  now = millis();

  if (btn_start_stop_pressed && (now - lastTrigger > (200))) {
    btn_start_stop_pressed = false;
    time_running = !time_running;
    if (time_running){
      timer.every(500, timer_tick_cb);
    } else {
      // update the interface, otherwise the colon could be hidden and look weird
      update_icon_and_time();
      update_signals();
    }

  } else if (btn_mode_pressed && (now - lastTrigger > (200))) {
    btn_mode_pressed = false;
    time_running = false;
    curr_mode = (mode)(((char)curr_mode + 1) % (char)(NUM_MODES));
    set_new_mode();
    update_icon_and_time();
    update_signals();

    // make sure the motor and leds are off
    digitalWrite(MOTOR_PIN, HIGH);    // LOW active
    digitalWrite(UV_LEDS_PIN, HIGH);  // LOW active
  }
}


ICACHE_RAM_ATTR void on_btn_mode_pressed() {
  if (btn_mode_pressed == false) {
    btn_mode_pressed = true;
    lastTrigger = millis();
  }
}


ICACHE_RAM_ATTR void on_btn_start_stop_pressed() {
  if (btn_start_stop_pressed == false) {
    btn_start_stop_pressed = true;
    lastTrigger = millis();
  }
}


void set_new_mode(){
  switch (curr_mode){
    case MODE_WASH_01:
    case MODE_CURE_01:
    case MODE_WASHCURE_01:
      time_ms = 60000;
      break;
    case MODE_WASH_05:
    case MODE_CURE_05:
    case MODE_WASHCURE_05:
      time_ms = 60000 * 5;
      break;
    case MODE_WASH_10:
    case MODE_CURE_10:
    case MODE_WASHCURE_10:
      time_ms = 60000 * 10;
      break;
    case MODE_WASH_15:
    case MODE_CURE_15:
    case MODE_WASHCURE_15:
      time_ms = 60000 * 15;
      break;
    case MODE_WASH_20:
    case MODE_CURE_20:
    case MODE_WASHCURE_20:
      time_ms = 60000 * 20;
      break;
  }
  colon = true;
}


bool timer_tick_cb(void *) {

  if (time_running) {
    // Set colon when second changes (it looks nicer :D)
    if ((time_ms - 500) % 1000 == 0)
      colon = true;
    else
      colon = false;
    
    // Set new time
    if (time_ms >= 500){
      time_ms -= 500;
    } else {
      time_ms = 0;
      colon = true;
      time_running = false;
      update_signals();
    }
    // Update the display
    update_icon_and_time();
    // Update actuator signals
    update_signals();
  } 

  if (time_running) {
    return true;
  } else {
    return false; // Stop timer
  }
}


char get_seconds(){
  return ((time_ms + 500) / 1000) % 60; // + 500  ms to round up 
}


char get_minutes(){
  return ((time_ms + 500) / 1000) / 60; // + 500  ms to round up 
}


void get_img_size(char* buffer, image img){
  char size_x;
  char size_y;

  switch(img){
    case IMG_LOGO:
      size_x = LOGO16_GLCD_WIDTH;
      size_y = LOGO16_GLCD_HEIGHT;
      break;
    case IMG_WASH:
      size_x = WASH40_GLCD_WIDTH;
      size_y = WASH40_GLCD_HEIGHT;
      break;
    case IMG_CURE:
      size_x = CURE40_GLCD_WIDTH;
      size_y = CURE40_GLCD_HEIGHT;
      break;
    case IMG_WASHCURE:
      size_x = WASHCURE40_GLCD_WIDTH;
      size_y = WASHCURE40_GLCD_HEIGHT;
      break;
  }
  buffer[0] = size_x;
  buffer[1] = size_y;
  return;
}


const unsigned char* getImg(image img){
  const unsigned char* image;
  switch(img){
    case IMG_LOGO:
      image = logo32_glcd_bmp;
      break;
    case IMG_WASH:
      image = wash40_glcd_bmp;
      break;
    case IMG_CURE:
      image = cure40_glcd_bmp;
      break;
    case IMG_WASHCURE:
      image = washcure40_glcd_bmp;
      break;
  }
  return image;
}


image get_mode_index(){
   // Assumes there's the same number of modes for each function (wash cure and washcure)
  return (image)(curr_mode / (NUM_MODES / 3));
}


void update_signals(){
  int motor = HIGH;
  int uv_leds = HIGH;
  image img = get_mode_index();
  if (time_running){
    switch (img) {
      case IMG_WASH:
        motor = LOW;   
        uv_leds = HIGH; 
        break;
      case IMG_CURE:
        motor = HIGH;   
        uv_leds = LOW; 
        break;
      case IMG_WASHCURE:
        motor = LOW;   
        uv_leds = LOW; 
        break;
      default:
        motor = HIGH;   
        uv_leds = HIGH; 
        break;
    }
  }
  digitalWrite(MOTOR_PIN, motor);    // LOW active
  digitalWrite(UV_LEDS_PIN, uv_leds);  // LOW active
}


void update_icon_and_time(){
  
  bool colon_displayed;
  image img = get_mode_index();

  // Clear the buffer.
  display.clearDisplay();
  
  // Get image size
  char img_size[2] = {0};
  get_img_size(img_size, img);

  // Set text string
  char *text = (char*)malloc(6 * sizeof(char));
  char minutes = get_minutes();
  char seconds = get_seconds();

  if (time_running == false) {
    colon_displayed = true;
  } else {
    colon_displayed = colon; 
  }
  sprintf(text, "%02d%s%02d", minutes, colon_displayed ? ":" : " ", seconds);
  digitalWrite(LED_PIN, colon_displayed ? LOW : HIGH);

  // Get text position
  char text_pos_x = ((SSD1306_LCDWIDTH - (5 * FONT_SIZE_X_PIXELS)) / 2) - (FONT_SIZE_X_PIXELS / 2); // 5 = num characters
  char text_pos_y = img_size[1] + 1;

  // Print text to buffer
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(text_pos_x, text_pos_y);
  display.print(text);

  // Set image
  const unsigned char* image = getImg(img);
  char pos_x = (SSD1306_LCDWIDTH - img_size[0]) / 2;
  char pos_y = 0;
  display.drawBitmap(pos_x, pos_y, image, img_size[0], img_size[1], 1);

  display.display();
}
