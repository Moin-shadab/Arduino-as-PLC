#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>                  // Include the EEPROM library
#include <Wire.h>                    // Include Wire library for I2C
#include <RTClib.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);  // Initialize the LCD
RTC_DS3231 rtc;
const int openSettingsPin = 9;       // Pin for the button to open settings
const int incrementPin = 10;         // Pin for the button to increment the blinking digit
const int shiftPin = 11;             // Pin for the button to move the blinking cursor to the next digit

int pricePerBaleArray[4] = { 0, 0, 0, 0 };  // Store the 4 digits of Rs per Bale
int baleEjectArray[2] = { 0, 0 };           // Store the 2 digits of Bale Eject duration
int yarnFeederArray[2] = { 0, 0 };          // Store the 2 digits of Yarn Feeder duration
int gainFactorArray[1] = { 0 };             // Store the 1 digit of Gain Factor
int sirenSoundArray[1] = { 0 };             // Store the 1 digit of Siren Sound duration

int currentDigit = 0;              // Track the currently selected digit
unsigned long previousMillis = 0;  // To track time for blinking
const long blinkInterval = 500;    // Interval for blinking (in milliseconds)
bool inSettingsMode = false;       // Flag to track whether we're in settings mode
bool blinkState = false;           // State for blinking
bool dashboardMode = true;         // Track if we're in the dashboard mode
int settingIndex = 0;              // Track which setting we're editing
bool lcdCleared = false;           // Flag to track if the LCD has been cleared
//here
// Pin Definitions
const int hydraulicButtonPin = 2;      // Pin for hydraulic button (Manual mode)
const int manualMotoronButtonPin = 3;  // Pin for manual motor button (Manual mode)
const int selector1Pin = 4;            // Pin for selector1 (Auto mode)
const int selector2Pin = 5;            // Pin for selector2 (Auto mode)              // Pin for mode detection (Auto/Manual)
const int hydraulicRelayPin = 6;       // Pin for hydraulic relay
const int motorRelayPin = 7;           // Pin for motor relay
const int buzzerRelayPin = 8;          // Pin for buzzer relay
const int modePin = 13;
// Variables to store button states
bool hydraulicButtonState = false;
bool manualMotoronButtonState = false;
bool selector1State = false;
bool selector2State = false;
bool modeIsManual = false;
bool selector2FirstPress = false;           // Track first press of selector2
unsigned long selector2FirstPressTime = 0;  // Timer to track the delay between presses of selector2
// Cycle Count
int cycleCount = 0;
// Timing constants for Auto Mode
unsigned long setPricePerBale = 0;
unsigned long selector2Delay = 0;         // 6 seconds delay after the second press of selector2
unsigned long buzzerDuration = 0;        // 3 seconds for buzzer relay
unsigned long motorOnTimeInAuto = 0;      // 10 sec
unsigned long hydraulicOnTimeInAuto = 0;  // 10 sec
unsigned long totalCount  = 0;
unsigned long resetPressStartTime = 0;
bool isResetPressed = false;
const int resetTimeout = 3000; // 3 seconds timeout (3000 ms)
unsigned long lastTimeSaved = 0;  // Variable to track when to save RTC time
// end here
void setup() {
  //here
  // Initialize relay pins as output
  pinMode(hydraulicRelayPin, OUTPUT);
  pinMode(motorRelayPin, OUTPUT);
  pinMode(buzzerRelayPin, OUTPUT);
  // Initialize button pins as input
  pinMode(hydraulicButtonPin, INPUT_PULLUP);  // Use pull-up resistors for button inputs
  pinMode(manualMotoronButtonPin, INPUT_PULLUP);
  pinMode(selector1Pin, INPUT_PULLUP);
  pinMode(selector2Pin, INPUT_PULLUP);
  pinMode(modePin, INPUT_PULLUP);  // Set modePin as input to detect manual mode
  // Set relays to HIGH for safety at startup (turn off relays initially)
  digitalWrite(hydraulicRelayPin, HIGH);
  digitalWrite(motorRelayPin, HIGH);
  digitalWrite(buzzerRelayPin, HIGH);
  // Read the saved cycle count from EEPROM (from address 10)
  cycleCount = EEPROM.read(10);  // Read the cycle count from EEPROM at address 10
  // Initially check mode at startup
  checkMode();
  //end here
  pinMode(openSettingsPin, INPUT_PULLUP);  // Set Pin 9 as input with internal pull-up
  pinMode(incrementPin, INPUT_PULLUP);     // Set Pin 10 as input with internal pull-up
  pinMode(shiftPin, INPUT_PULLUP);         // Set Pin 11 as input with internal pull-up
  // pricePerBarelfrom = 10;
  lcd.init();       // Initialize the LCD
  lcd.backlight();  // Turn on the LCD backlight
  if (!rtc.begin()) {
    lcd.print("RTC not found!");
    while (1);
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Adjust RTC to compile time
  lcd.clear();      // Clear the LCD display
    // slideText("AUM AUTOMATION ENGINEERING", 250);
      String text = " AUM AUTOMATION ENGINEERING ";
  
  // Scroll the text
  for (int position = 0; position < text.length(); position++) {
    lcd.clear();
    lcd.setCursor(0, 0);  // Start at the first column, first row
    lcd.print(text.substring(position, position + 16)); // Display 16 characters
    delay(300); // Adjust delay for scroll speed
  }
  // lcd.setCursor(0, 0);
  // lcd.print("AUM AUTOMATION");  // Display "HI" at power-on
  // delay(2000);                  // Display "HI" for 2 seconds
  lcd.clear();                  // Clear the screen after the greeting
  // Load settings from EEPROM
  loadSettingsFromEEPROM();
   restoreRTCFromEEPROM();
}

void loop() {
    // Read the state of pin 10 and pin 11
  int pin10State = digitalRead(incrementPin);
  int pin11State = digitalRead(shiftPin);
    restCountAndTime(pin10State, pin11State);
  // Continuously check if the mode needs to change
  checkMode();
  // Handle Manual Mode actions
  if (modeIsManual) {
    handleManualMode();
  }
  // Handle Auto Mode actions
  else {
    handleAutoMode();
  }
  // If Pin 9 is shorted to GND, toggle between dashboard and settings mode
  if (digitalRead(openSettingsPin) == LOW) {
    delay(500);  // Debounce delay to avoid repeated triggering
    if (dashboardMode) {
      dashboardMode = false;
      inSettingsMode = true;
      settingIndex = 0;
      if (!lcdCleared) {  // Check if LCD has been cleared already
        lcd.clear();
        lcdCleared = true;  // Set flag to indicate LCD has been cleared
      }
    } else {
      dashboardMode = true;
      inSettingsMode = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Count:B");  // Display "HI" when returning to the dashboard
      lcdCleared = true;     // Set flag to indicate LCD has been cleared
    }
  }

  if (dashboardMode) {
    showDashboardCurrentCount();
  }

  // If we're in settings mode, handle all settings
  if (inSettingsMode) {
    unsigned long currentMillis = millis();

    // Display current setting based on settingIndex
    if (settingIndex == 0) {
      lcd.setCursor(0, 0);
      lcd.print("1.Rs[Per/Bale]    ");
      handleSetting(pricePerBaleArray);  // Handle Rs per Bale setting
    } else if (settingIndex == 1) {
      lcd.setCursor(0, 0);
      lcd.print("2.Bale Eject     ");
      handleBaleEjectSetting();  // Handle Bale Eject setting
    } else if (settingIndex == 2) {
      lcd.setCursor(0, 0);
      lcd.print("3.Yarn Feeder    ");
      handleYarnFeederSetting();  // Handle Yarn Feeder setting
    } else if (settingIndex == 3) {
      lcd.setCursor(0, 0);
      lcd.print("4.Gain Factor    ");
      handleGainFactorSetting();  // Handle Gain Factor setting
    } else if (settingIndex == 4) {
      lcd.setCursor(0, 0);
      lcd.print("5.Siren Sound    ");
      handleSirenSoundSetting();  // Handle Siren Sound setting
    }
  }
  DateTime now = rtc.now();  // Get current time from RTC
  // Calculate hours and minutes
  int hours = now.hour();
  int minutes = now.minute();
    lcd.setCursor(10, 1);
  lcd.print("H");
  if (hours < 10) lcd.print("0");  // Add leading zero if hours < 10
  lcd.print(hours);
  lcd.print(":");
  if (minutes < 10) lcd.print("0");  // Add leading zero if minutes < 10
  lcd.print(minutes);
    // Save RTC time to EEPROM every 2 minutes
  unsigned long currentMillis = millis();
  if (currentMillis - lastTimeSaved >= 60000) {  // Check if 2 minutes have passed (120000 ms)
    saveRTCToEEPROM(now);
    lastTimeSaved = currentMillis;  // Update the last saved time
  }
}

// Load settings from EEPROM
void loadSettingsFromEEPROM() {
  setPricePerBale = calculatePricePerBale(0);
  hydraulicOnTimeInAuto = calculateHydraulicTime(4);
  motorOnTimeInAuto = calculateMotorTime(6);
  selector2Delay = calculateDelay(8);
  buzzerDuration = calculateBuzzerDuration(9);
  // motorOnTimeInAuto = motorOnTimeInAuto - buzzerDuration ;
  totalCount = EEPROM.read(10); // Read total count from EEPROM
}

// Handle setting values for other settings
void handleSetting(int settingArray[]) {
  unsigned long currentMillis = millis();
  static bool pricePerBarrelCleared = false;  // Flag to track if it's cleared already
  if (!pricePerBarrelCleared) {
    lcd.clear();
    pricePerBarrelCleared = true;  // Set flag to indicate it has been cleared
  }
  // Check if it's time to blink the digit
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;  // Update the time for next blink
    blinkState = !blinkState;        // Toggle the blink state

    // Print the current setting values
    for (int i = 0; i < 4; i++) {
      lcd.setCursor(i, 1);      // Set the cursor for each digit
      if (i == currentDigit) {  // Only the selected digit should blink
        if (blinkState) {
          lcd.print(" ");  // Blank the current digit (blink effect)
        } else {
          lcd.print(settingArray[i]);  // Reprint the current digit
        }
      } else {
        lcd.print(settingArray[i]);  // Print the other digits normally
      }
    }
  }

  // If Pin 10 is shorted to GND, increment the blinking digit
  if (digitalRead(incrementPin) == LOW) {
    delay(500);                            // Debounce delay to avoid repeated triggering
    settingArray[currentDigit]++;          // Increment the currently selected digit
    if (settingArray[currentDigit] > 9) {  // If the digit exceeds 9, reset to 0
      settingArray[currentDigit] = 0;
    }
  }

  // If Pin 11 is shorted to GND, move the blinking cursor to the next digit
  if (digitalRead(shiftPin) == LOW) {
    delay(500);              // Debounce delay to avoid repeated triggering
    currentDigit++;          // Move to the next digit
    if (currentDigit > 3) {  // If we've reached the last digit, go back to the first digit
      currentDigit = 0;
    }
  }

  // Once the user finishes setting, save it to EEPROM
  if (digitalRead(openSettingsPin) == LOW) {
    // Save the setting to EEPROM
    int startAddress = settingIndex * 4;
    for (int i = 0; i < 4; i++) {
      EEPROM.write(startAddress + i, settingArray[i]);
    }

    settingIndex++;  // Move to the next setting
    if (settingIndex > 4) {
      settingIndex = 0;
      dashboardMode = true;
      inSettingsMode = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Setting Saved");
      delay(2000);  // Wait for 2 seconds
      lcd.clear();
      loadSettingsFromEEPROM();
    }
    delay(500);  // Debounce delay
  }
}

// Handle Bale Eject setting values (only 2 digits)
void handleBaleEjectSetting() {
  unsigned long currentMillis = millis();
  // Only clear once when entering Yarn Feeder setting
  static bool baleEjectCleared = false;  // Flag to track if it's cleared already
  if (!baleEjectCleared) {
    lcd.clear();
    baleEjectCleared = true;  // Set flag to indicate it has been cleared
  }
  // Clear the screen before showing Bale Eject
  // lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("2.Bale Eject");

  // Check if it's time to blink the digit
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;  // Update the time for next blink
    blinkState = !blinkState;        // Toggle the blink state

    // Display Bale Eject setting with blinking digit
    for (int i = 0; i < 2; i++) {
      lcd.setCursor(i, 1);      // Set the cursor for each digit
      if (i == currentDigit) {  // Only the selected digit should blink
        if (blinkState) {
          lcd.print(" ");  // Blank the current digit (blink effect)
        } else {
          lcd.print(baleEjectArray[i]);  // Reprint the current digit
        }
      } else {
        lcd.print(baleEjectArray[i]);  // Print the other digits normally
      }
    }
    lcd.setCursor(2, 1);  // After printing digits, display "deg" for Bale Eject
    lcd.print(" deg");
  }

  // If Pin 10 is shorted to GND, increment the blinking digit
  if (digitalRead(incrementPin) == LOW) {
    delay(500);                              // Debounce delay to avoid repeated triggering
    baleEjectArray[currentDigit]++;          // Increment the currently selected digit
    if (baleEjectArray[currentDigit] > 9) {  // If the digit exceeds 9, reset to 0
      baleEjectArray[currentDigit] = 0;
    }
  }

  // If Pin 11 is shorted to GND, move the blinking cursor to the next digit
  if (digitalRead(shiftPin) == LOW) {
    delay(500);              // Debounce delay to avoid repeated triggering
    currentDigit++;          // Move to the next digit
    if (currentDigit > 1) {  // If we've reached the last digit, go back to the first digit
      currentDigit = 0;
    }
  }

  // Once the user finishes setting, save Bale Eject to EEPROM
  if (digitalRead(openSettingsPin) == LOW) {
    // Save the Bale Eject setting to EEPROM
    EEPROM.write(4, baleEjectArray[0]);
    EEPROM.write(5, baleEjectArray[1]);

    settingIndex++;  // Move to the next setting
    if (settingIndex > 4) {
      settingIndex = 0;
      dashboardMode = true;
      inSettingsMode = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Setting Saved");
      lcd.clear();
      delay(2000);  // Wait for 2 seconds
      lcd.clear();
      showDashboardCurrentCount();
    }
    delay(500);  // Debounce delay
    lcd.clear();
  }
}

// Handle Yarn Feeder setting values (only 2 digits)
void handleYarnFeederSetting() {
  unsigned long currentMillis = millis();

  // Only clear once when entering Yarn Feeder setting
  static bool yarnFeederCleared = false;  // Flag to track if it's cleared already
  if (!yarnFeederCleared) {
    lcd.clear();
    yarnFeederCleared = true;  // Set flag to indicate it has been cleared
  }

  lcd.setCursor(0, 0);
  lcd.print("3.Yarn Feeder");

  // Check if it's time to blink the digit
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;  // Update the time for next blink
    blinkState = !blinkState;        // Toggle the blink state

    // Display Yarn Feeder setting with blinking digit
    for (int i = 0; i < 2; i++) {
      lcd.setCursor(i, 1);      // Set the cursor for each digit
      if (i == currentDigit) {  // Only the selected digit should blink
        if (blinkState) {
          lcd.print(" ");  // Blank the current digit (blink effect)
        } else {
          lcd.print(yarnFeederArray[i]);  // Reprint the current digit
        }
      } else {
        lcd.print(yarnFeederArray[i]);  // Print the other digits normally
      }
    }
    lcd.setCursor(2, 1);  // After printing digits, display "cm" for Yarn Feeder
    lcd.print(" cm");
  }

  // If Pin 10 is shorted to GND, increment the blinking digit
  if (digitalRead(incrementPin) == LOW) {
    delay(500);                               // Debounce delay to avoid repeated triggering
    yarnFeederArray[currentDigit]++;          // Increment the currently selected digit
    if (yarnFeederArray[currentDigit] > 9) {  // If the digit exceeds 9, reset to 0
      yarnFeederArray[currentDigit] = 0;
    }
  }

  // If Pin 11 is shorted to GND, move the blinking cursor to the next digit
  if (digitalRead(shiftPin) == LOW) {
    delay(500);              // Debounce delay to avoid repeated triggering
    currentDigit++;          // Move to the next digit
    if (currentDigit > 1) {  // If we've reached the last digit, go back to the first digit
      currentDigit = 0;
    }
  }

  // Once the user finishes setting, save Yarn Feeder to EEPROM
  if (digitalRead(openSettingsPin) == LOW) {
    // Save the Yarn Feeder setting to EEPROM
    EEPROM.write(6, yarnFeederArray[0]);
    EEPROM.write(7, yarnFeederArray[1]);

    settingIndex++;  // Move to the next setting
    if (settingIndex > 4) {
      settingIndex = 0;
      dashboardMode = true;
      inSettingsMode = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Setting Saved");
      delay(2000);  // Wait for 2 seconds
      lcd.clear();
      lcdCleared = false;  // Reset LCD cleared flag when returning to dashboard
    }
    delay(500);  // Debounce delay
  }
}

// Handle Gain Factor setting (1 digit)
void handleGainFactorSetting() {
  unsigned long currentMillis = millis();
  static bool gainFactorCleared = false;  // Flag to track if it's cleared already
  if (!gainFactorCleared) {
    lcd.clear();
    gainFactorCleared = true;  // Set flag to indicate it has been cleared
  }
  // Clear the screen before showing Gain Factor
  // lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("4.Gain Factor");

  // Check if it's time to blink the digit
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;  // Update the time for next blink
    blinkState = !blinkState;        // Toggle the blink state

    // Display Gain Factor setting with blinking digit
    lcd.setCursor(0, 1);
    if (blinkState) {
      lcd.print(" ");  // Blank the current digit (blink effect)
    } else {
      lcd.print(gainFactorArray[0]);  // Reprint the current digit
    }
    lcd.print(" db");
  }

  // If Pin 10 is shorted to GND, increment the blinking digit
  if (digitalRead(incrementPin) == LOW) {
    delay(500);                    // Debounce delay to avoid repeated triggering
    gainFactorArray[0]++;          // Increment the current digit
    if (gainFactorArray[0] > 9) {  // If the digit exceeds 9, reset to 0
      gainFactorArray[0] = 0;
    }
  }

  // Once the user finishes setting, save Gain Factor to EEPROM
  if (digitalRead(openSettingsPin) == LOW) {
    // Save the Gain Factor setting to EEPROM
    EEPROM.write(8, gainFactorArray[0]);

    settingIndex++;  // Move to the next setting
    if (settingIndex > 4) {
      settingIndex = 0;
      dashboardMode = true;
      inSettingsMode = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Setting Saved");
      delay(2000);  // Wait for 2 seconds
      lcd.clear();
    }
    delay(500);  // Debounce delay
  }
}

// Handle Siren Sound setting (1 digit)
void handleSirenSoundSetting() {
  unsigned long currentMillis = millis();
  static bool sirenSoundCleared = false;  // Flag to track if it's cleared already
  if (!sirenSoundCleared) {
    lcd.clear();
    sirenSoundCleared = true;  // Set flag to indicate it has been cleared
  }
  // Clear the screen before showing Siren Sound
  // lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("5.Siren Sound");

  // Check if it's time to blink the digit
  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;  // Update the time for next blink
    blinkState = !blinkState;        // Toggle the blink state

    // Display Siren Sound setting with blinking digit
    lcd.setCursor(0, 1);
    if (blinkState) {
      lcd.print(" ");  // Blank the current digit (blink effect)
    } else {
      lcd.print(sirenSoundArray[0]);  // Reprint the current digit
    }
    lcd.print(" sec");
  }

  // If Pin 10 is shorted to GND, increment the blinking digit
  if (digitalRead(incrementPin) == LOW) {
    delay(500);                    // Debounce delay to avoid repeated triggering
    sirenSoundArray[0]++;          // Increment the current digit
    if (sirenSoundArray[0] > 9) {  // If the digit exceeds 9, reset to 0
      sirenSoundArray[0] = 0;
    }
  }

  // Once the user finishes setting, save Siren Sound to EEPROM
  if (digitalRead(openSettingsPin) == LOW) {
    // Save the Siren Sound setting to EEPROM
    EEPROM.write(9, sirenSoundArray[0]);

    settingIndex++;  // Move to the next setting
    if (settingIndex > 4) {
      settingIndex = 0;
      dashboardMode = true;
      inSettingsMode = false;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Setting Saved");
      delay(2000);  // Wait for 2 seconds
      lcd.clear();
    }
    delay(500);  // Debounce delay
  }
}

// Function to handle Manual Mode
void handleManualMode() {
  static unsigned long lastHydraulicPressTime = 0;  // Track the last time the hydraulic button was pressed
  static bool lastHydraulicButtonState = false;    // Track the last hydraulic button state

  // Read hydraulic button state
  hydraulicButtonState = digitalRead(hydraulicButtonPin) == LOW;          // Active LOW
  manualMotoronButtonState = digitalRead(manualMotoronButtonPin) == LOW;  // Active LOW

  // Handle hydraulic button with debounce
  if (hydraulicButtonState && !lastHydraulicButtonState) {
    unsigned long currentTime = millis();

    // Check if enough time has passed since the last valid press
    if (currentTime - lastHydraulicPressTime > 300) {  // Debounce time in milliseconds
      cycleCount++;
      EEPROM.write(10, cycleCount);  // Save the updated cycle count to EEPROM
      totalCount = EEPROM.read(10);
      showDashboardCurrentCount();
      lastHydraulicPressTime = currentTime;  // Update the last press time
    }
  }

  // Update the last button state
  lastHydraulicButtonState = hydraulicButtonState;

  // Control hydraulic relay
  if (hydraulicButtonState) {
    digitalWrite(hydraulicRelayPin, LOW);  // Turn ON hydraulic relay (Active LOW)
  } else {
    digitalWrite(hydraulicRelayPin, HIGH);  // Turn OFF hydraulic relay
  }

  // Manual Motor button control (turn motor relay on/off)
  if (manualMotoronButtonState) {
    digitalWrite(motorRelayPin, LOW);  // Turn ON motor relay (Active LOW)
  } else {
    digitalWrite(motorRelayPin, HIGH);  // Turn OFF motor relay
  }
}

// Function to handle Auto Mode
void handleAutoMode() {
  // Read the state of the selector buttons
  selector1State = digitalRead(selector1Pin) == LOW;  // Active LOW
  selector2State = digitalRead(selector2Pin) == LOW;  // Active LOW

  // Handle Selector1 (Motor and Buzzer Relay simultaneous on)
  if (selector1State) {
    digitalWrite(motorRelayPin, LOW);  // Turn ON Motor Relay (Active LOW)
    digitalWrite(buzzerRelayPin, LOW); // Turn ON Buzzer Relay (Active LOW)
    delay(buzzerDuration);  // Wait for buzzer duration
    digitalWrite(buzzerRelayPin, HIGH); // Turn OFF Buzzer Relay (Active HIGH)
    delay(motorOnTimeInAuto);  // Motor relay stays on for some time (10 seconds)
    digitalWrite(motorRelayPin, HIGH);  // Turn OFF Motor Relay (Active HIGH)
  }

  // Handle Selector2 behavior (first and second press alternation)
  if (selector2State) {
    // If first press of Selector2
    if (!selector2FirstPress) {
      selector2FirstPress = true;  // Mark as first press
      selector2FirstPressTime = millis();  // Store time of first press
      // Turn on the buzzer for 3 seconds
      digitalWrite(buzzerRelayPin, LOW);   // Turn ON Buzzer Relay (Active LOW)
      delay(buzzerDuration);               // Wait for 3 seconds
      digitalWrite(buzzerRelayPin, HIGH);  // Turn OFF Buzzer Relay (Active HIGH)
    } 
    else {
      // If second press of Selector2 (cycle continues)
      if (millis() - selector2FirstPressTime >= selector2Delay) {
        // Turn on the buzzer for 3 seconds again
        digitalWrite(buzzerRelayPin, LOW);   // Turn ON Buzzer Relay (Active LOW)
        delay(buzzerDuration);               // Wait for 3 seconds
        digitalWrite(buzzerRelayPin, HIGH);  // Turn OFF Buzzer Relay (Active HIGH)

        // After 6-second delay, activate Hydraulic Relay for 10 seconds
        digitalWrite(hydraulicRelayPin, LOW);   // Turn ON Hydraulic Relay (Active LOW)
        delay(hydraulicOnTimeInAuto);           // Wait for 10 seconds
        digitalWrite(hydraulicRelayPin, HIGH);  // Turn OFF Hydraulic Relay (Active HIGH)

        // Increment the cycle count and save it to EEPROM
        cycleCount++;
        EEPROM.write(10, cycleCount);  // Save the updated cycle count to EEPROM
        totalCount = EEPROM.read(10);  // Read the total count back from EEPROM
        showDashboardCurrentCount();   // Update the display with the new count

        // Reset first press flag to start a new cycle for next press
        selector2FirstPress = false;
      }
    }
  }
}

// Function to check if the mode should be Manual or Auto
void checkMode() {
  // If Pin 13 is shorted to GND, it's Manual Mode
  if (digitalRead(modePin) == LOW) {
    modeIsManual = true;
  } else {
    modeIsManual = false;
  }
}

long calculatePricePerBale(int address) {
  pricePerBaleArray[0] = EEPROM.read(address);
  pricePerBaleArray[1] = EEPROM.read(address + 1);
  pricePerBaleArray[2] = EEPROM.read(address + 2);
  pricePerBaleArray[3] = EEPROM.read(address + 3);

  // Combine the four bytes into one number
  return pricePerBaleArray[0] * 1000 + pricePerBaleArray[1] * 100 + pricePerBaleArray[2] * 10 + pricePerBaleArray[3];
}

// Function to calculate Hydraulic Time (in milliseconds)
long calculateHydraulicTime(int address) {
  baleEjectArray[0] = EEPROM.read(address);
  baleEjectArray[1] = EEPROM.read(address + 1);

  // Combine the two bytes and convert to milliseconds (multiplied by 1000)
  return (baleEjectArray[0] * 10 + baleEjectArray[1]) * 1000;
}

// Function to calculate Motor Time (in milliseconds)
long calculateMotorTime(int address) {
  yarnFeederArray[0] = EEPROM.read(address);
  yarnFeederArray[1] = EEPROM.read(address + 1);

  // Combine the two bytes and convert to milliseconds (multiplied by 1000)
  return (yarnFeederArray[0] * 10 + yarnFeederArray[1]) * 1000;
}

// Function to calculate Selector 2 Delay (in milliseconds)
long calculateDelay(int address) {
  gainFactorArray[0] = EEPROM.read(address);

  // Convert gain factor to milliseconds
  return gainFactorArray[0] * 1000;
}

// Function to calculate Buzzer Duration (in milliseconds)
long calculateBuzzerDuration(int address) {
  sirenSoundArray[0] = EEPROM.read(address);

  // Convert siren sound value to milliseconds
  return sirenSoundArray[0] * 1000;
}

void showDashboardCurrentCount() {
    // lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Count:");
    lcd.print(totalCount);  // Assuming totalCount is updated to the correct value
    lcd.setCursor(0, 1);
    lcd.print("Rs");
    lcd.print(totalCount * setPricePerBale);  // Assuming you have setPricePerBale defined
}

void restCountAndTime(int pin10State, int pin11State) { 
    // Check if both pin 10 and pin 11 are shorted to GND (low state)
  if (pin10State == LOW && pin11State == LOW) {
    if (!isResetPressed) {
      // Start counting when the reset condition is pressed for the first time
      resetPressStartTime = millis();
      isResetPressed = true;
    }

    // Check if 3 seconds have passed
    if (millis() - resetPressStartTime >= resetTimeout) {
      // Set EEPROM value at address 10 to 0 (reset the value)
      EEPROM.write(10, 0);      
      
      // After resetting EEPROM, update the totalCount variable
      totalCount = EEPROM.read(10);  // Read the value back from EEPROM to ensure it's updated
      
      // Optionally reset the reset value to 0 after updating EEPROM
      int resetValue = 0; // Reset the variable to 0
      // Reset the RTC time to 00:00 (midnight)
      rtc.adjust(DateTime(2024, 1, 1, 0, 0, 0));  // Set date and time to January 1st, 00:00:00
            // Refresh the display
      showDashboardCurrentCount();
    }
  } else {
    // If reset condition is not met, reset the isResetPressed flag
    isResetPressed = false;
  }
}

// Function to restore the RTC time from EEPROM (if saved)
void restoreRTCFromEEPROM() {
  int hour = EEPROM.read(11);   // Read hour from EEPROM address 11
  int minute = EEPROM.read(12); // Read minute from EEPROM address 12
  int second = EEPROM.read(13); // Read second from EEPROM address 13

  // If we have valid time values in EEPROM, set the RTC time accordingly
  if (hour >= 0 && hour < 24 && minute >= 0 && minute < 60 && second >= 0 && second < 60) {
    rtc.adjust(DateTime(2024, 1, 1, hour, minute, second));  // Restore RTC time
  }
}

// Function to save the RTC time to EEPROM (starting from address 11)
void saveRTCToEEPROM(DateTime now) {
  int addr = 11;  // Starting EEPROM address after 10 (which is reserved for totalCount)
  // Store the hour, minute, and second in EEPROM
  EEPROM.write(addr, now.hour());     // Store hour at address 11
  EEPROM.write(addr + 1, now.minute()); // Store minute at address 12
  EEPROM.write(addr + 2, now.second()); // Store second at address 13

}

// Function to slide text from right to left

