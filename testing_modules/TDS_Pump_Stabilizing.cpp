/*
    TENTANG FILE TEST INI:
        File ini merupakan test dari fitur-fitur yang ada
        Yang ada pada ini berisi program untuk mengambil data dari sistem.
        Nilai variabel yang digunakan akan
        
            Pengambilan dilakukan pada pukul 09:00 WIB (jere virna apik ok,
        aku yo terimo manut ro mbak cantik). Untung saja esp32 memiliki fitur
        untuk mengambil waktu melaui ntp.
            Gunakan fitur time.h untuk mengambil data waktu dari internet.
        Pengambilan waktu dilakukan setiap 1 jam. Setiap 15 menit akan dilakukan
        pengambilan data sensor-sensor sing rodo rapenting (cahaya, suhu udara,
        kelembapan dll). 
    +>      Apabila waktu sudah menunjukan 900 WIB, servo akan menggerakan probe
        untuk masuk ke dalam air. 
    +>      Setelah masuk, probe akan menyetabilkan nilai TDS.
    +>      Setelah TDS baru menyetabilkan nilai pH
    +>      Setelah stabil maka akan kembali ke loop utama yaitu nunggu 15 menit.
        Nunggu 15 menitnya pakainya milis aja

            
*/

#include <Arduino.h>
#include <EEPROM.h>
#include <ESP32_Servo.h>

#define pin_tds 14
#define sample_count 30
#define VREF 3.3

#define pompa_mix_a 3
#define pompa_mix_b 4
#define pin_sole 14
#define pin_pH_down 13
#define pin_pH_up 20

int analogBuffer[sample_count];
int analogBufferTemp[sample_count];
int analogBufferIndex = 0, copyIndex = 0;
float averageVoltage = 0, tdsValue = 0, temperature = 25;
float nilai_TDS = 0;

float nilai_test = 100;
float nilai_test_pH = 9;
int jam;
const int servo = 5;
int pos = 0;
int modeServo;

float ph_setpoint_bawah = 5.5;
float ph_setpoint_atas = 7;

boolean statusStabilisasiTDS = true;
boolean statusStabilisasipH = true;

int set_point = 500; //Wajib diganti 0

Servo servoProbe;

/*
    Fungsi untuk mengeluarkan carian dari pompa
    @param pin_pompa    pin output yang digunakan untuk mengeluarkan cairan (int)
            mililiter    banyaknya cairan yang dikeluarkan dalam mL (int)
*/
void pompa(int pin_pompa, int mililiter)
{
    Serial.print("Menghidupkan pompa dan mengeluarkan air");
    Serial.print(mililiter);
    ledcWrite(pin_pompa, 50);
    delay((40 * mililiter));
    ledcWrite(pin_pompa, 0);
    Serial.println("Pompa selesai");
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

float get_ppm()
{
    static unsigned long analogSampleTimepoint = millis();
    if (millis() - analogSampleTimepoint > 40U) //every 40 milliseconds,read the analog value from the ADC
    {
        analogSampleTimepoint = millis();
        analogBuffer[analogBufferIndex] = analogRead(pin_tds); //read the analog value and store into the buffer
        analogBufferIndex++;
        if (analogBufferIndex == sample_count)
            analogBufferIndex = 0;
    }
    static unsigned long printTimepoint = millis();
    if (millis() - printTimepoint > 800U)
    {
        printTimepoint = millis();
        for (copyIndex = 0; copyIndex < sample_count; copyIndex++)
            analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
        averageVoltage = getMedianNum(analogBufferTemp, sample_count) * (float)VREF / 1024.0;
        float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
        float compensationVolatge = averageVoltage / compensationCoefficient;
        tdsValue = (133.42 * compensationVolatge * compensationVolatge * compensationVolatge - 255.86 * compensationVolatge * compensationVolatge + 857.39 * compensationVolatge) * 0.5; //convert voltage value to tds value
        return tdsValue;
    }
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
    case 1:
        Serial.println("Servo turun start");
        for (pos = 0; pos <= 180; pos += 1)
        {
            servoProbe.write(pos);
            delay(15);
        }
        break;
    case 0:
        Serial.println("Servo naik start");
        for (pos = 0; pos <= 180; pos += 1)
        {
            servoProbe.write(pos);
            delay(15);
        }
        break;
    default:
        Serial.println("TF? Bukan mode itu cuk");
        break;
    }

    Serial.println("Servo done");
}

void setup()
{
    Serial.begin(115200);
    ledcSetup(0, 1000, 8);
    ledcAttachPin(33, 0); //Pompa B
    servoProbe.attach(servo);

    Serial.println("Starting device");
}

void loop()
{
    
    if (jam == 9)
    {
        kontrol_servo(0);
        while (statusStabilisasiTDS == true)
        {
            if (nilai_test < (set_point - 50))
            {
                Serial.println("TDS Kurang, akan ditambahkan");
                pompa(pompa_mix_a, 5);
                delay(10); //Ganti dengan 300000ms (5 menit)
                nilai_test += 5;
                pompa(pompa_mix_b, 5);
                delay(10); //Ganti dengan 300000ms (5 menit)
                nilai_test += 5;
                Serial.println("Penambahan Selesai");
            }

            else if (nilai_test > (set_point + 50))
            {
                Serial.println("TDS Kelebihan, akan dikurang");
                hidupkanSolenoid(pin_sole, 1000);
                nilai_test -= 5;
                Serial.println("Pengurangan Selesai");
            }

            else
            {
                Serial.print("TDS Sudah stabil di nilai: ");
                Serial.println(nilai_test);
                nilai_test += 5;
                statusStabilisasiTDS = false;
            }
        }

        //Habis ini stabilisasi pH
        while (statusStabilisasipH == true)
        {
            if (nilai_test_pH > ph_setpoint_atas)
            {
                Serial.println("Nilai pH terlalu tinggi, menurunkan");
                pompa(pin_pH_down, 10);
                Serial.println("Mengunggu 5 menit");
                delay(10); //Tunggu 5 menit
                nilai_test_pH -= 0.5;
                Serial.println("Penurunan Selesai");
            }
            else if (nilai_test_pH < ph_setpoint_bawah)
            {
                Serial.println("Nilai pH terlalu rendah, menaikan");
                pompa(pin_pH_up, 10);
                Serial.println("Mengunggu 5 menit");
                delay(10); //Tunggu 5 menit
                nilai_test_pH += 0.5;
                Serial.println("Selesai");
            }
            else
            {
                Serial.println("Nilai pH sudah stabil, penyesuaian selesai");
                statusStabilisasipH = false;
            }
        }
    }
    else
    {
        Serial.println("Durung wayahe");
    }
    delay(1000);
}
