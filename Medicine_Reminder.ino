#define BLYNK_TEMPLATE_ID "YOUR_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN "YOUR_AUTH_TOKEN"

#define BLYNK_PRINT Serial

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <time.h>
#include <string.h>

// ===== Wi-Fi Credentials =====
char ssid[] = "YOUR_WIFI_NAME";
char pass[] = "YOUR_WIFI_PASSWORD";

// ===== ESP32 Pin Configuration =====
const int BUZZER_PIN = 25;
const int LED_PIN = 26;
const int BUTTON_PIN = 19;

// ===== LCD Configuration =====
// SDA = GPIO 21
// SCL = GPIO 22

LiquidCrystal_I2C lcd(0x27, 16, 2);

BlynkTimer timer;

// ===== Medicine Schedule (IST) =====

struct Medicine {
  const char* name;
  int hour;
  int minute;
  int lastTriggeredDay;
};

Medicine medicines[] = {
  {"Paracetamol", 9, 0, -1},
  {"Nazel drops", 14, 0, -1},
  {"Dolo", 21, 0, -1}
};

const int MEDICINE_COUNT =
  sizeof(medicines) / sizeof(medicines[0]);

// ===== Alarm State =====

bool alarmActive = false;
bool thankYouScreen = false;

int activeMedicine = -1;

unsigned long alarmStartTime = 0;

const unsigned long ALARM_DURATION = 30000UL;

unsigned long thankYouStartTime = 0;

const unsigned long THANK_YOU_DURATION = 4000UL;

// ===== Button Debounce =====

int lastButtonReading = HIGH;
int stableButtonState = HIGH;

unsigned long lastDebounceTime = 0;

const unsigned long DEBOUNCE_DELAY = 40;

// ===== Buzzer Melody =====

struct Note {
  int frequency;
  int duration;
};

Note melody[] = {
  {1047, 130},
  {1319, 130},
  {1568, 220},
  {1319, 130},
  {1568, 130},
  {2093, 260},
  {1568, 130},
  {1319, 130},
  {1047, 260},
  {1175, 130},
  {1397, 130},
  {1760, 220},
  {1397, 130},
  {1760, 130},
  {2093, 260},
  {1760, 130},
  {1397, 130},
  {1175, 300}
};

const int NOTE_COUNT =
  sizeof(melody) / sizeof(melody[0]);

int melodyIndex = 0;

bool notePlaying = false;

unsigned long noteDeadline = 0;
unsigned long nextNoteAt = 0;

unsigned long lastBlynkAttempt = 0;

// ===== Function Declarations =====

void connectBlynk();
void checkSchedule();
void startReminder(int index);
void updateBuzzer();
void checkButton();
void updateLCD();
void acknowledgeMedicine();
void sendMedicineNotification();

// ===== BLYNK PHONE BUTTON (V0) =====

BLYNK_WRITE(V0) {

  int value = param.asInt();

  if (value == 1 && alarmActive) {
    acknowledgeMedicine();
  }

  Blynk.virtualWrite(V0, 0);
}

// ===== BLYNK CONNECTION =====

BLYNK_CONNECTED() {

  Blynk.virtualWrite(V0, 0);

}

// ===== SETUP =====

void setup() {

  Serial.begin(115200);

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(LED_PIN, LOW);

  noTone(BUZZER_PIN);

  // Initialize LCD

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Medicine Reminder");

  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  // Connect Wi-Fi

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  // Configure Blynk

  Blynk.config(BLYNK_AUTH_TOKEN);

  // IST = UTC + 5:30

  configTime(
    19800,
    0,
    "pool.ntp.org",
    "time.google.com"
  );

  // Timers

  timer.setInterval(1000L, checkSchedule);

  timer.setInterval(10L, updateBuzzer);

  timer.setInterval(30L, checkButton);

  timer.setInterval(500L, updateLCD);

  Serial.println("Medicine Reminder started.");

}

// ===== MAIN LOOP =====

void loop() {

  if (WiFi.status() == WL_CONNECTED) {

    if (!Blynk.connected()) {
      connectBlynk();
    }

    if (Blynk.connected()) {
      Blynk.run();
    }

  }

  timer.run();

}

// ===== BLYNK RECONNECTION =====

void connectBlynk() {

  if (millis() - lastBlynkAttempt >= 5000UL) {

    lastBlynkAttempt = millis();

    Blynk.connect(1000);

  }

}

// ===== CHECK MEDICINE SCHEDULE =====

void checkSchedule() {

  struct tm timeInfo;

  if (!getLocalTime(&timeInfo, 10)) {
    return;
  }

  int today = timeInfo.tm_yday;

  for (int i = 0; i < MEDICINE_COUNT; i++) {

    if (
      timeInfo.tm_hour == medicines[i].hour &&
      timeInfo.tm_min == medicines[i].minute &&
      medicines[i].lastTriggeredDay != today
    ) {

      medicines[i].lastTriggeredDay = today;

      startReminder(i);

      break;

    }

  }

}

// ===== START MEDICINE REMINDER =====

void startReminder(int index) {

  activeMedicine = index;

  alarmActive = true;

  thankYouScreen = false;

  // Reset Blynk phone button

  if (Blynk.connected()) {
    Blynk.virtualWrite(V0, 0);
  }

  alarmStartTime = millis();

  melodyIndex = 0;

  notePlaying = false;

  nextNoteAt = millis();

  digitalWrite(LED_PIN, HIGH);

  noTone(BUZZER_PIN);

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Take medicine:");

  lcd.setCursor(0, 1);
  lcd.print(medicines[index].name);

  Serial.print("Medicine due: ");

  Serial.println(medicines[index].name);

  // Send phone notification

  sendMedicineNotification();

}

// ===== SEND BLYNK NOTIFICATION =====

void sendMedicineNotification() {

  if (!Blynk.connected() || activeMedicine < 0) {

    Serial.println(
      "Blynk offline: notification not sent."
    );

    return;

  }

  String message = "Time to take ";

  message += medicines[activeMedicine].name;

  message += "!";

  // Create event code medicine_time
  // in Blynk Console

  Blynk.logEvent("medicine_time", message);

  Serial.println("Blynk notification submitted.");

}

// ===== BUZZER MELODY =====

void updateBuzzer() {

  if (!alarmActive) {
    return;
  }

  unsigned long now = millis();

  // Automatic shutoff after 30 seconds

  if (now - alarmStartTime >= ALARM_DURATION) {

    alarmActive = false;

    notePlaying = false;

    noTone(BUZZER_PIN);

    digitalWrite(LED_PIN, LOW);

    Serial.println(
      "Alarm stopped after 30 seconds."
    );

    return;

  }

  if (notePlaying) {

    if ((long)(now - noteDeadline) >= 0) {

      noTone(BUZZER_PIN);

      notePlaying = false;

      melodyIndex++;

      if (melodyIndex >= NOTE_COUNT) {
        melodyIndex = 0;
      }

      nextNoteAt = now + 45;

    }

    return;

  }

  if ((long)(now - nextNoteAt) >= 0) {

    tone(
      BUZZER_PIN,
      melody[melodyIndex].frequency
    );

    notePlaying = true;

    noteDeadline =
      now + melody[melodyIndex].duration;

  }

}

// ===== PHYSICAL PUSH BUTTON =====
// Button connected between GPIO 19 and GND

void checkButton() {

  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonReading) {

    lastDebounceTime = millis();

    lastButtonReading = reading;

  }

  if (
    millis() - lastDebounceTime >=
    DEBOUNCE_DELAY
  ) {

    if (reading != stableButtonState) {

      stableButtonState = reading;

      if (
        stableButtonState == LOW &&
        alarmActive
      ) {

        acknowledgeMedicine();

      }

    }

  }

}

// ===== ACKNOWLEDGE MEDICINE =====

void acknowledgeMedicine() {

  if (!alarmActive) {
    return;
  }

  alarmActive = false;

  notePlaying = false;

  noTone(BUZZER_PIN);

  digitalWrite(LED_PIN, LOW);

  thankYouScreen = true;

  thankYouStartTime = millis();

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Thank you for");

  lcd.setCursor(0, 1);
  lcd.print("taking medicine");

  Serial.println("Reminder acknowledged.");

}

// ===== LCD CLOCK AND NEXT MEDICINE =====

void updateLCD() {

  if (alarmActive) {
    return;
  }

  if (thankYouScreen) {

    if (
      millis() - thankYouStartTime <
      THANK_YOU_DURATION
    ) {

      return;

    }

    thankYouScreen = false;

    lcd.clear();

  }

  struct tm timeInfo;

  if (!getLocalTime(&timeInfo, 10)) {

    lcd.setCursor(0, 0);
    lcd.print("Waiting for time");

    lcd.setCursor(0, 1);
    lcd.print("Check Wi-Fi     ");

    return;

  }

  // Display current time

  char timeText[17];

  strftime(
    timeText,
    sizeof(timeText),
    "%I:%M:%S %p",
    &timeInfo
  );

  lcd.setCursor(0, 0);

  lcd.print("Time ");

  lcd.print(timeText);

  // Find next medicine

  int currentMinutes =
    timeInfo.tm_hour * 60 +
    timeInfo.tm_min;

  int nextIndex = -1;

  int nextMinutes = 1441;

  for (int i = 0; i < MEDICINE_COUNT; i++) {

    int scheduled =
      medicines[i].hour * 60 +
      medicines[i].minute;

    if (
      scheduled > currentMinutes &&
      medicines[i].lastTriggeredDay !=
      timeInfo.tm_yday &&
      scheduled < nextMinutes
    ) {

      nextMinutes = scheduled;

      nextIndex = i;

    }

  }

  // Display next medicine

  lcd.setCursor(0, 1);

  if (nextIndex >= 0) {

    char line[17];

    snprintf(
      line,
      sizeof(line),
      "Next %02d:%02d %.5s",
      medicines[nextIndex].hour,
      medicines[nextIndex].minute,
      medicines[nextIndex].name
    );

    lcd.print(line);

    int length = strlen(line);

    for (int i = length; i < 16; i++) {
      lcd.print(' ');
    }

  } else {

    lcd.print("No more today   ");

  }

}