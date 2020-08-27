#include <Arduino.h>
#include <time.h>

#define trigLev 2
#define echoLev 4
#define sole    22

float kedalmanAir;
float jumlah;
float a;

float getLevelAir()
{
    float kedalaman;
    long durasiPantul;
    digitalWrite(trigLev, LOW);
    delayMicroseconds(2);
    digitalWrite(trigLev, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigLev, LOW);
    durasiPantul = (pulseIn(echoLev, HIGH));
    kedalaman = durasiPantul * 0.034 / 2;
    return kedalaman;
}

float rataAir()
{
    float tempKedalaman[30];
    for (int i = 0; i < 30; i++)
    {
        tempKedalaman[i] = 0;
    }

    for (int i = 0; i < 30; i++)
    {
        tempKedalaman[i] = getLevelAir();
        jumlah = jumlah + tempKedalaman[i];
        Serial.print("Mengambil nilai jarak dengan nilai: ");
        Serial.println(tempKedalaman[i]);
        delay(500);
    }
    return jumlah / 30;
}

void setup()
{
    Serial.begin(112500);
    pinMode(sole, OUTPUT);
    pinMode(trigLev, OUTPUT);
    pinMode(echoLev, INPUT);
}

void loop()
{
    delay(5000);

    if (Serial.available())
    {
        if (Serial.parseInt() == 1)
        {
            Serial.println("ON");
            a = rataAir();
            if (a > 7){
                Serial.println("Menghidupkan solenoid");
                digitalWrite(sole, HIGH);
                delay(30000);
                digitalWrite(sole, LOW);
                delay(2000);
                a = rataAir();
            }
            else
            {
                Serial.print("Nilai sudah sampai: ");
                Serial.println(rataAir());
            }
        }
    }
}