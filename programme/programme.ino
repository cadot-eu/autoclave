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
int timeRemaining = 45;         // durée initiale en minutes
const int initialTime = 15;     // valeur de reset
bool pumpRunning = false;
bool waitingPressure = false;   // 🔴 pompe ON mais attente pression
bool cycleFinished = false;     // 🔴 indique si cycle terminé (affiche OF)
bool blinkState = false;
// --- Mode de fonctionnement ---
const bool DEBUG_MODE = false;        // true = mode test, false = mode production

// --- Seuils de pression (dépendant du mode) ---
float PRESSURE_MAX;
float PRESSURE_MIN;

// --- Seuil détection eau ---
int WATER_THRESHOLD = 300;
bool heatingPaused = false;          // 🔴 indique si chauffage en pause pour régulation pression
unsigned long lastDebugTime = 0;     // Pour limiter la fréquence des messages debug
unsigned long lastDisplayTime = 0;   // Pour limiter la fréquence de mise à jour affichage
int lastDisplayedRaw = -1;           // Dernière valeur affichée

// --- Setup ---
void setup() {
  Serial.begin(115200);
  // Suppression du while (!Serial) pour fonctionner sans USB
  delay(1000);  // Délai pour stabilisation
  
  // Configuration des seuils selon le mode
  if (DEBUG_MODE) {
    PRESSURE_MAX = 0.05;
    PRESSURE_MIN = 0.04;
    Serial.println("=== MODE TEST - Contrôleur de pompe avec minuteur ===");
  } else {
    PRESSURE_MAX = 0.15;
    PRESSURE_MIN = 0.14;
    Serial.println("=== MODE PRODUCTION - Contrôleur de pompe avec minuteur ===");
  }

  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, HIGH);  // HIGH = OFF pour relais inversé
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

  // --- Mise à jour affichage pression quand pompe arrêtée (toutes les 500ms) ---
  if (!pumpRunning && (currentTime - lastDisplayTime > 500)) {
    if (cycleFinished) {
      displayOFPressure();
    } else {
      displayMinutes(timeRemaining);
    }
    lastDisplayTime = currentTime;
  }

  // --- Gestion boutons (quand pompe arrêtée uniquement) ---
  if (!pumpRunning && (currentTime - lastButtonTime > 200)) {
    bool incPressed = (digitalRead(PIN_SWITCH_INC) == LOW);
    bool decPressed = (digitalRead(PIN_SWITCH_DEC) == LOW);

    if (incPressed && decPressed) {
      // --- DOUBLE APPUI ---
      Serial.println("🔄 RESET + START demandé");
      cycleFinished = false;   // 🔴 Reset du flag OF
      // timeRemaining conserve sa valeur actuelle (ne pas réinitialiser à initialTime)
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

  // --- Affichage compact en mode test (limité à 1Hz pour réduire charge USB) ---
  if (DEBUG_MODE && (currentTime - lastDebugTime > 1000)) {
    int waterValue = analogRead(PIN_WATER);
    bool waterPresent = (waterValue > WATER_THRESHOLD);
    bool relayState = !digitalRead(PIN_RELAY); // Inversé : LOW=ON, HIGH=OFF
    Serial.print("Pression:");
    Serial.print(pressureMPa, 3);
    Serial.print(", Eau:");
    Serial.print(waterValue);
    Serial.print(waterPresent ? "/ON" : "/OFF");
    Serial.print(", Relay:");
    Serial.println(relayState ? "ON" : "OFF");
    lastDebugTime = currentTime;
  }

  // --- Attente pression avant démarrage du timer ---
  if (waitingPressure) {
    // Affichage de la pression convertie au format "PXX" (mise à jour toutes les 300ms)
    if (currentTime - lastDisplayTime > 300 || abs(raw - lastDisplayedRaw) > 5) {
      // Format: "P" + pression sur deux chiffres (ex: P15 pour 0.15 MPa)
      String display_str = "P";
      if (pressureDisplay < 10) {
        display_str += "0";
      }
      display_str += String(pressureDisplay);
      tm.display(display_str.c_str());
      lastDisplayTime = currentTime;
      lastDisplayedRaw = raw;
    }

    // démarrage minuteur seulement si pression >= seuil max
    if (pressureMPa >= PRESSURE_MAX) {
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

    // --- NOUVELLE LOGIQUE DE RÉGULATION PRESSION ---
    // Si pression >= 0.13 MPa et chauffage pas encore en pause
    if (pressureMPa >= PRESSURE_MAX && !heatingPaused) {
      heatingPaused = true;
      digitalWrite(PIN_RELAY, HIGH); // Arrêt chauffage (HIGH = OFF)
      Serial.print("🛑 Chauffage arrêté - Pression: ");
      Serial.print(pressureMPa, 3);
      Serial.println(" MPa");
    }
    // Si pression <= 0.125 MPa et chauffage en pause (hystérésis)
    else if (pressureMPa <= PRESSURE_MIN && heatingPaused) {
      heatingPaused = false;
      digitalWrite(PIN_RELAY, LOW);  // Redémarrage chauffage (LOW = ON)
      Serial.print("🔥 Chauffage redémarré - Pression: ");
      Serial.print(pressureMPa, 3);
      Serial.println(" MPa");
    }

    int minutesLeft = (totalSeconds - elapsed) / 60;
    displayTimePressure(minutesLeft, pressureDisplay);
    
    // Affichage état chauffage dans le moniteur série
    Serial.print("Timer: ");
    Serial.print(minutesLeft);
    Serial.print("min - Pression: ");
    Serial.print(pressureMPa, 3);
    Serial.print(" MPa - Chauffage: ");
    Serial.println(heatingPaused ? "PAUSE" : "ACTIF");
  }

  delay(100);
}

// --- Démarrage pompe (relais ON) ---
void startPump() {
  pumpRunning = true;
  // Commutation relais avec délai pour éviter parasites USB
  delay(50);  // Petit délai avant commutation
  digitalWrite(PIN_RELAY, LOW);   // LOW = ON pour relais inversé
  delay(100); // Délai après commutation pour stabilisation
  digitalWrite(PIN_LED, HIGH);
  tm.display("ON  ");
  delay(1000);
  if (DEBUG_MODE) Serial.println("Pompe enclenchée (attente pression)");
}

// --- Arrêt pompe ---
void stopPump() {
  pumpRunning = false;
  waitingPressure = false;
  heatingPaused = false;  // 🔴 Reset de l'état de régulation
  // Commutation relais avec délai pour éviter parasites USB
  delay(50);
  digitalWrite(PIN_RELAY, HIGH);  // HIGH = OFF pour relais inversé
  delay(100); // Stabilisation après commutation
  digitalWrite(PIN_LED, LOW);
  if (DEBUG_MODE) Serial.println("Pompe arrêtée");
}

// --- Fin de cycle ---
void finishCycle() {
  stopPump();
  cycleFinished = true;  // 🔴 Active l'affichage OF
  tm.display("OFF ");
  delay(2000);

  for (int i = 5; i >= 1; i--) {
    tm.display(i);
    delay(1000);
  }

  tm.display("FIN ");
  delay(3000);

  timeRemaining = initialTime;   // reset du timer
  displayOFPressure();           // Affiche OF + pression
}

// --- Affiche MMPP (minutes + pression actuelle) ---
void displayMinutes(int minutes) {
  if (minutes > 99) minutes = 99;
  
  // Lecture de la pression actuelle
  int raw = analogRead(PIN_PRESSURE);
  float voltage = raw * (5.0 / 1023.0);
  float pressurePSI = (voltage - 0.5) * 7.5;
  if (pressurePSI < 0) pressurePSI = 0;
  float pressureMPa = pressurePSI * 0.00689476;
  int pressureDisplay = (int)(pressureMPa * 100.0 + 0.5);
  
  // Toujours afficher format MMPP (minutes + pression)
  displayTimePressure(minutes, pressureDisplay);
}

// --- Affiche MMPP (minutes + pression) ---
void displayTimePressure(int minutes, int pressure_val) {
  if (minutes > 99) minutes = 99;
  if (pressure_val > 99) pressure_val = 99;
  
  // Format: MMPP (MM=minutes avec zéro initial, PP=pression)
  // Exemples: 0816 = 08 min + 0.16 MPa, 1504 = 15 min + 0.04 MPa
  
  // Formatage correct pour affichage 4 digits avec zéros initiaux
  String display_str = "";
  if (minutes < 10) {
    display_str += "0";
  }
  display_str += String(minutes);
  
  if (pressure_val < 10) {
    display_str += "0";
  }
  display_str += String(pressure_val);
  
  tm.display(display_str.c_str());
}

// --- Affiche OFPP (OF + pression actuelle) ---
void displayOFPressure() {
  // Lecture de la pression actuelle
  int raw = analogRead(PIN_PRESSURE);
  float voltage = raw * (5.0 / 1023.0);
  float pressurePSI = (voltage - 0.5) * 7.5;
  if (pressurePSI < 0) pressurePSI = 0;
  float pressureMPa = pressurePSI * 0.00689476;
  int pressureDisplay = (int)(pressureMPa * 100.0 + 0.5);
  
  // Format: OFPP (OF + pression)
  // Exemples: OF16 = cycle fini + 0.16 MPa, OF04 = cycle fini + 0.04 MPa
  
  String display_str = "OF";
  if (pressureDisplay < 10) {
    display_str += "0";
  }
  display_str += String(pressureDisplay);
  
  tm.display(display_str.c_str());
}

// --- Fonction lecture eau ---
bool readWater() {
  int valeur = analogRead(PIN_WATER);
  return (valeur > WATER_THRESHOLD); // retourne vrai si eau présente
}
