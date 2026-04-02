/*MIT License

Copyright (c) 2026 adisorin

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.*/
/* https://github.com/adisorin?tab=repositories */


///////////*T-QT-Pro-DHT11-SHT31-v4*///////////


#include <Arduino.h>
#include <SPI.h>
//#include <WiFi.h>
//#include <WiFiMulti.h>
#include <Arduino_GFX_Library.h>
#include <time.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_SHT31.h> // Bibliotecă adăugată pentru SHT31

// CULORI RGB565
#define GC9107_BLACK   0x0000
#define GC9107_WHITE   0xFFFF
#define GC9107_RED     0xF800
#define GC9107_GREEN   0x07E0
#define GC9107_BLUE    0x001F
#define GC9107_YELLOW  0xFFE0
#define GC9107_DARKGREY  0x4208
#define GC9107_CYAN    0x07FF


// TFT PINS
#define TFT_MOSI 2
#define TFT_SCLK 3
#define TFT_CS   5
#define TFT_DC   6
#define TFT_RST  1
#define TFT_BL   4

// BUTOANE
#define BTN_IO0   0
#define BTN_IO47  47

// TFT INIT
Arduino_DataBus *bus = new Arduino_SWSPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, -1);
Arduino_GFX *gfx = new Arduino_GC9107(bus, TFT_RST, TFT_BL, true, 128, 128);

// SENZORI
#define DHTPIN 16
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// SHT31 PINS (I2C)
#define I2C_SDA 17  // Conectează pinul SDA al SHT31 aici
#define I2C_SCL 18  // Conectează pinul SCL al SHT31 aici
Adafruit_SHT31 sht31 = Adafruit_SHT31();

bool isSHT31 = false; // Flag pentru detectarea automată

unsigned long lastDHTUpdate = 0;

// PAGINA ACTUALA
enum Pagina { MAIN_UI, SYSTEM_INFO };
Pagina paginaCurenta = MAIN_UI;

void drawStaticUI() {
  gfx->fillScreen(GC9107_BLACK);
  gfx->setTextColor(GC9107_WHITE);
  gfx->setTextSize(2);
  gfx->setCursor(10, 1);
  gfx->println("ESP CLIMA");
}

void setup() {

  Serial.begin(115200);

  // Initializare Backlight TFT
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Initializare SPI si Ecran
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  gfx->begin();
  gfx->fillScreen(GC9107_BLACK);

  // Desenare interfata statica
  drawStaticUI();

  // --- DETECTIE AUTOMATA SENZOR ---
  
  // Initializare bus I2C pentru SHT31
  Wire.begin(I2C_SDA, I2C_SCL); 

  // Incearca sa porneasca SHT31 (adresa default 0x44)
  if (sht31.begin(0x44)) { 
    isSHT31 = true;
    Serial.println("SHT31 detectat pe I2C!");
  } else {
    // Daca SHT31 nu raspunde, configuram DHT11
    isSHT31 = false;
    dht.begin(); 
    Serial.println("SHT31 nu a fost gasit, se foloseste DHT11 pe pin 16");
  }
    // --- INITIALIZARE BUTOANE ---
  pinMode(BTN_IO0, INPUT_PULLUP);   // Pin 0
  pinMode(BTN_IO47, INPUT_PULLUP);  // Pin 47
}
bool blinkState = false;

void afiseazaDHT() {
  float temp, hum;

  // Încercăm să citim de la senzorul activat la boot
  if (isSHT31) {
    temp = sht31.readTemperature();
    hum = sht31.readHumidity();
  } else {
    temp = dht.readTemperature();
    hum = dht.readHumidity();
  }

  // Schimbăm starea pentru clipire/alternare la fiecare apel (2 secunde)
  blinkState = !blinkState;

  gfx->fillRect(0, 50, 128, 55, GC9107_BLACK);

  // VERIFICARE EROARE (Dacă senzorul curent nu răspunde)
  if (isnan(temp) || isnan(hum)) {
    gfx->setTextSize(2);

    // ALTERNARE MESAJE: Dacă nu avem date, afișăm pe rând eroarea de SHT și DHT
    if (blinkState) {
      gfx->setCursor(20, 55);
      gfx->setTextColor(GC9107_RED);
      gfx->println("SHT ERR");
    } else {
      gfx->setCursor(20, 75);
      gfx->setTextColor(GC9107_BLUE);
      gfx->println("DHT ERR");
    }
    return; // Oprim execuția aici dacă avem eroare
  }

  // --- AFIȘARE NORMALĂ (Dacă senzorul funcționează) ---
  gfx->setTextSize(2);
  gfx->setCursor(27, 55);
  gfx->setTextColor(GC9107_GREEN);
  gfx->print(temp, 1);
  gfx->print(" C");
  
  if ((temp < 18.0 || temp > 26.0) && blinkState) {
    gfx->setCursor(100, 55);
    gfx->setTextColor(GC9107_RED);
    gfx->print(" !"); 
  }

  gfx->setCursor(37, 80);
  gfx->setTextColor(GC9107_CYAN);
  gfx->print(hum, 0);
  gfx->print(" %");

  if ((hum < 30.0 || hum > 60.0) && blinkState) {
    gfx->setCursor(100, 80);
    gfx->setTextColor(GC9107_RED);
    gfx->print("!");
  }
}

// FUNCTIE CITIRE BATERIE
float citesteBaterie() {
  // 1. Eliberăm pinul ca să nu mai forțeze 3.3V (Backlight-ul se va stinge imperceptibil)
  pinMode(4, INPUT);
  delay(10); // Pauză scurtă pentru stabilizarea tensiunii pe divizor

  int raw = analogRead(4);
  
  // 2. Calculăm tensiunea (folosind factorul 2.0 sau 1.85 pentru reglaj fin)
  // Pe USB ar trebui să vezi acum între 4.1V și 4.4V
  float voltage = (raw / 4095.0) * 3.3 * 2.0; 

  // 3. Reactivăm Backlight-ul imediat
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);

  return voltage;
}


//////////////////////////////
// INFO SYSTEM
void showSystemInfo() {
  gfx->fillScreen(GC9107_BLACK);
  gfx->setTextSize(1);
  gfx->setTextColor(GC9107_WHITE);
  gfx->setCursor(0, 0);
  gfx->println("=== SYSTEM INFO ===\n");
  
  gfx->print(" Chip: "); gfx->println(ESP.getChipModel());
  gfx->print(" CPU MHz: "); gfx->println(ESP.getCpuFreqMHz());
  gfx->print(" Free RAM: "); gfx->println(ESP.getFreeHeap());
  gfx->print(" Flash MB: "); gfx->println(ESP.getFlashChipSize()/(1024*1024));
  gfx->println(" Display: GC9107");
  gfx->print(" DHT Type: "); gfx->println(DHTTYPE);
  gfx->print(" SHT31:"); 
  uint16_t stat = sht31.readStatus(); 
  gfx->print(" Stat: "); gfx->println(stat, HEX);
  gfx->print(" Battery: "); gfx->print(citesteBaterie(),2); gfx->println(" V");
}

// BUTOANE - verificare
void checkButtons() {
  if (digitalRead(BTN_IO0) == LOW) {  // Apăsat
    Serial.println("BTN_IO0 apăsat!");
    paginaCurenta = SYSTEM_INFO;  // schimbă pagina
    showSystemInfo();             // afișează imediat SYSTEM INFO
    delay(200);                   // debounce simplu
  }

  if (digitalRead(BTN_IO47) == LOW) {
    Serial.println("BTN_IO47 apăsat!");
    paginaCurenta = MAIN_UI;      // revine la UI principal
    drawStaticUI();
    delay(200);                   // debounce
  }
}

void loop() {
  unsigned long currentMillis = millis();

  checkButtons(); // verifică butoanele înainte

  if (paginaCurenta == MAIN_UI) {
    if (currentMillis - lastDHTUpdate > 1000) {
      afiseazaDHT();
      lastDHTUpdate = currentMillis;
    }
  }
  // Dacă e SYSTEM_INFO, nu rescrie ecranul, rămâne static până revii
}
