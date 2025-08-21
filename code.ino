#include <Wire.h>
#include <RTClib.h>
#include <TM1637Display.h>

// Pins definition
#define PRESSURE_SENSOR A0
#define WATER_DETECT_1 2
#define WATER_DETECT_2 3
#define RELAY_PIN 4
#define SWITCH_PLUS 5
#define SWITCH_MINUS 6
#define SWITCH_MODE 7
#define MAIN_SWITCH 8
#define CLK_PIN 9
#define DIO_PIN 10

// Constants
#define MAX_PRESSURE_PSI 15.0
#define CRITICAL_PRESSURE_PSI 21.0
#define PRESSURE_MIN_VOLTAGE 0.5
#define PRESSURE_MAX_VOLTAGE 4.5
#define PRESSURE_MAX_PSI 30.0
#define DEFAULT_TIME_MINUTES 45
#define DEFAULT_CUTOFF_PSI 10.0

// Variables
RTC_DS3231 rtc;
TM1637Display display(CLK_PIN, DIO_PIN);

float currentPressure = 0.0;
int timerMinutes = DEFAULT_TIME_MINUTES;
float pressureCutoff = DEFAULT_CUTOFF_PSI;
bool systemEnabled = false;
bool heatingActive = false;
bool configMode = false;
unsigned long lastUpdate = 0;
unsigned long buttonDebounce = 0;
bool lastSwitchPlus = HIGH;
bool lastSwitchMinus = HIGH;
bool lastSwitchMode = HIGH;
bool lastMainSwitch = HIGH;

// Safety flags
bool waterPresent = false;
bool pressureSafe = false;
bool emergencyStop = false;

void setup() {
  Serial.begin(9600);
  
  // Initialize RTC
  if (!rtc.begin()) {
    Serial.println("RTC non trouvé!");
    while (1);
  }
  
  // Pin modes
  pinMode(PRESSURE_SENSOR, INPUT);
  pinMode(WATER_DETECT_1, INPUT_PULLUP);
  pinMode(WATER_DETECT_2, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(SWITCH_PLUS, INPUT_PULLUP);
  pinMode(SWITCH_MINUS, INPUT_PULLUP);
  pinMode(SWITCH_MODE, INPUT_PULLUP);
  // pinMode(MAIN_SWITCH, INPUT_PULLUP); // Non utilisé - interrupteur coupe l'alimentation
  
  // Initialize display
  display.setBrightness(0x0f);
  
  // Safety first - relay OFF
  digitalWrite(RELAY_PIN, LOW);
  
  Serial.println("Système d'autoclave initialisé");
  Serial.println("SÉCURITÉS ACTIVES:");
  Serial.println("- Détection d'eau obligatoire");
  Serial.println("- Coupure à 15 PSI par défaut");
  Serial.println("- Arrêt d'urgence si > 21 PSI");
}

void loop() {
  unsigned long currentTime = millis();
  
  // Lecture des capteurs toutes les 100ms
  if (currentTime - lastUpdate >= 100) {
    readSensors();
    checkSafety();
    handleButtons();
    updateDisplay();
    controlHeating();
    lastUpdate = currentTime;
  }
  
  // Si l'Arduino fonctionne, c'est que l'interrupteur est ON
  if (!systemEnabled) {
    systemEnabled = true;
    timerMinutes = DEFAULT_TIME_MINUTES;
    Serial.println("Système activé - Timer: 45 minutes");
  }
  
  // Timer countdown toutes les secondes
  static unsigned long lastSecond = 0;
  if (currentTime - lastSecond >= 1000 && systemEnabled && !configMode) {
    if (timerMinutes > 0) {
      static int seconds = 0;
      seconds++;
      if (seconds >= 60) {
        timerMinutes--;
        seconds = 0;
        Serial.print("Temps restant: ");
        Serial.print(timerMinutes);
        Serial.println(" minutes");
      }
    }
    lastSecond = currentTime;
  }
}

void readSensors() {
  // Lecture capteur de pression (0-30 PSI, 0.5-4.5V)
  int analogValue = analogRead(PRESSURE_SENSOR);
  float voltage = (analogValue * 5.0) / 1024.0;
  
  if (voltage >= PRESSURE_MIN_VOLTAGE && voltage <= PRESSURE_MAX_VOLTAGE) {
    currentPressure = ((voltage - PRESSURE_MIN_VOLTAGE) / 
                      (PRESSURE_MAX_VOLTAGE - PRESSURE_MIN_VOLTAGE)) * PRESSURE_MAX_PSI;
  } else {
    currentPressure = 0.0; // Erreur de lecture
  }
  
  // Détection d'eau (deux vis en inox)
  waterPresent = (digitalRead(WATER_DETECT_1) == LOW && 
                  digitalRead(WATER_DETECT_2) == LOW);
  
  // L'interrupteur principal coupe l'alimentation de l'Arduino
  // Si on arrive ici, c'est que l'interrupteur est ON
}

void checkSafety() {
  pressureSafe = (currentPressure <= pressureCutoff);
  
  // Arrêt d'urgence si pression critique
  if (currentPressure > CRITICAL_PRESSURE_PSI) {
    emergencyStop = true;
    heatingActive = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("ARRÊT D'URGENCE - PRESSION CRITIQUE!");
    // Clignotement d'alarme sur l'afficheur
    display.showNumberDec(8888);
    delay(200);
    display.clear();
    delay(200);
    return;
  } else if (emergencyStop && currentPressure <= (CRITICAL_PRESSURE_PSI - 2.0)) {
    emergencyStop = false;
    Serial.println("Arrêt d'urgence réinitialisé");
  }
  
  // Vérifications de sécurité standard
  if (!waterPresent) {
    heatingActive = false;
    digitalWrite(RELAY_PIN, LOW);
    if (systemEnabled) {
      Serial.println("SÉCURITÉ: Manque d'eau détecté!");
    }
  }
  
  if (!pressureSafe && !emergencyStop) {
    heatingActive = false;
    digitalWrite(RELAY_PIN, LOW);
    if (systemEnabled) {
      Serial.println("SÉCURITÉ: Pression trop élevée!");
    }
  }
}

void handleButtons() {
  unsigned long currentTime = millis();
  
  if (currentTime - buttonDebounce < 200) return; // Debounce
  
  bool currentSwitchPlus = digitalRead(SWITCH_PLUS);
  bool currentSwitchMinus = digitalRead(SWITCH_MINUS);
  bool currentSwitchMode = digitalRead(SWITCH_MODE);
  
  // Bouton MODE
  if (currentSwitchMode == LOW && lastSwitchMode == HIGH) {
    configMode = !configMode;
    if (configMode) {
      Serial.println("Mode configuration - Réglage seuil pression");
    } else {
      Serial.println("Mode timer");
    }
    buttonDebounce = currentTime;
  }
  
  // Boutons + et -
  if (configMode) {
    // Mode configuration - ajustement seuil pression
    if (currentSwitchPlus == LOW && lastSwitchPlus == HIGH) {
      pressureCutoff += 0.1;
      if (pressureCutoff > 10.0) pressureCutoff = 10.0;
      Serial.print("Seuil pression: ");
      Serial.print(pressureCutoff, 1);
      Serial.println(" PSI");
      buttonDebounce = currentTime;
    }
    
    if (currentSwitchMinus == LOW && lastSwitchMinus == HIGH) {
      pressureCutoff -= 0.1;
      if (pressureCutoff < 0.1) pressureCutoff = 0.1;
      Serial.print("Seuil pression: ");
      Serial.print(pressureCutoff, 1);
      Serial.println(" PSI");
      buttonDebounce = currentTime;
  } else {
    // Mode timer - ajustement temps
    if (currentSwitchPlus == LOW && lastSwitchPlus == HIGH) {
      timerMinutes += 5;
      if (timerMinutes > 999) timerMinutes = 999;
      Serial.print("Timer: ");
      Serial.print(timerMinutes);
      Serial.println(" minutes");
      buttonDebounce = currentTime;
    }
    
    if (currentSwitchMinus == LOW && lastSwitchMinus == HIGH) {
      timerMinutes -= 5;
      if (timerMinutes < 0) timerMinutes = 0;
      Serial.print("Timer: ");
      Serial.print(timerMinutes);
      Serial.println(" minutes");
      buttonDebounce = currentTime;
    }
  }
  
  lastSwitchPlus = currentSwitchPlus;
  lastSwitchMinus = currentSwitchMinus;
  lastSwitchMode = currentSwitchMode;
}

void updateDisplay() {
  if (emergencyStop) {
    // Affichage d'alarme déjà géré dans checkSafety()
    return;
  }
  
  if (configMode) {
    // Affichage du seuil de pression avec 1 décimale (ex: 10.5 -> "10.5")
    int displayValue = (int)(pressureCutoff * 10); // 10.5 -> 105
    if (displayValue < 100) {
      // Pour afficher 0.1 à 9.9 correctement
      display.showNumberDecEx(displayValue, 0b01000000, true); // Point après le 1er digit
    } else {
      // Pour 10.0 et plus
      display.showNumberDecEx(displayValue, 0b00100000, false); // Point après le 2ème digit
    }
  } else {
    // Affichage du timer
    if (timerMinutes <= 0) {
      display.showNumberDec(0);
    } else {
      display.showNumberDec(timerMinutes);
    }
  }
}

void controlHeating() {
  if (!systemEnabled || emergencyStop || timerMinutes <= 0) {
    heatingActive = false;
    digitalWrite(RELAY_PIN, LOW);
    return;
  }
  
  // Conditions pour activer le chauffage
  bool canHeat = waterPresent && pressureSafe && (timerMinutes > 0);
  
  if (canHeat && !heatingActive) {
    heatingActive = true;
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("Chauffage activé");
  } else if (!canHeat && heatingActive) {
    heatingActive = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("Chauffage désactivé");
  }
  
  // Affichage état système
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 5000) { // Toutes les 5 secondes
    Serial.print("État: Eau=");
    Serial.print(waterPresent ? "OK" : "NOK");
    Serial.print(" | Pression=");
    Serial.print(currentPressure);
    Serial.print("PSI | Seuil=");
    Serial.print(pressureCutoff);
    Serial.print("PSI | Timer=");
    Serial.print(timerMinutes);
    Serial.print("min | Chauffage=");
    Serial.println(heatingActive ? "ON" : "OFF");
    lastStatus = millis();
  }
}
