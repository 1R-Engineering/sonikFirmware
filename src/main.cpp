/*                                                         
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                 ,--.                  ,--.
  .--.--.                      ,--.'|              ,--/  /|
 /  /    '.                ,--,:  : |   ,--,    ,---,': / '
|  :  /`. /     ,---.   ,`--.'`|  ' : ,--.'|    :   : '/ / 
;  |  |--`     '   ,'\  |   :  :  | | |  |,     |   '   ,  
|  :  ;_      /   /   | :   |   \ | : `--'_     '   |  /   
 \  \    `.  .   ; ,. : |   : '  '; | ,' ,'|    |   ;  ;   
  `----.   \ '   | |: : '   ' ;.    ; '  | |    :   '   \  
  __ \  \  | '   | .; : |   | | \   | |  | :    |   |    ' 
 /  /`--'  / |   :    | '   : |  ; .' '  : |__  '   : |.  \
'--'.     /   \   \  /  |   | '`--'   |  | '.'| |   | '_\.'
  `--'---'     `----'   '   : |       ;  :    ; '   : |    
                        ;   |.'       |  ,   /  ;   |,'    
                        '---'          ---`-'   '---'      
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  +++++++++++++++++++LOS DOL ENGINEERING+++++++++++++++++++
  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  Smart hidropONIK Firmware (build06082020)                
    > Sebuah inovasi dalam berhidroponik

  Board:  >ESP32 DEV-KITC   (Untested)
          >WEMOS Lolin32 v2 (Tested)

  TO DO:
    []  Wifi provisioning via Bluetooth serial
            Gunakan bluetooth serial untuk mengatur wifi yang ada 
            cukup dimasukan ssid dan password melalui android
            kemudian keluaran dari android berupa json string berisi
            ssid dan password
    []  Cari tahu kedalaman air (Perhitungannya)
            Cari tahu cara ngukur kedalaman air. Data bak diambil
            dari cloud. Kemudian nanti diitung disini. Fungsinya nanti
            di return presentase level air (int)
*/

//Inisialisasi library/file yang penting
#include <Arduino.h>
#include <esp32-hal-ledc.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <ArduinoJson.h>
#include "WiFi.h"
#include "esp32-mqtt.h"
#include "BluetoothSerial.h"
#include <ESP32Servo.h>

//Inisialisasi pin gpio mikrokontroller
#define TX_0 GPIO_NUM_1  // Pin untuk komunikasi antar board
#define RX_0 GPIO_NUM_3  // Pin untuk komunikasi antar board
#define TX_2 GPIO_NUM_17 // Pin untuk komunikasi antar board
#define RX_2 GPIO_NUM_16 // Pin untuk komunikasi antar board
#define pompamixA 26     // Pin pompa larutan A
#define pompamixB 27     // Pin pompa larutan B
#define pompaAcid 25     // Pin pompa larutan Asam
#define pompaBase 33     // Pin pompa larutan Basa
#define solenoid 22      // Pin solenoid
#define pumpAC 21        // Pin pompa AC (tapi rasido kan
#define pin_ds_temp 13   // Pin sensor temperatur air dengan DS
#define pin_sensor_pH 12 // Pin sensor sensor pH
#define TDSSensorPin 14  // Pin sensor TDS
#define echoLev 4        // Pin echo Ultrasonik
#define trigLev 2        // Pin trigger Ultrasonik
#define pin_servo 32
#define TdsSensorPin 14
#define VREF 3.3
#define SCOUNT 30

float nilai_test, set_point, ph_setpoint_atas,
    ph_setpoint_bawah, nilai_ph;

//Nama device
char namaDevice[] = "Sonik32";

//Inisialisasi klas untuk komunikasi Serial
HardwareSerial Sender(1);
HardwareSerial Receiver(2);

//Deklarasi variabel
#define pubDelay 10000 // Waktu interval pengiriman data telemetri ke cloud (15 menit)

//Deklarasi dari tool penting (lib)
OneWire oneWire(pin_ds_temp);
DallasTemperature sensors(&oneWire);
Servo servoProbe;
int pos = 0;

unsigned long lastMillis = 0;    //Penganti delay
unsigned long lastMillisTDS = 0; //Penganti delay

boolean statusStabilisasipH = true;
boolean statusStabilisasiTDS = true;

String data; //Variabel yang menyimpan data yang akan dikirim

bool bluetoothConStatus = false; //Status koneksi Bluetooth, apabila sudah terkoneksi maka ini akan true

//Variabel penyimpan string ssid dan password wifi
String ssidPrim, passPrim; //Menyimpan variabel ssid dari wifi
char server_jam[3], server_menit[3], server_detik[3];
char jam[] = "09";
char menit[] = "00";
char cmd[] = "test";
/*
    Inisialisasi bluetooth yang nantinya akan digunakan
    untuk mengambil pengaturan wifi dari android (hp user)
*/
BluetoothSerial SerialBT;

int analogBuffer[SCOUNT]; // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;
float nilai_TDS = 0;

int getMedianNum(int bArray[], int iFilterLen);
float ambil_nilai_pH(int pin_pH, int banyak_sample_pH);
float ambil_nilai_TDS();
float getLevelAir();
void pompa(int pinPompa, int miliLiter);
void hidupkanSolenoid(int pin_ledeng, int durasi);
void kontrol_servo(int modeServo);

void messageReceived(String &topic, String &payload)
{
    Serial.println("incoming: " + topic + " - " + payload);
    // char payloadBuffer(payload.length());
    // payload.toCharArray(payloadBuffer, payload.length());
}

void setup()
{
    //Setting up komunikasi 2 esp dengan protokol serial
    Sender.begin(115200, SERIAL_8N1, RX_0, TX_0);
    Receiver.begin(115200, SERIAL_8N1, RX_2, TX_2);

    pinMode(TDSSensorPin, INPUT);

    //Setup pwm pompa
    ledcSetup(0, 3000, 8);
    ledcAttachPin(pompamixA, 0);
    ledcAttachPin(pompamixB, 0);
    ledcAttachPin(pompaAcid, 0);
    ledcAttachPin(pompaBase, 0);

    //Setting up komunikasi serial dengan usb (untuk debug)
    Serial.begin(115200);
    Serial.println("SONIK Init");

    ESP32PWM::allocateTimer(0);
    ESP32PWM::allocateTimer(1);
    ESP32PWM::allocateTimer(2);
    ESP32PWM::allocateTimer(3);
    servoProbe.setPeriodHertz(50); // standard 50 hz servo
    servoProbe.attach(pin_servo, 1000, 2000);

    pinMode(solenoid, OUTPUT);
    pinMode(trigLev, OUTPUT);
    pinMode(echoLev, INPUT);

    sensors.begin();

    setupCloudIoT();
}

void loop()
{
    DynamicJsonDocument jsonSensor(200);
    mqttClient->loop();
    struct tm timeinfo;
    strftime(server_detik, 3, "%S", &timeinfo);
    strftime(server_menit, 3, "%M", &timeinfo);
    strftime(server_jam, 3, "%H", &timeinfo);

    if (!mqttClient->connected())
    {
        connect();
    }

    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
    }

    //Keperluan debug sensor
    if (millis() - lastMillis > pubDelay)
    {
        lastMillis = millis();

        // nilai_TDS = ambil_nilai_TDS();
        Serial.print(server_jam);
        Serial.print(" ");
        Serial.print(server_menit);
        Serial.print(" ");
        Serial.println(server_detik);

        /*
            Setiap jam 9 pagi akan dilakukan
        */
        if ((strcmp(server_jam, jam) == 0 && strcmp(server_menit, menit) == 0))
        {
            /*
                Disini jalankan pengecekan tds dan ph
            */

            Serial.println("cek/stabilke");
            data = "";
            Serial.println("Servo turun start");
            for (pos = 75; pos <= 180; pos += 1)
            {
                servoProbe.write(pos); // tell servo to go to position in variable 'pos'
                delay(15);             // waits 15ms for the servo to reach the position
            }
            
            //jalankan fungsi pengecekan
            kontrol_servo(0);

            nilai_ph = ambil_nilai_pH(pin_sensor_pH, 30); //Menjalankan test dengan 30 sample (30 detik)
            nilai_TDS = ambil_nilai_TDS();

            Serial.print("Nilai TDS: ");
            Serial.print(nilai_TDS);
            Serial.print("\t||\t");
            Serial.print("Nilai pH: ");
            Serial.println(nilai_ph);

            while (statusStabilisasiTDS == true)
            {
                if (nilai_TDS < (set_point - 50))
                {
                    Serial.println("TDS Kurang, akan ditambahkan");
                    pompa(pompamixA, 5);
                    delay(300000); //Ganti dengan 300000ms (5 menit)
                    pompa(pompamixB, 5);
                    delay(300000); //Ganti dengan 300000ms (5 menit)
                    nilai_TDS = ambil_nilai_TDS();
                    Serial.println("Penambahan Selesai");
                }

                //Rasido tambah banyu
                // else if (nilai_TDS > (set_point + 50))
                // {
                //     Serial.println("TDS Kelebihan, akan dikurang");
                //     hidupkanSolenoid(solenoid, 500);
                //     delay(300000); //Delay 5 min
                //     nilai_TDS = ambil_nilai_TDS();
                //     Serial.println("Pengurangan Selesai");
                // }

                else
                {
                    Serial.print("TDS Sudah stabil di nilai: ");
                    Serial.println(nilai_TDS);
                    statusStabilisasiTDS = false;
                }
            }

            //Habis ini stabilisasi pH
            while (statusStabilisasipH == true)
            {
                if (nilai_ph > ph_setpoint_atas)
                {
                    Serial.println("Nilai pH terlalu tinggi, menurunkan");
                    pompa(pompaAcid, 10);
                    Serial.println("Mengunggu 5 menit");
                    delay(10); //Tunggu 5 menit
                    nilai_ph = ambil_nilai_pH(pin_sensor_pH, 30);
                    Serial.println("Penurunan Selesai");
                }
                else if (nilai_ph < ph_setpoint_bawah)
                {
                    Serial.println("Nilai pH terlalu rendah, menaikan");
                    pompa(pompaBase, 10);
                    Serial.println("Mengunggu 5 menit");
                    delay(10); //Tunggu 5 menit
                    nilai_ph = ambil_nilai_pH(pin_sensor_pH, 30);
                    Serial.println("Selesai");
                }
                else
                {
                    Serial.println("Nilai pH sudah stabil, penyesuaian selesai");
                    statusStabilisasipH = false;
                }
            }
            delay(5000); //Delay diluk
            jsonSensor["pH"] = nilai_ph;
            jsonSensor["TDS"] = nilai_TDS;
            jsonSensor["suhuAir"] = random(0, 100);
            jsonSensor["levelAir"] = getLevelAir();
            serializeJson(jsonSensor, data);
            Serial.println(data);
            publishTelemetry(data);
            data = "";
            kontrol_servo(1); //Servo naik
        }
        else
        {
            Serial.println("rung wayaje");
        }
        jsonSensor["levelAir"] = getLevelAir();
        serializeJson(jsonSensor, data);
        Serial.println(data);
        publishTelemetry(data);
        data = "";
    }
}

float ambil_nilai_TDS()
{
    static unsigned long analogSampleTimepoint = millis();
    if (millis() - analogSampleTimepoint > 40U) //every 40 milliseconds,read the analog value from the ADC
    {
        analogSampleTimepoint = millis();
        analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin); //read the analog value and store into the buffer
        analogBufferIndex++;
        if (analogBufferIndex == SCOUNT)
            analogBufferIndex = 0;
    }
    static unsigned long printTimepoint = millis();
    if (millis() - printTimepoint > 800U)
    {
        printTimepoint = millis();
        for (copyIndex = 0; copyIndex < SCOUNT; copyIndex++)
            analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
        averageVoltage = getMedianNum(analogBufferTemp, SCOUNT) * (float)VREF / 1024.0;
        float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
        float compensationVolatge = averageVoltage / compensationCoefficient;
        tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value

        return tdsValue;
    }
}

int getMedianNum(int bArray[], int iFilterLen)
{
    int bTab[iFilterLen];
    for (byte i = 0; i < iFilterLen; i++)
        bTab[i] = bArray[i];
    int i, j, bTemp;
    for (j = 0; j < iFilterLen - 1; j++)
    {
        for (i = 0; i < iFilterLen - j - 1; i++)
        {
            if (bTab[i] > bTab[i + 1])
            {
                bTemp = bTab[i];
                bTab[i] = bTab[i + 1];
                bTab[i + 1] = bTemp;
            }
        }
    }
    if ((iFilterLen & 1) > 0)
        bTemp = bTab[(iFilterLen - 1) / 2];
    else
        bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
    return bTemp;
}

/*Fungsi untuk mengambil nilai kedalaman air (level air)*/
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

/*
    @brief Fungsi untuk mengeluarkan air sebanyak n milii
    @param pin_Pompa   Pin kontrol solenoid 
    @param durasi    Durasi penghidupan solenoid
    @return void
*/
void pompa(int pin_Pompa, int miliLiter)
{
    ledcWrite(pin_Pompa, 50);
    delay(40 * miliLiter);
    ledcWrite(pin_Pompa, 0);
}

/*
    @brief Fungsi untuk mengambil pH
    @param pin_pH   Pin masukan pH 
    @param banyak_sample_pH Banyaknya sampel yang diambil (1 sample berdurasi 1 detik)
    @return Nilai rata-rata pH (float)
*/
float ambil_nilai_pH(int pin_pH, int banyak_sample_pH)
{
    float jumlah = 0;
    float Po[banyak_sample_pH];
    //Mengosongkan nilai ph
    for (size_t i = 0; i < banyak_sample_pH; i++)
    {
        Po[i] = 0;
    }
    //Mengambil data dan menjumlah
    for (int i = 0; i < banyak_sample_pH; i++)
    {
        float nilaiPengukuranPh = analogRead(pin_sensor_pH);
        float TeganganPh = 3.3 / 4096.0 * nilaiPengukuranPh;
        Po[i] = 7.00 + ((2.44 - TeganganPh) / 0.18);
        jumlah = jumlah + Po[i];
        delay(1000);
    }
    //Hasil rata-rata
    return (float)jumlah / banyak_sample_pH;
}

/*
    @brief menghidupkan solenoid untuk menambah air
    @param pin_ledeng   Pin kontrol solenoid 
    @param delay    Durasi penghidupan solenoid
    @return void
*/
void hidupkanSolenoid(int pin_ledeng, int durasi)
{
    Serial.print("Menghidupkan solenoid selama ");
    Serial.print(durasi);
    Serial.println(" Detik!");

    digitalWrite(pin_ledeng, HIGH);
    delay(durasi);
    digitalWrite(pin_ledeng, LOW);
}

/*
    Fungsi ini digunakan untuk mengatur naik turunnya servo 
    @param modeServo    Mode "1" untuk naik dan mode "0" untuk turunv (int)
*/
void kontrol_servo(int modeServo)
{
    switch (modeServo)
    {
    case 0:
        Serial.println("Servo turun start");
        for (pos = 75; pos <= 180; pos += 1)
        {
            servoProbe.write(pos); // tell servo to go to position in variable 'pos'
            delay(15);             // waits 15ms for the servo to reach the position
        }

        break;
    case 1:
        Serial.println("Servo naik start");
        for (pos = 180; pos >= 75; pos -= 1)
        {                          // goes from 180 degrees to 0 degrees
            servoProbe.write(pos); // tell servo to go to position in variable 'pos'
            delay(15);             // waits 15ms for the servo to reach the position
        }
        break;
    default:
        Serial.println("TF? Bukan mode itu cuk");
        break;
    }

    Serial.println("Servo done");
}