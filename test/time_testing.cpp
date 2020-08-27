#include <Arduino.h>
#include <time.h>

//Test

#define trigLev 2
#define echoLev 4
#define sole 22

float kedalmanAir;
float jumlah;
float a;

boolean modeSet = false;
volatile int interruptCounter;
int totalInterruptCounter;

hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer()
{
    //testing
    portENTER_CRITICAL_ISR(&timerMux);
    interruptCounter++;
    portEXIT_CRITICAL_ISR(&timerMux);
}

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
    float tempKedalaman[10];
    for (int i = 0; i < 10; i++)
    {
        tempKedalaman[i] = 0;
    }

    for (int i = 0; i < 10; i++)
    {
        tempKedalaman[i] = getLevelAir();
        jumlah = jumlah + tempKedalaman[i];
        Serial.print("Mengambil nilai jarak dengan nilai: ");
        Serial.println(tempKedalaman[i]);
        delay(10);
    }
    return jumlah / 10;
}

void setup()
{
    Serial.begin(112500);
    pinMode(sole, OUTPUT);
    pinMode(trigLev, OUTPUT);
    pinMode(echoLev, INPUT);

    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, 100000, true);
    timerAlarmEnable(timer);
}

void loop()
{
    delay(100);
    a = getLevelAir();

    if (interruptCounter > 0)
    {
        portENTER_CRITICAL(&timerMux);
        interruptCounter--;
        portEXIT_CRITICAL(&timerMux);
        
        totalInterruptCounter++;

        Serial.print("An interrupt as occurred. Total number: ");
        Serial.println(totalInterruptCounter);
        if (a > 7)
        {
            Serial.print("Menghidupkan solenoid, ketinggian air: ");
            Serial.println(a);
            modeSet = true;
        }
        else if (a <= 5)
        {
            Serial.print("Nilai sudah sampai: ");
            Serial.println(a);
            modeSet = false;
        }
        else
        {
            Serial.println("Sudah pas");
            modeSet = false;
        }
    }
    
    if(modeSet == true)
        digitalWrite(sole, HIGH);

    else
        digitalWrite(sole, LOW);
}