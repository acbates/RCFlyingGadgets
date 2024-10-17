
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "sbus.h" // Include the Bolder Flight SBUS library

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// SBUS data
bfs::SbusData data;

// Button pin
const int channelButtonPin = 8; // GPIO8 (D8)

// Variables
int currentChannel = 1;
int lastChannelButtonState = HIGH;

unsigned long lastDisplayUpdateTime = 0;
const unsigned long displayUpdateInterval = 60; // Update display every 60ms

// SBUS RX pin (Connect SBUS signal to this pin)
#define SBUS_RX_PIN 20 // GPIO20 (D7)

// Create a HardwareSerial instance for UART0 for SBUS communication
HardwareSerial SBUS_Serial(0); // UART0

// Initialize SBUS receiver
bfs::SbusRx sbus_rx(&SBUS_Serial, SBUS_RX_PIN, -1, true, false);

void setup() {
  // Initialize USB-CDC serial communication for debugging
  Serial.begin(115200);

  // Initialize OLED I2C with specified SDA and SCL pins
  Wire.begin(6, 7); // SDA (GPIO7), SCL (GPIO6)

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Initialize OLED display
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Loop forever if display initialization fails
  }

  display.clearDisplay();

  // Set up button pin with internal pull-up resistor
  pinMode(channelButtonPin, INPUT_PULLUP);

  // Initialize SBUS communication on UART0
  SBUS_Serial.begin(100000, SERIAL_8E2, SBUS_RX_PIN, -1, true);

  // Begin SBUS reception
  sbus_rx.Begin();

  displayValue(); // Display initial values
}

void loop() {
  // Read SBUS data
  if (sbus_rx.Read()) {
    // SBUS data has been updated
    data = sbus_rx.data();
  }

  // Handle channel selection button
  int channelButtonState = digitalRead(channelButtonPin);
  if (channelButtonState == LOW && lastChannelButtonState == HIGH) {
    // Button just pressed
    currentChannel++;
    if (currentChannel > 16) {
      currentChannel = 1;
    }
  }
  lastChannelButtonState = channelButtonState;

  // Update the display every 60 milliseconds
  unsigned long currentTime = millis();
  if (currentTime - lastDisplayUpdateTime >= displayUpdateInterval) {
    displayValue();
    lastDisplayUpdateTime = currentTime;
  }
}

uint16_t sbusToPwm(uint16_t sbusValue) {
  // Correct mapping from SBUS to PWM
  return map(sbusValue, 172, 1810, 988, 2012);
}

void displayValue() {
  // Clear the display buffer
  display.clearDisplay();

  // Set text size and color
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Prepare the channel string
  char channelStr[20];
  snprintf(channelStr, sizeof(channelStr), "Channel: %d", currentChannel);

  // Get the SBUS value
  uint16_t sbusValue = data.ch[currentChannel - 1]; // Array index starts at 0

  // Convert SBUS value to PWM value
  uint16_t pwmValue = sbusToPwm(sbusValue);

  // Prepare the SBUS value string
  char sbusStr[20];
  snprintf(sbusStr, sizeof(sbusStr), "SBUS: %d", sbusValue);

  // Prepare the PWM value string
  char pwmStr[20];
  snprintf(pwmStr, sizeof(pwmStr), " PWM: %d", pwmValue);

  // Set cursor position for the first line
  display.setCursor(0, 0);
  display.println(channelStr);

  // Draw a line under the first line
  display.drawLine(0, 15, SCREEN_WIDTH, 15, SSD1306_WHITE);

  // Display SBUS value
  display.setTextSize(1);
  display.setCursor(0, 18);
  display.println(sbusStr);

  // Display PWM value
  display.setCursor(0, 30);
  display.println(pwmStr);

  // Update the display with the new data
  display.display();
}
