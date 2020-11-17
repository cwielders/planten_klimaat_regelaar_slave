#include <Arduino.h>
#include <Wire.h> // must be included here so that Arduino library object file references work (clock)
#include <RtcDS3231.h>

#include <SD.h>

#include <DHT.h>
#include <DHT_U.h>

#include<SPI.h>  //to get the meaning of MISO                                                                

#define DAGTEMPERATUUR 0
#define NACHTTEMPERATUUR 1
#define LUCHTVOCHTIGHEID 2
#define ISDAG 3
#define ISDAUW 4
#define ISBEWOLKING 5
#define ISREGEN 6
#define ISPOMP 7
#define RESERVE1 8
#define RESERVE2 9

#define TEMPERATUURNU 10
#define LUCHTVOCHTIGHEIDNU 11
#define POTVOCHTIGHEIDNU 12
#define LICHTNU 13
#define WATERSTANDNU 14
#define LAMPENAAN1 15
#define LAMPENAAN2 16
#define VENTILATORAAN 17
#define VERNEVELAARAAN 18
#define RESERVE3 19
//#define ZONOP 39
//#define ZONONDER 40

#define WINTER 0
#define ZOMER 1
#define REGEN 2

volatile byte c;
volatile byte j = 0;
volatile byte i = 0;
volatile bool flag1 = false; //weer false na iedere loop omdat startcode 1x moet worden opgestuurd, true als de startcode als laatste is ontvangen 
volatile bool flag2 = false; //wordt true na uiwisseling laatste element array

byte pinArray1[8] = {A0, 9, A1, 24, 25, 20, 8, 27}; 
byte pinArray2[8] = {A2, 11, A3, 24, 25, 20, 10, 27};
byte pinArray3[8] = {A4, 13, A5, 24, 25, 20, 12, 27};
//1soilsensorPin1, 2soilPower1, 3lightsensorPin1, 4lampenPin1, 5ventilatorpin1, 6vernevelaarpin1, 7dhtpin1

byte klimaatDataNu[3][20]= {{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19},{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19},{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19}};

class LichtSensor {

    int pin;
    float rawRange = 1024;
    float logRange = 5.0;

    public:
    LichtSensor(int myPin) {
        pin = myPin;
    }

    void initialisatie() {
        Serial.println("lichtsensor geinitialiseerd");
        Serial.print("lichtsensorpin = ");
        Serial.println(pin);
    }
    
    byte readLightValue() {
        int rawValue = analogRead(pin); 
        Serial.print(rawValue);
        //delay(10);
        // rawValue = byte(round(analogRead(pin)/10.24));//10.24 om van 1024 analoge waardes terug te gaan na 100
        return(byte(round(analogRead(pin)/10.24)));//10.24 om van 1024 analoge waardes terug te gaan na 100
    }

    // // read the log value from the light sensor:
    // float readLogValue() {
    //     float rawValue = analogRead(pin);
    //     float logLux = rawValue * logRange / rawRange;
    //     float luxValue = pow(10, logLux);
    //     return(luxValue);
    // }

};

class SoilHumiditySensor {

    byte pin;
    byte powerPin;

    public:
    SoilHumiditySensor(byte myPin, byte myPowerPin) {
        pin = myPin;
        powerPin = myPowerPin;
        pinMode(powerPin, OUTPUT);//Set D2 as an OUTPUT
        digitalWrite(powerPin, LOW);//Set to LOW so no power is flowing through the sensor
    }

    void initialisatie() {
        Serial.println("SoilHumiditySensor geinitialiseerd");
        Serial.print("SoilHumiditySensorPin = ");
        Serial.println(pin);
        Serial.print("SoilHumiditySensorPowerPin = ");
        Serial.println(powerPin);
    }

    byte readSoilHumidity() {
        digitalWrite(powerPin, HIGH);
        delay(10);
        byte soilmoisture = byte(round(analogRead(pin)/10.24));
        delay(10);
        soilmoisture = byte(round(analogRead(pin)/10.24));
        Serial.print("Soilhumidity  raw= ");
        Serial.println(soilmoisture);
        digitalWrite(powerPin, LOW);
        
        return(byte(soilmoisture));
    } 
};

class LuchtVochtigheidTemperatuurSensor {

    DHT dht;
    byte pin;

    public:
    LuchtVochtigheidTemperatuurSensor(byte myPin) :
        dht(myPin, DHT22) 
        {
            dht.begin();
            pin = myPin;
        }

    void initialisatie() {
        Serial.println("LuchtVochtigheidTemperatuurSensor geinitialiseerd");
        Serial.print("LuchtVochtigheidTemperatuurSensorPin = ");
        Serial.println(pin);
    }

    byte readTempValue() {
        Serial.print("nieuwe temp meting");
        byte temperatuur = byte(round(dht.readTemperature()));
        if (isnan(temperatuur)) {
            Serial.println(F("Failed to read from Temperature sensor!"));
            return;
        }
        return(temperatuur);
    }

    byte readHumidityValue() {
        Serial.println("nieuwe humidity meting");
        byte vochtigheid = byte(round(dht.readHumidity()));
        if (isnan(vochtigheid)) {
            Serial.println(F("Failed to read from humidity sensor!"));
            return;
        }
        return(vochtigheid);    //Print temp and humidity values to serial monitor
    }
};

class KlimaatRegelaar {
    
    byte lampenPin1;
    byte lampenPin2;
    byte nevelPin;
    byte ventilatorPin;
    int plantenBakNummer;
    boolean ventilatorIsAan = false;
    boolean vernevelaarIsAan = false;
    boolean lampIsAan1 = false;
    boolean lampIsAan2 = false;
    boolean luchtIsDroog = false;
    boolean dag = false;
    boolean dauw = false;
    boolean regen = false;

    public:
    KlimaatRegelaar(byte myLampenPin1, byte myLampenPin2, byte myNevelPin, byte myVentilatorPin, int myPlantenBakNummer) {
        plantenBakNummer = myPlantenBakNummer;
        lampenPin1 = myLampenPin1;
        lampenPin2 = myLampenPin2;
        nevelPin = myNevelPin;
        ventilatorPin = myNevelPin;
        pinMode(lampenPin1, OUTPUT);
        digitalWrite(lampenPin1, LOW);
        pinMode(lampenPin2, OUTPUT);
        digitalWrite(lampenPin1, LOW);
        pinMode(nevelPin, OUTPUT);
        digitalWrite(nevelPin, LOW);
        pinMode(ventilatorPin, OUTPUT);
        digitalWrite(nevelPin, LOW);
        digitalWrite(ventilatorPin, LOW);
        }

    void initialisatie() {
        Serial.println("KlimaatRegelaar geinitialiseerd");
        Serial.print("lampenPin1 = ");
        Serial.println(lampenPin1);
        Serial.print("lampenPin1 = ");
        Serial.println(lampenPin2);
        Serial.print("nevelPin = ");
        Serial.println(nevelPin);
        Serial.print("ventilatorPin = ");
        Serial.println(ventilatorPin);
        Serial.println();
    }

    void doeJeKlimaatDing() {
        Serial.print("plantenBakNummer = ");
        Serial.println(plantenBakNummer);
        regelLicht();
        regelDauw();
        regelRegenWolken();
        regelVochtigheid();
        regelTemperatuur();
        standen();
    }
    
    void regelLicht() {
        if (!dag && klimaatDataNu[plantenBakNummer][ISDAG]) {
            digitalWrite(lampenPin1, HIGH);
            digitalWrite(lampenPin2, HIGH);
            Serial.println("Lampen aangeschakeld");
            dag = true;
            lampIsAan2 = true;
            klimaatDataNu[plantenBakNummer][LAMPENAAN2] = lampIsAan2;
            lampIsAan1 = true;
            klimaatDataNu[plantenBakNummer][LAMPENAAN1] = lampIsAan1;
        }
        if (dag && !klimaatDataNu[plantenBakNummer][ISDAG]) {
            digitalWrite(lampenPin1, LOW); 
            digitalWrite(lampenPin2, LOW); 
            Serial.println("Lampen uitgeschakeld");
            dag = false;
            lampIsAan2 = false;
            klimaatDataNu[plantenBakNummer][LAMPENAAN2] = lampIsAan2;
            lampIsAan1 = false;
            klimaatDataNu[plantenBakNummer][LAMPENAAN1] = lampIsAan1;
        }
    }

    void regelRegenWolken() {
        // In volgende regel 100 aanpassen op basis van resultaten
        if (!regen && klimaatDataNu[plantenBakNummer][ISREGEN] && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] < 100) {
            if (!vernevelaarIsAan && (klimaatDataNu[plantenBakNummer][TEMPERATUURNU] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR])) {
                digitalWrite(nevelPin, HIGH);
                Serial.println("vernevelaar aan (regenwolken)");
                vernevelaarIsAan = true;
                klimaatDataNu[plantenBakNummer][VERNEVELAARAAN] = vernevelaarIsAan;
                regen = true;
            }    
            if (lampIsAan2) {
                digitalWrite(lampenPin2, LOW);
                Serial.println("lampen2 uit (regenwolken)");
                lampIsAan2 = false;
                klimaatDataNu[plantenBakNummer][LAMPENAAN2] = lampIsAan2;
            }
        }

        if (regen && (!klimaatDataNu[plantenBakNummer][ISREGEN]||klimaatDataNu[plantenBakNummer][TEMPERATUURNU] < klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR])) {
            regen = false;
            if (vernevelaarIsAan && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID]) {
                digitalWrite(nevelPin, LOW); 
                Serial.println("vernevelaar uit (regenwolken)"); 
                regen = false;
                vernevelaarIsAan = false;
                klimaatDataNu[plantenBakNummer][VERNEVELAARAAN] = vernevelaarIsAan;
            }  
            if (!lampIsAan2 && klimaatDataNu[plantenBakNummer][ISDAG]) {
                digitalWrite(lampenPin2, HIGH); 
                Serial.println("lampen2 aan (regenwolken)"); 
                lampIsAan2 = true;
                klimaatDataNu[plantenBakNummer][LAMPENAAN2] = lampIsAan2;
            }
        }
    }
    
    void regelDauw() {
        if (!dauw && klimaatDataNu[plantenBakNummer][ISDAUW]) {
            if (!vernevelaarIsAan && klimaatDataNu[plantenBakNummer][TEMPERATUURNU] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR]) {
                digitalWrite(nevelPin, HIGH);
                Serial.println("vernevelaar aan (dauw)");
                vernevelaarIsAan = true;
                dauw = true;
                klimaatDataNu[plantenBakNummer][VERNEVELAARAAN] = vernevelaarIsAan;
                }    
            if (!ventilatorIsAan && klimaatDataNu[plantenBakNummer][TEMPERATUURNU] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR]) {
                digitalWrite(ventilatorPin, HIGH);
                Serial.println("ventilator aan (dauw)");
                ventilatorIsAan = true;
                klimaatDataNu[plantenBakNummer][VENTILATORAAN] = ventilatorIsAan;
                }
        }

        if (dauw && (klimaatDataNu[plantenBakNummer][ISDAUW]|| klimaatDataNu[plantenBakNummer][TEMPERATUURNU] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR])) {
            if (vernevelaarIsAan && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID]) {
                digitalWrite(nevelPin, LOW); 
                Serial.println("vernevelaar uit (dauw)"); 
                vernevelaarIsAan = false;
                klimaatDataNu[plantenBakNummer][VERNEVELAARAAN] = vernevelaarIsAan;
            }  
            if (ventilatorIsAan) {
                digitalWrite(ventilatorPin, LOW); 
                Serial.println("ventilator uit (dauw)"); 
                ventilatorIsAan = false;
                klimaatDataNu[plantenBakNummer][VENTILATORAAN] = ventilatorIsAan;
            }
        dauw = false;
        }
    }

    void regelVochtigheid() {

        if (!luchtIsDroog && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] < klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID] && klimaatDataNu[plantenBakNummer][TEMPERATUURNU] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR]) {
            luchtIsDroog = true;
            if (!vernevelaarIsAan) {
                digitalWrite(nevelPin, HIGH);
                Serial.println("vernevelaar aan (luchtvochtigheid)");
                vernevelaarIsAan = true;
                klimaatDataNu[plantenBakNummer][VERNEVELAARAAN] = vernevelaarIsAan;
            } 
        }
        if (vernevelaarIsAan && !dauw && !regen && (klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU]  > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID] || klimaatDataNu[plantenBakNummer][TEMPERATUURNU] < klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR] || klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > 100)) {
            luchtIsDroog = false;
            digitalWrite(nevelPin, LOW);
            Serial.print("vernevelaar uit (luchtvochtigheid)");
            vernevelaarIsAan = false;
            klimaatDataNu[plantenBakNummer][VERNEVELAARAAN] = vernevelaarIsAan;
        }
        if (luchtIsDroog && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID] ) { 
            luchtIsDroog = false;    
        }
        if (klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID] && klimaatDataNu[plantenBakNummer][TEMPERATUURNU] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR]) {
            luchtIsDroog = false;
            if (!ventilatorIsAan) {
                digitalWrite(ventilatorPin, HIGH);
                Serial.println("ventilator aan (luchtvochtigheid)");
                ventilatorIsAan = true;
                klimaatDataNu[plantenBakNummer][VENTILATORAAN] = ventilatorIsAan;
            } 
        }
    }

    void regelTemperatuur(){

        if (klimaatDataNu[plantenBakNummer][TEMPERATUURNU] > klimaatDataNu[plantenBakNummer][DAGTEMPERATUUR]) {
            if (!ventilatorIsAan) {
                digitalWrite(ventilatorPin, HIGH);
                Serial.print("vernevelaar aan (temperatuur)");
                ventilatorIsAan = true;
                klimaatDataNu[plantenBakNummer][VENTILATORAAN] = ventilatorIsAan;
            }
            if (lampIsAan2) {
                digitalWrite(lampenPin2, LOW);
                Serial.println("lampen2 uit (temperatuur)");
                lampIsAan2 = false;
                klimaatDataNu[plantenBakNummer][LAMPENAAN2] = lampIsAan2;
            }
            if (!vernevelaarIsAan) {
                digitalWrite(nevelPin, HIGH);
                Serial.print("vernevelaar aan (temperatuur)");
                vernevelaarIsAan = true;
                klimaatDataNu[plantenBakNummer][VERNEVELAARAAN] = vernevelaarIsAan;
            }
        }
        if (klimaatDataNu[plantenBakNummer][TEMPERATUURNU] < klimaatDataNu[plantenBakNummer][DAGTEMPERATUUR]) {
            if (ventilatorIsAan && !dauw) {
                digitalWrite(ventilatorPin, LOW);
                Serial.print("vernevelaar uit (temperatuur)");
                ventilatorIsAan = false;
                klimaatDataNu[plantenBakNummer][VENTILATORAAN] = ventilatorIsAan;
            }
            if (!lampIsAan2 && !regen && dag) {
                digitalWrite(lampenPin2, HIGH);
                Serial.println("lampen2 aan (temperatuur)");
                lampIsAan2 = true;
                klimaatDataNu[plantenBakNummer][LAMPENAAN2] = lampIsAan2;
            }
            if (vernevelaarIsAan && !dauw && !regen && !luchtIsDroog) {
                digitalWrite(nevelPin, LOW);
                Serial.print("vernevelaar uit (temperatuur)");
                vernevelaarIsAan = false;
                klimaatDataNu[plantenBakNummer][VERNEVELAARAAN] = vernevelaarIsAan;
            }
        }
    }

    void standen() {
        Serial.print("plantenBakNummer = " );
        Serial.println(plantenBakNummer);
        Serial.print("isDag = ");
        Serial.println(dag);
        Serial.print("vernevelaarIsAan = ");
        Serial.println(vernevelaarIsAan);
        Serial.print("isDauw = ");
        Serial.println(dauw);
        Serial.print("ventilatorIsAan = ");
        Serial.println(ventilatorIsAan);
        Serial.print("isRegenWolk = ");
        Serial.println(regen);
        Serial.print("lampenAan2 = ");
        Serial.println(lampIsAan2);
        Serial.print("luchtIsDroog = ");
        Serial.println(luchtIsDroog);
        
    }
};

class Plantenbak {

    KlimaatRegelaar klimaatRegelaar;
    SoilHumiditySensor soilHumiditySensor;
    LuchtVochtigheidTemperatuurSensor luchtVochtigheidTemperatuurSensor;
    LichtSensor lichtSensor;
    boolean ventilatorIsUit = true;
    boolean vernevelaarIsUit = true;
    int plantenBakNummer;
    int maxPotVocht; //gelijkstellen aan getlal gemeten na ingebruikname
    int maxLicht; //gelijkstellen aan getlal gemeten na ingebruikname
    
    public:
    Plantenbak(byte (&myPins)[8], int myPlantenBakNummer) :
        soilHumiditySensor(myPins[0], myPins[1]),
        lichtSensor(myPins[2]),
        luchtVochtigheidTemperatuurSensor(myPins[6]),
        klimaatRegelaar(myPins[3] , myPins[7], myPins[5], myPins[4], myPlantenBakNummer),
        plantenBakNummer(myPlantenBakNummer)
    {
    }
    //1soilsensorPin1, 2soilPower1, 3lightsensorPin1, 4lampenPin1, 5ventilatorpin1, 6vernevelaarpin1, 7dhtpin1

    void setup() {

        Serial.print("plantenBakNummer = ");
        Serial.println(plantenBakNummer);
        lichtSensor.initialisatie();
        soilHumiditySensor.initialisatie();
        luchtVochtigheidTemperatuurSensor.initialisatie();
        klimaatRegelaar.initialisatie();
        Serial.println("plantenbak geinitialseerd");
    }

    void regelKlimaat(int plantenBakNummer) {
        Serial.print("plantenBakNummer in plantenBak regelKlimaat = ");
        Serial.println(plantenBakNummer);
        klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] = luchtVochtigheidTemperatuurSensor.readHumidityValue();
        Serial.print("Humidity = ");
        Serial.println(klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU]);
        //Serial.print(luchtVochtigheidTemperatuurSensor.readHumidityValue());
        klimaatDataNu[plantenBakNummer][TEMPERATUURNU] = luchtVochtigheidTemperatuurSensor.readTempValue();
        Serial.print("Temp  ");
        Serial.print(klimaatDataNu[plantenBakNummer][TEMPERATUURNU]);
        Serial.println(" Celsius");
        //Serial.println(luchtVochtigheidTemperatuurSensor.readTempValue());
        byte potVochtigheid = soilHumiditySensor.readSoilHumidity();
        if(potVochtigheid > maxPotVocht) {
            maxPotVocht = potVochtigheid;
            //klimaatDataNu[plantenBakNummer][HOOGSTEPOTVOCHTIGHEID] = potVochtigheid;
        }
        klimaatDataNu[plantenBakNummer][POTVOCHTIGHEIDNU] = potVochtigheid;
        //klimaatDataNu[plantenBakNummer][POTVOCHTIGHEIDNU] = (potVochtigheid/maxPotVocht)*99; //100% past niet op display
        Serial.print("Soil Moisture = ");  
        Serial.println(potVochtigheid);
        // Serial.print("Soil Moisture = ");  
        // Serial.println((potVochtigheid/maxPotVocht)*99);//100% past niet op display
        // Serial.println(klimaatDataNu[plantenBakNummer][POTVOCHTIGHEIDNU]);
        byte licht = lichtSensor.readLightValue();
        if(licht >= maxLicht) {
            maxLicht = licht;
            //klimaatDataNu[plantenBakNummer][MEESTELICHT] = licht;
        }
        //klimaatDataNu[plantenBakNummer][LICHT] = (licht/maxLicht)*99;//100% past niet op display
        klimaatDataNu[plantenBakNummer][LICHTNU] = licht;
        Serial.print("Licht = ");
        Serial.println(licht);
        klimaatRegelaar.doeJeKlimaatDing();
    }  
};


class DataUitwisselaarSlave {

    public:
    DataUitwisselaarSlave(){
        pinMode(MISO, OUTPUT);// have to send on master in, *slave out*
        SPCR |= _BV(SPE);  // turn on SPI in slave mode
        SPCR |= _BV(SPIE); // turn on interrupts
        flag1 = false;
    } 
};

ISR (SPI_STC_vect){
    //flag2 = false;
    c = SPDR;
    if (c == 0xCD){ 
        SPDR = 0xEF; //teruggezonden startcode voor Master 0xEF=239
        i = 0;
        j = 0;
    } else {
        if (c == 0xF3){//0xF3=243
            SPDR = klimaatDataNu[i][j+10];

        } else {
            if (i < 3){
                klimaatDataNu[i][j] = c;
                j++;
                if (j < 10){
                    SPDR = klimaatDataNu[i][j+10];
                }
                if (j == 10){
                    j = 0;
                    i++;
                    SPDR = klimaatDataNu[i][j+10];
                    if (i == 3){
                        flag2 = true;
                        Serial.println("klimaatDataNu voledig ontvangen");
                        for ( byte n = 0 ; n < 3 ; n++){
                            for (byte t = 0 ; t < 20 ; t++){
                                Serial.print(klimaatDataNu[n][t]);
                                Serial.print("/");
                            }
                        Serial.println();    
                        }
                        Serial.println();
                    }
                }
            }
        }
    }
}

DataUitwisselaarSlave dataUitwisselaarSlave;

int bakNummer1 = 0;
Plantenbak plantenbak1(pinArray1, bakNummer1);
int bakNummer2 = 1;
Plantenbak plantenbak2(pinArray2, bakNummer2);
int bakNummer3 = 2;
Plantenbak plantenbak3(pinArray3, bakNummer3);

void setup() {
    Serial.begin(9600);
    analogReference(EXTERNAL); 
    plantenbak1.setup();
    plantenbak2.setup();
    plantenbak3.setup();
}

void loop() {
    if (flag2 == true){
        Serial.println("begin loop slave");

        plantenbak1.regelKlimaat(bakNummer1);
        delay(1000);
        plantenbak2.regelKlimaat(bakNummer2);
        delay(1000);
        plantenbak3.regelKlimaat(bakNummer3);
        flag2 = false;

        Serial.println();
        Serial.println("end of loop Slave");
    }   
}