// Temporary RGB diagnostic for the board's WS2812 indicator on GPIO12.
#include <DFRobot_NeoPixel.h>

void showColor(uint8_t red, uint8_t green, uint8_t blue) {
  RGB_LED.setPixelColor(0, RGB_LED.Color(red, green, blue));
  RGB_LED.show();
  delay(2000);
}

void setup() {
  RGB_LED.begin();
  RGB_LED.setBrightness(64);
}

void loop() {
  showColor(255, 0, 0);
  showColor(0, 255, 0);
  showColor(0, 0, 255);
}
