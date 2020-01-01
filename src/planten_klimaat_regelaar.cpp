#include <Arduino.h>
#include <Wire.h> // must be included here so that Arduino library object file references work (clock)
#include <RtcDS3231.h>

#include <SD.h>

#include <DHT.h>
#include <DHT_U.h>

#include <UTFT.h>
#include <URTouch.h>
#include <UTFT_Buttons.h>                                                                  

#define DAGTEMPERATUUR 0
#define NACHTTEMPERATUUR 1
#define LUCHTVOCHTIGHEID 2
#define WATERGEVEN 3
#define LAMPENVERVANGEN 4
#define DUURDAUW 5
#define DUURREGEN 6
#define DUURDAG 7
#define STARTDAG 8
#define EINDDAG 9
#define STARTDAUW 10
#define STARTREGEN 11
#define EINDREGEN 12
#define SEIZOEN 13
#define VENTILATOR 14
#define VERNEVELAAR 15
#define TEMPERATUUR 16
#define LUCHTVOCHTIGHEIDNU 17
#define POTVOCHTIGHEID 18
#define LICHT 19
#define LAMPENAAN1 20
#define LAMPENAAN2 21
#define JAAR 22
#define MAAND 23
#define DAG 24
#define UUR 25
#define MINUUT 26
#define PLANTENBAKNUMMER 27
#define HOOGSTEPOTVOCHTIGHEID 28
#define MEESTELICHT 29
#define ISDAG 30
#define ISREGEN 31
#define ISDAUW 32

#define WINTER 0
#define ZOMER 1
#define REGEN 2

#define CHIANGMAI 0
#define MANAUS 1
#define SUMATRA 2

//volgende regel is voor tijdweergave klok
#define countof(a) (sizeof(a) / sizeof(a[0]))

extern uint8_t BigFont[];
extern uint8_t SmallFont[];

const int defaultPlantenBakSettings[3][4][12] = {{{25, 14, 60, 20, 75, 3, 0, 11, 8}, {35, 28, 55, 10, 75, 2, 0, 12, 8}, {30, 25, 35, 80, 4, 5, 85, 13, 8}, {0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 0}}, {{8, 21, 20, 14, 4, 0, 70, 20, 75}, {8, 22, 30, 20, 4, 1, 55, 20, 75}, {8, 23, 35, 30, 4, 5, 85, 20, 75}, {1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1}}, {{8, 21, 20, 14, 4, 0, 70, 20, 75}, {8, 22, 30, 20, 4, 1, 55, 20, 75}, {8, 23, 35, 30, 4, 5, 85, 20, 75}, {2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2}}};
int customPlantenBakSettings[3][4][12] = {};

byte pinArray1[8] = {A0, 9, A1, 4, 5, 10, 8, 7}; 
byte pinArray2[8] = {A0, 9, A1, 4, 5, 10, 8, 7};
byte pinArray3[8] = {A0, 9, A1, 4, 5, 10, 8, 7};

int currentPage; //indicates the page that is active
int klimaatDataNu[3][31];

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
    
    float readValue() {
        digitalWrite(powerPin, HIGH);
        delay(10);
        float soilmoisture = analogRead(pin);
        digitalWrite(powerPin, LOW);
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
        int seizoenNu = customPlantenBakSettings[plantenBakNummer][3][(now.Month()-1)];
        klimaatDataNu[plantenBakNummer][JAAR] = now.Year();
        klimaatDataNu[plantenBakNummer][MAAND] = now.Month();
        klimaatDataNu[plantenBakNummer][DAG] = now.Day();
        klimaatDataNu[plantenBakNummer][UUR] = now.Hour();
        klimaatDataNu[plantenBakNummer][MINUUT] = now.Minute();
        klimaatDataNu[plantenBakNummer][PLANTENBAKNUMMER] = plantenBakNummer;
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
                for (int i = 0; i < 7; i++) {
                    klimaatDataNu[plantenBakNummer][i] = customPlantenBakSettings[plantenBakNummer][WINTER][i];
                }
                klimaatDataNu[plantenBakNummer][SEIZOEN] = WINTER;
                break;
            case ZOMER:
                for (int i = 0; i < 7; i++) {
                    klimaatDataNu[plantenBakNummer][i] = customPlantenBakSettings[plantenBakNummer][ZOMER][i];
            }
                klimaatDataNu[plantenBakNummer][SEIZOEN] = ZOMER;
                break;
            case REGEN:
                for (int i = 0; i < 7; i++) {
                    klimaatDataNu[plantenBakNummer][i] = customPlantenBakSettings[plantenBakNummer][REGEN][i];
                }
                klimaatDataNu[plantenBakNummer][SEIZOEN] = REGEN;
                break;
        }       
        float eindDag = klimaatDataNu[plantenBakNummer][STARTDAG] + uurMinuutNu <= klimaatDataNu[plantenBakNummer][DUURDAG];
        klimaatDataNu[plantenBakNummer][EINDDAG] = eindDag;
        if (uurMinuutNu >= klimaatDataNu[plantenBakNummer][STARTDAG] && uurMinuutNu <= eindDag) {
            isDag = true;
        }   else {
                isDag = false;
            }
        klimaatDataNu[plantenBakNummer][ISDAG] = isDag;
        float startDauw = klimaatDataNu[plantenBakNummer][STARTDAG] - klimaatDataNu[plantenBakNummer][DUURDAUW];
        klimaatDataNu[plantenBakNummer][STARTDAUW] = startDauw;
        if (uurMinuutNu >= startDauw && uurMinuutNu <= klimaatDataNu[plantenBakNummer][STARTDAG]) {
            isDauw = true;
        }   else {
                isDauw = false;
            }
        klimaatDataNu[plantenBakNummer][ISDAUW] = isDauw;
        float startBewolking = klimaatDataNu[plantenBakNummer][STARTDAG] + ((klimaatDataNu[plantenBakNummer][DUURDAG] - klimaatDataNu[plantenBakNummer][DUURREGEN]) / 2);
        float eindBewolking = startBewolking + klimaatDataNu[plantenBakNummer][DUURREGEN];
        klimaatDataNu[plantenBakNummer][STARTREGEN] = startBewolking;
        klimaatDataNu[plantenBakNummer][EINDREGEN] = eindBewolking;
        
        if (uurMinuutNu >= startBewolking && uurMinuutNu <= eindBewolking) {
            isRegen = true;
        }   else {
                isRegen = false;
            }
        klimaatDataNu[plantenBakNummer][ISREGEN] = isRegen;
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
            if (!vernevelaarIsAan && (klimaatDataNu[plantenBakNummer][TEMPERATUUR] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR])) {
                digitalWrite(nevelPin, HIGH);
                Serial.println("vernevelaar aan (regenwolken)");
                vernevelaarIsAan = true;
                klimaatDataNu[plantenBakNummer][VERNEVELAAR] = vernevelaarIsAan;
                regen = true;
            }    
            if (lampIsAan2) {
                digitalWrite(lampenPin2, LOW);
                Serial.println("lampen2 uit (regenwolken)");
                lampIsAan2 = false;
                klimaatDataNu[plantenBakNummer][LAMPENAAN2] = lampIsAan2;
            }
        }

        if (regen && (!klimaatDataNu[plantenBakNummer][ISREGEN]||klimaatDataNu[plantenBakNummer][TEMPERATUUR] < klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR])) {
            regen = false;
            if (vernevelaarIsAan && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID]) {
                digitalWrite(nevelPin, LOW); 
                Serial.println("vernevelaar uit (regenwolken)"); 
                regen = false;
                vernevelaarIsAan = false;
                klimaatDataNu[plantenBakNummer][VERNEVELAAR] = vernevelaarIsAan;
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
            if (!vernevelaarIsAan && klimaatDataNu[plantenBakNummer][TEMPERATUUR] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR]) {
                digitalWrite(nevelPin, HIGH);
                Serial.println("vernevelaar aan (dauw)");
                vernevelaarIsAan = true;
                dauw = true;
                klimaatDataNu[plantenBakNummer][VERNEVELAAR] = vernevelaarIsAan;
                }    
            if (!ventilatorIsAan && klimaatDataNu[plantenBakNummer][TEMPERATUUR] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR]) {
                digitalWrite(ventilatorPin, HIGH);
                Serial.println("ventilator aan (dauw)");
                ventilatorIsAan = true;
                klimaatDataNu[plantenBakNummer][VENTILATOR] = ventilatorIsAan;
                }
        }

        if (dauw && (klimaatDataNu[plantenBakNummer][ISDAUW]|| klimaatDataNu[plantenBakNummer][TEMPERATUUR] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR])) {
            if (vernevelaarIsAan && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID]) {
                digitalWrite(nevelPin, LOW); 
                Serial.println("vernevelaar uit (dauw)"); 
                vernevelaarIsAan = false;
                klimaatDataNu[plantenBakNummer][VERNEVELAAR] = vernevelaarIsAan;
            }  
            if (ventilatorIsAan) {
                digitalWrite(ventilatorPin, LOW); 
                Serial.println("ventilator uit (dauw)"); 
                ventilatorIsAan = false;
                klimaatDataNu[plantenBakNummer][VENTILATOR] = ventilatorIsAan;
            }
        dauw = false;
        }
    }

    void regelVochtigheid() {

        if (!luchtIsDroog && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] < klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID] && klimaatDataNu[plantenBakNummer][TEMPERATUUR] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR]) {
            luchtIsDroog = true;
            if (!vernevelaarIsAan) {
                digitalWrite(nevelPin, HIGH);
                Serial.println("vernevelaar aan (luchtvochtigheid)");
                vernevelaarIsAan = true;
                klimaatDataNu[plantenBakNummer][VERNEVELAAR] = vernevelaarIsAan;
            } 
        }
        if (vernevelaarIsAan && !dauw && !regen && (klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU]  > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID] || klimaatDataNu[plantenBakNummer][TEMPERATUUR] < klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR] || klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > 100)) {
            luchtIsDroog = false;
            digitalWrite(nevelPin, LOW);
            Serial.print("vernevelaar uit (luchtvochtigheid)");
            vernevelaarIsAan = false;
            klimaatDataNu[plantenBakNummer][VERNEVELAAR] = vernevelaarIsAan;
        }
        if (luchtIsDroog && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID] ) { 
            luchtIsDroog = false;    
        }
        if (klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] > klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEID] && klimaatDataNu[plantenBakNummer][TEMPERATUUR] > klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR]) {
            luchtIsDroog = false;
            if (!ventilatorIsAan) {
                digitalWrite(ventilatorPin, HIGH);
                Serial.println("ventilator aan (luchtvochtigheid)");
                ventilatorIsAan = true;
                klimaatDataNu[plantenBakNummer][VENTILATOR] = ventilatorIsAan;
            } 
        }
    }

    void regelTemperatuur(){

        if (klimaatDataNu[plantenBakNummer][TEMPERATUUR] > klimaatDataNu[plantenBakNummer][DAGTEMPERATUUR]) {
            if (!ventilatorIsAan) {
                digitalWrite(ventilatorPin, HIGH);
                Serial.print("vernevelaar aan (temperatuur)");
                ventilatorIsAan = true;
                klimaatDataNu[plantenBakNummer][VENTILATOR] = ventilatorIsAan;
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
                klimaatDataNu[plantenBakNummer][VERNEVELAAR] = vernevelaarIsAan;
            }
        }
        if (klimaatDataNu[plantenBakNummer][TEMPERATUUR] < klimaatDataNu[plantenBakNummer][DAGTEMPERATUUR]) {
            if (ventilatorIsAan && !dauw) {
                digitalWrite(ventilatorPin, LOW);
                Serial.print("vernevelaar uit (temperatuur)");
                ventilatorIsAan = false;
                klimaatDataNu[plantenBakNummer][VENTILATOR] = ventilatorIsAan;
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
                klimaatDataNu[plantenBakNummer][VERNEVELAAR] = vernevelaarIsAan;
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

class KlimaatDataLogger {
    
    File myFile;
    int csPin = 53;
    String naamFile = "datafile.txt";
    
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
            myFile.println(data);
            int size = myFile.size();
            Serial.print("FileSize = ");
            Serial.println(size);
            myFile.close();
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
        static int metingNummer = 100;
        String a;
        String result;
        for(int plantenbak = 0; plantenbak < 3; plantenbak++) {
            for(int variable = 0; variable < 30; variable++) {
                a = klimaatDataNu[plantenbak][variable];
                result = result + a + ",";
            }
        }
        result = result + metingNummer +"\n";
        metingNummer = metingNummer + 1;
        return result;
    }
};

class Klok {
    
    RtcDS3231<TwoWire> Rtc;
        
    public:
    Klok() : 
        Rtc(Wire)
    {}

    void setup () {
        
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
        //return dateTime;
    }

    RtcDateTime getTime(){
        if (!Rtc.IsDateTimeValid()) 
        {
            if (Rtc.LastError() != 0) {
            // we have a communications error
            // see https://www.arduino.cc/en/Reference/WireEndTransmission for 
            // what the number means
            Serial.print("RTC communications error = ");
            Serial.println(Rtc.LastError());
            }   else {
                // Common Causes:
                //    1) the battery on the device is low or even missing and the power line was disconnected
                Serial.println("RTC lost confidence in the DateTime! Check battery");
            }
        }
        RtcDateTime now = Rtc.GetDateTime();
        return(now);
    }
    
    void printDateTime(const RtcDateTime& dt) {
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

    void regelKlimaat(RtcDateTime myTime, int plantenBakNummer) {
        Serial.print("plantenBakNummer in plantenBak regelKlimaat = ");
        Serial.println(plantenBakNummer);
        int luchtVochtigheid = luchtVochtigheidTemperatuurSensor.readHumidityValue();
        klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] = luchtVochtigheid;
        Serial.print("Humidity = ");
        Serial.print(luchtVochtigheid);
        int temperatuur = luchtVochtigheidTemperatuurSensor.readTempValue();
        klimaatDataNu[plantenBakNummer][TEMPERATUUR] = temperatuur;
        Serial.print(" %, Temp + ");
        Serial.print(temperatuur);
        Serial.println(" Celsius");
        Serial.println(soilHumiditySensor.readValue());
        int potVochtigheid = soilHumiditySensor.readValue();
        if(potVochtigheid > maxPotVocht) {
            maxPotVocht = potVochtigheid;
            klimaatDataNu[plantenBakNummer][HOOGSTEPOTVOCHTIGHEID] = potVochtigheid;
        }
        klimaatDataNu[plantenBakNummer][POTVOCHTIGHEID] = (potVochtigheid/maxPotVocht)*99; //100% past niet op display
        Serial.print("Soil Moisture = ");  
        Serial.println((potVochtigheid/maxPotVocht)*100);//100% past niet op display
        int licht = lichtSensor.readLogValue();
        if(licht > maxLicht) {
            maxLicht = licht;
            klimaatDataNu[plantenBakNummer][MEESTELICHT] = licht;
        }
        klimaatDataNu[plantenBakNummer][LICHT] = (licht/maxLicht)*99;//100% past niet op display
        Serial.print("Licht = ");
        Serial.println((licht/maxLicht)*99); //100% past niet op display
        klimaatRegelaar.doeJeKlimaatDing(myTime);
    }  
};

class TouchScreen {
    
    UTFT myGLCD;
    URTouch myTouch;
    UTFT_Buttons myButtons;
    
    int x, y;
    char stCurrent[2]="";
    int stCurrentLen=0;
    char stLast[2]=""; 

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
        currentPage = 1; // Indicates wich screen is active
    }

    void toonStartScherm(String myDatumTijd) {
        Serial.println("begin toonstartscherm");
        currentPage = 1;
        Serial.print("currentPage = ");
        Serial.println(currentPage);
        myGLCD.clrScr();
        int seizoen;
        for (int i = 0; i < 3; i++) {
            switch (klimaatDataNu[i][SEIZOEN]) {
                case WINTER :
                    myGLCD.setColor(VGA_BLUE);
                    myGLCD.setBackColor(VGA_BLUE);
                    seizoen = WINTER;
                    break;
                case ZOMER :
                    myGLCD.setColor(VGA_RED);
                    myGLCD.setBackColor(VGA_RED);
                    seizoen = ZOMER;
                    break;
                case REGEN : 
                    myGLCD.setColor(VGA_GRAY);
                    myGLCD.setBackColor(VGA_GRAY);
                    seizoen = REGEN;
                    break;
            }
            myGLCD.fillRoundRect(2, (i*71) + 2, 318, (i*71) + 71);
            myGLCD.setColor(VGA_WHITE);
            myGLCD.drawRoundRect(2, (i*71) + 2, 318, (i*71) + 71);
            myGLCD.setFont(SmallFont);
            myGLCD.print(String("Temperature"), 6, (i*71) +6);
            myGLCD.print(String("Humidity"), 6, (i*71) + 39);
            myGLCD.print(String("Soil"), 140, (i*71) + 6);
            myGLCD.print(String("Light"),140, (i*71) + 39);
            myGLCD.print(String(customPlantenBakSettings[i][seizoen][NACHTTEMPERATUUR]) + "/", 6, (i*71) + 21);
            myGLCD.print(String("/") + String(customPlantenBakSettings[i][seizoen][DAGTEMPERATUUR]), 76, (i*71) + 21);
            myGLCD.print(String("/") + String(customPlantenBakSettings[i][seizoen][LUCHTVOCHTIGHEID]), 53, (i*71) + 53);
            myGLCD.print(String("/") + String(customPlantenBakSettings[i][seizoen][WATERGEVEN]), 190, (i*71) + 21);
            myGLCD.print(String("/") + String(customPlantenBakSettings[i][seizoen][LAMPENVERVANGEN]), 190, (i*71) + 53);
            myGLCD.setColor(VGA_YELLOW);
            if(klimaatDataNu[i][ISDAUW] == 1) {
                myGLCD.print(String("Dew"), 265, (i*71) + 21);
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
            myGLCD.setColor(VGA_YELLOW);
            myGLCD.print(String(klimaatDataNu[i][TEMPERATUUR]), 28, (i*71) + 18);
            myGLCD.print(String("C"), 60, (i*71) + 18);
            myGLCD.fillCircle(61, (i*71) + 19, 2);
            myGLCD.print(String(klimaatDataNu[i][LUCHTVOCHTIGHEIDNU]) + "%", 5, (i*71) + 51);
            myGLCD.print(String(klimaatDataNu[i][POTVOCHTIGHEID]) + "%", 140, (i*71) + 18);
            myGLCD.print(String(klimaatDataNu[i][LICHT]) + "%", 140, (i*71) + 51);
        }
        myGLCD.setColor(VGA_WHITE);
        myGLCD.setBackColor(VGA_BLACK);
        myGLCD.setFont(BigFont);
        myGLCD.print(myDatumTijd, CENTER, 220);
    }

    void tekenSettingsOverzicht(int bak) {
        myGLCD.clrScr();
        for (int i = 0; i < 3; i++) {
            if(i == 0) {
                myGLCD.setColor(VGA_WHITE);
                myGLCD.setFont(BigFont);
                myGLCD.print("WINTER", 205, 51 + (i*71));
                myGLCD.setColor(VGA_BLUE);
                myGLCD.setBackColor(VGA_BLUE);
            }  
            if(i == 1) { 
                myGLCD.setColor(VGA_WHITE);
                myGLCD.setFont(BigFont);
                myGLCD.print("SUMMER", 205, 51 + (i*71));
                myGLCD.setColor(VGA_RED);
                myGLCD.setBackColor(VGA_RED);
            }
            if(i == 2) { 
                myGLCD.setColor(VGA_WHITE);
                myGLCD.setFont(BigFont);
                myGLCD.print("RAIN", 221, 51 + (i*71));
                myGLCD.setColor(VGA_GRAY);
                myGLCD.setBackColor(VGA_GRAY);
            }
            myGLCD.fillRoundRect(1, (i*71) + 1, 318, (i*71) + 71);
            myGLCD.setColor(VGA_WHITE);
            myGLCD.drawRoundRect (1, (i*71) + 1, 318, (i*71) + 71);
            myGLCD.setFont(SmallFont);
            myGLCD.print(String("Temperature"), 6, (i*71) + 2);
            myGLCD.print(String("Humidity"), 6, (i*71) + 43);
            myGLCD.print(String("Soil"), 140, (i*71) + 2);
            myGLCD.print(String("Light"),140, (i*71) + 43);
            myGLCD.print(String("Daylight"), 200, (i*71) + 2);
            myGLCD.print(String("Dew"), 260, (i*71) + 43);
            myGLCD.print(String("Clouds"),200, (i*71) + 43);
            myGLCD.setFont(SmallFont);
            myGLCD.print(String("min"), 6, (i*71) + 16);
            myGLCD.print(String("max"), 63, (i*71) + 16);
            myGLCD.print(String("start"), 232, (i*71) + 16);
            myGLCD.print(String("average"), 6, (i*71) + 58);
            myGLCD.setFont(BigFont);
            myGLCD.setColor(VGA_YELLOW);
            myGLCD.print(String(customPlantenBakSettings[bak][i][NACHTTEMPERATUUR]), 29, (i*71) + 13);
            myGLCD.print(String(customPlantenBakSettings[bak][i][DAGTEMPERATUUR]), 85, (i*71) + 13);
            myGLCD.print(String(customPlantenBakSettings[bak][i][DUURDAG]), 200, (i*71) + 13);
            myGLCD.print(String(customPlantenBakSettings[bak][i][STARTDAG])+ String("H"), 270, (i*71) + 13);
            myGLCD.print(String("C"), 115, (i*71) + 13);
            myGLCD.fillCircle(116, (i*71) + 15, 2);
            myGLCD.print(String(customPlantenBakSettings[bak][i][LUCHTVOCHTIGHEID]) + "%", 60, (i*71) + 55);
            myGLCD.print(String(customPlantenBakSettings[bak][i][WATERGEVEN]) + "%", 140, (i*71) + 13);
            myGLCD.print(String(customPlantenBakSettings[bak][i][LAMPENVERVANGEN]) + "%", 140, (i*71) + 55);
            myGLCD.setFont(SmallFont);
            for(int j = 0; j <12; j++) {
                myGLCD.setFont(SmallFont);
                switch (customPlantenBakSettings[bak][3][j]) {
                case WINTER :
                    myGLCD.setColor(0,0,225);
                    myGLCD.setBackColor(0,0,225);
                    myGLCD.fillRoundRect((j*26) + 4, (i*71) + 29, (j*26) + 30, (i*71) + 42);
                    myGLCD.setColor(VGA_WHITE);
                    if(j== 0)
                        myGLCD.print("jan", 6, (i*71) + 29);
                    if(j==1) 
                        myGLCD.print("feb", 32, (i*71) + 29);
                    if(j==2) 
                        myGLCD.print("mar", 58, (i*71) + 29);
                    if(j==3)
                        myGLCD.print("apr", 84, (i*71) + 29);
                    if(j==4)
                        myGLCD.print("may", 110, (i*71) + 29);
                    if(j==5)
                        myGLCD.print("jun", 136, (i*71) + 29);
                    if(j==6)
                        myGLCD.print("jul", 162, (i*71) + 29);
                    if(j==7)
                        myGLCD.print("aug", 188, (i*71) + 29);
                    if(j==8)
                        myGLCD.print("sep", 214, (i*71) + 29);
                    if(j==9)
                        myGLCD.print("oct", 240, (i*71) + 29);
                    if(j==10)
                        myGLCD.print("nov", 266, (i*71) + 29);
                    if(j==11)
                        myGLCD.print("dec", 292, (i*71) + 29);
                    break;
                case ZOMER :
                    myGLCD.setColor(235,0,0);
                    myGLCD.setBackColor(235,0,0);
                    myGLCD.fillRoundRect((j*26) + 4, (i*71) + 29, (j*26) + 30, (i*71) + 42);
                    myGLCD.setColor(VGA_WHITE);
                    if(j== 0) 
                        myGLCD.print("jan", 6, (i*71) + 29);
                    if(j==1)
                        myGLCD.print("feb", 32, (i*71) + 29);
                    if(j==2)
                        myGLCD.print("mar", 58, (i*71) + 29);
                    if(j==3)
                        myGLCD.print("apr", 84, (i*71) + 29);
                    if(j==4)
                        myGLCD.print("may", 110, (i*71) + 29);
                    if(j==5)
                        myGLCD.print("jun", 136, (i*71) + 29);
                    if(j==6)
                        myGLCD.print("jul", 162, (i*71) + 29);
                    if(j==7)
                        myGLCD.print("aug", 188, (i*71) + 29);
                    if(j==8)
                        myGLCD.print("sep", 214, (i*71) + 29);
                    if(j==9)
                        myGLCD.print("oct", 240, (i*71) + 29);
                    if(j==10)
                        myGLCD.print("nov", 266, (i*71) + 29);
                    if(j==11)
                        myGLCD.print("dec", 292, (i*71) + 29);
                    break;
                case REGEN : 
                    myGLCD.setColor(VGA_SILVER);
                    myGLCD.setBackColor(VGA_SILVER);
                    myGLCD.fillRoundRect((j*26) + 4, (i*71) + 29, (j*26) + 30, (i*71) + 42);
                    myGLCD.setColor(VGA_WHITE);
                    if(j== 0)
                        myGLCD.print("jan", 6, (i*71) + 29);
                    if(j==1)
                        myGLCD.print("feb", 32, (i*71) + 29);
                    if(j==2)
                        myGLCD.print("mar", 58, (i*71) + 29);
                    if(j==3)
                        myGLCD.print("apr", 84, (i*71) + 29);
                    if(j==4)
                        myGLCD.print("may", 110, (i*71) + 29);
                    if(j==5)
                        myGLCD.print("jun", 136, (i*71) + 29);
                    if(j==6)
                        myGLCD.print("jul", 162, (i*71) + 29);
                    if(j==7)
                        myGLCD.print("aug", 188, (i*71) + 29);
                    if(j==8)
                        myGLCD.print("sep", 214, (i*71) + 29);
                    if(j==9)
                        myGLCD.print("oct", 240, (i*71) + 29);
                    if(j==10)
                        myGLCD.print("nov", 266, (i*71) + 29);
                    if(j==11)
                        myGLCD.print("dec", 292, (i*71) + 29);
                    break;
                }
                myGLCD.setColor(VGA_WHITE);
                myGLCD.drawRoundRect((j*26) + 4, (i*71) + 29, (j*26) + 30, (i*71) + 42);
            }          
        } 
    }

    void tekenSettingsManipulatieScherm (int bak) {
        tekenSettingsOverzicht(bak);
        int variable;
        currentPage = 3;
    Serial.print("currentPage = ");
    Serial.println(currentPage);
        myGLCD.setColor(VGA_BLACK);
        myGLCD.setBackColor(VGA_BLACK);
        myGLCD.fillRoundRect(2, 215, 80, 238);
        myGLCD.fillRoundRect(82, 215, 160, 238);
        myGLCD.fillRoundRect(162, 215, 240, 238);
        myGLCD.fillRoundRect(242, 215, 318, 238);
        myGLCD.setColor(VGA_WHITE);
        myGLCD.drawRoundRect(2, 215, 80, 238);
        myGLCD.drawRoundRect(82, 215, 160, 238);
        myGLCD.drawRoundRect(162, 215, 240, 238);
        myGLCD.drawRoundRect(242, 215, 318, 238);
        myGLCD.print("Back", 266, 221);
        myGLCD.print("ChiangMai", 5, 221);
        myGLCD.print("Mai", 58, 221);
        myGLCD.print("Manaus", 99, 221);
        myGLCD.print("Sumatra", 175, 221);
        while (currentPage == 3) {
            if (myTouch.dataAvailable()) {
                myTouch.read();
                x=myTouch.getX();
                y=myTouch.getY();
                if ((y>=210) && (y<=240)) {
                    if ((x>=0) && (x<=80)) {
                        betast(2, 215, 80, 238);
                        for(int i=0; i<4; i++) {
                            for(int j=0; j<12; j++) {
                                customPlantenBakSettings[bak][i][j] = defaultPlantenBakSettings[CHIANGMAI][i][j];
                            }
                        }
                        tekenSettingsManipulatieScherm(bak);
                        //toonStartScherm("changmai");
                    }
                    if ((x>=80) && (x<=160)) {
                        betast(2, 215, 80, 238);
                        for(int i=0; i<3; i++) {
                            for(int j=0; j<12; j++) {
                                customPlantenBakSettings[bak][i][j] = defaultPlantenBakSettings[MANAUS][i][j];
                            }
                        }
                        //toonStartScherm("minaus");
                        tekenSettingsManipulatieScherm(bak);
                    }
                    if ((x>=160) && (x<=240)) {
                        betast(2, 215, 80, 238);
                        for(int i=0; i<3; i++) {
                            for(int j=0; j<12; j++) {
                                customPlantenBakSettings[bak][i][j] = defaultPlantenBakSettings[SUMATRA][i][j];
                            }
                        }
                        tekenSettingsManipulatieScherm(bak);
                    }
                    if ((x>=240) && (x<=320)) {
                        //break;
                        betast(242, 215, 318, 238);
                        String test = "nogmaals";
                        toonStartScherm(test);
                        currentPage = 1;
                    }
                }
                for(int i=0;i<3;i++) {
                    int seizoen = i;
                    if ((y<=(i*71) + 29) && (y>=(i*71) + 13) && (x>=29 && x<=61)) { 
                    drawButtons();
                    variable = NACHTTEMPERATUUR;
                    leesGetal(bak, seizoen, variable);
                    }
                    if ((y<=(i*71) + 29) && (y>=(i*71) + 13) && (x>=85 && x<=117)) { 
                    drawButtons();
                    variable = DAGTEMPERATUUR;
                    leesGetal(bak, seizoen, variable);
                    }
                    if ((y<=(i*71) + 29) && (y>=(i*71) + 13) && (x>=200 && x<=232)) { 
                    drawButtons();
                    variable = DUURDAG;
                    leesGetal(bak, seizoen, variable);
                    }
                    if ((y<=(i*71) + 29) && (y>=(i*71) + 13) && (x>=270 && x<=338)) { 
                    drawButtons();
                    variable = STARTDAG;
                    leesGetal(bak, seizoen, variable);
                    }
                    if ((y<=(i*71) + 29) && (y>=(i*71) + 13) && (x>=140 && x<=172)) { 
                    drawButtons();
                    variable = WATERGEVEN;
                    leesGetal(bak, seizoen, variable);
                    }
                    if ((y<=(i*71) + 71) && (y>=(i*71) + 55) && (x>=60 && x<=92)) { 
                    drawButtons();
                    variable = LUCHTVOCHTIGHEID;
                    leesGetal(bak, seizoen, variable);
                    }
                }
                int k = 0;
                for(int j = 0; j <12; j++) {
                    if (y>=(k*71) + 29 && y<=(k*71) + 42 && x>=(j*26) + 4 && x<=(j*26) + 30) { 
                        Serial.print("maand geselecteerd");
                        betast((j*26) + 4, (k*71) + 29, (j*26) + 30, (k*71) + 42);
                        customPlantenBakSettings[bak][3][j] = k;
                        myGLCD.setColor(0,0,225);
                        myGLCD.setBackColor(0,0,225);
                        for (int i = 0; i < 3; i++) {
                            myGLCD.fillRoundRect((j*26) + 4, (i*71) + 29, (j*26) + 30, (i*71) + 42);
                            myGLCD.setColor(VGA_WHITE);
                            if(j== 0)
                                myGLCD.print("jan", 6, (i*71) + 29);
                            if(j==1) 
                                myGLCD.print("feb", 32, (i*71) + 29);
                            if(j==2) 
                                myGLCD.print("mar", 58, (i*71) + 29);
                            if(j==3)
                                myGLCD.print("apr", 84, (i*71) + 29);
                            if(j==4)
                                myGLCD.print("may", 110, (i*71) + 29);
                            if(j==5)
                                myGLCD.print("jun", 136, (i*71) + 29);
                            if(j==6)
                                myGLCD.print("jul", 162, (i*71) + 29);
                            if(j==7)
                                myGLCD.print("aug", 188, (i*71) + 29);
                            if(j==8)
                                myGLCD.print("sep", 214, (i*71) + 29);
                            if(j==9)
                                myGLCD.print("oct", 240, (i*71) + 29);
                            if(j==10)
                                myGLCD.print("nov", 266, (i*71) + 29);
                            if(j==11)
                                myGLCD.print("dec", 292, (i*71) + 29);
                        
                        }
                    }
                }
                k = 1;
                for(int j = 0; j <12; j++) {
                    if (y>=(k*71) + 29 && y<=(k*71) + 42 && x>=(j*26) + 4 && x<=(j*26) + 30) { 
                        Serial.print("maand geselecteerd");
                        betast((j*26) + 4, (k*71) + 29, (j*26) + 30, (k*71) + 42);
                        customPlantenBakSettings[bak][3][j] = k;
                        myGLCD.setColor(235,0,0);
                        myGLCD.setBackColor(235,0,0);
                        for (int i = 0; i < 3; i++) {
                            myGLCD.fillRoundRect((j*26) + 4, (i*71) + 29, (j*26) + 30, (i*71) + 42);
                            myGLCD.setColor(VGA_WHITE);
                            if(j== 0) 
                                myGLCD.print("jan", 6, (i*71) + 29);
                            if(j==1)
                                myGLCD.print("feb", 32, (i*71) + 29);
                            if(j==2)
                                myGLCD.print("mar", 58, (i*71) + 29);
                            if(j==3)
                                myGLCD.print("apr", 84, (i*71) + 29);
                            if(j==4)
                                myGLCD.print("may", 110, (i*71) + 29);
                            if(j==5)
                                myGLCD.print("jun", 136, (i*71) + 29);
                            if(j==6)
                                myGLCD.print("jul", 162, (i*71) + 29);
                            if(j==7)
                                myGLCD.print("aug", 188, (i*71) + 29);
                            if(j==8)
                                myGLCD.print("sep", 214, (i*71) + 29);
                            if(j==9)
                                myGLCD.print("oct", 240, (i*71) + 29);
                            if(j==10)
                                myGLCD.print("nov", 266, (i*71) + 29);
                            if(j==11)
                                myGLCD.print("dec", 292, (i*71) + 29);
                        }
                    }
                }
                k = 2;
                for(int j = 0; j <12; j++) {
                    if (y>=(k*71) + 29 && y<=(k*71) + 42 && x>=(j*26) + 4 && x<=(j*26) + 30) { 
                        Serial.print("maand geselecteerd");
                        betast((j*26) + 4, (k*71) + 29, (j*26) + 30, (k*71) + 42);
                        customPlantenBakSettings[bak][3][j] = k;
                        myGLCD.setColor(VGA_SILVER);
                        myGLCD.setBackColor(VGA_SILVER);
                        for (int i = 0; i < 3; i++) {
                            myGLCD.fillRoundRect((j*26) + 4, (i*71) + 29, (j*26) + 30, (i*71) + 42);
                            myGLCD.setColor(VGA_WHITE);
                            if(j== 0)
                                myGLCD.print("jan", 6, (i*71) + 29);
                            if(j==1)
                                myGLCD.print("feb", 32, (i*71) + 29);
                            if(j==2)
                                myGLCD.print("mar", 58, (i*71) + 29);
                            if(j==3)
                                myGLCD.print("apr", 84, (i*71) + 29);
                            if(j==4)
                                myGLCD.print("may", 110, (i*71) + 29);
                            if(j==5)
                                myGLCD.print("jun", 136, (i*71) + 29);
                            if(j==6)
                                myGLCD.print("jul", 162, (i*71) + 29);
                            if(j==7)
                                myGLCD.print("aug", 188, (i*71) + 29);
                            if(j==8)
                                myGLCD.print("sep", 214, (i*71) + 29);
                            if(j==9)
                                myGLCD.print("oct", 240, (i*71) + 29);
                            if(j==10)
                                myGLCD.print("nov", 266, (i*71) + 29);
                            if(j==11)
                                myGLCD.print("dec", 292, (i*71) + 29);
                        }
                    }
                }
            }
        }        
    }       
    

// myGLCD.print(String(plantenBakSettings1[bak][i][LUCHTVOCHTIGHEID]) + "%", 60, (i*71) + 55);
// myGLCD.print(String(plantenBakSettings1[bak][i][WATERGEVEN]) + "%", 140, (i*71) + 13);
// myGLCD.print(String(plantenBakSettings1[bak][i][LAMPENVERVANGEN]) + "%", 140, (i*71) + 55);
    void tekenSettingsScherm(int bak) {
         tekenSettingsOverzicht(bak);
    Serial.println("begin tekenSettingsscherm");
        currentPage = 2;
        Serial.print("currentPage = ");
        Serial.println(currentPage);
            myGLCD.setColor(VGA_BLACK);
            myGLCD.setBackColor(VGA_BLACK);
            myGLCD.fillRoundRect(2, 215, 80, 238);
            myGLCD.fillRoundRect(82, 215, 160, 238);
            myGLCD.fillRoundRect(162, 215, 240, 238);
            myGLCD.fillRoundRect(242, 215, 318, 238);
            myGLCD.setColor(VGA_WHITE);
            myGLCD.drawRoundRect(2, 215, 80, 238);
            myGLCD.drawRoundRect(82, 215, 160, 238);
            myGLCD.drawRoundRect(162, 215, 240, 238);
            myGLCD.drawRoundRect(242, 215, 318, 238);
            myGLCD.print("Back", 266, 221);
            myGLCD.print("Settings", 10, 221);
            while (currentPage == 2) {
        
        Serial.print("currentPage = ");
        Serial.println(currentPage);
                String test = "opnieuw";
                if (myTouch.dataAvailable()) {
                    myTouch.read();
                    x=myTouch.getX();
                    y=myTouch.getY();
                    if ((y>=210) && (y<=240)) { 
                        if ((x>=0) && (x<=80)) {
                            betast(2, 215, 80, 238);
                            tekenSettingsManipulatieScherm(bak);
                            break;;
                        }
                        if ((x>=80) && (x<=160)) {
                            betast(82, 215, 160, 238);
                            //break;
                        }
                        if ((x>=160) && (x<=240)) {
                            betast(162, 215, 240, 238);
                            //break;
                        }
                        if ((x>=240) && (x<=320)) {
                            betast(242, 215, 318, 238);
                            Serial.print("ikwas aangeraakt in settingsschermin ");
                            toonStartScherm(test);
                            break;
                        }
                    }
                }
        }
    }

    void drawButtons() {
        currentPage = 4;
    Serial.println("begin drawbuttons");
    Serial.print("currentPage = ");
    Serial.println(currentPage);
        myGLCD.clrScr();
        myGLCD.setFont(BigFont);
        myGLCD.setBackColor(VGA_BLUE);
        for (x=0; x<5; x++) {// Draw the upper row of buttons
            myGLCD.setColor(0, 0, 255);
            myGLCD.fillRoundRect (10+(x*60), 10, 60+(x*60), 60);
            myGLCD.setColor(255, 255, 255);
            myGLCD.drawRoundRect (10+(x*60), 10, 60+(x*60), 60);
            myGLCD.printNumI(x+1, 27+(x*60), 27);
        }
   
        for (x=0; x<5; x++) {  // Draw the center row of buttons
            myGLCD.setColor(0, 0, 255);
            myGLCD.fillRoundRect (10+(x*60), 70, 60+(x*60), 120);
            myGLCD.setColor(255, 255, 255);
            myGLCD.drawRoundRect (10+(x*60), 70, 60+(x*60), 120);
            if (x<4)
            myGLCD.printNumI(x+6, 27+(x*60), 87);
        }
        myGLCD.print("0", 267, 87); // Draw the lower row of buttons
        myGLCD.setColor(0, 0, 255);
        myGLCD.fillRoundRect (10, 130, 150, 180);
        myGLCD.setColor(255, 255, 255);
        myGLCD.drawRoundRect (10, 130, 150, 180);
        myGLCD.print("Clear", 40, 147);
        myGLCD.setColor(0, 0, 255);
        myGLCD.fillRoundRect (160, 130, 300, 180);
        myGLCD.setColor(255, 255, 255);
        myGLCD.drawRoundRect (160, 130, 300, 180);
        myGLCD.print("Enter", 190, 147);
        myGLCD.setBackColor (0, 0, 0);
        myGLCD.setBackColor(VGA_BLACK);
        myGLCD.setColor(VGA_WHITE);
        myGLCD.drawRoundRect(242, 215, 318, 238);
        myGLCD.setFont(SmallFont);
        myGLCD.print("Back", 266, 221);
    }

    void updateStr(int val) {
    if (stCurrentLen<20) {
        stCurrent[stCurrentLen]=val;
        stCurrent[stCurrentLen+1]='\0';
        stCurrentLen++;
        myGLCD.setColor(0, 255, 0);
        myGLCD.print(stCurrent, LEFT, 224);
    }   else {
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

    void leesGetal(int plantbak, int seizoen, int kolom) {
    Serial.println("begin leesgetal");
        while (currentPage ==4) {
        Serial.print("currentPage = ");
        Serial.println(currentPage);
            if (myTouch.dataAvailable()) {
                myTouch.read();
                x=myTouch.getX();
                y=myTouch.getY();
                if ((y>=10) && (y<=60)) { // Upper row
                    if ((x>=10) && (x<=60))  { // Button: 1
                        betast(10, 10, 60, 60);
                        updateStr('1');
                    }
                    if ((x>=70) && (x<=120)) { // Button: 2
                        betast(70, 10, 120, 60);
                        updateStr('2');
                    }
                    if ((x>=130) && (x<=180))  // Button: 3
                    {
                    betast(130, 10, 180, 60);
                    updateStr('3');
                    }
                    if ((x>=190) && (x<=240))  // Button: 4
                    {
                    betast(190, 10, 240, 60);
                    updateStr('4');
                    }
                    if ((x>=250) && (x<=300))  // Button: 5
                    {
                    betast(250, 10, 300, 60);
                    updateStr('5');
                    }
                }

                if ((y>=70) && (y<=120)) { // Center row
                    if ((x>=10) && (x<=60))  {// Button: 6
                        betast(10, 70, 60, 120);
                        updateStr('6');
                    }
                    if ((x>=70) && (x<=120))  // Button: 7
                    {
                    betast(70, 70, 120, 120);
                    updateStr('7');
                    }
                    if ((x>=130) && (x<=180))  // Button: 8
                    {
                    betast(130, 70, 180, 120);
                    updateStr('8');
                    }
                    if ((x>=190) && (x<=240))  // Button: 9
                    {
                    betast(190, 70, 240, 120);
                    updateStr('9');
                    }
                    if ((x>=250) && (x<=300))  // Button: 0
                    {
                    betast(250, 70, 300, 120);
                    updateStr('0');
                    }
                }
                if ((y>=130) && (y<=180))  {// Upper row
                    if ((x>=10) && (x<=150))  {// Button: Clear
                        betast(10, 130, 150, 180);
                        stCurrent[0]='\0';
                        stCurrentLen=0;
                        myGLCD.setColor(0, 0, 0);
                        myGLCD.fillRect(0, 224, 319, 239);
                        }
                        if ((x>=160) && (x<=300))  // Button: Enter
                        {
                        betast(160, 130, 300, 180);
                        if (stCurrentLen>0) {
                            for (x=0; x<stCurrentLen+1; x++)
                            {
                            stLast[x]=stCurrent[x];
                            }
                            stCurrent[0]='\0';
                            stCurrentLen=0;
                            myGLCD.setColor(0, 0, 0);
                            myGLCD.fillRect(0, 208, 319, 239);
                            myGLCD.setColor(0, 255, 0);
                            myGLCD.print(String(stLast), LEFT, 208);
                            int number = atoi(stLast);
                            customPlantenBakSettings[plantbak][seizoen][kolom] = number;
                            
                            stCurrent[2]="";
                            stCurrentLen=0;
                            stLast[2]=""; 
                            tekenSettingsManipulatieScherm(plantbak); 
                        }   else  {
                                myGLCD.setColor(255, 0, 0);
                                myGLCD.print("BUFFER EMPTY", CENTER, 192);
                                delay(500);
                                myGLCD.print("            ", CENTER, 192);
                                delay(500);
                                myGLCD.print("BUFFER EMPTY", CENTER, 192);
                                delay(500);
                                myGLCD.print("            ", CENTER, 192);
                                myGLCD.setColor(0, 255, 0);
                            }
                        }
                    }
                }
                if ((y>=210) && (y<=240)) {
                    if ((x>=240) && (x<=320)) {
                        betast(242, 215, 318, 238);
                        tekenSettingsManipulatieScherm(plantbak);
                    }
                }    
        }
    }

    


// Draw a red frame while a button is touched
    void betast(int x1, int y1, int x2, int y2) {
        Serial.println("begin betast");
        myGLCD.setColor(VGA_GREEN);
        myGLCD.drawRoundRect (x1, y1, x2, y2);
        while (myTouch.dataAvailable())
            myTouch.read();
    }

    void kiesPlantenBak() {
        
        int gekozenBak = 3; //3 is niet bestaandebestaande plantenbak
        if (currentPage == 1 && myTouch.dataAvailable()) {
    Serial.print("currentPage = ");
    Serial.println(currentPage);
            myTouch.read();
            x=myTouch.getX();
            y=myTouch.getY();
            if ((y>=10) && (y<=70)) { // bovenste bak
                gekozenBak = 0;
                betast(2, 5, 315, 70);
                tekenSettingsScherm(gekozenBak);
            }
            if ((y>=73) && (y<=143)) {// middelste bak
                gekozenBak = 1;
                betast(2, 76, 315, 141);
                tekenSettingsScherm(gekozenBak);
                
            }
            if ((y>=130) && (y<=180)) { // Button: 3
                gekozenBak = 2;
                betast(2, 147, 315, 212);
                tekenSettingsScherm(gekozenBak);
            }
            Serial.print("Het getal(kiesplantenbak) is:");
            Serial.println(klimaatDataNu[2][LUCHTVOCHTIGHEIDNU]);
        ;
        }
    }
};

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
    Serial.begin(57600);
    klok.setup();
    touchScreen.setup();
    klimaatDataLogger.setup();
    plantenbak1.setup();
    plantenbak2.setup();
    plantenbak3.setup();
}

void loop() {
    Serial.println("begin main");
    RtcDateTime tijd = klok.getTime();
    plantenbak1.regelKlimaat(tijd, bakNummer1);
    plantenbak2.regelKlimaat(tijd, bakNummer2);
    plantenbak3.regelKlimaat(tijd, bakNummer3);
    String datumTijd = klok.geefDatumTijdString(tijd);
    if(currentPage == 1) {
        touchScreen.toonStartScherm(datumTijd);
    }
    int minuut = tijd.Minute();
    int minuutNu = tijd.Minute();
    while (currentPage == 1 && minuutNu == minuut) {
        touchScreen.kiesPlantenBak();
        RtcDateTime nieuweTijd = klok.getTime();
        minuutNu = nieuweTijd.Minute();
    }
    String klimaatDataString = klimaatDataLogger.maakKlimaatDataString();
    Serial.println(klimaatDataString);
    klimaatDataLogger.writeToFile(klimaatDataString);
    //klimaatDataLogger.readFromFile();
    Serial.println();
    
}

