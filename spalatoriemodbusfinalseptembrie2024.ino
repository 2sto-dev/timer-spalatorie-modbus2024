#include <ModbusMaster.h>
#include <Bounce2.h>
#include <SoftwareSerial.h>

// Configurare SoftwareSerial și ModbusMaster
SoftwareSerial mySerial(9, 10); // RX, TX pentru SoftwareSerial
ModbusMaster node;

const int Ban10Pin = A2;
const int Ban5Pin = A1;
const int RelayPin = 11;
const int LampPin = 12;
const int SpumaPin = A3;
const int StopPin = A4;
const int ClatirePin = A5;

unsigned long countBan5 = 0;
unsigned long countBan10 = 0;
unsigned long totalDuration = 0;
unsigned long remainingTime = 0;
unsigned long pausedTime = 0;
bool spumaActivated = false;
bool clatireActivated = false;
bool stopPressed = false;
bool colonVisible = true;
bool waitingForMoney = false; // Variabilă pentru a verifica dacă așteptăm bani

const unsigned long printInterval = 3000; // Intervalul pentru afișarea în consolă (3 secunde)
const unsigned long updateInterval = 1000; // Intervalul de actualizare a timpului (1 secundă)
unsigned long lastPrintTime = 0;
unsigned long lastTimeUpdate = 0;

Bounce debouncerBan5 = Bounce();
Bounce debouncerBan10 = Bounce();
Bounce debouncerSpuma = Bounce();
Bounce debouncerStop = Bounce();
Bounce debouncerClatire = Bounce();

void setup() {
  pinMode(Ban10Pin, INPUT_PULLUP);
  pinMode(Ban5Pin, INPUT_PULLUP);
  pinMode(RelayPin, OUTPUT);
  pinMode(LampPin, OUTPUT);
  pinMode(SpumaPin, INPUT_PULLUP);
  pinMode(StopPin, INPUT_PULLUP);
  pinMode(ClatirePin, INPUT_PULLUP);

  debouncerBan5.attach(Ban5Pin, INPUT_PULLUP);
  debouncerBan10.attach(Ban10Pin, INPUT_PULLUP);
  debouncerSpuma.attach(SpumaPin, INPUT_PULLUP);
  debouncerStop.attach(StopPin, INPUT_PULLUP);
  debouncerClatire.attach(ClatirePin, INPUT_PULLUP);

  debouncerBan5.interval(50);
  debouncerBan10.interval(50);
  debouncerSpuma.interval(50);
  debouncerStop.interval(50);
  debouncerClatire.interval(50);

  Serial.begin(9600);
  mySerial.begin(9600);
  node.begin(1, mySerial);

  Serial.println("Setup complet. Timp initial: 0 secunde");
}

void loop() {
  unsigned long currentMillis = millis();

  // Actualizare debouncere
  debouncerBan5.update();
  debouncerBan10.update();
  debouncerSpuma.update();
  debouncerStop.update();
  debouncerClatire.update();

  // Verificare butoane și actualizare
  banCounter();
  startProgramSpuma();
  startProgramClatire();
  stopProgram();

  // Actualizare timp și display
  if (currentMillis - lastTimeUpdate >= updateInterval) {
    if ((spumaActivated || clatireActivated) && remainingTime > 0) {
      remainingTime -= updateInterval;
      Serial.print("Remaining time: ");
      printTime(remainingTime);

      updateDisplay(remainingTime);
      displayColon();
    }
    lastTimeUpdate = currentMillis;
  }

  // Oprire lampa și dezactivare programe
  if (remainingTime == 0 && (spumaActivated || clatireActivated)) {
    digitalWrite(RelayPin, LOW);
    digitalWrite(LampPin, LOW);

    // Resetează contorii și variabilele
    if (spumaActivated || clatireActivated) {
      spumaActivated = false;
      clatireActivated = false;
      stopPressed = true; // Indică că timpul a expirat
      waitingForMoney = true; // Așteptăm bani pentru a reporni

      Serial.println("Timpul a expirat. Așteptăm bani pentru a reporni.");
      updateDisplay(remainingTime); // Afișează 00:00 pe display
      displayColon();
    }
  }
}

void banCounter() {
  if (debouncerBan5.fell()) {
    countBan5++;
    totalDuration += 240000; // 4 minute
    // Dacă programul este activ sau a fost oprit, adaugă timpul la remainingTime
    if (spumaActivated || clatireActivated || stopPressed) {
      remainingTime += 240000; // 4 minute
    } else {
      remainingTime = totalDuration; // Dacă programul nu este activ, sincronizează cu totalDuration
    }
    Serial.print("Ban5 apasat, total Ban5 count: ");
    Serial.println(countBan5);

    updateDisplay(remainingTime); // Actualizează display-ul cu timpul rămas
    displayColon();
  }

  if (debouncerBan10.fell()) {
    countBan10++;
    totalDuration += 480000; // 8 minute
    // Dacă programul este activ sau a fost oprit, adaugă timpul la remainingTime
    if (spumaActivated || clatireActivated || stopPressed) {
      remainingTime += 480000; // 8 minute
    } else {
      remainingTime = totalDuration; // Dacă programul nu este activ, sincronizează cu totalDuration
    }
    Serial.print("Ban10 apasat, total Ban10 count: ");
    Serial.println(countBan10);

    updateDisplay(remainingTime); // Actualizează display-ul cu timpul rămas
    displayColon();
  }
}

void startProgramSpuma() {
  // Protecție: permite activarea doar dacă nici Spuma, nici Clatire nu sunt active
  if (debouncerSpuma.fell() && !spumaActivated && !clatireActivated) {
    spumaActivated = true;
    remainingTime = (pausedTime > 0) ? pausedTime : totalDuration;
    pausedTime = 0;

    digitalWrite(RelayPin, HIGH);
    digitalWrite(LampPin, HIGH);

    Serial.print("Remaining time: ");
    printTime(remainingTime);

    updateDisplay(remainingTime);
    displayColon();
  }
}

void startProgramClatire() {
  // Protecție: permite activarea doar dacă nici Clatire, nici Spuma nu sunt active
  if (debouncerClatire.fell() && !clatireActivated && !spumaActivated) {
    clatireActivated = true;
    remainingTime = (pausedTime > 0) ? pausedTime : totalDuration;
    pausedTime = 0;

    digitalWrite(RelayPin, HIGH);
    digitalWrite(LampPin, HIGH);

    Serial.print("Remaining time: ");
    printTime(remainingTime);

    updateDisplay(remainingTime);
    displayColon();
  }
}

void stopProgram() {
  if (debouncerStop.fell()) {
    Serial.println("Program oprit.");
    digitalWrite(RelayPin, LOW);
    pausedTime = remainingTime;
    spumaActivated = false;
    clatireActivated = false;
    stopPressed = true;

    Serial.print("Timpul curent salvat: ");
    printTime(remainingTime);

    updateDisplay(remainingTime);
    displayColon();
  }
}

void printTime(unsigned long millisecs) {
  int minutes = millisecs / 60000;
  int seconds = (millisecs % 60000) / 1000;
  Serial.print((minutes < 10) ? "0" : "");
  Serial.print(minutes);
  Serial.print(":");
  Serial.print((seconds < 10) ? "0" : "");
  Serial.print(seconds);
  Serial.println("");
}

void updateDisplay(unsigned long timeMillis) {
  uint16_t displayValue = (timeMillis / 60000) * 100 + (timeMillis % 60000) / 1000;
  uint8_t result = node.writeSingleRegister(0x0000, displayValue);
  if (result != node.ku8MBSuccess) {
    Serial.print("Failed to update display. Error code: ");
    Serial.println(result);
  }
}

void displayColon() {
  uint8_t colonValue = colonVisible ? 0x02 : 0x00;
  uint8_t result = node.writeSingleRegister(0x0001, colonValue);
  if (result != node.ku8MBSuccess) {
    Serial.print("Failed to update colon visibility on Modbus. Error code: ");
    Serial.println(result);
  }
}



