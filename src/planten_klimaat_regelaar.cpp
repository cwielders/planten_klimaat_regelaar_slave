#include <Arduino.h>
#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>

#include <SD.h>

#include <DHT.h>
#include <DHT_U.h>

#include <UTFT.h>
#include <URTouch.h>
#include <UTFT_Buttons.h>                                                                  
//Volgende twee regels zijn crusiaal voor touch display MAAR NIET VOOR BUTTONS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//#define YM 23 // can be a digital pin
//#define XP 22 // can be a digital pin

#define STARTDAG 0
#define EINDDAG 1
#define DUURDAUW 2
#define DUURREGEN 3
#define DAGTEMPERATUUR 4
#define NACHTTEMPERATUUR 5
#define LUCHTVOCHTIGHEID 6
#define ISDAG 7
#define ISDAUW 8
#define ISREGEN 9
#define SEIZOEN 10
#define SEIZOEN 10
#define TEMPERATUUR 11
#define LUCHTVOCHTIGHEIDNU 12
#define POTVOCHTIGHEID 13
#define LICHT 14

#define WINTER 0
#define ZOMER 1
#define REGEN 2


//volgende regel is voor tijdweergave klok
#define countof(a) (sizeof(a) / sizeof(a[0]))

extern uint8_t BigFont[];
extern uint8_t SmallFont[];

int plantenBakSettings1[3][4][12] = {{{12, 40, 10, 20, 35, 14, 70}, {8, 40, 1, 0, 14, 25, 55}, {8, 40, 2, 0, 30, 23, 85}, {0, 0, 1, 1, 1, 1, 2, 2, 2, 0, 0, 0}}, {{84, 240, 236, 15, 35, 14, 70}, {15, 30, 4, 4, 14, 25, 55}, {66, 44, 55, 0, 30, 23, 85}, {1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1}}, {{8, 40, 6, 15, 35, 14, 70}, {8, 40, 1, 0, 14, 25, 55}, {8, 50, 6, 18, 30, 23, 85}, {2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2}}};

byte pinArray1[8] = {A0, 9, A1, 4, 5, 10, 8, 7}; 
byte pinArray2[8] = {A0, 9, A1, 4, 5, 10, 8, 7};
byte pinArray3[8] = {A0, 9, A1, 4, 5, 10, 8, 7};

int klimaatDataNu[3][15];

class LichtSensor {

    int pin;
    float rawRange = 1024;
    float logRange = 5.0;

    public:
    LichtSensor(int myPin) {
        pin = myPin;
        analogReference(EXTERNAL); 
    }

    void initialisatie() {
        Serial.println("lichtsensor geinitialiseerd");
        Serial.print("lichtsensorpin = ");
        Serial.println(pin);
    }
    
    // read the raw value from the light sensor:
    float readRawValue() {
        float rawValue = analogRead(pin);
        return(rawValue);
    }

    // read the raw value from the light sensor:
    float readLogValue() {
        float rawValue = analogRead(pin);
        float logLux = rawValue * logRange / rawRange;
        float luxValue = pow(10, logLux);
        return(luxValue);
    }

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
    // read the raw value from the soil sensor:
    
    float readValue() {
        digitalWrite(powerPin, HIGH);//turn D2 "On"
        delay(10);//wait 10 milliseconds 
        float soilmoisture = analogRead(pin);//Read the SIG value form sensor 
        digitalWrite(powerPin, LOW);//turn D7 "Off"
        return(soilmoisture);
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

    float readTempValue() {
        delay(500);//drie bakken te snel achterelkaar meten lukt niet meten
        return(dht.readTemperature());
    }

    float readHumidityValue() {
        return(dht.readHumidity());    //Print temp and humidity values to serial monitor
    }
};

class DataKlimaat {
    int klimaatDataArray[3][15];
    
    public:
    DataKlimaat() 
    {}
    void addDataKlimaat(int bak, int tweede, int waarde) {
        klimaatDataArray[bak][tweede] = waarde;
    }

    int** geefDataKlimaat() {
        return (int**)klimaatDataArray;
    }
};

class KlimaatRegelaar {
    DataKlimaat dataKlimaat;
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
        // lampenaan, lampenuit, dauwaan, dauwuit, dag temperatuur, nacht temperatuur, dag vochtigheid, nacht vochtigheid, bewolkingaan, bewolkinguit)
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

    void doeJeKlimaatDing(RtcDateTime now) {
        Serial.print("plantenBakNummer = ");
        Serial.println(plantenBakNummer);
        getSettingsNu(now);
        regelLicht();
        regelDauw();
        regelRegenWolken();
        regelVochtigheid();
        regelTemperatuur();
        standen();
    }
    void getSettingsNu(RtcDateTime now) {

        //static int klimaatData[3][15];
        int seizoenNu = plantenBakSettings1[plantenBakNummer][3][(now.Month()-1)];
        int uurNu = now.Second();// terugveranderen naat hour()
        int minuutNu = now.Minute();
        float uurMinuutNu = uurNu + (minuutNu / 60);
        Serial.print("uurMinuutNu = ");
        Serial.println(uurMinuutNu);
        boolean isDag;
        boolean isDauw;
        boolean isRegen;
        
        switch (seizoenNu) {
            case WINTER:
                //Serial.println("case 0");
                for (int i = 0; i < 7; i++) {
                    klimaatDataNu[plantenBakNummer][i] = plantenBakSettings1[plantenBakNummer][WINTER][i];
                    dataKlimaat.addDataKlimaat(plantenBakNummer, i, plantenBakSettings1[plantenBakNummer][WINTER][i]);
                    //Serial.println(klimaatDataNu[plantenBakNummer][i]);
                }
                klimaatDataNu[plantenBakNummer][10] = WINTER;
                break;
            case ZOMER:
                Serial.println("case 1");
                for (int i = 0; i < 7; i++) {
                    klimaatDataNu[plantenBakNummer][i] = plantenBakSettings1[plantenBakNummer][ZOMER][i];
                    dataKlimaat.addDataKlimaat(plantenBakNummer, i, plantenBakSettings1[plantenBakNummer][ZOMER][i]);
                    //Serial.println(klimaatDataNu[plantenBakNummer][i]);
                }
                klimaatDataNu[plantenBakNummer][10] = ZOMER;
                break;
            case REGEN:
                Serial.println("case 2");
                for (int i = 0; i < 7; i++) {
                    klimaatDataNu[plantenBakNummer][i] = plantenBakSettings1[plantenBakNummer][REGEN][i];
                    dataKlimaat.addDataKlimaat(plantenBakNummer, i, plantenBakSettings1[plantenBakNummer][REGEN][i]);
                    //Serial.println(klimaatDataNu[plantenBakNummer][i]);
                }
                klimaatDataNu[plantenBakNummer][10] = REGEN;
                break;
        }       
        
        if (uurMinuutNu >= klimaatDataNu[plantenBakNummer][STARTDAG] && uurMinuutNu <= klimaatDataNu[plantenBakNummer][EINDDAG]) {
            isDag = true;
        }   else {
                isDag = false;
            }
        klimaatDataNu[plantenBakNummer][7] = isDag;
        float startDauw = klimaatDataNu[plantenBakNummer][STARTDAG] - klimaatDataNu[plantenBakNummer][DUURDAUW];
        if (uurMinuutNu >= startDauw && uurMinuutNu <= klimaatDataNu[plantenBakNummer][STARTDAG]) {
            isDauw = true;
        }   else {
                isDauw = false;
            }
        klimaatDataNu[plantenBakNummer][8] = isDauw;
        float startBewolking = ((klimaatDataNu[plantenBakNummer][STARTDAG] + klimaatDataNu[plantenBakNummer][EINDDAG]) / 2) - (klimaatDataNu[plantenBakNummer][DUURREGEN] / 2);
        float eindBewolking = startBewolking + klimaatDataNu[plantenBakNummer][DUURREGEN];
        if (uurMinuutNu >= startBewolking && uurMinuutNu <= eindBewolking) {
            isRegen = true;
        }   else {
                isRegen = false;
            }
        klimaatDataNu[plantenBakNummer][9] = isRegen;
        // Serial.print("getal in getsettings =" );
        // Serial.println(klimaatDataNu[plantenBakNummer][1]);
    }

    void regelLicht() {
            
        if (!dag && klimaatDataNu[plantenBakNummer][ISDAG]) {
            digitalWrite(lampenPin1, HIGH);
            digitalWrite(lampenPin2, HIGH);
            Serial.println("Lampen aangeschakeld");
            lampIsAan2 = true;
            lampIsAan1 = true;
        }
        if (dag && !klimaatDataNu[plantenBakNummer][ISDAG]) {
            digitalWrite(lampenPin1, LOW); 
            digitalWrite(lampenPin2, LOW); 
            Serial.println("Lampen uitgeschakeld"); 
            lampIsAan2 = false;
        }
    }

    void regelRegenWolken() {

        if (!regen && klimaatDataNu[plantenBakNummer][ISREGEN] && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] < 100) {
            if (!vernevelaarIsAan && (klimaatDataNu[plantenBakNummer][TEMPERATUUR] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR])) {
                digitalWrite(nevelPin, HIGH);
                Serial.println("vernevelaar aan (regenwolken)");
                vernevelaarIsAan = true;
                regen = true;
            }    
            if (lampIsAan2) {
                digitalWrite(lampenPin2, LOW);
                Serial.println("lampen2 uit (regenwolken)");
                lampIsAan2 = false;
            }
        }

        if (regen && (!klimaatDataNu[plantenBakNummer][ISREGEN]||klimaatDataNu[plantenBakNummer][TEMPERATUUR] < klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR])) {
            regen = false;
            if (vernevelaarIsAan && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID]) {
                digitalWrite(nevelPin, LOW); 
                Serial.println("vernevelaar uit (regenwolken)"); 
                vernevelaarIsAan = false;
            }  
            if (!lampIsAan2 && klimaatDataNu[plantenBakNummer][ISDAG]) {
                digitalWrite(lampenPin2, HIGH); 
                Serial.println("lampen2 aan (regenwolken)"); 
                lampIsAan2 = true;
            }
        }
    }
    
    void regelDauw() {
        if (!dauw && klimaatDataNu[plantenBakNummer][ISDAUW]) {
            if (!vernevelaarIsAan && klimaatDataNu[plantenBakNummer][TEMPERATUUR] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR]) {
                digitalWrite(nevelPin, HIGH);
                Serial.println("vernevelaar aan (dauw)");
                vernevelaarIsAan = true;
                dauw = true;
                }    
            if (!ventilatorIsAan && klimaatDataNu[plantenBakNummer][TEMPERATUUR] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR]) {
                digitalWrite(ventilatorPin, HIGH);
                Serial.println("ventilator aan (dauw)");
                ventilatorIsAan = true;
                }
        }

        if (dauw && (klimaatDataNu[plantenBakNummer][ISDAUW]|| klimaatDataNu[plantenBakNummer][TEMPERATUUR] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR])) {
            if (vernevelaarIsAan && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID]) {
                digitalWrite(nevelPin, LOW); 
                Serial.println("vernevelaar uit (dauw)"); 
                vernevelaarIsAan = false;
            }  
            if (ventilatorIsAan) {
                digitalWrite(ventilatorPin, LOW); 
                Serial.println("ventilator uit (dauw)"); 
                ventilatorIsAan = false;
            }
            dauw = false;
        }
    }

    void regelVochtigheid() {

        if (klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] < klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID] && klimaatDataNu[plantenBakNummer][TEMPERATUUR] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR]) {
            luchtIsDroog = true;
            if (!vernevelaarIsAan) {
                digitalWrite(nevelPin, HIGH);
                Serial.println("vernevelaar aan (luchtvochtigheid)");
                vernevelaarIsAan = true;
            } 
        }
        if (vernevelaarIsAan && !dauw && !regen && (klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU]  > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID] || klimaatDataNu[plantenBakNummer][TEMPERATUUR] < klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR])) {
            digitalWrite(nevelPin, LOW);
            Serial.print("vernevelaar uit (luchtvochtigheid)");
            vernevelaarIsAan = false;
        }
        if (klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID] ) { 
            luchtIsDroog = false;
        }
        if (vernevelaarIsAan && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > 100) {
            digitalWrite(nevelPin, LOW);
            Serial.print("vernevelaar uit (luchtvochtigheid)");
            vernevelaarIsAan = false;
        }
    }

    void regelTemperatuur(){

        if (klimaatDataNu[plantenBakNummer][TEMPERATUUR] > klimaatDataNu[plantenBakNummer][DAGTEMPERATUUR]) {
            if (!ventilatorIsAan) {
                digitalWrite(ventilatorPin, HIGH);
                Serial.print("vernevelaar aan (temperatuur)");
                ventilatorIsAan = true;
            }
            if (lampIsAan2) {
                digitalWrite(lampenPin2, LOW);
                Serial.println("lampen2 uit (temperatuur)");
                lampIsAan2 = false;
            }
            if (!vernevelaarIsAan) {
                digitalWrite(nevelPin, HIGH);
                Serial.print("vernevelaar aan (temperatuur)");
                vernevelaarIsAan = true;
            }
        }
        if (klimaatDataNu[plantenBakNummer][TEMPERATUUR] < klimaatDataNu[plantenBakNummer][DAGTEMPERATUUR]) {
            if (ventilatorIsAan && !dauw) {
                digitalWrite(ventilatorPin, LOW);
                Serial.print("vernevelaar uit (temperatuur)");
                ventilatorIsAan = false;
            }
            if (!lampIsAan2 && !regen && dag) {
                digitalWrite(lampenPin2, HIGH);
                Serial.println("lampen2 aan (temperatuur)");
                lampIsAan2 = true;
            }
            if (vernevelaarIsAan && !dauw && !regen && !luchtIsDroog) {
                digitalWrite(nevelPin, LOW);
                Serial.print("vernevelaar uit (temperatuur)");
                vernevelaarIsAan = false;
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

class Klok {
    
    RtcDS3231<TwoWire> Rtc;
    KlimaatDataLogger klimaatDataLoggerKlok;
    
    public:
    Klok() : 
        Rtc(Wire),
        klimaatDataLoggerKlok()
    { }

    String setup () {
        
        Serial.print("compiled: ");
        Serial.println(__DATE__);
        Serial.println(__TIME__);
        

        //--------RTC SETUP ------------
        // if you are using ESP-01 then uncomment the line below to reset the pins to
        // the available pins for SDA, SCL
        // Wire.begin(0, 2); // due to limited pins, use pin 0 and 2 for SDA, SCL
        
        Rtc.Begin();

        RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
        Serial.print("Dit is de gecompileerde tijd: ");
        printDateTime(compiled);
        Serial.println();

        if (!Rtc.IsDateTimeValid())
        {
            if (Rtc.LastError() != 0)
            {
                // we have a communications error
                // see https://www.arduino.cc/en/Reference/WireEndTransmission for 
                // what the number means
                Serial.print("RTC communications error = ");
                Serial.println(Rtc.LastError());
            }
            else
            {
                // Common Causes:
                //    1) first time you ran and the device wasn't running yet
                //    2) the battery on the device is low or even missing

                Serial.println("RTC lost confidence in the DateTime!");

                // following line sets the RTC to the date & time this sketch was compiled
                // it will also reset the valid flag internally unless the Rtc device is
                // having an issue

                Rtc.SetDateTime(compiled);
            }
        }

        if (!Rtc.GetIsRunning())
        {
            Serial.println("RTC was not actively running, starting now");
            Rtc.SetIsRunning(true);
        }

        RtcDateTime now = Rtc.GetDateTime();
        if (now < compiled) 
        {
            Serial.println("RTC is older than compile time!  (Updating DateTime)");
            Rtc.SetDateTime(compiled);
        }
        else if (now > compiled) 
        {
            Serial.println("RTC is newer than compile time. (this is expected)");
        }
        else if (now == compiled) 
        {
            Serial.println("RTC is the same as compile time! (not expected but all is fine)");
        }

        // never assume the Rtc was last configured by you, so
        // just clear them to your needed state
        Rtc.Enable32kHzPin(false);
        Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 
        Serial.println("Klok geinitieerd");
        String dateTime = geefDatumString(now);
        return dateTime;
    }

    RtcDateTime getTime(){
        if (!Rtc.IsDateTimeValid()) 
        {
            if (Rtc.LastError() != 0)
                {
            // we have a communications error
            // see https://www.arduino.cc/en/Reference/WireEndTransmission for 
            // what the number means
            Serial.print("RTC communications error = ");
            Serial.println(Rtc.LastError());
            }
            else
            {
                // Common Causes:
                //    1) the battery on the device is low or even missing and the power line was disconnected
                Serial.println("RTC lost confidence in the DateTime! Check battery");
            }
        }

        RtcDateTime now = Rtc.GetDateTime();
        Serial.println("Dit is de loop tijd ");
        printDateTime(now);
        Serial.println();

        return(now);
    }
    
    void printDateTime(const RtcDateTime& dt)
    {
        char datestring[20];

        snprintf_P(datestring, 
        countof(datestring),
        PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
        dt.Day(),
        dt.Month(),
        dt.Year(),
        dt.Hour(),
        dt.Minute(),
        dt.Second() );
        Serial.println(datestring);
    }

    String geefDatumTijdString(const RtcDateTime& dt)
    {
        char datestring[17];
        snprintf_P(datestring, 
        countof(datestring),
        PSTR("%02u/%02u/%04u %02u:%02u"),
        dt.Day(),
        dt.Month(),
        dt.Year(),
        dt.Hour(),
        dt.Minute());
        return(datestring);
    }

    String geefDatumString(const RtcDateTime& dt ) {
        char datestring[9];
        snprintf_P(datestring, 
        countof(datestring),
        PSTR("%02u%02u%04u"),
        dt.Day(),
        dt.Month(),
        dt.Year());
        Serial.println(datestring);
        return(datestring);
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

    // int potVochtigheid;
    // int licht;
    // int luchtVochtigheid;
    // int temperatuur; 
    
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
        //klimaatRegelaar.initialisatie();
        Serial.println("plantenbak geinitialseerd");
    }

    void regelKlimaat(RtcDateTime myTime, int platenBakNummer) {
        Serial.print("plantenBakNummer in plantenBak regelKlimaat = ");
        Serial.println(plantenBakNummer);
        int luchtVochtigheid = luchtVochtigheidTemperatuurSensor.readHumidityValue();
        klimaatDataNu[platenBakNummer][LUCHTVOCHTIGHEIDNU] = luchtVochtigheid;
        Serial.print("Humidity: ");
        Serial.print(luchtVochtigheid);
        int temperatuur = luchtVochtigheidTemperatuurSensor.readTempValue();
        klimaatDataNu[platenBakNummer][TEMPERATUUR] = temperatuur;
        Serial.print(" %, Temp: ");
        Serial.print(temperatuur);
        Serial.println(" Celsius");
        Serial.print("Soil Moisture = ");  
        int potVochtigheid = soilHumiditySensor.readValue();
        klimaatDataNu[platenBakNummer][POTVOCHTIGHEID] = potVochtigheid;
        Serial.println(soilHumiditySensor.readValue());
        // Serial.print("Raw = ");
        // Serial.print(lichtSensor.readRawValue());
        Serial.print(" - Lux = ");
        int licht = lichtSensor.readLogValue();
        klimaatDataNu[platenBakNummer][LICHT] = licht;
        Serial.println(lichtSensor.readLogValue());
        klimaatRegelaar.doeJeKlimaatDing(myTime);
    }  
};

class KlimaatDataLogger {
    
    File myFile;
    int csPin = 53;
    String naamFile = "datafile.txt";
    Static vector<int> klimaatGevens;

    public:
    KlimaatDataLogger() :
        myFile()
    {
        pinMode(53, OUTPUT);
    }

    void setup() {
        Serial.begin(9600);
        Serial.print("Initializing card...");
        if (!SD.begin(csPin)) {
            Serial.println("initialization of the SD card failed!");
            return;
        }
        Serial.println("initialization of the SDcard is done.");
        Serial.print("Creating file named: ");
        Serial.print(naamFile);
        myFile = SD.open(naamFile, FILE_WRITE);
    }
    
    void writeToFile(String data) {
        myFile = SD.open(naamFile, FILE_WRITE);
        if (myFile) {
            Serial.print("Writing to the text file...");
            myFile.println(data);
            myFile.close(); // close the file:
            Serial.println("done closing.");
        }   else {
                // if the file didn't open, report an error:
                Serial.println("error opening the text file!");
            }
    }

    void readFromFile() {
        myFile = SD.open(naamFile);
        Serial.println(naamFile);
        if (myFile) {
            Serial.println("datafile.txt bevat het volgende:");
            // read all the text written on the file
            while (myFile.available()) {
                //Serial.write(myFile.read());
                //Serial.end();
                String list = String(myFile.readStringUntil("\n"));
                //Serial.begin();
                Serial.print("De eerste regel van de file");
                Serial.println(list);
                Serial.print("De volgende regel van de file");
            }
            // close the file:
            myFile.close();
        }   else {
                // if the file didn't open, report an error:
                Serial.println("error opening the text file!");
            }
    }

    String maakKlimaatDataString() {
        String a;
        String result;
        for(int plantenbak = 0; plantenbak < 3; plantenbak++) {
            for(int variable = 0; variable < 15; variable++) {
                a = klimaatDataNu[plantenbak][variable];
                result = result + a + ",";
            }
        }
        result = result + "\n";
        return result;
    }
};

class TouchScreen {
    
    UTFT myGLCD;
    URTouch myTouch;
    UTFT_Buttons myButtons;
    
    int x, y;
    char stCurrent[20]="";
    int stCurrentLen=0;
    char stLast[20]=""; 

    public:
    TouchScreen() :
    myGLCD(CTE32_R2,38,39,40,41),
    myTouch( 6, 5, 4, 3, 2),
    myButtons(&myGLCD, &myTouch)
    {}

    void setup() {
        
        myGLCD.InitLCD();
        myGLCD.clrScr();
        myGLCD.setFont(BigFont);
        myTouch.InitTouch();
        myTouch.setPrecision(PREC_MEDIUM);
    // currentPage = '0'; // Indicates that we are at Home Screen
    // selectedUnit = '0'; // Indicates the selected unit for the first example, cms or inches

    }

    void toonStartScherm(String myDatumTijd) {

        myGLCD.clrScr();
        for (int i = 0; i < 3; i++) {
            switch (klimaatDataNu[i][SEIZOEN]) {
                case WINTER :
                    myGLCD.setColor(VGA_BLUE);
                    myGLCD.setBackColor(VGA_BLUE);
                    break;
                case ZOMER :
                    myGLCD.setColor(VGA_RED);
                    myGLCD.setBackColor(VGA_RED);
                    break;
                case REGEN : 
                    myGLCD.setColor(VGA_GRAY);
                    myGLCD.setBackColor(VGA_GRAY);
                    break;
            }
            // int but = myButtons.addButton(2, (i*71)+5, 310, 70, "");
            // myButtons.drawButton(but);
            myGLCD.fillRoundRect(2, (i*71) + 5, 315, (i*71) + 70);
            myGLCD.setColor(VGA_WHITE);
            myGLCD.drawRoundRect (2, (i*71) + 5, 315, (i*71) + 70);
            myGLCD.setFont(SmallFont);
            myGLCD.print(String("Temperature"), 11, (i*71) +6);
            myGLCD.print(String("Humidity"), 11, (i*71) + 39);
            myGLCD.print(String("Soilmoisture"), 150, (i*71) + 6);
            myGLCD.print(String("Light"),150, (i*71) + 39);
            myGLCD.print(String(klimaatDataNu[i][NACHTTEMPERATUUR]) + "/", 11, (i*71) + 21);
            //myGLCD.drawLine(1,31, 315, 31);
            myGLCD.print(String("/") + String(klimaatDataNu[i][DAGTEMPERATUUR]), 90, (i*71) + 21);
            myGLCD.print(String("/") + String(klimaatDataNu[i][LUCHTVOCHTIGHEID]), 59, (i*71) + 53);
            myGLCD.setColor(VGA_YELLOW);
            
            if(klimaatDataNu[i][ISDAUW] == 1) {
                myGLCD.print(String("Dew"), 265, (i*751) + 21);
            }
            if(klimaatDataNu[i][ISREGEN] == 1) {
                myGLCD.print(String("Rain"), 265, (i*71) + 39);
            }
            if(klimaatDataNu[i][ISDAG] == 1) {
                myGLCD.print(String("Day"), 265, (i*71) + 6);
            }   else {
                myGLCD.print(String("Night"), 265, (i*71) + 6);
            }
            if (klimaatDataNu[i][SEIZOEN] == WINTER) {
                myGLCD.print(String("Winter"), 265, (i*71) + 55);
            }   
            if (klimaatDataNu[i][SEIZOEN] == ZOMER) {
                myGLCD.print(String("Summer"), 265, (i*71) + 55);
            } 
            if (klimaatDataNu[i][SEIZOEN] == REGEN) {
                myGLCD.print(String("Wet"), 265, (i*71) + 55);
            }
            myGLCD.setFont(BigFont);
            myGLCD.print(String(klimaatDataNu[i][TEMPERATUUR]), 35, (i*71) + 18);
            myGLCD.print(String("C"), 75, (i*71) + 18);
            myGLCD.print(String(klimaatDataNu[i][LUCHTVOCHTIGHEIDNU]) + "%", 11, (i*71) + 51);
            myGLCD.print(String(klimaatDataNu[i][POTVOCHTIGHEID]) , 150, (i*71) + 18);
            myGLCD.print(String(klimaatDataNu[i][LICHT]), 150, (i*71) + 51);
            myGLCD.setColor(VGA_YELLOW);
            myGLCD.fillCircle(73, (i*71) + 19, 2);
        }
        myGLCD.setColor(VGA_WHITE);
        myGLCD.setBackColor(VGA_BLACK);
        myGLCD.setFont(BigFont);
        myGLCD.print(myDatumTijd, CENTER, 220);
        delay(9000);
    }

void updateStr(int val)
{
  if (stCurrentLen<20)
  {
    stCurrent[stCurrentLen]=val;
    stCurrent[stCurrentLen+1]='\0';
    stCurrentLen++;
    myGLCD.setColor(0, 255, 0);
    myGLCD.print(stCurrent, LEFT, 224);
  }
  else
  {
    myGLCD.setColor(255, 0, 0);
    myGLCD.print("BUFFER FULL!", CENTER, 192);
    delay(500);
    myGLCD.print("            ", CENTER, 192);
    delay(500);
    myGLCD.print("BUFFER FULL!", CENTER, 192);
    delay(500);
    myGLCD.print("            ", CENTER, 192);
    myGLCD.setColor(0, 255, 0);
  }
}

// Draw a red frame while a button is touched
void waitForIt(int x1, int y1, int x2, int y2)
{
  myGLCD.setColor(255, 0, 0);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
  while (myTouch.dataAvailable())
    myTouch.read();
  myGLCD.setColor(255, 255, 255);
  myGLCD.drawRoundRect (x1, y1, x2, y2);
    }

    void drawFrame(int x1, int y1, int x2, int y2) {
        myGLCD.setColor(VGA_YELLOW);
        myGLCD.drawRoundRect (x1, y1, x2, y2);
        while (myTouch.dataAvailable()==true) {}
            myTouch.read();
            myGLCD.setColor(VGA_GREEN);
            myGLCD.drawRoundRect (x1, y1, x2, y2);
        
    }
 
    void kiesPlantenBak() {
        if (myTouch.dataAvailable()) {
            myTouch.read();
            x=myTouch.getX();
            y=myTouch.getY();
            // myGLCD.drawRoundRect (2, (i*71) + 5, 315, (i*71) + 70);
            if ((y>=10) && (y<=70)) { // bovenste bak
                waitForIt(2, 5, 315, 70);
            }
            if ((y>=73) && (y<=143)) {// middelste bak
                waitForIt(2, 76, 315, 141);
            }
            if ((y>=130) && (y<=180)) { // Button: 3
                waitForIt(2, 142, 315, 212);
            }
        }
    }
};

KlimaatDataLogger klimaatDataLogger;
vector<int> KlimaatDataLogger::klimaatGevens;

KlimaatDataLogger klimaatDataLogger;
TouchScreen touchScreen;
Klok klok;

int bakNummer1 = 0;
Plantenbak plantenbak1(pinArray1, bakNummer1);
int bakNummer2 = bakNummer1 + 1;
Plantenbak plantenbak2(pinArray2, bakNummer2);
int bakNummer3 = bakNummer2 + 1;
Plantenbak plantenbak3(pinArray3, bakNummer3);

void setup() {
    Serial.begin(9600);
    klok.setup();
    touchScreen.setup();
    klimaatDataLogger.setup();
    plantenbak1.setup();
    plantenbak2.setup();
    plantenbak3.setup();
}

void loop() {
    RtcDateTime tijd = klok.getTime();
    plantenbak1.regelKlimaat(tijd, bakNummer1);
    plantenbak2.regelKlimaat(tijd, bakNummer2);
    plantenbak3.regelKlimaat(tijd, bakNummer3);
    // Serial.println(klimaatDataNu[0][1]);
    // Serial.println(klimaatDataNu[1][1]);
    // Serial.println(klimaatDataNu[2][1]);
    String datumTijd = klok.geefDatumTijdString(tijd);
    touchScreen.toonStartScherm(datumTijd);
    
    int minuut = tijd.Minute();
    Serial.println(minuut);
    int minuutNu = tijd.Minute();
    while ((minuutNu - minuut) < 1) {
        Serial.print("minuutNu in while loop =");
        Serial.println(minuutNu);
        Serial.println(minuutNu - minuut);
        touchScreen.kiesPlantenBak();
        Serial.println("while loop!!!!!!");
        RtcDateTime nieuweTijd = klok.getTime();
        minuutNu = nieuweTijd.Minute();
        Serial.println(minuutNu);
        Serial.println(minuutNu - minuut);
        
    }
    // String klimaatDataString = klimaatDataLogger.maakKlimaatDataString();
    //Serial.println(klimaatDataString);
    // klimaatDataLogger.writeToFile(klimaatDataString);
    // klimaatDataLogger.readFromFile();
    Serial.println();
    
}

