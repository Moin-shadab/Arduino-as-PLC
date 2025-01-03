#include <LiquidCrystal.h>

// Pin definitions for LCD
const int LCD_RS = 8;
const int LCD_ENABLE = 9;
const int LCD_D4 = 4;
const int LCD_D5 = 5;
const int LCD_D6 = 6;
const int LCD_D7 = 7;
const int BACKLIGHT = 10;
const int BUTTON_PIN = A0; // Button connected to A0 pin

// Initialize the LCD with the interface pins
LiquidCrystal lcd(LCD_RS, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Button value thresholds (based on analog values read)
int buttonValues[5] = {50, 200, 400, 600, 800}; // Approximate thresholds for Right, Up, Down, Left, Select

void setup() {
  lcd.begin(16, 2); // Initialize the LCD with 16x2 characters
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, HIGH); // Turn on the backlight
  lcd.print("Press a button");
}

void loop() {
  int buttonValue = analogRead(BUTTON_PIN); // Read the analog value from A0

  // Determine which button was pressed based on analog value
  if (buttonValue < buttonValues[0]) { // Right button
    lcd.clear();
    lcd.print("Right Pressed");
  } 
  else if (buttonValue < buttonValues[1]) { // Up button
    lcd.clear();
    lcd.print("Up Pressed");
  } 
  else if (buttonValue < buttonValues[2]) { // Down button
    lcd.clear();
    lcd.print("Down Pressed");
  } 
  else if (buttonValue < buttonValues[3]) { // Left button
    lcd.clear();
    lcd.print("Left Pressed");
  } 
  else if (buttonValue < buttonValues[4]) { // Select button
    lcd.clear();
    lcd.print("Select Pressed");
  }

  delay(200); // Delay to avoid multiple presses being detected
}