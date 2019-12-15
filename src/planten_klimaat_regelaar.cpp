#include <Arduino.h>
#include <Wire.h> // must be included here so that Arduino library object file references work
#include <RtcDS3231.h>

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

#define WINTER 0
#define ZOMER 1
#define REGEN 2


//volgende regel is voor tijdweergave klok
#define countof(a) (sizeof(a) / sizeof(a[0]))
extern uint8_t BigFont[];
extern uint8_t SmallFont[];
// int settingsNu[15];
int plantenBakSettings1[3][4][12] = {{{6, 6, 6, 15, 35, 14, 70}, {8, 40, 1, 0, 14, 25, 55}, {8, 40, 2, 0, 30, 23, 85}, {0, 0, 1, 1, 1, 1, 2, 2, 2, 0, 0, 0}}, {{84, 240, 236, 15, 35, 14, 70}, {4, 4, 4, 4, 14, 25, 55}, {66, 44, 55, 0, 30, 23, 85}, {1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1}}, {{8, 40, 6, 15, 35, 14, 70}, {8, 40, 1, 0, 14, 25, 55}, {3, 3, 3, 3, 30, 23, 85}, {2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2}}};
//int plantenBakSettings2[4][12] = {{84, 240, 236, 15, 35, 14, 70}, {8, 40, 1, 0, 14, 25, 55}, {8, 40, 2, 0, 30, 23, 85}, {1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 1}};
//int plantenBakSettings3[4][12] = {{8, 40, 6, 15, 35, 14, 70}, {8, 40, 1, 0, 14, 25, 55}, {8, 40, 2, 0, 30, 23, 85}, {2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2}};
// lampenaan, lampenuit, duurdauw, DUUR REGEN, dag temperatuur, nacht temperatuur, VOCHTIGHED
byte pinArray1[8] = {A0, 9, A1, 4, 5, 10, 8, 7}; //1soilsensorPin1, 2soilPower1, 3lightsensorPin1, 4lampenPin1, 5ventilatorpin1, 6vernevelaarpin1, 7dhtpin, lampenpin21
byte pinArray2[8] = {A0, 9, A1, 4, 5, 10, 8, 7};
byte pinArray3[8] = {A0, 9, A1, 4, 5, 10, 8, 7};

int klimaatDataNu[3][15];

// String dataVoorScherm [3][15];

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
    boolean ventilatorIsAan = false;
    boolean vernevelaarIsAan = false;
    boolean lampIsAan2 = false;
    boolean luchtIsDroog = false;

    public:
    KlimaatRegelaar(byte myLampenPin1, byte myLampenPin2, byte myNevelPin, byte myVentilatorPin) 
   
        {
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

    void doeJeKlimaatDing(RtcDateTime now, int myPlantenbakNummer) {

        getSettingsNu(now, myPlantenbakNummer);
        regelLicht(now);
        


    }
    void getSettingsNu(RtcDateTime now, int myPlantenbakNummer) {

        int plantenBakNummer = myPlantenbakNummer;
        //static int klimaatData[3][15];
        int seizoenNu = plantenBakSettings1[plantenBakNummer][3][(now.Month()-1)];
        Serial.println("seizoenNu = ");
        Serial.println(seizoenNu);
        
        int uurNu = now.Second();// terugveranderen naat hour()
        int minuutNu = now.Minute();
        float uurMinuutNu = uurNu + (minuutNu / 60);
        Serial.println("uurMinuutNu = ");
        Serial.println(uurMinuutNu);
        boolean isDag;
        boolean isDauw;
        boolean isRegen;
        
        switch (seizoenNu) {
            case WINTER:
                Serial.println("case 0");
                for (int i = 0; i < 7; i++) {
                    klimaatDataNu[plantenBakNummer][i] = plantenBakSettings1[plantenBakNummer][WINTER][i];
                    Serial.println(klimaatDataNu[plantenBakNummer][i]);
                }
                klimaatDataNu[plantenBakNummer][10] = WINTER;
                break;
            case ZOMER:
                Serial.println("case 1");
                for (int i = 0; i < 7; i++) {
                    klimaatDataNu[plantenBakNummer][i] = plantenBakSettings1[plantenBakNummer][ZOMER][i];
                    Serial.println(klimaatDataNu[plantenBakNummer][i]);
                }
                klimaatDataNu[plantenBakNummer][10] = ZOMER;
                break;
            case REGEN:
                Serial.println("case 2");
                for (int i = 0; i < 7; i++) {
                    klimaatDataNu[plantenBakNummer][plantenBakNummer][i] = plantenBakSettings1[plantenBakNummer][REGEN][i];
                    Serial.println(klimaatDataNu[plantenBakNummer][i]);
                }
                klimaatDataNu[plantenBakNummer][10] = REGEN;
                break;
        }       
        
        if (uurMinuutNu >= klimaatDataNu[STARTDAG] && uurMinuutNu <= klimaatDataNu[EINDDAG]) {
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
        float startBewolking = ((klimaatDataNu[plantenBakNummer][STARTDAG] + klimaatDataNu[plantenBakNummer][EINDDAG]) / 2) - (settingsNu[DUURREGEN] / 2);
        float eindBewolking = startBewolking + klimaatDataNu[plantenBakNummer][DUURREGEN];
        if (uurMinuutNu >= startBewolking && uurMinuutNu <= eindBewolking) {
            isRegen = true;
        }   else {
                isRegen = false;
            }
        klimaatDataNu[plantenBakNummer][9] = isRegen;
        Serial.print("getal in getsettings =" );
        Serial.println(klimaatDataNu[plantenBakNummer][1]);
    }

    void regelLicht(int plantenBakNummer) {
            
        if (klimaatDataNu[plantenBakNummer][ISDAG]) {
            digitalWrite(lampenPin1, HIGH);
            digitalWrite(lampenPin2, HIGH);
            Serial.println("Lampen aangeschakeld");
            lampIsAan2 = true;
        }
        if (!settings[ISDAG]) {
            digitalWrite(lampenPin1, LOW); 
            digitalWrite(lampenPin2, LOW); 
            Serial.println("Lampen uitgeschakeld"); 
            lampIsAan2 = false;
        }
    }

    void regelRegenWolken(int plantenBakNummer) {

        if (klimaatDataNu[plantenBakNummer][ISREGEN] && klimaatDataNu[plantenBakNummer][LUCHTVOCHTIGHEIDNU] < 100) {
            if (!vernevelaarIsAan) {
                digitalWrite(nevelPin, HIGH);
                Serial.println("vernevelaar aan (regenwolken)");
                vernevelaarIsAan = true;
            }    
            if (lampIsAan2) {
                digitalWrite(lampenPin2, LOW);
                Serial.println("lampen2 is uit (regenwolken)");
                lampIsAan2 = false;
            }
        }

        if (!klimaatDataNu[plantenBakNummer][ISREGEN]||temperatuur < klimaatDataNu[plantenBakNummer][NACHTTEMPERATUUR])) {
            if (vernevelaarIsAan && luchtVochtigheid > settings[LUCHTVOCHTIGHEID]) {
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
        if (!isDauw && klimaatDataNu[plantenBakNummer][ISDAuw]) {
            if (!vernevelaarIsAan) {
                digitalWrite(nevelPin, HIGH);
                Serial.println("vernevelaar aan (dauw)");
                vernevelaarIsAan = true;
                }    
            if (!ventilatorIsAan) {
                digitalWrite(ventilatorPin, HIGH);
                Serial.println("ventilator aan (dauw)");
                ventilatorIsAan = true;
                }
            isDauw = true;
        }

        if (isDauw && (uurMinuutNu < startDauw || uurMinuutNu > eindDauw || temperatuur < settings[NACHTTEMPERATUUR])) {
            if (vernevelaarIsAan && luchtVochtigheid > settings[LUCHTVOCHTIGHEID]) {
                digitalWrite(nevelPin, LOW); 
                Serial.println("vernevelaar uit (dauw)"); 
                vernevelaarIsAan = false;
            }  
            if (ventilatorIsAan) {
                digitalWrite(ventilatorPin, LOW); 
                Serial.println("ventilator uit (dauw)"); 
                ventilatorIsAan = false;
            }
            isDauw = false;
        }
    }

    void regelVochtigheid(float temperatuur, float luchtVochtigheid, float settings[]) {

        if (luchtVochtigheid < settings[LUCHTVOCHTIGHEID] && temperatuur > settings[NACHTTEMPERATUUR]) {
            luchtIsDroog = true;
            if (!vernevelaarIsAan) {
                digitalWrite(nevelPin, HIGH);
                Serial.println("vernevelaar aan (luchtvochtigheid)");
                vernevelaarIsAan = true;
            } 
        }
        if (vernevelaarIsAan && !isDauw && !isRegenWolk && (luchtVochtigheid > settings[LUCHTVOCHTIGHEID] || temperatuur < settings[NACHTTEMPERATUUR])) {
            digitalWrite(nevelPin, LOW);
            Serial.print("vernevelaar uit (luchtvochtigheid)");
            vernevelaarIsAan = false;
        }
        if (luchtVochtigheid > settings[LUCHTVOCHTIGHEID] ) { 
            luchtIsDroog = false;
        }
        if (vernevelaarIsAan && luchtVochtigheid > 100) {
            digitalWrite(nevelPin, LOW);
            Serial.print("vernevelaar uit (luchtvochtigheid)");
            vernevelaarIsAan = false;
        }
    }

    void regelTemperatuur(float temperatuur, float settings[]){

        if (temperatuur > settings[DAGTEMPERATUUR]) {
            if (!ventilatorIsAan) {
                digitalWrite(ventilatorPin, HIGH);
                Serial.print("vernevelaar aan (temperatuur)");
                ventilatorIsAan = true;
            }
            if (lampIsAan2) {
                digitalWrite(lampenPin2, LOW);
                Serial.println("lampen2 is uit (temperatuur)");
                lampIsAan2 = false;
            }
            if (!vernevelaarIsAan) {
                digitalWrite(nevelPin, HIGH);
                Serial.print("vernevelaar aan (temperatuur)");
                vernevelaarIsAan = false;
            }
        }
        if (temperatuur < settings[DAGTEMPERATUUR]) {
            if (ventilatorIsAan && !isDauw) {
                digitalWrite(ventilatorPin, LOW);
                Serial.print("vernevelaar uit (temperatuur)");
                ventilatorIsAan = false;
            }
            if (!lampIsAan2 && !isRegenWolk && isDag) {
                digitalWrite(lampenPin2, HIGH);
                Serial.println("lampen2 aan (temperatuur)");
                lampIsAan2 = true;
            }
            if (vernevelaarIsAan && !isDauw && !isRegenWolk && !luchtIsDroog) {
                digitalWrite(nevelPin, LOW);
                Serial.print("vernevelaar uit (temperatuur)");
                vernevelaarIsAan = false;
            }
        }
    }

    void standen(float temperatuur, float luchtVochtigheid, float pot, float licht, float settings[]) {
        
        String standen[9]; 
        Serial.print("isDag = ");
        Serial.println(isDag);
        Serial.print("vernevelaarIsAan = ");
        Serial.println(vernevelaarIsAan);
        Serial.print("isDauw = ");
        Serial.println(isDauw);
        Serial.print("ventilatorIsAan = ");
        Serial.println(ventilatorIsAan);
        Serial.print("isRegenWolk = ");
        Serial.println(isRegenWolk);
        Serial.print("lampenAan2 = ");
        Serial.println(lampIsAan2);
        Serial.print("luchtIsDroog = ");
        Serial.println(luchtIsDroog);
        
    }
};

class Klok {
    
    RtcDS3231<TwoWire> Rtc;
    
    public:
    Klok(): Rtc(Wire)
    { }

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
        Serial.print("Dit is de loop tijd ");
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
        klimaatRegelaar(myPins[3] , myPins[7], myPins[5], myPins[4]),
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
        Serial.print("Raw = ");
        Serial.print(lichtSensor.readRawValue());
        Serial.print(" - Lux = ");
        int licht = lichtSensor.readLogValue();
        klimaatDataNu[platenBakNummer][LICHT] = licht;
        Serial.println(lichtSensor.readLogValue());
        klimaatRegelaar.doeJeKlimaatDing(myTime, plantenBakNummer);
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
    }

    void toonStartScherm(RtcDateTime mydatumTijd) {

        myGLCD.clrScr();
        // myGLCD.print((String) settings[0][NACHTTEMPERATUUR] + "/", 11, 22);
        // myGLCD.print(String(settings[0][NACHTTEMPERATUUR]) + "/", 201, 22);
        //for (int i = 0; i < 3; i++) {}
        if (settings[0][SEIZOEN] == 0) {
            myGLCD.setColor(VGA_BLUE);
            myGLCD.setBackColor(VGA_BLUE);
        }   
        if (settings[0][SEIZOEN] == 1) {
            myGLCD.setColor(VGA_RED);
            myGLCD.setBackColor(VGA_RED);
        } 
        if (settings[0][SEIZOEN] == 1) {
            myGLCD.setColor(VGA_GRAY);
            myGLCD.setBackColor(VGA_GRAY);
        }
        myGLCD.fillRoundRect(5, 5, 315, 70);
        myGLCD.setColor(VGA_WHITE);
        myGLCD.setFont(SmallFont);
        myGLCD.print("Temperature", 11, 5);
        myGLCD.print("Humidity", 11, 38);
        myGLCD.print("Soilmoisture", 150, 5);
        myGLCD.print("Light",150, 38);
        myGLCD.print(String(settings[0][NACHTTEMPERATUUR]) + "/", 11, 22);
        myGLCD.print("/" + String(settings[0][DAGTEMPERATUUR]), 101, 22);
        myGLCD.print("/" + String(settings[0][LUCHTVOCHTIGHEID]), 59, 55);
        myGLCD.setColor(VGA_YELLOW);
        // if ( std::getline( std::cin, input1 ) && input1 == "log in" )
        if(settings[0][ISDAUW] == 1) {
            myGLCD.print("Dew", 265, 22);
        }
        if(settings[0][ISREGEN] == 1) {
            myGLCD.print("Rain", 265, 38);
        }
        if(settings[0][ISDAG] == 1) {
            myGLCD.print("Day", 265, 5);
        }   else {
            myGLCD.print("Night", 265, 5);
        }
        if (settings[0][SEIZOEN] == 0) {
            myGLCD.print("Winter", 265, 55);
        }   
        if (settings[0][SEIZOEN] == 1) {
            myGLCD.print("Summer", 265, 55);
        } 
        if (settings[0][SEIZOEN] == 1) {
            myGLCD.print("Wet", 265, 55);
        }
            //    
        myGLCD.setFont(BigFont);
       
        myGLCD.print(String(metingen[0][TEMPERATUUR]) + "'C", 35, 18);
        myGLCD.print(String(metingen[0][VOCHTIGHEID]) + "%", 11, 53);
        myGLCD.print(String(metingen[0][POTVOCHTIGHEID]) , 150, 18);
        myGLCD.print(String(metingen[0][LICHT]), 150, 52);
        delay(9000);
    }
};


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
    touchScreen.setup();
    klok.setup();
    plantenbak1.setup();
    plantenbak2.setup();
    plantenbak3.setup();
}

void loop()
{
    
    RtcDateTime tijd = klok.getTime();
    // String datumTijd = klok.geefDatumTijdString(tijd);

    plantenbak1.regelKlimaat(tijd);
    plantenbak2.regelKlimaat(tijd);
    plantenbak3.regelKlimaat(tijd);
    
    Serial.println(klimaatDataNu[0][1]);
    Serial.println(klimaatDataNu[1][1]);
    Serial.println(klimaatDataNu[2][1]);
    touchScreen.toonStartScherm(tijd);
    Serial.println();
    delay(3000);
}

