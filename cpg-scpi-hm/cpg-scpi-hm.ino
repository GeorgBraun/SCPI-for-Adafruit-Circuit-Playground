/*
  Software for Circuit Plaground from Adafruit with SCPI-Interface

  To enable robust code upload from Arduino IDE to CPG:
  Activate "Show verbose output during upload" within File->Preferences.

  Attention: Circuit Playground Express is currently not supported!
*/

#include <Adafruit_CircuitPlayground.h>

const String Version = "V 2.0.0";

const int Baudrate = 115200; //9600; //115200; //9600;

const String Trennzeichen = " "; // Trennzeichen für die Datenübertragung

boolean stringComplete = false;

String empfangen = "" ;


String Code;

/////////////////////////////////////////////////

int SystConTimestamp = 1;           
// 0 = off
// 1 = ms - Standart
// 2 = us  // Sinnlos -> Wurde entfernt
// 3 = s - Evtl Später Einfügen für langsame Messungen ??

int SystConMeasTint = 200;   // Timeintervall zwischen 2 Messungen: Standart 1 ms (Einheit durch Zeitstempel?)

int SystConMeasCount = 1;  //Anzahl der zu Messenden Werte - Standart -1

bool SystConMeasTypeSi = true; // Ob Messdaten als Si (umgerechnet) oder als rohdaten verschickt werden. War in früheren Versionen false.

int SystConMeasCapLimit = 50; //Limit ab wann Cap = 1

//////////////////////////////////////////////////


int SystConLedCol = 160;  //Standartfarbe der LEDs (0=rot, 32=gelb-orange, 64=grün, 96=türkis, 128=blau, 160=dunkelblau, 192=blau-violett, 224=violett, 255=rot)
int SystConLedColAlt = SystConLedCol;

unsigned int LedMuster = 0;      //Standartwert für das LED Muster
unsigned int LedMusteralt = LedMuster;

bool RedLed = false;     // Status der RotenLed
int RedLedPin = 13;

//////////////////////////////////////////////////

//Variablen für die Messschleifen

int Meastype = 0;
int AnzMessungen = 0;
int AnzZuMessendeWerte = SystConMeasCount;

String Messwert = "";

long millivergleich = 0;
long millioffset = 0;

bool sendetrue = false;

unsigned long millisCurrent; //Zeitstempel Variable

//////////////////////////////////////////////////

// Settings for development mode. Do not change.
//#define HMDEV

//////////////////////////////////////////////////
// Setup                                       ///
//////////////////////////////////////////////////

void setup() {
  empfangen.reserve(200);
  pinMode(RedLedPin, OUTPUT);
  reset();
  Serial.begin(Baudrate);
  while(!Serial); // wait for Arduino Serial Monitor (native USB boards)
  // Meldungen auskommentiert. Sind nur bei Terminalprogrammen hilfreich, stören bei Messprogrammen in Python, C# oer LabVIEW
  //Serial.println("==============================================================");
  //Serial.println("Hello from Adafruit Circuit Playground with SCPI-Interface");
  //Serial.println("==============================================================");
  CircuitPlayground.begin();

  // Play acoustic and visual signals to acknowledge serial connection
  //void playTone(uint16_t freq, uint16_t time, bool wait = true);
  #ifdef HMDEV
    CircuitPlayground.playTone(1760, 30, true); //delay(160);
  #endif
  CircuitPlayground.playTone(880, 50, true);  // delay(160);
  CircuitPlayground.playTone(1760, 50, true); //delay(160);
  DemoLED();
}

////////////////////
/// Loop         ///
////////////////////

void loop() {
  serialEvent();
  if (stringComplete) { stringverarbeitung();}
  Aktionen();
  millisCurrent = millis(); // "Zeitstempel" des Aktuellen Loops
  SendeMessdaten(); 
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Funktionen                                                                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////
/// Reset der Config Werte    ///
/////////////////////////////////

void reset() { // *RST
  SystConTimestamp = 1;
  SystConMeasTint = 200;
  SystConMeasCount = 1;
  SystConMeasTypeSi = true; // War in früheren Versionen false
  SystConLedCol = 160;
  SystConMeasCapLimit = 50;

  AnzZuMessendeWerte = SystConMeasCount;

  LedMuster = 0;
  RedLed = false;

  // Play acoustic signal to acknowledge reset
  //void playTone(uint16_t freq, uint16_t time, bool wait = true);
  #ifdef HMDEV
    CircuitPlayground.playTone(880, 30, true); //delay(160);
  #endif
  CircuitPlayground.playTone(440, 50, true); //delay(160);
  CircuitPlayground.playTone(880, 50, true); // delay(160);
}

/////////////////////////
// Identität schicken ///
/////////////////////////

void IDN() { // *IDN?
  Serial.println("==============================================================");
  Serial.println("Circuit Plaground from Adafruit with SCPI-Interface");
  //Serial.println("Serial Nr: ......................");
  //Serien Nr leider nicht vorhanden
  Serial.println("Hochschule Muenchen / Munich University of Applied Sciences");
  Serial.println("Fakultaet fuer angewandte Naturwissenschaften und Mechatronik");
  Serial.print(Version);
  Serial.println(" by Alexander Eras and Georg Braun");
  Serial.println("==============================================================");
}

/////////////////////
/// Messung       ///
/////////////////////

void Messungen() {
          if (Meastype == 0) {Messwert = "";}
          else if (Meastype == 1) {
                    if ( (digitalRead(4) | digitalRead(19)) == 1) {Messwert = "1";}
                    else                                          {Messwert = "0";}
          }
          else if (Meastype == 2)                                 {Messwert = digitalRead(4);}
          else if (Meastype == 3)                                 {Messwert = digitalRead(19);}
          else if (Meastype == 4)                                 {Messwert = digitalRead(21);}
          else if (Meastype == 5){
                  if (SystConMeasTypeSi == true)                  {Messwert = CircuitPlayground.temperature();} 
                  else                                            {Messwert = analogRead(A0);}
          }
          else if (Meastype == 6) {
                  // Beschleunigungsmessung durchführen:
                  CircuitPlayground.lis.read();
                  // Jetzt stehen in CircuitPlayground.lis.x, y, z die Rohdaten als int16_t
                  // und in CircuitPlayground.lis.x_g, y_g, z_g die auf 1g normierten Werte als float.
                  if (SystConMeasTypeSi == true) {
                            // SI-Ergebnisse in m/s^2:
                            Messwert = "";
                            // SENSORS_GRAVITY_STANDARD wird definiert als 9.80665F in Datei Documents\Arduino\libraries\Adafruit_Circuit_Playground\utility\Adafruit_CPlay_Sensor.h
                            Messwert += CircuitPlayground.lis.x_g * SENSORS_GRAVITY_STANDARD;
                            Messwert += Trennzeichen;
                            Messwert += CircuitPlayground.lis.y_g * SENSORS_GRAVITY_STANDARD;
                            Messwert += Trennzeichen;
                            Messwert += CircuitPlayground.lis.z_g * SENSORS_GRAVITY_STANDARD;
                  }
                  else {
                            // RAW-Ergebnisse:
                            Messwert = "";
                            Messwert += CircuitPlayground.lis.x;
                            Messwert += Trennzeichen;
                            Messwert += CircuitPlayground.lis.y;
                            Messwert += Trennzeichen;
                            Messwert += CircuitPlayground.lis.z;
                }
          }
          else if (Meastype == 7) {
                    Messwert = analogRead(A5);
                    // Previous versions generated an error message if in SI mode.
                    // For improved operational robustness, this is not done any longer.
                    // if (SystConMeasTypeSi == true) {
                    //   Messwert = ""; 
                    //   Serial.print("ERROR: LIGHT measurement ");
                    //   Serial.println("is only supported in RAW mode.");
                    // }
                    // else {Messwert = analogRead(A5);}
          }
          else if (Meastype == 8) {
                    Messwert = analogRead(A4);
                    // Previous versions generated an error message if in SI mode.
                    // For improved operational robustness, this is not done any longer.
                    // if (SystConMeasTypeSi == true) {
                    //   Messwert = ""; 
                    //   Serial.print("ERROR: SOUND measurement ");
                    //   Serial.println("is only supported in RAW mode.");
                    // }
                    // else {Messwert = analogRead(A4);}
          }
          else if (Meastype == 9) {
                    int CapWithLimit = 0;
                    bitWrite(CapWithLimit,0,(CircuitPlayground.readCap(3)>SystConMeasCapLimit));
                    bitWrite(CapWithLimit,1,(CircuitPlayground.readCap(2)>SystConMeasCapLimit));
                    bitWrite(CapWithLimit,2,(CircuitPlayground.readCap(0)>SystConMeasCapLimit));
                    bitWrite(CapWithLimit,3,(CircuitPlayground.readCap(1)>SystConMeasCapLimit));
                    bitWrite(CapWithLimit,4,(CircuitPlayground.readCap(12)>SystConMeasCapLimit));
                    bitWrite(CapWithLimit,5,(CircuitPlayground.readCap(6)>SystConMeasCapLimit));
                    bitWrite(CapWithLimit,6,(CircuitPlayground.readCap(9)>SystConMeasCapLimit));
                    bitWrite(CapWithLimit,7,(CircuitPlayground.readCap(10)>SystConMeasCapLimit));
                    Messwert = CapWithLimit;
          }
          else if (Meastype == 10) {
                    Messwert = "";
                    Messwert += CircuitPlayground.readCap(3);
                    Messwert += Trennzeichen;
                    Messwert += CircuitPlayground.readCap(2);
                    Messwert += Trennzeichen;
                    Messwert += CircuitPlayground.readCap(0);
                    Messwert += Trennzeichen;
                    Messwert += CircuitPlayground.readCap(1);
                    Messwert += Trennzeichen;
                    Messwert += CircuitPlayground.readCap(12);
                    Messwert += Trennzeichen;
                    Messwert += CircuitPlayground.readCap(6);
                    Messwert += Trennzeichen;
                    Messwert += CircuitPlayground.readCap(9);
                    Messwert += Trennzeichen;
                    Messwert += CircuitPlayground.readCap(10);
        }
        else if (Meastype == 11) {Messwert = "";}
}

////////////////////////
// SendeMessdaten    ///
////////////////////////

void SendeMessdaten() {
  //sendetrue = true, wenn zu Anz.zuMessendeWerte > 0 oder -1 (dann endlosschleife), wenn Tint vergangen ist, wenn Meastype != 0
  //   &&((millis()-millivergleich) > SystConMeasTint) && (Meastype != 0)
  sendetrue = ((AnzZuMessendeWerte > 0) || (AnzZuMessendeWerte == -1))  && (Meastype != 0) && ((millisCurrent - millivergleich) >= SystConMeasTint  );
  if (sendetrue) {
    Messungen();                                      // 1) Messung durchführen
    if (Messwert != "" || Meastype == 11) {           // 2) Timestamp einfügen + senden
      if (SystConTimestamp == 1) {
        Serial.print(millisCurrent);  // Offset ??
        Serial.print(Trennzeichen);
      }
      //else if (SystConTimestamp == 2) {             // Da micro nicht sinvoll ist auskommentiert
      //  Serial.print(micros());       
      //  Serial.print(Trennzeichen);
      //}
      Serial.println(Messwert);                         // 3) Messwert Senden
    }
    millivergleich = millisCurrent;  //Zurücksetzen der Werte
    if (AnzZuMessendeWerte != -1)  {
      AnzZuMessendeWerte--;
    }
    if (AnzZuMessendeWerte == 0)   {
      Meastype = 0;
      millivergleich = 0;
      AnzZuMessendeWerte = SystConMeasCount;
    }
  }
}

/////////////////////////////
///Aktualisiert die LEDs ///
/////////////////////////////

void Aktionen() {
  digitalWrite(RedLedPin, RedLed);
  if ((LedMusteralt != LedMuster) | (SystConLedCol != SystConLedColAlt))  {
    CircuitPlayground.clearPixels();
    LedMusteralt = LedMuster;
    SystConLedColAlt = SystConLedCol;
    for (int i = 0; i < 11; i++) {
      int state = bitRead(LedMuster, i);
      if (state != 0)  {
        CircuitPlayground.setPixelColor((i), CircuitPlayground.colorWheel(SystConLedCol));
      }
    }
  }
}

////////////////////////
/// Demo Led Kringel ///
////////////////////////

void DemoLED() {
  CircuitPlayground.clearPixels();
  for (int pixeln = 0; pixeln < 11; pixeln++) {
    CircuitPlayground.setPixelColor(pixeln, CircuitPlayground.colorWheel(25 * pixeln));
    delay(5);
  }
  delay(50);
  CircuitPlayground.clearPixels();
  LedMuster = 0; // Damit für den nächsten OUT:LED-Befehl die Bedingung (LedMusteralt != LedMuster) in void Aktionen() erfüllt ist.
}

////////////////////////////////////////
///Sendet die aktuelle Configuration ///
////////////////////////////////////////

void ConPrint() {
  Serial.println("=====================");
  Serial.println("Config:");
  Serial.print("  Timestamp: ");
  if (SystConTimestamp == 1)         {
    Serial.println("ms");
  }
  else if (SystConTimestamp == 0)    {
    Serial.println("off");
  }
  else if (SystConTimestamp == 2 )  {
    Serial.println("us");
  }
  Serial.print("  MeasCount: "); Serial.println(SystConMeasCount);
  Serial.print("  MeasTint:  "); Serial.println(SystConMeasTint);
  Serial.print("  Unit Type: "); Serial.println(SystConMeasTypeSi? "SI" : "RAW");
  Serial.print("  Led-Color: "); Serial.println(SystConLedCol);
  Serial.print("  Cap-Limit: "); Serial.println(SystConMeasCapLimit);
  Serial.println("=====================");
}

//////////////////////////////////////////////////////////////////
//// Serielle Daten auslesen und zu einem Befehl zusammenfügen ///
//////////////////////////////////////////////////////////////////

void serialEvent() {
  while (Serial.available()) {
    // digitalWrite(13,stringComplete); Für Testzwecke um "Schleifenhängen" zu untersuchen
    // get the new byte
    char inChar = (char)Serial.read();
    //add it to the inputString:
    //Wenn inchar nicht \n oder \r ist wird er angehangen
    if (inChar != '\n' && inChar != '\r') {
      empfangen += inChar;
    }
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n' || inChar == '\r' ) {
      if (empfangen.length() > 0) {
        stringComplete = true;
      }
      empfangen.trim();         //Trimmt Whitespace
      empfangen.toUpperCase();  //erzeugt Großbuchstaben
      break; //Schleife wird beendet sobald ein Befehl vollständig empfangen wurde
    }
  }
}

////////////////////////////////////////////////////
//// Falls Empfangener Befehl nicht Korrekt:  //////
////////////////////////////////////////////////////

void elsecap() {
  Serial.print("ERROR: The received message >>");
  Serial.print(empfangen);
  Serial.println("<< is not a valid command.");
  //Serial.println(empfangen);
  //Serial.println("Dies ist kein Gueltiger Befehl");
}

////////////////////////////////////////////////////
// Hier wird der Empfangene String Verarbeitet    //
////////////////////////////////////////////////////

void stringverarbeitung() {
        if (empfangen == "*IDN?") {
                IDN();
        }
        else if (empfangen == "*RST") {
                reset();
        }
  //////////////////////////////////OUT BLOCK///////////////////////////////
        else if (empfangen.startsWith("OUT:") && empfangen.length()>4) {
                if (empfangen.startsWith("LED:", 4) && empfangen.length()>8) {
                        //if (empfangen.startsWith("SINGLE", 8)) {
                        //        int indexofkomma = empfangen.indexOf(','); // Muss Noch implementiert werden
                        //}
                        if (empfangen.startsWith("RED", 8)) {
                                if (empfangen.endsWith("1"))  {RedLed = true;  }
                                else                          {RedLed = false; }                  
                        }
                        else {  elsecap();}
                }
                else if (empfangen.startsWith("LED", 4) && empfangen.length()>7) {
                        LedMuster = empfangen.substring(7).toInt();
                }
                else if (empfangen.startsWith("DEMO:LED", 4)) {
                        DemoLED();
                }
                else {elsecap();}
        }
  //////////////////////////////CONFIG BLOCK//////////////////////////////
        else if (empfangen.startsWith("SYST:") && empfangen.length()>5) {
                if (empfangen.startsWith("CON?", 5)) {
                        ConPrint();
                }
                else if (empfangen.startsWith("CON:", 5) && empfangen.length()>9) {
                        if (empfangen.startsWith("TIMESTAMP", 9)) {
                                if (empfangen.endsWith("OFF"))     {    SystConTimestamp = 0; }// 0 = off 
                                else if (empfangen.endsWith("MS")) {    SystConTimestamp = 1; }// ms = Standart
                                //else if (empfangen.endsWith("US")) {  //SystConTimestamp = 2;  //us 
                                //        Serial.println("A us timestamp is currently not supported."); }
                                else { elsecap(); } 
                        }
                        else if (empfangen.startsWith("MEAS:", 9) && empfangen.length()>14) {
                                if (empfangen.startsWith("TINT", 14) && empfangen.length()>18) {
                                        SystConMeasTint = empfangen.substring(18).toInt();
                                }
                                else if (empfangen.startsWith("COUNT", 14) && empfangen.length()>19) {
                                        SystConMeasCount = empfangen.substring(19).toInt();
                                        AnzZuMessendeWerte = SystConMeasCount;
                                }
                                else if (empfangen.startsWith("TYPE", 14) && empfangen.length()>18){
                                        if (empfangen.endsWith("RAW"))      {SystConMeasTypeSi = false;}
                                        else if (empfangen.endsWith("SI"))  {SystConMeasTypeSi = true; }       
                                        else {elsecap();}
                                }
                                else if (empfangen.startsWith("CAPLIM", 14) && empfangen.length()>20) {
                                        SystConMeasCapLimit = empfangen.substring(20).toInt();
                                }
                                else {elsecap();}
                        }
                        else if (empfangen.startsWith("LED", 9) && empfangen.length()>13) {
                                if (empfangen.startsWith("COL", 13) && empfangen.length()>16) {
                                        SystConLedCol = empfangen.substring(16).toInt();
                                }
                                else {elsecap();}
                        }
                        else {elsecap();}
                        }
                 else {elsecap();}      
        }
    ///////////Meas Block//////////////////////////
        else if (empfangen.startsWith("MEAS:") && empfangen.length()>5) {
                if (empfangen.startsWith("STOP", 5))                  {Meastype = 0;  Messwert = ""; }
                else if (empfangen.startsWith("BUTTON?", 5))          {Meastype = 1; }
                else if (empfangen.startsWith("BUTTON:", 5) && empfangen.length()>12) {
                        if (empfangen.startsWith("RIGHT?", 12))       {Meastype = 3;}
                        else if (empfangen.startsWith("LEFT?", 12))   {Meastype = 2;}
                        else {elsecap();}
                }
                else if (empfangen.startsWith("SWITCH?", 5))          {Meastype = 4;}
                else if (empfangen.startsWith("TEMP?", 5))            {Meastype = 5;}
                else if (empfangen.startsWith("ACC?", 5))             {Meastype = 6;}
                else if (empfangen.startsWith("LIGHT?", 5))           {Meastype = 7;}
                else if (empfangen.startsWith("SOUND?", 5))           {Meastype = 8;}
                else if (empfangen.startsWith("CAP:", 5) && empfangen.length()>9) {
                        if (empfangen.startsWith("TAP?", 9))          {Meastype = 9;}
                        else if (empfangen.startsWith("SENSE?", 9))   {Meastype = 10;}
                        else {elsecap();}
                }
                else if (empfangen.startsWith("TIME?", 5))            {Meastype = 11;}
                else {elsecap();}
        }
        else {elsecap();}
    // clear the string:
    empfangen = "";
    stringComplete = false;
}
  
