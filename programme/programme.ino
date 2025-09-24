#include <TM1637.h>

// --- Pins ---
#define PIN_WATER A0      // Sonde d'eau
#define PIN_RELAY 9       // Relais 5V
#define PIN_LED 10        // LED témoin
#define PIN_SWITCH_INC 11 // Bouton incrément
#define PIN_SWITCH_DEC 12 // Bouton décrément
#define PIN_PRESSURE A5   // Capteur pression 0-30 PSI
#define CLK_PIN 2         // TM1637 CLK
#define DIO_PIN 3         // TM1637 DIO

TM1637 tm(CLK_PIN, DIO_PIN);

// --- Variables ---
unsigned long startTime = 0;
unsigned long lastButtonTime = 0;
unsigned long blinkTime = 0;
int timeRemaining = 15;         // durée initiale en minutes
const int initialTime = 15;     // valeur de reset
bool pumpRunning = false;
bool waitingPressure = false;   // 🔴 pompe ON mais attente pression
bool blinkState = false;
const float PRESSURE_START = 1.5;   // 🔴 seuil en MPa pour démarrage timer
const float PRESSURE_THRESHOLD = 20.0; // seuil sécurité en PSI (ancienne logique)

// --- Setup ---
void setup() {
  Serial.begin(115200);
  while (!Serial) { ; }
  Serial.println("=== Contrôleur de pompe avec minuteur ===");

  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  pinMode(PIN_SWITCH_INC, INPUT_PULLUP);
  pinMode(PIN_SWITCH_DEC, INPUT_PULLUP);

  tm.init();
  tm.setBrightnessPercent(80);

  displayMinutes(timeRemaining);
}

// --- Boucle principale ---
void loop() {
  unsigned long currentTime = millis();

  // --- Vérification eau ---
  if (!readWater()) {
    if (pumpRunning) {
      Serial.println("Pas d'eau - Arrêt immédiat ❌");
      stopPump();
    }
    tm.display("EAU ");
    delay(500);
    return;
  }

  // --- Gestion boutons (quand pompe arrêtée uniquement) ---
  if (!pumpRunning && (currentTime - lastButtonTime > 200)) {
    bool incPressed = (digitalRead(PIN_SWITCH_INC) == LOW);
    bool decPressed = (digitalRead(PIN_SWITCH_DEC) == LOW);

    if (incPressed && decPressed) {
      // --- DOUBLE APPUI ---
      Serial.println("🔄 RESET + START demandé");
      timeRemaining = initialTime;
      displayMinutes(timeRemaining);
      delay(200);
      startPump();             // Pompe ON mais timer pas lancé tant que pression < 1.5 MPa
      waitingPressure = true;  // 🔴 active la phase attente pression
      startTime = millis();
      lastButtonTime = currentTime;
      return; // on sort de la loop pour éviter faux départ
    }
    else if (incPressed) {
      timeRemaining++;
      if (timeRemaining > 99) timeRemaining = 99;
      displayMinutes(timeRemaining);
      Serial.print("Temps augmenté: ");
      Serial.println(timeRemaining);
      lastButtonTime = currentTime;
    }
    else if (decPressed) {
      timeRemaining--;
      if (timeRemaining < 1) timeRemaining = 1;
      displayMinutes(timeRemaining);
      Serial.print("Temps diminué: ");
      Serial.println(timeRemaining);
      lastButtonTime = currentTime;
    }
  }

  // --- Lecture pression ---
  int raw = analogRead(PIN_PRESSURE);
  float voltage = raw * (5.0 / 1023.0);
  float pressurePSI = (voltage - 0.5) * 7.5;
  if (pressurePSI < 0) pressurePSI = 0;
  float pressureMPa = pressurePSI * 0.00689476;
  int pressureDisplay = (int)(pressureMPa * 100.0 + 0.5); // ex : 0.15 MPa → affichage "15"

  // --- Attente pression avant démarrage du timer ---
  if (waitingPressure) {
    // clignotement affichage
    if (currentTime - blinkTime > 500) {
      blinkTime = currentTime;
      blinkState = !blinkState;
      if (blinkState) displayMinutes(timeRemaining);
      else tm.display("    ");
    }

    // démarrage minuteur seulement si pression >= 1.5 MPa
    if (pressureMPa >= PRESSURE_START) {
      waitingPressure = false;
      startTime = millis(); // vrai lancement chrono
      Serial.println("✅ Pression atteinte -> Minuterie démarrée");
      displayTimePressure(timeRemaining, pressureDisplay);
    }
    return;
  }

  // --- Gestion pompe active seulement une fois pression démarrée ---
  if (pumpRunning && !waitingPressure) {
    unsigned long elapsed = (currentTime - startTime) / 1000;
    unsigned long totalSeconds = timeRemaining * 60;

    if (elapsed >= totalSeconds) {
      Serial.println("Temps écoulé - Arrêt pompe");
      finishCycle();
      return;
    }

    int minutesLeft = (totalSeconds - elapsed) / 60;
    displayTimePressure(minutesLeft, pressureDisplay);
  }

  delay(100);
}

// --- Démarrage pompe (relais ON) ---
void startPump() {
  pumpRunning = true;
  digitalWrite(PIN_RELAY, HIGH);
  digitalWrite(PIN_LED, HIGH);
  tm.display("ON  ");
  delay(1000);
  Serial.println("Pompe enclenchée (attente pression)");
}

// --- Arrêt pompe ---
void stopPump() {
  pumpRunning = false;
  waitingPressure = false;
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_LED, LOW);
  Serial.println("Pompe arrêtée");
}

// --- Fin de cycle ---
void finishCycle() {
  stopPump();
  tm.display("OFF ");
  delay(2000);

  for (int i = 5; i >= 1; i--) {
    tm.display(i);
    delay(1000);
  }

  tm.display("FIN ");
  delay(3000);

  timeRemaining = initialTime;   // reset du timer
  displayMinutes(timeRemaining);
}

// --- Affiche MM00 quand réglage ---
void displayMinutes(int minutes) {
  if (minutes > 99) minutes = 99;
  int display_value = minutes * 100;
  tm.display(display_value);
}

// --- Affiche MMxx (minutes + pression) ---
void displayTimePressure(int minutes, int pressure_val) {
  if (minutes > 99) minutes = 99;
  if (pressure_val > 99) pressure_val = 99;
  int display_value = minutes * 100 + pressure_val;
  tm.display(display_value);
}

// --- Fonction lecture eau (ta version) ---
bool readWater() {
  int seuil = 300;
  int valeur = analogRead(PIN_WATER);
  if (valeur > seuil) Serial.print("Eau présente");
  else Serial.print("Pas d'eau");
  Serial.print("(");
  Serial.print(valeur);
  Serial.println(")");
  
  return (valeur > seuil); // retourne vrai si eau présente
}
