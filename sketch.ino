#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define RTC_ADDRESS 0x68

#define STEP_PIN PA0
#define DIR_PIN  PA1
#define EN_PIN   PA2

#define IR_PIN PA3
#define BUZZER_PIN PA4

#define LED_RED   PA5
#define LED_GREEN PA6
#define LED_BLUE  PA7

const byte ROWS = 4;
const byte COLS = 4;

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

byte rowPins[ROWS] = {PA8, PA9, PA10, PA11};
byte colPins[COLS] = {PB12, PB13, PB14, PB15};

Keypad keypad = Keypad(
  makeKeymap(keys), rowPins, colPins, ROWS, COLS
);

int medicineHour = 8;
int medicineMinute = 0;
bool doseDone = false;

byte bcdToDec(byte value)
{
  return ((value >> 4) * 10) + (value & 0x0F);
}

void readRTC(byte &hours, byte &minutes, byte &seconds)
{
  hours = 0;
  minutes = 0;
  seconds = 0;

  Wire.beginTransmission(RTC_ADDRESS);
  Wire.write(0x00);

  if (Wire.endTransmission() != 0)
    return;

  Wire.requestFrom(RTC_ADDRESS, (uint8_t)3);

  if (Wire.available() >= 3)
  {
    seconds = bcdToDec(Wire.read() & 0x7F);
    minutes = bcdToDec(Wire.read() & 0x7F);
    hours = bcdToDec(Wire.read() & 0x3F);
  }
}

void rotateMotor()
{
  digitalWrite(EN_PIN, LOW);
  digitalWrite(DIR_PIN, HIGH);

  for (int i = 0; i < 50; i++)
  {
    digitalWrite(STEP_PIN, HIGH);
    delay(3);
    digitalWrite(STEP_PIN, LOW);
    delay(3);
  }

  digitalWrite(EN_PIN, HIGH);
}

void showHomeScreen()
{
  byte hours, minutes, seconds;
  readRTC(hours, minutes, seconds);

  char timeText[17];
  char medicineText[17];

  snprintf(timeText, sizeof(timeText),
           "%02d:%02d:%02d        ",
           hours, minutes, seconds);

  snprintf(medicineText, sizeof(medicineText),
           "Med %02d:%02d A=Set ",
           medicineHour, medicineMinute);

  lcd.setCursor(0, 0);
  lcd.print(timeText);

  lcd.setCursor(0, 1);
  lcd.print(medicineText);
}

void setMedicineTime()
{
  String input = "";

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Set Time HHMM");
  lcd.setCursor(0, 1);
  lcd.print("____");

  while (true)
  {
    char key = keypad.getKey();

    if (!key)
      continue;

    if (key >= '0' && key <= '9')
    {
      if (input.length() < 4)
      {
        input += key;
        lcd.setCursor(input.length() - 1, 1);
        lcd.print(key);
      }
    }
    else if (key == '*')
    {
      if (input.length() > 0)
      {
        input.remove(input.length() - 1);

        lcd.setCursor(0, 1);
        lcd.print("                ");
        lcd.setCursor(0, 1);

        for (unsigned int i = 0; i < input.length(); i++)
          lcd.print(input[i]);
      }
    }
    else if (key == 'C')
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Cancelled");
      delay(1000);
      return;
    }
    else if (key == '#')
    {
      if (input.length() != 4)
        continue;

      int h = (input[0] - '0') * 10 + (input[1] - '0');
      int m = (input[2] - '0') * 10 + (input[3] - '0');

      if (h < 24 && m < 60)
      {
        medicineHour = h;
        medicineMinute = m;
        doseDone = false;

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Time Saved!");
        lcd.setCursor(0, 1);

        char savedTime[17];
        snprintf(savedTime, sizeof(savedTime),
                 "%02d:%02d            ", h, m);
        lcd.print(savedTime);

        delay(1500);
        return;
      }
      else
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Invalid Time!");
        lcd.setCursor(0, 1);
        lcd.print("Use 00-23/00-59");
        delay(1500);

        input = "";
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Set Time HHMM");
        lcd.setCursor(0, 1);
        lcd.print("____");
      }
    }
  }
}

void viewMedicineTime()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Medicine Time");

  lcd.setCursor(0, 1);

  char savedTime[17];
  snprintf(savedTime, sizeof(savedTime),
           "%02d:%02d            ",
           medicineHour, medicineMinute);

  lcd.print(savedTime);
  delay(2000);
}

void dispenseMedicine()
{
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, HIGH);

  tone(BUZZER_PIN, 1000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Take Medicine!");
  lcd.setCursor(0, 1);
  lcd.print("Please take it");
  delay(2000);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dispensing...");

  rotateMotor();
  delay(500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Take Tablet");
  lcd.setCursor(0, 1);
  lcd.print("Press GREEN");

  while (true)
  {
    if (digitalRead(IR_PIN) == LOW)
    {
      delay(50);

      if (digitalRead(IR_PIN) == LOW)
      {
        noTone(BUZZER_PIN);
        digitalWrite(LED_BLUE, LOW);
        digitalWrite(LED_GREEN, HIGH);

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Tablet Confirmed");
        lcd.setCursor(0, 1);
        lcd.print("Thank You!");

        while (digitalRead(IR_PIN) == LOW)
          delay(20);

        delay(3000);
        digitalWrite(LED_GREEN, LOW);
        return;
      }
    }

    delay(20);
  }
}

void setup()
{
  Wire.begin();

  lcd.init();
  lcd.backlight();
  lcd.clear();

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  digitalWrite(EN_PIN, HIGH);

  pinMode(IR_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_BLUE, LOW);

  lcd.setCursor(0, 0);
  lcd.print("Medicine");
  lcd.setCursor(0, 1);
  lcd.print("Dispenser");
  delay(2000);

  lcd.clear();
  showHomeScreen();
}

void loop()
{
  static unsigned long lastDisplay = 0;

  byte hours, minutes, seconds;
  readRTC(hours, minutes, seconds);

  char key = keypad.getKey();

  if (key == 'A')
  {
    setMedicineTime();
    lastDisplay = millis();
    showHomeScreen();
  }
  else if (key == 'B')
  {
    viewMedicineTime();
    lastDisplay = millis();
    showHomeScreen();
  }

  if (hours == medicineHour &&
      minutes == medicineMinute &&
      !doseDone)
  {
    doseDone = true;
    dispenseMedicine();

    lastDisplay = millis();
    showHomeScreen();
  }

  if (minutes != medicineMinute)
    doseDone = false;

  if (millis() - lastDisplay >= 1000)
  {
    lastDisplay = millis();
    showHomeScreen();
  }

  delay(20);
}
