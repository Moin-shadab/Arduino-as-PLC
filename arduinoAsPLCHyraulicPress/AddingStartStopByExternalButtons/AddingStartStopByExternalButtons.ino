#include <LiquidCrystal.h>
#include <EEPROM.h>

// Pin definitions for LCD
const int LCD_RS = 8;
const int LCD_ENABLE = 9;
const int LCD_D4 = 4;
const int LCD_D5 = 5;
const int LCD_D6 = 6;
const int LCD_D7 = 7;
const int BACKLIGHT = 10;
const int BUTTON_PIN = A0;  // Button connected to A0 pin
#define PNP_SENSOR_PIN A1
// Relay pin definitions
const int PumpCloseRelay = 2;
const int PumpOpenRelay = 3;
// Initialize the LCD with the interface pins
LiquidCrystal lcd(LCD_RS, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
// Button value thresholds
int buttonValues[5] = { 50, 200, 400, 600, 800 };
int currentSelection = 0;
bool inSettingsMode = false;
bool inMainMenu = true;
bool inSetR1Mode = false;
bool inSetDelayMode = false;
bool inSetR3Mode = false;
bool inStartMode = false;  // New flag for Start mode
bool isOff = false;
bool buttonPressed = false;
float SetPumpOnTime = 0.0;     // Time for Relay 1
float setDelayTime = 0.0;  // Time for Delay 
float SetPumpOfTime = 0.0;     // Time for Relay 2
float stepSize = 0.1;      // Increment/Decrement step size
unsigned long relay1StartTime = 0;
unsigned long relay2DelayTime = 0;
unsigned long relay3StartTime = 0;
bool isRelay1Active = false;             // Track if Relay 1 is active
bool isRelay2Active = false;             // Track if Relay 2 is active
bool flag = false;                       // Track if Relay 2 is active
unsigned long buttonPressStartTime = 0;  // Timer for long press
bool longPressDetected = false;          // Flag for long press
unsigned long relayDelayStartTime = 0;   // Timer for delay before Relay 2 activates
const int MainMotorbuttonDOne = 1;       // D1 pin for the external button
float previousR1Time = -1.0;             // Initialize with a value that's unlikely to match the default value
// EEPROM addresses for saving times
const int EEPROM_ADDR_PUMP_ON_TIME = 0;
const int EEPROM_ADDR_DELAY_TIME = 4;  // Different EEPROM address for Set R2
const int EEPROM_ADDR_PUMP_OFF_TIME = 8;  // Different EEPROM address for Set R2
const int MainMotorRelayA2Pin = A2;              // Pin A2 for controlling an external device
// Menu items in the main menu
char* menuItems[2] = { "1: Settings" };  // Add Settings option
char* settingItems[4] = { "Press. C Time", "Delay Time", "Press. O Time", "Back" };
int currentSettingSelection = 0;
bool isCycleRunning = false;  // Flag to track if the cycle is running
const int GarrariMotorButtonDzero = 0;         // Pin D0 to detect the short circuit
const int GarrariMotorRelayA3Pin = A3;        // Pin A3 to toggle between HIGH and LOW
bool previousD0State = LOW;  // To store the previous state of D0
bool currentStateA3 = LOW;   // The current state of A3 (starting as LOW)

void setup() {
  // Serial.begin(9600);  // Initialize serial communication
  lcd.begin(16, 2);
  pinMode(BACKLIGHT, OUTPUT);
  digitalWrite(BACKLIGHT, LOW);
  pinMode(PumpCloseRelay, OUTPUT);
  pinMode(PumpOpenRelay, OUTPUT);
  pinMode(PNP_SENSOR_PIN, INPUT);              // Set PNP sensor pin as input
  pinMode(MainMotorbuttonDOne, INPUT_PULLUP);  // External button pin (with internal pull-up resistor)
  pinMode(MainMotorRelayA2Pin, OUTPUT);
  digitalWrite(MainMotorRelayA2Pin, LOW);  // A2 pin set as output
  digitalWrite(PumpCloseRelay, LOW);
  digitalWrite(PumpOpenRelay, LOW);
  pinMode(GarrariMotorButtonDzero, INPUT_PULLUP);         // Set D0 as an input with pull-up resistor enabled
  pinMode(GarrariMotorRelayA3Pin, OUTPUT);               // Set A3 as an output
  digitalWrite(GarrariMotorRelayA3Pin, currentStateA3);  // Initialize A3 to LOW
  // Scroll text "AUMAUTOMATION ENGINEERING" for 2 seconds
  // lcd.clear();
  // lcd.print("AUMAUTOMATION Engg.");
  
  // Scroll the text from right to left
  for (int i = 0; i < 16 + strlen("AUMAUTOMATION ENGINEERING"); i++) {
    lcd.scrollDisplayLeft();  // Scroll the text
    delay(300);  // Adjust the speed of scrolling (increase or decrease the delay)
  }
  lcd.clear();
  // Wait for 2 seconds after the scrolling is done
  // delay(1000);  // Wait for 2 seconds
  // Load saved times from EEPROM with a default value fallback
  EEPROM.get(EEPROM_ADDR_PUMP_ON_TIME, SetPumpOnTime);
  EEPROM.get(EEPROM_ADDR_DELAY_TIME, setDelayTime);
  EEPROM.get(EEPROM_ADDR_PUMP_OFF_TIME, SetPumpOfTime);
  // If R1 or R2 time are uninitialized, assign default values
  if (isnan(SetPumpOnTime) || SetPumpOnTime < 0.0 || SetPumpOnTime > 99.9) {
    SetPumpOnTime = 0.0;
  }
  if (isnan(setDelayTime) || setDelayTime < 0.0 || setDelayTime > 99.9) {
    setDelayTime = 0.0;
  }
  if (isnan(SetPumpOfTime) || SetPumpOfTime < 0.0 || SetPumpOfTime > 99.9) {
    SetPumpOfTime = 0.0;
  }
  displayMenu();
}

void loop() {
  int buttonValue = analogRead(BUTTON_PIN);
  int externalButtonState = digitalRead(MainMotorbuttonDOne);  // Read external button state (D1)
  bool currentD0State = digitalRead(GarrariMotorButtonDzero);                   // Read the current state of D0
  // Check if D0 is being shorted to GND (i.e., D0 goes LOW)
  if (previousD0State == HIGH && currentD0State == LOW) {
    // D0 was just shorted, toggle A3
    currentStateA3 = !currentStateA3;      // Toggle the state of A3
    digitalWrite(GarrariMotorRelayA3Pin, currentStateA3);  // Apply the new state to A3

    delay(200);  // Debounce delay to avoid multiple toggles from the same short
  }

  // Store the current state of D0 for the next loop iteration
  previousD0State = currentD0State;
  // Check if the external button is pressed (shorted to GND) to toggle Start/Stop mode
  if (externalButtonState == LOW && !buttonPressed) {
    buttonPressed = true;
    if (inStartMode) {
      // If already in Start Mode, stop and go back to the main menu
      stopSystem();
    } else {
      // If not in Start Mode, enter Start Mode
      enterStartMode();
    }
  } else if (externalButtonState == HIGH && buttonPressed) {
    buttonPressed = false;  // Button released
  }
  // Check if the external button (D1) is pressed (GND) to enter Start Mode
  if (externalButtonState == LOW && !buttonPressed) {
    // Button is pressed (shorted to GND)
    buttonPressed = true;
    // Serial.println("External button pressed, entering Start Mode...");
    enterStartMode();  // Function to enter Start Mode
  } else if (externalButtonState == HIGH && buttonPressed) {
    // Button was released
    buttonPressed = false;
    // Serial.println("External button released.");
  }
  // Handle other modes based on buttonValue (main menu, settings, etc.)
  if (inMainMenu) {
    handleMainMenu(buttonValue);
  } else if (inSettingsMode) {
    handleSettingsMode(buttonValue);
  } else if (inSetR1Mode) {
    handleSetR1(buttonValue);
  } else if (inSetDelayMode) {
    handleDelayR2(buttonValue);
  }  else if (inSetR3Mode) {
    handleDelayR3(buttonValue);
  }else if (inStartMode) {
    handleStartMode();
  }
  // Handle button press to navigate back
  if (buttonValue < buttonValues[0] && !buttonPressed) {  // Right button
    buttonPressed = true;
    if (inSetR1Mode) {
      inSetR1Mode = false;
      inSettingsMode = true;
      displaySettingsMenu();
    } else if (inSetDelayMode) {
      inSetDelayMode = false;
      inSettingsMode = true;
      displaySettingsMenu();
    }  else if (inSetR3Mode) {
      inSetR3Mode = false;
      inSettingsMode = true;
      displaySettingsMenu();
    }else if (inStartMode) {
      // Turn off both relays
      digitalWrite(PumpCloseRelay, LOW);
      digitalWrite(PumpOpenRelay, LOW);
      lcd.clear();
      lcd.print("Relays OFF");
      delay(1000);
      buttonPressed = false;
      inMainMenu = true;
      displayMenu();
    }
  }

  if (buttonValue >= buttonValues[0]) {
    buttonPressed = false;
  }
  delay(200);  // Small delay to avoid bouncing issues
}

// Function to enter the Start Mode when the external button is pressed
void enterStartMode() {
  if (!isCycleRunning) {
    isCycleRunning = true;
    inStartMode = true;
    inMainMenu = false;
    lcd.clear();
    lcd.print(" Machine started ");
    delay(500);
    // digitalWrite(MainMotorRelayA2Pin, LOW);  // Set A2 to LOW when start mode is entered
  } else {
    // Stop the system and set A2 to HIGH
    stopSystem();
  }
}

void stopSystem() {
  digitalWrite(MainMotorRelayA2Pin, LOW);
  digitalWrite(PumpCloseRelay, LOW);
  digitalWrite(PumpOpenRelay, LOW);
  inStartMode = false;
  inMainMenu = true;
  lcd.clear();
  lcd.print("System Stopped");
  delay(1000);
  isCycleRunning = false;  // Clear the flag
  displayMenu();
}

void handleMainMenu(int buttonValue) {
  if (buttonValue < buttonValues[1] && !buttonPressed) {  // Down button pressed
    buttonPressed = true;
    if (currentSelection < 0) {  // Prevent scrolling beyond the only menu option
      currentSelection++;
      displayMenu();  // Refresh the display with updated selection
    }
  } else if (buttonValue < buttonValues[2] && !buttonPressed) {  // Up button pressed
    buttonPressed = true;
    if (currentSelection > 0) {  // Prevent scrolling beyond the only menu option
      currentSelection--;
      displayMenu();  // Refresh the display with updated selection
    }
  } else if (buttonValue < buttonValues[4] && !buttonPressed) {  // Select button pressed
    buttonPressed = true;
    if (currentSelection == 0) {  // If "Settings" is selected
      enterSettingsMode();        // Enter the Settings menu
    }
  }
}

void displayMenu() {
  lcd.clear();
  for (int i = 0; i < 2; i++) {
    lcd.setCursor(1, i);
    if (currentSelection + i < 2) {  // Changed from 4 to 2
      lcd.print(menuItems[currentSelection + i]);
    }
  }
  lcd.setCursor(0, 0);
  lcd.write('>');
}

void enterSettingsMode() {
  inSettingsMode = true;
  inMainMenu = false;
  currentSettingSelection = 0;
  displaySettingsMenu();
}

void handleSettingsMode(int buttonValue) {
  if (buttonValue < buttonValues[1] && !buttonPressed) {
    if (currentSettingSelection > 0) {
      currentSettingSelection--;
      displaySettingsMenu();
    }
  } else if (buttonValue < buttonValues[2] && !buttonPressed) {
    if (currentSettingSelection < 2) {
      currentSettingSelection++;
      displaySettingsMenu();
    }
  } else if (buttonValue < buttonValues[4] && !buttonPressed) {
    buttonPressed = true;
    if (currentSettingSelection == 3) {
      inSettingsMode = false;
      inMainMenu = true;
      displayMenu();
    } else if (currentSettingSelection == 0) {
      inSetR1Mode = true;
      inSettingsMode = false;
      displaySetR1();
    } else if (currentSettingSelection == 1) {
      inSetDelayMode = true;
      inSettingsMode = false;
      displaySetR2();
    }
    else if (currentSettingSelection == 2) {
      inSetR3Mode = true;
      inSettingsMode = false;
      displaySetR3();
    }
  }
}

// New function to display the start mode
void displayStartMode() {
  // Serial.println("start menu...");
  inMainMenu = false;
  inStartMode = true;
  lcd.clear();
  lcd.print(" Machine started ");
  delay(500);
}

// Add these declarations at the top with other global variables
unsigned long delayStartTime = 0;  // Timer for delay before Relay 2 activates
float remainingDelayTime = 0.0;    // Remaining delay time before Relay 2 turns on

void handleStartMode() {
  static bool delayTimerStarted = false;
  static bool IsPumpClose = false;
  static bool IsPumpOn = false;
  static bool cycleCompleted = false;
  static bool metalDetected = false;    // Metal detection flag
  static bool cycleInProgress = false;  // Flag to track if the cycle is in progress
  digitalWrite(MainMotorRelayA2Pin, HIGH);            // Set A2 to LOW when start mode is entered

  // Check for metal detection using the PNP sensor pin
  if (digitalRead(PNP_SENSOR_PIN) == HIGH) {  // Metal detected
    metalDetected = true;
  }

  // Step 1: If metal is detected, start/restart the cycle
  if (metalDetected && !cycleInProgress) {
    // Metal detected and cycle is not in progress, start the cycle
    cycleInProgress = true;  // Mark that a cycle has started
    metalDetected = false;   // Reset the metal detection flag

    // Reset all flags and states to restart the cycle
    IsPumpClose = false;
    IsPumpOn = false;
    cycleCompleted = false;
    delayTimerStarted = false;
    isRelay1Active = false;
    digitalWrite(PumpCloseRelay, LOW);  // Turn off Relay 1
    digitalWrite(PumpOpenRelay, LOW);  // Turn off Relay 2

    // Start the cycle from Relay 1
    digitalWrite(PumpCloseRelay, HIGH);     // Turn on Relay 1
    relay1StartTime = millis();        // Note start time for Relay 1
    delayStartTime = relay1StartTime;  // Start delay timer simultaneously
    isRelay1Active = true;
    delayTimerStarted = true;

    // Display R1 ON status and Delay Time text
    lcd.clear();
    lcd.print("Pressure Close ");
    lcd.setCursor(13, 0);
    lcd.print(SetPumpOnTime, 1);
    lcd.setCursor(0, 1);
    lcd.print("Delay Time     ");  // Show "Delay Time" on line 2
    lcd.setCursor(13, 1);
    lcd.print(setDelayTime, 1);
  }

  // Step 2: If Relay 1 is running, handle Relay 1 timing
  if (isRelay1Active && !IsPumpClose && !cycleCompleted) {
    float elapsedR1 = (millis() - relay1StartTime) / 1000.0;
    float remainingR1 = SetPumpOnTime - elapsedR1;

    // Update LCD for Relay 1 status
    lcd.setCursor(13, 0);
    lcd.print(remainingR1 > 0 ? remainingR1 : 0.0, 1);

    // Step 3: Turn off Relay 1 when its timer completes
    if (remainingR1 <= 0 && isRelay1Active) {
      digitalWrite(PumpCloseRelay, LOW);
      isRelay1Active = false;
      IsPumpClose = true;

      // Display R1 OFF, delay countdown
      lcd.clear();
      lcd.print("Complete OFF");
      lcd.setCursor(0, 1);
      lcd.print("Delay Time     ");
      lcd.setCursor(13, 1);
      lcd.print(setDelayTime, 1);
    }
  }

  // Step 4: Track delay time for Relay 2
  if (delayTimerStarted) {
    float elapsedDelay = (millis() - delayStartTime) / 1000.0;
    float remainingDelayTime = setDelayTime - elapsedDelay;

    // Update LCD for delay time
    lcd.setCursor(13, 1);
    lcd.print(remainingDelayTime > 0 ? remainingDelayTime : 0.0, 1);

    // Step 5: Start Relay 2 when delay completes
    if (remainingDelayTime <= 0 && IsPumpClose && !IsPumpOn) {
      delayTimerStarted = false;      // Stop delay timer
      digitalWrite(PumpOpenRelay, HIGH);  // Turn on Relay 2
      relay2DelayTime = millis();     // Note start time for Relay 2
      IsPumpOn = true;

      // Display R2 ON status
      lcd.clear();
      lcd.print("Pressure ON");
      lcd.setCursor(13, 0);
      lcd.print(SetPumpOfTime, 1);  // Show Relay 2 time (same as R1)
    }
  }

  // Step 6: Track Relay 2’s remaining time
  if (IsPumpOn) {
    float elapsedR2 = (millis() - relay2DelayTime) / 1000.0;
    float remainingR2 = SetPumpOfTime - elapsedR2;

    // Update LCD for Relay 2 status
    lcd.setCursor(13, 0);
    lcd.print(remainingR2 > 0 ? remainingR2 : 0.0, 1);

    // Step 7: Turn off Relay 2 and end the cycle when R2's time completes
    if (remainingR2 <= 0) {
      digitalWrite(PumpOpenRelay, LOW);  // Turn off Relay 2
      IsPumpOn = false;
      cycleCompleted = true;    // Mark cycle as completed
      cycleInProgress = false;  // Reset cycle in progress flag

      // Display final OFF status for both relays
      lcd.clear();
      lcd.print("Complete OFF");
      delay(1000);  // Optional: brief delay to show final status
    }
  }

  // Step 8: If metal is detected during the cycle, restart the cycle (only if cycle completed)
  if (metalDetected && cycleCompleted) {
    // Reset the entire cycle
    cycleInProgress = false;  // Mark that cycle is not in progress
    IsPumpClose = false;
    IsPumpOn = false;
    cycleCompleted = false;
    delayTimerStarted = false;
    isRelay1Active = false;
    digitalWrite(PumpCloseRelay, LOW);  // Turn off Relay 1
    digitalWrite(PumpOpenRelay, LOW);  // Turn off Relay 2

    // Restart the cycle (from Relay 1)
    digitalWrite(PumpCloseRelay, HIGH);     // Turn on Relay 1
    relay1StartTime = millis();        // Note start time for Relay 1
    delayStartTime = relay1StartTime;  // Start delay timer simultaneously
    isRelay1Active = true;
    delayTimerStarted = true;

    // Display R1 ON status and Delay Time text
    lcd.clear();
    lcd.print("Pressure ON");
    lcd.setCursor(13, 0);
    lcd.print(SetPumpOnTime, 1);
    lcd.setCursor(0, 1);
    lcd.print("Delay Time     ");  // Show "Delay Time" on line 2
    lcd.setCursor(13, 1);
    lcd.print(setDelayTime, 1);

    metalDetected = false;  // Reset metal detection flag to avoid unwanted multiple triggers
  }

  // Step 9: If metal is detected during the cycle, restart the cycle
  if (metalDetected && cycleInProgress) {
    // Reset the entire cycle
    cycleInProgress = false;  // Mark that cycle is not in progress
    IsPumpClose = false;
    IsPumpOn = false;
    cycleCompleted = false;
    delayTimerStarted = false;
    isRelay1Active = false;
    digitalWrite(PumpCloseRelay, LOW);  // Turn off Relay 1
    digitalWrite(PumpOpenRelay, LOW);  // Turn off Relay 2

    // Restart the cycle (from Relay 1)
    digitalWrite(PumpCloseRelay, HIGH);     // Turn on Relay 1
    relay1StartTime = millis();        // Note start time for Relay 1
    delayStartTime = relay1StartTime;  // Start delay timer simultaneously
    isRelay1Active = true;
    delayTimerStarted = true;

    // Display R1 ON status and Delay Time text
    lcd.clear();
    lcd.print("Pressure ON");
    lcd.setCursor(13, 0);
    lcd.print(SetPumpOnTime, 1);
    lcd.setCursor(0, 1);
    lcd.print("Delay Time     ");  // Show "Delay Time" on line 2
    lcd.setCursor(13, 1);
    lcd.print(setDelayTime, 1);

    metalDetected = false;  // Reset metal detection flag to avoid unwanted multiple triggers
  }
}

// Display the settings menu
void displaySettingsMenu() {
  // Serial.println("display menu setting");
  lcd.clear();
  for (int i = 0; i < 2; i++) {
    lcd.setCursor(1, i);
    if (currentSettingSelection + i < 4) {
      lcd.print(settingItems[currentSettingSelection + i]);
    }
  }
  lcd.setCursor(0, 0);
  lcd.write('>');
}

// Display the R1 time setting screen
void displaySetR1() {
  lcd.clear();
  lcd.print("Set Press. C Time:");
  lcd.setCursor(0, 1);
  lcd.print(SetPumpOnTime, 1);
  lcd.print(" sec");
}

void handleSetR1(int buttonValue) {
  if (buttonValue < buttonValues[1] && !buttonPressed) {
    SetPumpOnTime += stepSize;
    if (SetPumpOnTime > 99.9) SetPumpOnTime = 99.9;
    displaySetR1();
  } else if (buttonValue < buttonValues[2] && !buttonPressed) {
    SetPumpOnTime -= stepSize;
    if (SetPumpOnTime < 0.0) SetPumpOnTime = 0.0;
    displaySetR1();
  } else if (buttonValue < buttonValues[4] && !buttonPressed) {
    buttonPressed = true;
    EEPROM.put(EEPROM_ADDR_PUMP_ON_TIME, SetPumpOnTime);
    lcd.clear();
    lcd.print("Successfully set");
    delay(2000);
    inSetR1Mode = false;
    inSettingsMode = true;
    displaySettingsMenu();
  }
}

void displaySetR2() {
  lcd.clear();
  lcd.print("Delay Time:");
  lcd.setCursor(0, 1);
  lcd.print(setDelayTime, 1);  // Display time with one decimal place
  lcd.print(" sec");
}

void handleDelayR2(int buttonValue) {
  if (buttonValue < buttonValues[1] && !buttonPressed) {  // Up button
    setDelayTime += stepSize;
    if (setDelayTime > 99.9) setDelayTime = 99.9;  // Max limit
    displaySetR2();
  } else if (buttonValue < buttonValues[2] && !buttonPressed) {  // Down button
    setDelayTime -= stepSize;
    if (setDelayTime < 0.0) setDelayTime = 0.0;  // Min limit
    displaySetR2();
  } else if (buttonValue < buttonValues[4] && !buttonPressed) {  // Select button
    buttonPressed = true;
    // Save time to EEPROM
    EEPROM.put(EEPROM_ADDR_DELAY_TIME, setDelayTime);
    lcd.clear();
    lcd.print("Successfully set");
    delay(2000);
    inSetDelayMode = false;
    inSettingsMode = true;
    displaySettingsMenu();
  }
}

void displaySetR3() {
  lcd.clear();
  lcd.print("Press. O Time:");
  lcd.setCursor(0, 1);
  // lcd.print(SetPumpOfTime, 1);  // Display time with one decimal place
  lcd.print(SetPumpOfTime, 1);  // Display time with one decimal place

  lcd.print(" sec");
}

void handleDelayR3(int buttonValue) {
  if (buttonValue < buttonValues[1] && !buttonPressed) {  // Up button
    SetPumpOfTime += stepSize;
    if (SetPumpOfTime > 99.9) SetPumpOfTime = 99.9;  // Max limit
    displaySetR3();
  } else if (buttonValue < buttonValues[2] && !buttonPressed) {  // Down button
    SetPumpOfTime -= stepSize;
    if (SetPumpOfTime < 0.0) SetPumpOfTime = 0.0;  // Min limit
    displaySetR3();
  } else if (buttonValue < buttonValues[4] && !buttonPressed) {  // Select button
    buttonPressed = true;
    // Save time to EEPROM
    EEPROM.put(EEPROM_ADDR_PUMP_OFF_TIME, SetPumpOfTime);
    lcd.clear();
    lcd.print("Successfully set");
    delay(2000);
    inSetR3Mode = false;
    inSettingsMode = true;
    displaySettingsMenu();
  }
}