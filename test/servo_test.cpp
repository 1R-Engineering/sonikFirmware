#include <Arduino.h>
#include <ESP32Servo.h>

int servo_pin = 3;
int pos = 0;

Servo probe;

void setup()
{
    probe.attach(14);
}

void loop()
{
    for (int i = 0; i < 180; i++)
    {
        probe.write(i);
        delay(15);
    }
    

    for (int i = 180; i > 0; i--)
    {
        probe.write(i);
        delay(15);
    }
    
}