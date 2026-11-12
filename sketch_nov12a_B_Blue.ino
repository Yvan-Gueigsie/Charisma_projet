#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

// ======================================================
// ESP32 DevKit V1 + DRV8833
// Suivi horizontal d'une personne
// Bluetooth Low Energy (BLE) compatible iPhone
// ======================================================

// ======================================================
// CONFIGURATION BLUETOOTH BLE
// ======================================================

const char* BT_NAME = "SUIVI_PERSONNE_ESP32";

// Service et caracteristique BLE
#define SERVICE_UUID \
  "6e400001-b5a3-f393-e0a9-e50e24dcca9e"

#define RX_UUID \
  "6e400002-b5a3-f393-e0a9-e50e24dcca9e"

#define TX_UUID \
  "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

BLEServer* pServer = nullptr;
BLECharacteristic* pTxCharacteristic = nullptr;

volatile bool bluetoothConnected = false;
volatile bool previousConnection = false;

// ======================================================
// BROCHES DRV8833
// ======================================================

const int AIN1 = 25;
const int AIN2 = 26;
const int nSLEEP = 27;

// ======================================================
// PARAMETRES DE COMMANDE
// ======================================================

const float DEAD_ZONE = 0.10f;

const int MIN_PWM = 209;
const int MAX_PWM = 255;

// ======================================================
// SECURITE COMMUNICATION
// ======================================================

const unsigned long COMMUNICATION_TIMEOUT = 500;

unsigned long lastReceptionTime = 0;

// Communication entre callback BLE et loop
portMUX_TYPE dataMux = portMUX_INITIALIZER_UNLOCKED;

float pendingPosition = 0.0f;
bool pendingPositionAvailable = false;
bool pendingInvalidData = false;

// ======================================================
// COMMANDE DU MOTEUR
// ======================================================

void moteurStop()
{
    analogWrite(AIN1, 0);
    analogWrite(AIN2, 0);

    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
}

void moteurGauche(int pwm)
{
    pwm = constrain(pwm, 0, 255);

    digitalWrite(AIN2, LOW);
    analogWrite(AIN1, pwm);
}

void moteurDroite(int pwm)
{
    pwm = constrain(pwm, 0, 255);

    digitalWrite(AIN1, LOW);
    analogWrite(AIN2, pwm);
}

// ======================================================
// TRAITEMENT DE LA POSITION HORIZONTALE
// ======================================================

void traiterPosition(float x)
{
    if (!isfinite(x) || x < -1.0f || x > 1.0f)
    {
        moteurStop();
        Serial.println("Position invalide !");
        return;
    }

    x = constrain(x, -1.0f, 1.0f);

    lastReceptionTime = millis();

    Serial.print("Position x = ");
    Serial.println(x, 3);

    // PERSONNE AU CENTRE
    if (fabsf(x) < DEAD_ZONE)
    {
        moteurStop();
        Serial.println("CENTRE -> STOP");
    }

    // PERSONNE A GAUCHE
    else if (x < 0)
    {
        int pwm = map(
            (int)(fabsf(x) * 1000),
            100,
            1000,
            MIN_PWM,
            MAX_PWM
        );

        pwm = constrain(pwm, MIN_PWM, MAX_PWM);

        moteurGauche(pwm);

        Serial.print("GAUCHE -> PWM = ");
        Serial.println(pwm);
    }

    // PERSONNE A DROITE
    else
    {
        int pwm = map(
            (int)(x * 1000),
            100,
            1000,
            MIN_PWM,
            MAX_PWM
        );

        pwm = constrain(pwm, MIN_PWM, MAX_PWM);

        moteurDroite(pwm);

        Serial.print("DROITE -> PWM = ");
        Serial.println(pwm);
    }
}

// ======================================================
// VALIDATION DES DONNEES RECUES
// ======================================================

bool convertirPosition(const String& data, float& x)
{
    if (data.length() == 0 || data.length() > 20)
        return false;

    const char* start = data.c_str();
    char* endPtr = nullptr;

    x = strtof(start, &endPtr);

    if (endPtr == start)
        return false;

    while (*endPtr && isspace((unsigned char)*endPtr))
        endPtr++;

    if (*endPtr != '\0')
        return false;

    if (!isfinite(x))
        return false;

    if (x < -1.0f || x > 1.0f)
        return false;

    return true;
}

// ======================================================
// CALLBACK CONNEXION BLE
// ======================================================

class ServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer* server) override
    {
        portENTER_CRITICAL(&dataMux);
        pendingPositionAvailable = false;
        pendingInvalidData = false;
        portEXIT_CRITICAL(&dataMux);

        bluetoothConnected = true;
    }

    void onDisconnect(BLEServer* server) override
    {
        bluetoothConnected = false;
    }
};

// ======================================================
// CALLBACK RECEPTION BLE
// ======================================================

class RXCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic* characteristic) override
    {
        if (!bluetoothConnected)
            return;

        String data = String(characteristic->getValue().c_str());

        data.trim();

        float x = 0.0f;

        bool valid = convertirPosition(data, x);

        portENTER_CRITICAL(&dataMux);

        if (valid)
        {
            pendingPosition = x;
            pendingPositionAvailable = true;
            pendingInvalidData = false;
        }
        else
        {
            pendingPositionAvailable = false;
            pendingInvalidData = true;
        }

        portEXIT_CRITICAL(&dataMux);
    }
};

// ======================================================
// RECEPTION BLUETOOTH BLE
// ======================================================

void recevoirBluetooth()
{
    float x = 0.0f;
    bool nouvellePosition = false;
    bool donneeInvalide = false;

    portENTER_CRITICAL(&dataMux);

    nouvellePosition = pendingPositionAvailable;
    donneeInvalide = pendingInvalidData;

    if (nouvellePosition)
        x = pendingPosition;

    pendingPositionAvailable = false;
    pendingInvalidData = false;

    portEXIT_CRITICAL(&dataMux);

    if (donneeInvalide)
    {
        moteurStop();
        Serial.println("Donnee Bluetooth invalide !");
    }
    else if (nouvellePosition)
    {
        traiterPosition(x);
    }
}

// ======================================================
// SURVEILLANCE BLUETOOTH
// ======================================================

void surveillerBluetooth()
{
    bool currentConnection = bluetoothConnected;

    if (currentConnection != previousConnection)
    {
        previousConnection = currentConnection;

        if (currentConnection)
        {
            lastReceptionTime = millis();
            Serial.println("iPhone connecte !");
        }
        else
        {
            Serial.println("iPhone deconnecte !");

            moteurStop();

            portENTER_CRITICAL(&dataMux);
            pendingPositionAvailable = false;
            pendingInvalidData = false;
            portEXIT_CRITICAL(&dataMux);

            // Relancer la publicite BLE
            BLEDevice::startAdvertising();

            Serial.println("En attente de connexion...");
        }
    }
}

// ======================================================
// INITIALISATION BLUETOOTH BLE
// ======================================================

void initialiserBluetooth()
{
    Serial.println();
    Serial.println("Demarrage Bluetooth BLE...");

    BLEDevice::init(BT_NAME);

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    BLEService* service =
        pServer->createService(SERVICE_UUID);

    // Reception des positions depuis l'iPhone
    BLECharacteristic* pRxCharacteristic =
        service->createCharacteristic(
            RX_UUID,
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_WRITE_NR
        );

    pRxCharacteristic->setCallbacks(new RXCallbacks());

    // Caracteristique de notification disponible
    pTxCharacteristic =
        service->createCharacteristic(
            TX_UUID,
            BLECharacteristic::PROPERTY_NOTIFY
        );

    pTxCharacteristic->addDescriptor(new BLE2902());

    service->start();

    BLEAdvertising* advertising =
        BLEDevice::getAdvertising();

    advertising->addServiceUUID(SERVICE_UUID);
    advertising->setScanResponse(true);

    BLEDevice::startAdvertising();

    Serial.println("Bluetooth BLE actif !");
    Serial.println("En attente de connexion...");
}

// ======================================================
// SETUP
// ======================================================

void setup()
{
    Serial.begin(115200);

    delay(500);

    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
    pinMode(nSLEEP, OUTPUT);

    digitalWrite(nSLEEP, HIGH);

    moteurStop();

    initialiserBluetooth();

    Serial.println();
    Serial.println("====================================");
    Serial.println("          ESP32 TRACKING");
    Serial.println("       BLUETOOTH LOW ENERGY");
    Serial.println("====================================");

    Serial.print("Nom Bluetooth : ");
    Serial.println(BT_NAME);

    Serial.println("Format attendu : -1.0 a +1.0");
    Serial.println("Exemple : -0.50");
    Serial.println("Timeout : 500 ms");
    Serial.println("====================================");
}

// ======================================================
// LOOP
// ======================================================

void loop()
{
    surveillerBluetooth();

    if (bluetoothConnected)
    {
        recevoirBluetooth();

        if (millis() - lastReceptionTime >
            COMMUNICATION_TIMEOUT)
        {
            moteurStop();
        }
    }
    else
    {
        moteurStop();
    }

    delay(5);
}