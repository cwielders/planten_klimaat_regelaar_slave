#include <Arduino.h>
#include <SD.h>
#include <UTFT.h>
#include <URTouch.h>
#include <UTFT_Buttons.h>  
#include <SPI.h>

#define DAGTEMPERATUUR 0
#define NACHTTEMPERATUUR 1
#define LUCHTVOCHTIGHEID 2
#define WATERGEVEN 3
#define LAMPENVERVANGEN 4
#define DUURDAUW 5
#define DUURREGEN 6
#define DUURBEWOLKING 7


#define DUURDAG 8
#define STARTDAG 9
#define EINDDAG 10
#define STARTDAUW 11
#define STARTREGEN 12
#define EINDREGEN 13
#define SEIZOEN 14
#define VENTILATOR 15
#define VERNEVELAAR 16
#define TEMPERATUUR 17
#define LUCHTVOCHTIGHEIDNU 18
#define POTVOCHTIGHEID 19
#define LICHT 20
#define LAMPENAAN1 21
#define LAMPENAAN2 22
#define JAAR 23
#define MAAND 24
#define DAG 25
#define UUR 26
#define MINUUT 27
#define PLANTENBAKNUMMER 28
#define HOOGSTEPOTVOCHTIGHEID 29
#define MEESTELICHT 30
#define ISDAG 31
#define ISREGEN 32
#define ISDAUW 33

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
String datumTijd = "Nog niet";
int currentPage; //indicates the page that is active on touchscreen


volatile byte c;
volatile byte j = 0;
volatile byte i = 0;
volatile bool flag1 = false; //weer false na iedere loop omdat startcode 1x moet worden opgestuurd, true als de startcode als laatste is ontvangen 
volatile bool flag2 = false; //wordt true na uiwisseling laatste element array

byte defaultPlantenBakSettings[3][4][12] = {{{25, 14, 60, 20, 75, 3, 0, 0, 11, 8}, {35, 28, 55, 10, 75, 2, 0, 0, 12, 8}, {30, 25, 35, 80, 4, 5, 85, 4, 1,13, 8}, {0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 0}}, {{8, 21, 20, 14, 4, 0, 70, 1, 20, 75}, {8, 22, 30, 20, 4, 1, 55, 1, 20, 75}, {8, 23, 35, 30, 4, 5, 85, 1, 20, 75}, {1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1}}, {{8, 21, 20, 14, 4, 0, 70, 2, 20, 75}, {8, 22, 30, 20, 4, 1, 55, 2, 20, 75}, {8, 23, 35, 30, 4, 5, 85, 2, 20, 75}, {2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2}}};
byte customPlantenBakSettings[3][4][12] = {{{25, 14, 60, 20, 75, 3, 0, 0, 11, 8}, {35, 28, 55, 10, 75, 2, 0, 0, 12, 8}, {30, 25, 35, 80, 4, 5, 85, 4, 1,13, 8}, {0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 0}}, {{8, 21, 20, 14, 4, 0, 70, 1, 20, 75}, {8, 22, 30, 20, 4, 1, 55, 1, 20, 75}, {8, 23, 35, 30, 4, 5, 85, 1, 20, 75}, {1, 1, 1, 1, 1, 1, 2, 2, 2, 1, 1, 1}}, {{8, 21, 20, 14, 4, 0, 70, 2, 20, 75}, {8, 22, 30, 20, 4, 1, 55, 2, 20, 75}, {8, 23, 35, 30, 4, 5, 85, 2, 20, 75}, {2, 2, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2}}};

byte klimaatDataNu[3][31]= {{4,4,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,4},{5,5,5,5,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,5,5,5},{6,6,6,6,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,6,6,6,6}};

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
            myGLCD.print(String("Dew"), 250, (i*71) + 43);
            myGLCD.print(String("Clouds"),190, (i*71) + 43);
            myGLCD.print(String("Rain"),280, (i*71) + 43);
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
            myGLCD.print(String(customPlantenBakSettings[bak][i][DUURREGEN]) + "H", 280, (i*71) + 55);
            myGLCD.print(String(customPlantenBakSettings[bak][i][DUURDAUW]) + "H", 240, (i*71) + 55);
            myGLCD.print(String(customPlantenBakSettings[bak][i][DUURBEWOLKING]) + "H", 200, (i*71) + 55);
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
                        myGLCD.print(String("jan"), 6, (i*71) + 29);
                    if(j==1) 
                        myGLCD.print(String("feb"), 32, (i*71) + 29);
                    if(j==2) 
                        myGLCD.print(String("mar"), 58, (i*71) + 29);
                    if(j==3)
                        myGLCD.print(String("apr"), 84, (i*71) + 29);
                    if(j==4)
                        myGLCD.print(String("may"), 110, (i*71) + 29);
                    if(j==5)
                        myGLCD.print(String("jun"), 136, (i*71) + 29);
                    if(j==6)
                        myGLCD.print(String("jul"), 162, (i*71) + 29);
                    if(j==7)
                        myGLCD.print(String("aug"), 188, (i*71) + 29);
                    if(j==8)
                        myGLCD.print(String("sep"), 214, (i*71) + 29);
                    if(j==9)
                        myGLCD.print(String("oct"), 240, (i*71) + 29);
                    if(j==10)
                        myGLCD.print(String("nov"), 266, (i*71) + 29);
                    if(j==11)
                        myGLCD.print(String("dec"), 292, (i*71) + 29);
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
        myGLCD.print("Chiang", 5, 221);
        myGLCD.print("Mai", 58, 221);
        myGLCD.print("Manaus", 99, 221);
        myGLCD.print("Sumatra", 175, 221);
        while (currentPage == 3) {
            if (currentPage == 3 && myTouch.dataAvailable()) {
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
                        betast(242, 215, 318, 238);
                        //String test = "nogmaals";
                        //toonStartScherm(test);
                        currentPage = 0;
                        break;
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
                        myGLCD.setColor(VGA_BLUE);
                        myGLCD.setBackColor(VGA_BLUE);
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
                myGLCD.setColor(VGA_WHITE);
                myGLCD.drawRoundRect((j*26) + 4, (k*71) + 29, (j*26) + 30, (k*71) + 42);
                }
                k = 1;
                for(int j = 0; j <12; j++) {
                    if (y>=(k*71) + 29 && y<=(k*71) + 42 && x>=(j*26) + 4 && x<=(j*26) + 30) { 
                        Serial.print("maand geselecteerd");
                        betast((j*26) + 4, (k*71) + 29, (j*26) + 30, (k*71) + 42);
                        customPlantenBakSettings[bak][3][j] = k;
                        myGLCD.setColor(VGA_RED);
                        // myGLCD.setBackColor(235,0,0);
                        myGLCD.setBackColor(VGA_RED);
                        for (int i = 0; i < 3; i++) {
                            myGLCD.drawRoundRect((j*26) + 4, (k*71) + 29, (j*26) + 30, (k*71) + 42);
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
                myGLCD.setColor(VGA_WHITE);
                myGLCD.drawRoundRect((j*26) + 4, (k*71) + 29, (j*26) + 30, (k*71) + 42);
                }
                k = 2;
                for(int j = 0; j <12; j++) {
                    if (y>=(k*71) + 29 && y<=(k*71) + 42 && x>=(j*26) + 4 && x<=(j*26) + 30) { 
                        Serial.print("maand geselecteerd");
                        betast((j*26) + 4, (k*71) + 29, (j*26) + 30, (k*71) + 42);
                        customPlantenBakSettings[bak][3][j] = k;
                        myGLCD.setColor(VGA_GRAY);
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
                myGLCD.setColor(VGA_WHITE);
                myGLCD.drawRoundRect((j*26) + 4, (k*71) + 29, (j*26) + 30, (k*71) + 42);
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
                if (currentPage ==2 && myTouch.dataAvailable()) {
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
                            //toonStartScherm(test);
                            currentPage = 0;
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
        while (currentPage == 4) {
            Serial.print("currentPage = ");
            Serial.println(currentPage);
            if (currentPage == 4 && myTouch.dataAvailable()) {
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
                            
                            // stCurrent[2]="";
                            // stCurrentLen=0;
                            // stLast[2]=""; 
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
        }
    }
};
class DataUitwisselaarSlave {
  
  public:
  DataUitwisselaarSlave(){
    pinMode(MISO, OUTPUT);// have to send on master in, *slave out*
    SPCR |= _BV(SPE);  // turn on SPI in slave mode
    SPCR |= _BV(SPIE); // turn on interrupts
  } 
  
  void updatKlimaatDataArray(){
    if (flag2 == true){//voledige boodschap ontavangen
      for (int k = 0; k < 3; k++){
        for (int l = 0; l < 31; l++){
          Serial.print(klimaatDataNu[k][l]);
        }
        Serial.println();
      }
      Serial.println();
      Serial.println("================");
      flag1 = false;
      flag2 = false;
    }
  }
};
  
  ISR (SPI_STC_vect){
       
    c = SPDR;
    if (flag1 == false){ //flag1 is true tijdens de uitwisseling van het array en wordt weer false na voltooiing loop
      if (c == 0xCD){ //als de startcode werd ontvangen
        SPDR = 0xEF; //teruggezonden startcode voor Master
        i = 0;
        j = 0;
       flag1 = true; //startcode ontvangen, na eerste keer geen startcode meer terugsturen
     }
   } else {
      if (c == 0xF3){
        SPDR = SPDR = klimaatDataNu[i][j];
      } else {
          if (i < 3){
            klimaatDataNu[i][j] = c;
            j++;
            SPDR = klimaatDataNu[i][j];
            if (j == 31){
              j = 0;
              i++;
              if (i == 3){
                flag2 = true;
              }
            }
          }
        }
     }
  }

DataUitwisselaarSlave dataUitwisselaarSlave;
TouchScreen touchScreen;

void setup (void){
  Serial.begin (9600);
}

void loop(){
  dataUitwisselaarSlave.updatKlimaatDataArray();
  if(currentPage == 1 or currentPage == 0) { ///waar komt die nul vandaan????????
        touchScreen.toonStartScherm(datumTijd);
    }
     while (currentPage == 1) {
        touchScreen.kiesPlantenBak();
        break;
    }
}