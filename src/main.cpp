#include <Arduino.h>
#include <EEPROM.h>
#include <ESP32Servo.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "pin_config.h"
#include "time_func.h"
#include "ui/lv_setup.h"
#include "ui/ui.h"
bool shouldTakePill = false;
int servoPin = 13;
int buzzerPin = 12;
Servo myservo;
int pos = 0;

int BUTTON1 = 0;   // Define the pin for BUTTON1
int BUTTON2 = 14;  // Define the pin for BUTTON2

enum {
    EDITING_HOUR,
    EDITING_MINUTE,
    EDITING_SECOND
} EDITING_MODE;

bool isCounting = true;
bool isEditing = false;
int editing = EDITING_HOUR;

void setInterval(uint32_t interval) {
    EEPROM.write(0, (interval >> 24) & 0xFF);  // Most significant byte
    EEPROM.write(1, (interval >> 16) & 0xFF);
    EEPROM.write(2, (interval >> 8) & 0xFF);
    EEPROM.write(3, interval & 0xFF);  // Least significant byte
    EEPROM.commit();                   // Required for ESP32
}

uint32_t getInterval() {
    return (EEPROM.read(0) << 24) |
           (EEPROM.read(1) << 16) |
           (EEPROM.read(2) << 8) |
           EEPROM.read(3);
}

uint32_t getRemainingTime() {
    // Read 32-bit remaining time from EEPROM addresses 4-7
    return (EEPROM.read(4) << 24) |
           (EEPROM.read(5) << 16) |
           (EEPROM.read(6) << 8) |
           EEPROM.read(7);
}

void setRemainingTime(uint32_t remainingTime) {
    // Write 32-bit remaining time to EEPROM addresses 4-7
    EEPROM.write(4, (remainingTime >> 24) & 0xFF);
    EEPROM.write(5, (remainingTime >> 16) & 0xFF);
    EEPROM.write(6, (remainingTime >> 8) & 0xFF);
    EEPROM.write(7, remainingTime & 0xFF);
    EEPROM.commit();  // Required for ESP32
}

void lv_set_times(uint32_t interval, uint32_t remainingTime) {
    char interval_buf[16];
    char remainingTime_buf[16];
    // interval and remainingTime are in seconds, we want to represent them in hours, minutes and seconds
    uint32_t interval_hours = interval / 3600;
    uint32_t interval_minutes = (interval % 3600) / 60;
    uint32_t interval_seconds = interval % 60;
    uint32_t remainingTime_hours = remainingTime / 3600;
    uint32_t remainingTime_minutes = (remainingTime % 3600) / 60;
    uint32_t remainingTime_seconds = remainingTime % 60;
    sprintf(remainingTime_buf, "%02d:%02d:%02d", remainingTime_hours, remainingTime_minutes, remainingTime_seconds);
    char interval_Hours_buf[16];
    char interval_Minutes_buf[16];
    char interval_Seconds_buf[16];
    sprintf(interval_Hours_buf, "%02dHr", interval_hours);
    sprintf(interval_Minutes_buf, "%02dMin", interval_minutes);
    sprintf(interval_Seconds_buf, "%02dSec", interval_seconds);
    lv_label_set_text(ui_Interval_Hours_Value, interval_Hours_buf);
    lv_label_set_text(ui_Interval_Minutes_Value, interval_Minutes_buf);
    lv_label_set_text(ui_Interval_Seconds_Value, interval_Seconds_buf);
    lv_label_set_text(ui_Next_Pill_Time_Value, remainingTime_buf);
    setRemainingTime(remainingTime);
}

void buzzerTask(void* parameter) {
    while (true) {
        if (shouldTakePill) {
            tone(buzzerPin, 1000);
            delay(1000);
            noTone(buzzerPin);
            delay(1000);
        } else {
            delay(1000);
        }
    }
}

void buttonTask(void* parameter) {
    pinMode(BUTTON1, INPUT);
    pinMode(BUTTON2, INPUT);
    bool b1Pressed = false;
    bool b2Pressed = false;
    double b2PressedTime = 0;
    while (true) {
        bool b1 = !digitalRead(BUTTON1);
        bool b2 = !digitalRead(BUTTON2);
        if (b1 && !b1Pressed) {
            b1Pressed = true;
            Serial.println("Button 1 pressed");
            shouldTakePill = false;
            if (isEditing) {
                uint32_t interval = getInterval();
                uint32_t interval_hours = interval / 3600;
                uint32_t interval_minutes = (interval % 3600) / 60;
                uint32_t interval_seconds = interval % 60;
                if (editing == EDITING_HOUR) {
                    interval_hours += 1;
                } else if (editing == EDITING_MINUTE) {
                    interval_minutes += 15;
                } else if (editing == EDITING_SECOND) {
                    interval_seconds += 5;
                }
                if (interval_seconds >= 60) {
                    interval_seconds = 0;
                }
                if (interval_minutes >= 60) {
                    interval_minutes = 0;
                }
                if (interval_hours >= 24) {
                    interval_hours = 0;
                }
                interval = interval_hours * 3600 + interval_minutes * 60 + interval_seconds;
                setInterval(interval);
                lv_set_times(interval, interval);
            }
        } else if (!b1 && b1Pressed) {
            b1Pressed = false;
        }
        if (b2 && !b2Pressed) {
            b2Pressed = true;
            b2PressedTime = millis();
            Serial.println("Button 2 pressed");
            if (isEditing) {
                editing = (editing + 1) % 3;
            }
        } else if (!b2 && b2Pressed) {
            if (millis() - b2PressedTime > 3000) {
                Serial.println("Button 2 long pressed");
                isCounting = !isCounting;
                isEditing = !isEditing;
            }
            b2Pressed = false;
            b2PressedTime = 0;
        }
        // Serial.println("Button 1: " + String(b1));
        // Serial.println("Button 2: " + String(b2));
        delay(1);
    }
}

void editTask(void* parameter) {
    bool onoff = false;
    while (true) {
        if (isEditing) {
            if (editing == EDITING_HOUR) {
                lv_obj_set_style_text_color(ui_Interval_Hours_Value, lv_color_hex(0xFB0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(ui_Interval_Minutes_Value, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(ui_Interval_Seconds_Value, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else if (editing == EDITING_MINUTE) {
                lv_obj_set_style_text_color(ui_Interval_Hours_Value, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(ui_Interval_Minutes_Value, lv_color_hex(0xFB0000), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(ui_Interval_Seconds_Value, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            } else if (editing == EDITING_SECOND) {
                lv_obj_set_style_text_color(ui_Interval_Hours_Value, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(ui_Interval_Minutes_Value, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_text_color(ui_Interval_Seconds_Value, lv_color_hex(0xFB0000), LV_PART_MAIN | LV_STATE_DEFAULT);
            } 
            onoff = !onoff;
        } else {
            lv_obj_set_style_text_color(ui_Interval_Hours_Value, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_Interval_Minutes_Value, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(ui_Interval_Seconds_Value, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        delay(100);
    }
}

void setup() {
    EEPROM.begin(128);
    Serial.begin(115200);
    lv_begin();  // Initialize lvgl with display and touch
    ui_init();   // Initialize UI generated by Square Line

    // setInterval(10);       // Set interval to 1 hour
    // setRemainingTime(10);  // Set remaining time to 1 hour
    Serial.println("Interval: " + String(getInterval()));
    Serial.println("Remaining Time: " + String(getRemainingTime()));
    lv_set_times(10, 10);

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    myservo.setPeriodHertz(50);  // standard 50 hz servo
    // myservo.attach(servoPin, 1000, 2000);  // attaches the servo on pin 18 to the servo object

    pinMode(buzzerPin, OUTPUT);
    noTone(buzzerPin);
    digitalWrite(buzzerPin, HIGH);

    xTaskCreate(buzzerTask, "buzzerTask", 2048, NULL, 1, NULL);
    xTaskCreate(buttonTask, "buttonTask", 2048, NULL, 1, NULL);
    xTaskCreate(editTask, "editTask", 2048, NULL, 1, NULL);

    setInterval(3600);
    setRemainingTime(3600);
}

void loop() {
    lv_handler();   // Update UI
    update_time();  // Update time and date on UI
    if (isCounting) {
        uint32_t interval = getInterval();
        uint32_t remainingTime = getRemainingTime();
        if (remainingTime > 0) {
            remainingTime -= 1;
            lv_set_times(interval, remainingTime);
            setRemainingTime(remainingTime);
            if (remainingTime <= 0) {
                // Turn off the relay
                myservo.attach(servoPin, 1000, 2000);      // attaches the servo on pin 18 to the servo object
                for (pos = 3; pos <= 48 + 45; pos += 1) {  // goes from 0 degrees to 180 degrees
                    // in steps of 1 degree
                    myservo.write(pos);  // tell servo to go to position in variable 'pos'
                    delay(2);            // waits 15 ms for the servo to reach the position
                }
                delay(200);
                for (pos = 48 + 45; pos >= 3; pos -= 1) {  // goes from 180 degrees to 0 degrees
                    myservo.write(pos);                    // tell servo to go to position in variable 'pos'
                    delay(2);                              // waits 15 ms for the servo to reach the position
                }
                myservo.detach();
                shouldTakePill = true;
            }
        } else {
            if (interval > 0) {
                remainingTime = interval;
                setRemainingTime(remainingTime);
                lv_set_times(interval, remainingTime);
                // Turn on the relay
                // shouldTakePill = false;
            }
        }
    }
    delay(1000);

    Serial.println("Interval: " + String(getInterval()));
    Serial.println("Remaining Time: " + String(getRemainingTime()));
}