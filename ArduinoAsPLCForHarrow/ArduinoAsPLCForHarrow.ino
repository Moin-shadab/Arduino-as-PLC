// Define pin numbers for buttons and relays
const int button1Pin = 2;
const int button2Pin = 3;
const int button3Pin = 4;
const int button4Pin = 5;

const int relay1Pin = 6;
const int relay2Pin = 7;
const int relay3Pin = 8;

// Variables to track button states
int button1State = 0;
int button2State = 0;
int button3State = 0;
int button4State = 0;

unsigned long button3Timer = 0;
unsigned long button4Timer = 0;

// Timers for button presses
unsigned long button3PressTime = 0;
unsigned long button3Relay2Time = 0;
unsigned long button3Relay3Time = 0;

bool relay2Active = false;
bool relay3Active = false;

void setup() {
  // Set button pins as inputs with internal pull-up resistors
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  pinMode(button3Pin, INPUT_PULLUP);
  pinMode(button4Pin, INPUT_PULLUP);

  // Set relay pins as outputs
  pinMode(relay1Pin, OUTPUT);
  pinMode(relay2Pin, OUTPUT);
  pinMode(relay3Pin, OUTPUT);

  // Initially turn off all relays
  digitalWrite(relay1Pin, LOW);
  digitalWrite(relay2Pin, LOW);
  digitalWrite(relay3Pin, LOW);
}

void loop() {
  // Read the button states (buttons are active LOW, so the logic is inverted)
  button1State = digitalRead(button1Pin) == LOW;
  button2State = digitalRead(button2Pin) == LOW;
  button3State = digitalRead(button3Pin) == LOW;
  button4State = digitalRead(button4Pin) == LOW;

  // Button 1: Turn relay 1 on when pressed, off when released
  if (button1State) {
    digitalWrite(relay1Pin, HIGH);
  } else {
    digitalWrite(relay1Pin, LOW);
  }

  // Button 2: Turn relay 2 on when pressed, off when released
  if (button2State) {
    digitalWrite(relay2Pin, HIGH);
  } else {
    digitalWrite(relay2Pin, LOW);
  }

  // Button 3: When pressed and released, activate relays 2 for 10s, 3 for 3s
  // if (button3State && (millis() - button3Timer > 200)) {  // debounce and detect press
  //   button3Timer = millis();  // record the press time
  //   digitalWrite(relay2Pin, HIGH);
  //   delay(10000);  // 10 seconds
  //   digitalWrite(relay3Pin, HIGH);
  //   delay(3000);  // 3 seconds
  //   digitalWrite(relay2Pin, LOW);
  //   digitalWrite(relay3Pin, LOW);
  // }

  // Button 3: When pressed, activate relay 2 for 10s, relay 3 for 3s (non-blocking)
  if (button3State) {
    if (button3PressTime == 0) {
      // Start timing when the button is first pressed
      button3PressTime = millis();
      digitalWrite(relay2Pin, HIGH);  // Turn on relay 2 immediately
      relay2Active = true;
      button3Relay2Time = millis();  // Start timer for relay 2
    }

    // If relay 2 has been on for 10 seconds, turn it off
    if (relay2Active && millis() - button3Relay2Time >= 10000) {
      digitalWrite(relay2Pin, LOW);
      relay2Active = false;  // Stop relay 2
    }

    // Start relay 3 after 3 seconds and turn it off after 3 seconds
    if (millis() - button3PressTime >= 3000 && !relay3Active) {
      digitalWrite(relay3Pin, HIGH);
      relay3Active = true;  // Relay 3 is active
      button3Relay3Time = millis();  // Start timer for relay 3
    }

    // Turn off relay 3 after 3 seconds
    if (relay3Active && millis() - button3Relay3Time >= 3000) {
      digitalWrite(relay3Pin, LOW);
      relay3Active = false;  // Stop relay 3
    }
  } else {
    // Reset if button 3 is not pressed
    button3PressTime = 0;
    relay2Active = false;
    relay3Active = false;
    digitalWrite(relay2Pin, LOW);
    digitalWrite(relay3Pin, LOW);
  }


  // Button 4: When pressed and released, activate relay 3 for 3 seconds
  if (button4State && (millis() - button4Timer > 200)) {  // debounce and detect press
    button4Timer = millis();  // record the press time
    digitalWrite(relay3Pin, HIGH);
    delay(3000);  // 3 seconds
    digitalWrite(relay3Pin, LOW);
  }
}
