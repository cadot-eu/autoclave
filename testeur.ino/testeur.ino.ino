#include <TM1637.h>

// --- Pins ---
#define PIN_WATER 8       // sonde d'eau
#define PIN_RELAY 9       // relais 5V
#define PIN_LED 10        // LED témoin
#define PIN_SWITCH_INC 11 // bouton incrément
#define PIN_SWITCH_DEC 12 // bouton décrément
#define PIN_PRESSURE A0   // capteur pression 0-30 PSI
#define CLK_PIN 2         // TM1637 CLK
#define DIO_PIN 3         // TM1637 DIO

TM1637 tm(CLK_PIN, DIO_PIN);
int tmCounter = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println("=== Test eau + relais + TM1637 + boutons + pression ===");

  pinMode(PIN_WATER, INPUT_PULLUP);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  pinMode(PIN_SWITCH_INC, INPUT_PULLUP);
  pinMode(PIN_SWITCH_DEC, INPUT_PULLUP);

  tm.init();
  tm.display(tmCounter);
}

void loop() {
  // --- Détection d'eau ---
  int etat = digitalRead(PIN_WATER);
  if (etat == LOW) {
    Serial.println("Eau détectée ✅");
    digitalWrite(PIN_RELAY, HIGH);
    digitalWrite(PIN_LED, HIGH);
  } else {
    Serial.println("Pas d'eau ❌");
    digitalWrite(PIN_RELAY, LOW);
    digitalWrite(PIN_LED, LOW);
  }

  // --- Lecture pression ---
  int raw = analogRead(PIN_PRESSURE);
  float voltage = raw * (5.0 / 1023.0);          // conversion en volts
  float pressure = (voltage - 0.5) * 7.5;        // 0.5-4.5V -> 0-30 PSI
  Serial.print("Pression (PSI) : ");
  Serial.println(pressure, 1);

  // --- Bouton incrément ---
  if (digitalRead(PIN_SWITCH_INC) == LOW) {
    tmCounter++;
    Serial.print("Compteur incrémenté: ");
    Serial.println(tmCounter);
    tm.display(tmCounter);
    delay(200); // petite pause pour éviter répétition
  }

  // --- Bouton décrément ---
  if (digitalRead(PIN_SWITCH_DEC) == LOW) {
    tmCounter--;
    Serial.print("Compteur décrémenté: ");
    Serial.println(tmCounter);
    tm.display(tmCounter);
    delay(200); // petite pause pour éviter répétition
  }

  delay(100); // boucle rapide
}
