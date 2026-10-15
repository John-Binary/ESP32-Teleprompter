
#include <Arduino.h>
#include <RCSwitch.h>
#include <Preferences.h>

// ======================================================
// PIN SETTINGS
// ======================================================

#define BUTTON_RIGHT_PIN   32
#define BUTTON_LEFT_PIN    33

#define RF_RECEIVER_PIN    27


// ======================================================
// SETTINGS
// ======================================================

// Power ON 시 두 버튼을 이 시간 이상 같이 누르면 Learning Mode
#define LEARNING_HOLD_TIME 1000

// Physical button debounce
#define BUTTON_DEBOUNCE    30

// RF repeat 판단 시간
#define RF_RELEASE_TIME    300


// ======================================================
// OBJECTS
// ======================================================

RCSwitch rfSwitch = RCSwitch();
Preferences preferences;


// ======================================================
// STORED RF CODES
// ======================================================

uint32_t rfRightCode = 0;
uint32_t rfLeftCode  = 0;


// ======================================================
// BUTTON VARIABLES
// ======================================================

bool lastRightButton = HIGH;
bool lastLeftButton  = HIGH;

unsigned long rightDebounceTime = 0;
unsigned long leftDebounceTime  = 0;


// ======================================================
// RF REPEAT CONTROL
// ======================================================

uint32_t activeRFCode = 0;

unsigned long lastRFReceiveTime = 0;

bool rfActionAlreadySent = false;


// ======================================================
// SEND RIGHT TO PC
// ======================================================

void sendRight()
{
  Serial.println("RIGHT");
}


// ======================================================
// SEND LEFT TO PC
// ======================================================

void sendLeft()
{
  Serial.println("LEFT");
}


// ======================================================
// PRINT RF INFORMATION
// ======================================================

void printRFInfo(uint32_t code)
{
  Serial.print("RF CODE = ");
  Serial.print(code);

  Serial.print("   HEX = 0x");
  Serial.print(code, HEX);

  Serial.print("   BIT = ");
  Serial.print(rfSwitch.getReceivedBitlength());

  Serial.print("   PROTOCOL = ");
  Serial.println(rfSwitch.getReceivedProtocol());
}


// ======================================================
// WAIT FOR ONE RF CODE
// ======================================================

uint32_t waitForRFCode()
{
  rfSwitch.resetAvailable();

  while (true)
  {
    if (rfSwitch.available())
    {
      uint32_t code = rfSwitch.getReceivedValue();

      if (code != 0)
      {
        printRFInfo(code);

        rfSwitch.resetAvailable();

        delay(500);

        return code;
      }

      rfSwitch.resetAvailable();
    }

    delay(5);
  }
}


// ======================================================
// WAIT UNTIL RF BUTTON RELEASED
// ======================================================

void waitRFRelease()
{
  unsigned long lastSignal = millis();

  while (true)
  {
    if (rfSwitch.available())
    {
      rfSwitch.resetAvailable();

      lastSignal = millis();
    }

    if (millis() - lastSignal > 500)
    {
      break;
    }

    delay(5);
  }
}


// ======================================================
// RF LEARNING MODE
// ======================================================

void learningMode()
{
  Serial.println();
  Serial.println("=======================================");
  Serial.println("       RF LEARNING MODE");
  Serial.println("=======================================");
  Serial.println();

  delay(500);


  // --------------------------------------------------
  // RIGHT LEARNING
  // --------------------------------------------------

  Serial.println("Press RF RIGHT button...");
  Serial.println();

  rfRightCode = waitForRFCode();

  Serial.println();
  Serial.print("RIGHT RF CODE LEARNED = ");
  Serial.println(rfRightCode);

  waitRFRelease();

  delay(1000);


  // --------------------------------------------------
  // LEFT LEARNING
  // --------------------------------------------------

  Serial.println();
  Serial.println("Press RF LEFT button...");
  Serial.println();

  while (true)
  {
    rfLeftCode = waitForRFCode();

    // Same code cannot be used for RIGHT and LEFT
    if (rfLeftCode != rfRightCode)
    {
      break;
    }

    Serial.println();
    Serial.println("Same as RIGHT code!");
    Serial.println("Please press LEFT button again.");
    Serial.println();

    waitRFRelease();
  }


  Serial.println();
  Serial.print("LEFT RF CODE LEARNED = ");
  Serial.println(rfLeftCode);


  // --------------------------------------------------
  // SAVE TO NVS
  // --------------------------------------------------

  preferences.putULong("rightCode", rfRightCode);
  preferences.putULong("leftCode", rfLeftCode);


  Serial.println();
  Serial.println("RF codes saved to ESP32 memory.");
  Serial.println();

  Serial.println("---------------------------------------");

  Serial.print("RIGHT = ");
  Serial.println(rfRightCode);

  Serial.print("LEFT  = ");
  Serial.println(rfLeftCode);

  Serial.println("---------------------------------------");

  Serial.println();
  Serial.println("Learning complete.");
  Serial.println("NORMAL MODE STARTING...");
  Serial.println();

  delay(1500);


  // Clear old RF data
  rfSwitch.resetAvailable();

  activeRFCode = 0;
  rfActionAlreadySent = false;
}


// ======================================================
// CHECK PHYSICAL BUTTONS
// ======================================================

void checkPhysicalButtons()
{
  bool rightButton = digitalRead(BUTTON_RIGHT_PIN);
  bool leftButton  = digitalRead(BUTTON_LEFT_PIN);


  // --------------------------------------------------
  // RIGHT BUTTON
  // --------------------------------------------------

  if (rightButton != lastRightButton)
  {
    if (millis() - rightDebounceTime > BUTTON_DEBOUNCE)
    {
      rightDebounceTime = millis();

      if (rightButton == LOW)
      {
        sendRight();
      }

      lastRightButton = rightButton;
    }
  }


  // --------------------------------------------------
  // LEFT BUTTON
  // --------------------------------------------------

  if (leftButton != lastLeftButton)
  {
    if (millis() - leftDebounceTime > BUTTON_DEBOUNCE)
    {
      leftDebounceTime = millis();

      if (leftButton == LOW)
      {
        sendLeft();
      }

      lastLeftButton = leftButton;
    }
  }
}


// ======================================================
// CHECK RF REMOTE
// ======================================================

void checkRFRemote()
{
  if (!rfSwitch.available())
  {
    return;
  }


  uint32_t code = rfSwitch.getReceivedValue();

  unsigned long now = millis();


  if (code == 0)
  {
    rfSwitch.resetAvailable();
    return;
  }


  // --------------------------------------------------
  // Detect new RF button press
  // --------------------------------------------------

  if ((now - lastRFReceiveTime > RF_RELEASE_TIME) ||
      (code != activeRFCode))
  {
    activeRFCode = code;

    rfActionAlreadySent = false;
  }


  lastRFReceiveTime = now;


  // --------------------------------------------------
  // Only one action per press
  // --------------------------------------------------

  if (!rfActionAlreadySent)
  {
    if (code == rfRightCode && rfRightCode != 0)
    {
      sendRight();

      rfActionAlreadySent = true;
    }

    else if (code == rfLeftCode && rfLeftCode != 0)
    {
      sendLeft();

      rfActionAlreadySent = true;
    }
  }


  rfSwitch.resetAvailable();
}


// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(115200);

  delay(500);


  // --------------------------------------------------
  // Physical buttons
  // --------------------------------------------------

  pinMode(BUTTON_RIGHT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_LEFT_PIN, INPUT_PULLUP);


  // --------------------------------------------------
  // Preferences
  // --------------------------------------------------

  preferences.begin("RFremote", false);


  // Load previously saved codes

  rfRightCode = preferences.getULong("rightCode", 0);

  rfLeftCode = preferences.getULong("leftCode", 0);


  // --------------------------------------------------
  // RF Receiver
  // --------------------------------------------------

  rfSwitch.enableReceive(
      digitalPinToInterrupt(RF_RECEIVER_PIN)
  );


  Serial.println();
  Serial.println("=======================================");
  Serial.println(" ESP32 USB + RF PAGE CONTROLLER");
  Serial.println("=======================================");

  Serial.print("RF Receiver GPIO = ");
  Serial.println(RF_RECEIVER_PIN);

  Serial.println();


  // --------------------------------------------------
  // Display saved RF codes
  // --------------------------------------------------

  Serial.print("Saved RIGHT RF code = ");
  Serial.println(rfRightCode);

  Serial.print("Saved LEFT RF code  = ");
  Serial.println(rfLeftCode);

  Serial.println();


  // --------------------------------------------------
  // Check Learning Mode
  // --------------------------------------------------

  if ((digitalRead(BUTTON_RIGHT_PIN) == LOW) &&
      (digitalRead(BUTTON_LEFT_PIN) == LOW))
  {
    unsigned long startTime = millis();

    while ((digitalRead(BUTTON_RIGHT_PIN) == LOW) &&
           (digitalRead(BUTTON_LEFT_PIN) == LOW))
    {
      if (millis() - startTime >= LEARNING_HOLD_TIME)
      {
        learningMode();

        break;
      }

      delay(10);
    }
  }


  // Initial button states

  lastRightButton = digitalRead(BUTTON_RIGHT_PIN);
  lastLeftButton  = digitalRead(BUTTON_LEFT_PIN);


  Serial.println("NORMAL MODE");
  Serial.println();

  Serial.println("Physical RIGHT button -> RIGHT");
  Serial.println("Physical LEFT button  -> LEFT");
  Serial.println("RF RIGHT button       -> RIGHT");
  Serial.println("RF LEFT button        -> LEFT");

  Serial.println();
}


// ======================================================
// LOOP
// ======================================================

void loop()
{
  checkPhysicalButtons();

  checkRFRemote();

  delay(2);
}
