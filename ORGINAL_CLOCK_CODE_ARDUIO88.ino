#include "DHT.h"
#include "FastLED.h"
#include "Mau.h"
#include "Clock.h"
#include <Wire.h>
#include "RTClib.h"
#include "bluetooth.h"
#include <SoftwareSerial.h>

#define DHTPIN 4
#define DHTTYPE DHT11
#define NUM_LEDS 120
#define DATA_PIN 12
#define HL01_PIN A0
const int rxPin = 2;
const int txPin = 3;

RTC_DS1307 rtc;
DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial hc06(rxPin, txPin);
CRGBArray<NUM_LEDS> leds;

// متغیرهای زمان‌بندی
unsigned long previousMillis = 0;
const long timeInterval = 20000;  // 20 ثانیه نمایش زمان
const long tempInterval = 3000;   // 3 ثانیه نمایش دما
bool showTemp = false;

// متغیرهای عمومی
unsigned long t;
bool rainbow = 0;
String rgb = "";
String copyRgb = "";
int stt = 0;
int targetBrightness = 255;
int currentBrightness = 255;
unsigned long lastBrightnessUpdate = 0;
const unsigned long brightnessUpdateInterval = 50;

void setup() {
    Serial.begin(9600);
    delay(2000);
    t = millis();
    Wire.begin();
    rtc.begin();
    dht.begin();
    hc06.begin(9600);
    FastLED.addLeds<WS2811, DATA_PIN, RGB>(leds, NUM_LEDS);
    pinMode(HL01_PIN, INPUT);

    // نمایش HELLO با رنگ آبی در ابتدای کار
    unsigned long start = millis();
    while (millis() - start < 5000) {
        displayBlueHELLO();
        delay(50);
    }
    clearLEDs();
}

void loop() {
    handleBluetoothInput();
    
    unsigned long currentMillis = millis();
    
    if (stt == 0) { // حالت عادی
        if (!showTemp) {
            // نمایش زمان
            if (currentMillis - previousMillis >= timeInterval) {
                previousMillis = currentMillis;
                showTemp = true;
                clearLEDs();
            }
            displayTime();
        } else {
            // نمایش دما
            if (currentMillis - previousMillis >= tempInterval) {
                previousMillis = currentMillis;
                showTemp = false;
                clearLEDs();
            }
            displayTemperature();
        }
    } else {
        // حالات خاص بلوتوث
        handleSpecialModes();
    }
}

// نمایش HELLO با رنگ آبی
void displayBlueHELLO() {
  adjustBrightness((analogRead(HL01_PIN) > 512 ? 12 : 255));

    // رنگ آبی با Hue=160 در HSV
    CHSV blueColor(255, 255, 255);

    // اندیس‌های LEDهای مربوط به هر حرف
    int H_indices[] = {0, 1, 3, 4, 6};
    int E_indices[] = {7, 9, 10, 11, 12};
    int L_indices[] = {16, 18, 19, 21};
    int O_indices[] = {23, 24, 25, 26, 27, 28};

    // روشن کردن حروف به رنگ آبی
    for (int i : H_indices) leds[i] = blueColor;
    for (int i : E_indices) leds[i] = blueColor;
    for (int i : L_indices) leds[i] = blueColor;
    for (int i : O_indices) leds[i] = blueColor;

    FastLED.show();
}

void displayTime() {
    static uint8_t hue;
    Clock x;
    adjustBrightness();

    x.inputTime(rtc.now().hour(), rtc.now().minute());
    x.outputClock("Time", (int)rtc.now().second() % 2);

    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = x.outputArr(i) ? (CRGB)CHSV((hue + i*10) % 255, 255, 255) : CRGB::Black;
    }

    FastLED.show();
    hue += 4;
    delay(40);
}

void displayTemperature() {
    static uint8_t hue;
    Clock x;
    adjustBrightness();

    x.inputTemp((int)dht.readTemperature());
    x.outputClock("Temp", 0);

    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = x.outputArr(i) ? (CRGB)CHSV((hue + i*20) % 255, 255, 255) : CRGB::Black;
    }

    FastLED.show();
    hue += 4;
    delay(40);
}

void adjustBrightness(int target) {
    targetBrightness = target;
    if (millis() - lastBrightnessUpdate > brightnessUpdateInterval) {
        if (currentBrightness < targetBrightness) {
            currentBrightness = min(currentBrightness + 5, targetBrightness);
        } else if (currentBrightness > targetBrightness) {
            currentBrightness = max(currentBrightness - 5, targetBrightness);
        }
        lastBrightnessUpdate = millis();
        FastLED.setBrightness(currentBrightness);
    }
}

void adjustBrightness() {
    adjustBrightness((analogRead(HL01_PIN) > 512) ? 12 : 255);
}

void clearLEDs() {
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
    }
    FastLED.show();
}

void handleBluetoothInput() {
    while (hc06.available()) {
        char r = (char)hc06.read();
        if (r == ')') { stt = 1; rgb = ""; break; }
        if (r == 'a') { stt = 2; rgb = ""; break; }
        if (r == 'b') { stt = 3; rgb = ""; break; }
        if (r == 'c') { stt = 4; rgb = ""; break; }
        if (r == 'd') { stt = 5; rgb = ""; break; }
        if (r == 'e') { stt = 6; rgb = ""; break; }
        if (r == 'f') { stt = 7; rgb = ""; break; }
        if (r == '>') { stt = 8; break; }
        else { rgb += r; }
    }
}

void handleSpecialModes() {
    switch(stt) {
        case 3: modeRainbow(); break;
        case 4: modeBlueWave(); break;
        case 5: modePulse(); break;
        case 6: modeRandom(); break;
        case 7: modeTempShow(); break;
        case 8: setTime(); break;
    }
}

void modeRainbow() {
    Clock x;
    uint8_t hue = 50;
    static int direction = 1;
    
    while(!hc06.available()) {
        x.inputTime(rtc.now().hour(), rtc.now().minute());
        x.outputClock("Time", (int)rtc.now().second()%2);
        
        if(hue >= 170 || hue <= 50) direction *= -1;
        
        for(int i = 0; i < NUM_LEDS; i++) {
            leds[i] = x.outputArr(i) ? (CRGB)CHSV(hue, 255, 255) : CRGB::Black;
        }
        
        hue += direction;
        FastLED.delay(25);
    }
    stt = 0;
}

void modeBlueWave() {
    Clock x;
    uint8_t hue = 170;
    static int direction = 1;
    
    while(!hc06.available()) {
        x.inputTime(rtc.now().hour(), rtc.now().minute());
        x.outputClock("Time", (int)rtc.now().second()%2);
        
        if(hue >= 245 || hue <= 170) direction *= -1;
        
        for(int i = 0; i < NUM_LEDS; i++) {
            leds[i] = x.outputArr(i) ? (CRGB)CHSV(hue, 255, 255) : CRGB::Black;
        }
        
        hue += direction;
        FastLED.delay(25);
    }
    stt = 0;
}

void modePulse() {
    Clock x;
    static int direction = 1;
    int brightness = 0;
    
    while(!hc06.available()) {
        x.inputTime(rtc.now().hour(), rtc.now().minute());
        x.outputClock("Time", (int)rtc.now().second()%2);
        
        if(brightness >= 255 || brightness <= 0) direction *= -1;
        
        for(int i = 0; i < NUM_LEDS; i++) {
            leds[i] = x.outputArr(i) ? (CRGB)CHSV(i*10, 255, brightness) : CRGB::Black;
        }
        
        brightness += direction;
        FastLED.delay(30);
    }
    stt = 0;
}

void modeRandom() {
    Clock x;
    
    while(!hc06.available()) {
        x.inputTime(rtc.now().hour(), rtc.now().minute());
        x.outputClock("Time", (int)rtc.now().second()%2);
        
        for(int i = 0; i < NUM_LEDS; i++) {
            if(x.outputArr(i)) {
                leds[i] = CRGB(random(256), random(256), random(256));
            } else {
                leds[i] = CRGB::Black;
            }
        }
        FastLED.delay(800);
    }
    stt = 0;
}

void modeTempShow() {
    Clock x;
    unsigned long startTime = millis();
    
    while(!hc06.available() && (millis() - startTime < 3000)) {
        x.inputTemp((int)dht.readTemperature());
        x.outputClock("Temp", 0);
        
        for(int i = 0; i < NUM_LEDS; i++) {
            leds[i] = x.outputArr(i) ? (CRGB)CHSV(i*20, 255, 255) : CRGB::Black;
        }
        FastLED.delay(30);
    }
    stt = 0;
}

void setTime() {
    int s1 = rgb.indexOf(':');
    int s2 = rgb.indexOf('-', s1+1);
    int s3 = rgb.indexOf(':', s2+1);
    int s4 = rgb.indexOf(':', s3+1);
    
    DateTime dt(
        rgb.substring(s4+1).toInt(), // سال
        rgb.substring(s3+1, s4).toInt(), // ماه
        rgb.substring(s2+1, s3).toInt(), // روز
        rgb.substring(0, s1).toInt(), // ساعت
        rgb.substring(s1+1, s2).toInt(), // دقیقه
        0 // ثانیه
    );
    
    rtc.adjust(dt);
    delay(2000);
    rgb = "";
    stt = 0;
}