#include <Arduino.h>
#include <Wire.h> // must be included here so that Arduino library object file references work (clock)
#include <RtcDS3231.h>

#include <SD.h>

#include <DHT.h>
#include <DHT_U.h>

#include <UTFT.h>
#include <URTouch.h>
#include <UTFT_Buttons.h>                                                                  

#define STARTDAG 0
#define EINDDAG 1
#define DAGTEMPERATUUR 2
#define NACHTTEMPERATUUR 3
#define DUURDAUW 4
#define DUURREGEN 5
#define LUCHTVOCHTIGHEID 6
#define WATERGEVEN 7
#define LAMPENVERVANGEN 8

#define STARTDAUW 7
#define STARTREGEN 8
#define EINDREGEN 9
#define SEIZOEN 10
#define ISDAG 11
#define ISREGEN 12
#define ISDAUW 13
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
#define PLANTENBAKNUMMER 28
#define HOOGSTEPOTVOCHTIGHEID 29
#define MEESTELICHT 30

#define WINTER 0
#define ZOMER 1
#define REGEN 2


//volgende regel is voor tijdweergave klok
#define countof(a) (sizeof(a) / sizeof(a[0]))

extern uint8_t BigFont[];
extern uint8_t SmallFont[];

int plantenBakSettings1[3][4][12] = {{{8, 21, 20, 14, 4, 0, 70, 20, 75}, {8, 22, 30, 20, 4, 1, 55, 10, 75}, {8, 23, 35, 30, 4, 5, 85, 30, 75}, {0, 0, 1, 1, 1, 1, 2, 2, 2, 0, 0, 0}}, {{8, 21, 20, 14, 4, 0, 70, 20, 75}, {8, 22, 30, 20, 4, 1, 55, 20, 75}, {8, 23, 35, 30, 4, 5, 85, 20, 75}, {1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1}}, {{8, 21, 20, 14, 4, 0, 70, 20, 75}, {8, 22, 30, 20, 4, 1, 55, 20, 75}, {8, 23, 35, 30, 4, 5, 85, 20, 75}, {2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2}}};

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
        int seizoenNu = plantenBakSettings1[plantenBakNummer][3][(now.Month()-1)];
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
                    klimaatDataNu[plantenBakNummer][i] = plantenBakSettings1[plantenBakNummer][WINTER][i];
                }
                klimaatDataNu[plantenBakNummer][SEIZOEN] = WINTER;
                break;
            case ZOMER:
                for (int i = 0; i < 7; i++) {
                    klimaatDataNu[plantenBakNummer][i] = plantenBakSettings1[plantenBakNummer][ZOMER][i];
            }
                klimaatDataNu[plantenBakNummer][SEIZOEN] = ZOMER;
                break;
            case REGEN:
                for (int i = 0; i < 7; i++) {
                    klimaatDataNu[plantenBakNummer][i] = plantenBakSettings1[plantenBakNummer][REGEN][i];
                }
                klimaatDataNu[plantenBakNummer][SEIZOEN] = REGEN;
                break;
        }       

        if (uurMinuutNu >= klimaatDataNu[plantenBakNummer][STARTDAG] && uurMinuutNu <= klimaatDataNu[plantenBakNummer][EINDDAG]) {
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
        float startBewolking = ((klimaatDataNu[plantenBakNummer][STARTDAG] + klimaatDataNu[plantenBakNummer][EINDDAG]) / 2) - (klimaatDataNu[plantenBakNummer][DUURREGEN] / 2);
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
        klimaatDataNu[plantenBakNummer][POTVOCHTIGHEID] = (potVochtigheid/maxPotVocht)*100;
        Serial.print("Soil Moisture = ");  
        Serial.println((potVochtigheid/maxPotVocht)*100);
        int licht = lichtSensor.readLogValue();
        if(licht > maxLicht) {
            maxLicht = licht;
            klimaatDataNu[plantenBakNummer][MEESTELICHT] = licht;
        }
        klimaatDataNu[plantenBakNummer][LICHT] = (licht/maxLicht)*100;
        Serial.print("Licht = ");
        Serial.println((licht/maxLicht)*100);
        klimaatRegelaar.doeJeKlimaatDing(myTime);
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
        currentPage = 1; // Indicates wich screen is active
    }

    void toonStartScherm(String myDatumTijd) {
        //myGLCD.clrScr();
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
            myGLCD.fillRoundRect(2, (i*71) + 5, 318, (i*71) + 70);
            myGLCD.setColor(VGA_WHITE);
            myGLCD.drawRoundRect (2, (i*71) + 5, 318, (i*71) + 70);
            myGLCD.setFont(SmallFont);
            myGLCD.print(String("Temperature"), 11, (i*71) +6);
            myGLCD.print(String("Humidity"), 11, (i*71) + 39);
            myGLCD.print(String("Soil"), 150, (i*71) + 6);
            myGLCD.print(String("Light"),150, (i*71) + 39);
            myGLCD.print(String(klimaatDataNu[i][NACHTTEMPERATUUR]) + "/", 11, (i*71) + 21);
            myGLCD.print(String("/") + String(klimaatDataNu[i][DAGTEMPERATUUR]), 90, (i*71) + 21);
            myGLCD.print(String("/") + String(klimaatDataNu[i][LUCHTVOCHTIGHEID]), 59, (i*71) + 53);
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
            myGLCD.print(String(klimaatDataNu[i][TEMPERATUUR]), 35, (i*71) + 18);
            myGLCD.print(String("C"), 73, (i*71) + 18);
             myGLCD.fillCircle(73, (i*71) + 19, 2);
            myGLCD.print(String(klimaatDataNu[i][LUCHTVOCHTIGHEIDNU]) + "%", 11, (i*71) + 51);
            myGLCD.print(String(klimaatDataNu[i][POTVOCHTIGHEID]) + "%", 150, (i*71) + 18);
            myGLCD.print(String(klimaatDataNu[i][LICHT]) + "%", 150, (i*71) + 51);
            }
        myGLCD.setColor(VGA_WHITE);
        myGLCD.setBackColor(VGA_BLACK);
        myGLCD.setFont(BigFont);
        myGLCD.print(myDatumTijd, CENTER, 220);
    }

    void tekenSettingsScherm(int bak) {
        int but1, but2, but3, but4;

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
            myGLCD.fillRoundRect(2, (i*71) + 5, 318, (i*71) + 70);
            // myGLCD.setColor(VGA_WHITE);
            // myGLCD.drawRoundRect (2, (i*71) + 5, 318, (i*71) + 70);
            myGLCD.setColor(VGA_WHITE);
            myGLCD.drawRoundRect (2, (i*71) + 5, 318, (i*71) + 70);
            myGLCD.setFont(SmallFont);
            myGLCD.print(String("Temperature"), 11, (i*71) +6);
            myGLCD.print(String("Humidity"), 11, (i*71) + 39);
            myGLCD.print(String("Soil"), 150, (i*71) + 6);
            myGLCD.print(String("Light"),150, (i*71) + 39);
            myGLCD.setFont(SmallFont);
            myGLCD.print(String("min"), 11, (i*71) + 21);
            myGLCD.print(String("max"), 70, (i*71) + 21);
            myGLCD.print(String("average"), 11, (i*71) + 53);
            myGLCD.setFont(BigFont);
            myGLCD.setColor(VGA_YELLOW);
            myGLCD.print(String(plantenBakSettings1[bak][i][NACHTTEMPERATUUR]), 35, (i*71) + 18);
            myGLCD.print(String(plantenBakSettings1[bak][i][DAGTEMPERATUUR]), 94, (i*71) + 18);
            myGLCD.print(String("C"), 128, (i*71) + 18);
            myGLCD.fillCircle(128, (i*71) + 18, 2);
            myGLCD.print(String(plantenBakSettings1[bak][i][LUCHTVOCHTIGHEID]) + "%", 67, (i*71) + 51);
            myGLCD.print(String(plantenBakSettings1[bak][i][WATERGEVEN]) + "%", 150, (i*71) + 18);
            myGLCD.print(String(plantenBakSettings1[bak][i][LAMPENVERVANGEN]) + "%", 150, (i*71) + 51);
            Serial.print("Het BakNummer is:");
            Serial.println(bak);
            Serial.print("Het getal (settingsscherm) is:");
            Serial.println(klimaatDataNu[2][LUCHTVOCHTIGHEIDNU]);
            Serial.println(bak);
            myGLCD.setColor(VGA_YELLOW);
            switch (bak) {
                case 0 :
                    myGLCD.print("BOTTOM", 205, 35 + (i*71));
                    break;
                case 1 :
                    myGLCD.print("CENTER", 205, 35 + (i*71));      break;
                case 2 : 
                    myGLCD.print("TOP", 231, 35 + (i*71));
                    break;
            }
            if(i == 0) {
                myGLCD.setColor(VGA_WHITE);
                myGLCD.setFont(BigFont);
                myGLCD.print("WINTER", 205, 51 + (i*71));
            }  
            if(i == 1) { 
                myGLCD.setColor(VGA_WHITE);
                myGLCD.setFont(BigFont);
                myGLCD.print("SUMMER", 205, 51 + (i*71));
            }
            if(i == 2) { 
                myGLCD.setColor(VGA_WHITE);
                myGLCD.setFont(BigFont);
                myGLCD.print("RAIN", 221, 51 + (i*71));
            }
            currentPage = 2;
            for(int i; i < 5; i++) {

            }
            myGLCD.setColor(VGA_WHITE);
            myGLCD.fillRoundRect(2, 215, 80, 238);
            myGLCD.fillRoundRect(82, 215, 160, 238);
            myGLCD.fillRoundRect(162, 215, 240, 238);
            myGLCD.fillRoundRect(242, 215, 318, 238);
            
            // but1 = myButtons.addButton(2, 215, 55, 35,"setings");
            // but1 = myButtons.addButton(60, 215, 55, 35,"history");
            // but1 = myButtons.addButton(118, 215, 55, 35,"flowers");
            // but1 = myButtons.addButton(176, 215, 55, 35,"back");
            // myButtons.drawButtons()   ;

     }    
    }

// Draw a red frame while a button is touched
    void betast(int x1, int y1, int x2, int y2, int bakNummer) {
        myGLCD.setColor(VGA_GREEN);
        myGLCD.drawRoundRect (x1, y1, x2, y2);
        while (myTouch.dataAvailable())
            myTouch.read();
        //myGLCD.clrScr();
    }

    void kiesPlantenBak() {
        int gekozenBak = 3; //3 is niet bestaandebestaande plantenbak
        if (currentPage == 1 && myTouch.dataAvailable()) {
            myTouch.read();
            x=myTouch.getX();
            y=myTouch.getY();
            if ((y>=10) && (y<=70)) { // bovenste bak
                gekozenBak = 2;
                betast(2, 5, 315, 70, gekozenBak);
                tekenSettingsScherm(gekozenBak);
            }
            if ((y>=73) && (y<=143)) {// middelste bak
                gekozenBak = 1;
                betast(2, 76, 315, 141, gekozenBak);
                tekenSettingsScherm(gekozenBak);
                
            }
            if ((y>=130) && (y<=180)) { // Button: 3
                gekozenBak = 0;
                betast(2, 147, 315, 212, gekozenBak);
                tekenSettingsScherm(gekozenBak);
            }
            Serial.print("Het getal(kiesplantenbak) is:");
            Serial.println(klimaatDataNu[2][LUCHTVOCHTIGHEIDNU]);
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
    Serial.print("Het getal (main) is:");
    Serial.println(klimaatDataNu[2][LUCHTVOCHTIGHEIDNU]);
    while ((minuutNu - minuut) < 1) {
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

