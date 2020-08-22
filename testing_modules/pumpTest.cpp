/*
    Pump test with serial controll
    Mode:
        => (1) Change the k value
        => (2) Pump mililiters
        => (3) Change the duty cylce
*/
#include <Arduino.h>
#include <esp32-hal-ledc.h>

#define pinPompa 33

int mL;
int konstanta, duti;
int setMode = 0;

void crot_Pompa(int pin, int miliLiter, int k);

void setup() {
  ledcSetup(0, 1000, 8);
  ledcAttachPin(33, 0);
  Serial.begin(115200);
  pinMode(pinPompa, OUTPUT);
  Serial.println("Ready\n");

}

void loop() {
  if (Serial.available()) {
    Serial.println("Enter Mode");
    setMode = Serial.parseInt();
    if (setMode == 1) {
      Serial.println("Setting konstanta :");
      Serial.println("Waiting input");
      konstanta = Serial.parseInt();
      Serial.print("Konstanta set to: ");
      Serial.print(konstanta);
      Serial.println("");

    }
    if (setMode == 2) {
      Serial.println("Masukan ML");
      mL = Serial.parseInt();
      Serial.print("Mengecrot dengan: ");
      Serial.print(mL);
      Serial.println(" mL");
      crot_Pompa(pinPompa, mL, konstanta);
      Serial.println("Selesai");

    }
    if (setMode == 3) {
      Serial.println("Masukan Duti");
      duti = Serial.parseInt();
      Serial.print("Duit set to: ");
      Serial.print(duti);
      Serial.println("");
    }
  }
  setMode = 0;
  ledcWrite(0, 0);
}

void crot_Pompa(int pin, int miliLiter, int k) {
  int waktuPompa;
  waktuPompa = k * miliLiter;
  ledcWrite(0, duti);
  delay(waktuPompa);
  ledcWrite(0, 0);
  delay(waktuPompa);
}
