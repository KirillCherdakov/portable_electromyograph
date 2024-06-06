#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <iarduino_RTC.h>

#define WRITE_TO_SD 1

iarduino_RTC watch(RTC_DS1307);

File myFile;

LiquidCrystal_I2C lcd(0x27, 20, 4);

volatile uint32_t files_count = 0;
volatile uint32_t patient_number = 0;
volatile uint32_t button_debounce_time1 = 0;
volatile uint32_t button_debounce_time2 = 0;
volatile bool emg_is_recorded = false;
constexpr uint8_t button_delay_period = 175;

constexpr uint8_t BTN_EMG_RECORD_PIN = 2;
constexpr uint8_t LED_EMG_RECORD_PIN = 6;
constexpr uint8_t BTN_FOR_PATIENTS_PIN = 3;
constexpr uint8_t LED_FOR_PATIENTS_PIN = 4;
constexpr int16_t lower_bound = 0;
constexpr int16_t upper_bound = 1000;
constexpr int16_t average_bound = upper_bound / 2;
constexpr int16_t lower_average_bound = average_bound / 2;
constexpr int16_t average_upper_bound = (upper_bound + average_bound) / 2;

constexpr uint8_t EMG_PIN = A0;
constexpr uint8_t L_MINUS_PIN = 9;
constexpr uint8_t L_PLUS_PIN = 8;
uint32_t emg_debounce_time = 0;
constexpr uint8_t emg_delay_period = 10;
float filtered_signal;

float filtration_coefficient = 0.4;
String dir_name;
String file_name;
String patient_folder_name;

float expRunAvgFilter(float raw_value) {
  static float filtered_value = 0;
  filtered_value += (raw_value - filtered_value) * filtration_coefficient;
  return filtered_value;
}

void setup(void) {
  Serial.begin(9600);
  watch.begin();
  watch.settime(__TIMESTAMP__);

  pinMode(BTN_EMG_RECORD_PIN, INPUT_PULLUP);
  pinMode(LED_EMG_RECORD_PIN, OUTPUT);
  pinMode(BTN_FOR_PATIENTS_PIN, INPUT_PULLUP);
  pinMode(LED_FOR_PATIENTS_PIN, OUTPUT);
  pinMode(L_MINUS_PIN, INPUT);
  pinMode(L_PLUS_PIN, INPUT);

  attachInterrupt(0, ButtonIsr1, FALLING);
  attachInterrupt(1, ButtonIsr2, FALLING);

  if (!SD.begin(10)) {
    Serial.println(F("Initialization failed"));
    while (true)
      ;
  }
  Serial.println(F("Card OK"));
  dir_name = watch.gettime("d-m-y");
  SD.mkdir(dir_name);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(F("Portable"));
  lcd.setCursor(5, 1);
  lcd.print(F("Electromyograph"));
  lcd.setCursor(0, 2);
  lcd.print(F("Created by"));
  lcd.setCursor(4, 3);
  lcd.print(F("Kirill Cherdakov"));
  delay(3000);
  lcd.clear();
}

void loop(void) {
  emg_print();
  update_display();
}

void ButtonIsr1() {
  if (millis() - button_debounce_time1 >= button_delay_period) {
    button_debounce_time1 = millis();
    emg_is_recorded = !emg_is_recorded;
    digitalWrite(LED_EMG_RECORD_PIN, emg_is_recorded);
    (emg_is_recorded ? ++files_count : 0);
  }
}

void ButtonIsr2() {
  if (millis() - button_debounce_time2 >= button_delay_period) {
    button_debounce_time2 = millis();
    ++patient_number;
    SD.mkdir(dir_name + "/" + "case" + String(patient_number));
    files_count = 0;
  }
}

void emg_print() {
  if (millis() - emg_debounce_time >= emg_delay_period) {
    emg_debounce_time = millis();
    filtered_signal = expRunAvgFilter(analogRead(EMG_PIN));
    Serial.print(lower_bound);
    Serial.print(", ");
    Serial.print(lower_average_bound);
    Serial.print(", ");
    Serial.print(average_bound);
    Serial.print(", ");
    Serial.print(average_upper_bound);
    Serial.print(", ");
    Serial.print(upper_bound);
    Serial.print(", ");
    Serial.println(filtered_signal);
    if (emg_is_recorded) {
      patient_folder_name = "case" + String(patient_number);
      file_name = "emg_" + String(files_count) + ".csv";
      myFile = SD.open(dir_name + "/" + patient_folder_name + "/" + file_name, FILE_WRITE);
      myFile.print(watch.gettime("H:i:s"));
      myFile.print(", ");
      myFile.println(filtered_signal);
      myFile.close();
    }
  }
}

void update_display() {
  static uint32_t myTimer3 = 0;
  if (millis() - myTimer3 >= 1000) {
    myTimer3 = millis();
    lcd.setCursor(0, 0);
    lcd.print(watch.gettime("H:i:s"));
    lcd.print(emg_is_recorded ? " Record mode" : " Normal mode");
    lcd.setCursor(0, 1);
    lcd.print(watch.gettime("d-m-y"));
    lcd.setCursor(9, 2);
    lcd.print("Patient #" + String(patient_number));
    lcd.setCursor(9, 3);
    lcd.print("EMG #" + String(files_count));
  }
}
