#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// Configuration des pins
#define SOIL_MOISTURE_PIN A0
#define PH_SENSOR_PIN A1
#define WATER_LEVEL_PIN 2
#define MAIN_PUMP_RELAY 3
#define SECONDARY_PUMP_RELAY 4
#define ALERT_PIN 5
#define RAIN_SENSOR_PIN 6
#define FLOW_SENSOR_PIN 7

// Seuils
const int SOIL_DRY_THRESHOLD = 400;
const float PH_MIN = 5.5;
const float PH_MAX = 6.5;

// Variables globales
LiquidCrystal_I2C lcd(0x27, 16, 2);
volatile int flowPulseCount = 0;
float flowRate = 0;
float totalWaterUsed = 0;

void setup() {
  Serial.begin(9600);
  
  // Initialisation LCD
  lcd.init();
  lcd.backlight();
  afficherLogo();
  
  // Configuration pins
  pinMode(MAIN_PUMP_RELAY, OUTPUT);
  pinMode(SECONDARY_PUMP_RELAY, OUTPUT);
  pinMode(ALERT_PIN, OUTPUT);
  pinMode(WATER_LEVEL_PIN, INPUT);
  pinMode(RAIN_SENSOR_PIN, INPUT);
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  
  // Interruption pour flow sensor
  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), 
                  flowPulse, RISING);
  
  // État initial
  digitalWrite(MAIN_PUMP_RELAY, HIGH);
  digitalWrite(SECONDARY_PUMP_RELAY, HIGH);
  
  Serial.println("🌾 SIIAR Démarré");
}

void loop() {
  // Lecture des capteurs
  int soilMoisture = analogRead(SOIL_MOISTURE_PIN);
  float phValue = lirePH();
  bool waterLevelOK = digitalRead(WATER_LEVEL_PIN);
  bool rainDetected = digitalRead(RAIN_SENSOR_PIN);
  
  // Calcul débit
  calculerDebit();
  
  // Logique d'irrigation
  if (!rainDetected) {
    if (soilMoisture > SOIL_DRY_THRESHOLD) {
      // Sol sec
      if (waterLevelOK) {
        // Réservoir OK - Démarrer irrigation
        irriguer();
      } else {
        // Réservoir vide - Remplir d'abord
        remplirReservoir();
      }
    } else {
      // Sol humide - Arrêter irrigation
      arreterIrrigation();
    }
  } else {
    // Pluie détectée - Arrêter irrigation
    arreterIrrigation();
    lcd.setCursor(0, 1);
    lcd.print("Pluie detectee ");
  }
  
  // Vérification pH
  if (phValue < PH_MIN || phValue > PH_MAX) {
    alertePH(phValue);
  }
  
  // Affichage LCD
  afficherInfos(soilMoisture, phValue, waterLevelOK);
  
  // Envoi données série
  envoyerDonneesSerie(soilMoisture, phValue, waterLevelOK);
  
  delay(1000);
}

void afficherLogo() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  🌾 SIIAR  ");
  lcd.setCursor(0, 1);
  lcd.print(" RanoaVotra v1");
  delay(3000);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(" Irrigation");
  lcd.setCursor(0, 1);
  lcd.print(" Intelligente");
  delay(2000);
}

float lirePH() {
  int rawValue = analogRead(PH_SENSOR_PIN);
  // Conversion analogique vers pH (calibration)
  float voltage = rawValue * (5.0 / 1023.0);
  float phValue = 7.0 + ((2.5 - voltage) / 0.18);
  return phValue;
}

void flowPulse() {
  flowPulseCount++;
}

void calculerDebit() {
  static unsigned long lastTime = 0;
  unsigned long currentTime = millis();
  
  if (currentTime - lastTime >= 1000) {
    flowRate = (flowPulseCount / 7.5); // Conversion en L/min
    totalWaterUsed += flowRate / 60; // Total en Litres
    flowPulseCount = 0;
    lastTime = currentTime;
  }
}

void irriguer() {
  digitalWrite(MAIN_PUMP_RELAY, LOW); // Activation pompe
  digitalWrite(ALERT_PIN, LOW);
  lcd.setCursor(0, 1);
  lcd.print("Irrigation ON  ");
}

void arreterIrrigation() {
  digitalWrite(MAIN_PUMP_RELAY, HIGH);
  lcd.setCursor(0, 1);
  lcd.print("Irrigation OFF ");
}

void remplirReservoir() {
  digitalWrite(SECONDARY_PUMP_RELAY, LOW); // Pompe secondaire ON
  digitalWrite(ALERT_PIN, HIGH); // Alerte
  lcd.setCursor(0, 1);
  lcd.print("Remplissage... ");
  
  // Simulation de remplissage (dans la réalité, attendre capteur niveau)
  delay(5000);
  digitalWrite(SECONDARY_PUMP_RELAY, HIGH); // Pompe OFF
  digitalWrite(ALERT_PIN, LOW);
}

void alertePH(float ph) {
  digitalWrite(ALERT_PIN, HIGH);
  lcd.setCursor(0, 0);
  lcd.print("pH Anormal: ");
  lcd.print(ph, 1);
  delay(2000);
}

void afficherInfos(int soil, float ph, bool waterOK) {
  lcd.clear();
  
  // Ligne 1: Humidité et pH
  lcd.setCursor(0, 0);
  lcd.print("H:");
  lcd.print(soil);
  lcd.print(" pH:");
  lcd.print(ph, 1);
  
  // Ligne 2: Niveau et débit
  lcd.setCursor(0, 1);
  lcd.print("Niv:");
  lcd.print(waterOK ? "OK " : "BAS");
  lcd.print(" D:");
  lcd.print(flowRate, 1);
  lcd.print("L/m");
}

void envoyerDonneesSerie(int soil, float ph, bool waterOK) {
  Serial.print("🌾 Données SIIAR - ");
  Serial.print("Humidité: ");
  Serial.print(soil);
  Serial.print(" | pH: ");
  Serial.print(ph, 1);
  Serial.print(" | Niveau: ");
  Serial.print(waterOK ? "OK" : "BAS");
  Serial.print(" | Débit: ");
  Serial.print(flowRate, 1);
  Serial.println(" L/min");
}