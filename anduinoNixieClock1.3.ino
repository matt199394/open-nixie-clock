

      ////////////////////////////////////////
       // Nixie Clock Emiliano Fanfani FFF //
        ////////////////////////////////////

//variabili temperatura

#define INTERVALLO 5000

unsigned long t0, dt;
float tc = 0;
float decimal = 0;
     
// Includo le librerie
#include <Wire.h>
#include <RTClib.h>
#include <EEPROM.h>
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(12);
DallasTemperature temp(&oneWire);

// Decoder BCD k155id1/SN74141 IC 
const byte k155id1_a = 14;                
const byte k155id1_b = 15;
const byte k155id1_c = 16;
const byte k155id1_d = 17;

// Anodica Nixie
const byte Anode_S1 = 4;
const byte Anode_S10 = 5;
const byte Anode_M1 = 2;
const byte Anode_M10 = 3;
const byte Anode_H1 = 1;
const byte Anode_H10 = 0;

// Colonne separatori
const byte colons_1 = 6;
const byte colons_2 = 7;

// Assegno i pin di arduino all'encoder CLK, DT, e SW
const byte ROTCLKPIN = 8;
const byte ROTDTPIN = 9;
const byte ROTBTTNPIN = 10;

// Indirizzi EEPROM 
const byte twelveHourModeADDRESS = 0;
const byte ROTATETIMEADDRESS = 1;
const byte TRANSMODEADDRESS = 2;
const byte ONHOURADDRESS = 3;
const byte OFFHOURADDRESS = 4;

// Specifico che tipo di RTC utilizzo
RTC_DS3231 rtc; 

// Variabili intere ore, minuti, secondi, anno, mese e giorno
int hour, minute, second, year, month, day; 

int currentStateCLK;
int lastStateCLK;
unsigned long lastButtonPress = 0;

// Tempo in secondi, min 4, max 59
int rotatespeed = EEPROM.read(ROTATETIMEADDRESS);  
// Formato ora 0 per formato 12 ore, 1 per formato 24 ore        
bool twelveHourMode = EEPROM.read(twelveHourModeADDRESS); 
int TransMode = EEPROM.read(TRANSMODEADDRESS);
int onHour = EEPROM.read(ONHOURADDRESS);
int offHour = EEPROM.read(OFFHOURADDRESS);

bool dateOrTime = false;
bool displayOff = false;

unsigned long lastBlinkTime = 0; 
unsigned long blinkTime = 500;
unsigned long waitForDisplay = 0; 
int nextSwitch = 0;

// 0  Ora, 1 Data, 2 Rotazione Ora Data
int displayMode = 0;

// Ritardi bloccanti per multiplexing
static const int multiplexDelay = (2800);
static const int delayoff = (500);

// Variabili temporizzatrici
int debounce = 20;          
int DCgap = 250;            
int holdTime = 2000;        
int longHoldTime = 3000;    

// Variabili tasto
boolean buttonVal = HIGH;   
boolean buttonLast = HIGH;  
boolean DCwaiting = false;  
boolean DConUp = false;     
boolean singleOK = true;   
long downTime = -1;         
long upTime = -1;           
boolean ignoreUp = false;   
boolean waitForUp = false;        
boolean holdEventPast = false;    
boolean longHoldEventPast = false;

bool Anode_S1_sett = true;
bool Anode_S10_sett = true;
bool Anode_M1_sett = true;
bool Anode_M10_sett = true;
bool Anode_H1_sett = true;
bool Anode_H10_sett = true;

bool colons_1_sett = false;
bool colons_2_sett = false;

bool colonOFF = false;
bool colonON = false;

float fadeMax = 2.8f;
float fadeStep = 0.1f;
int NumberArray[6]={0,0,0,0,0,0};
int currNumberArray[6]={0,0,0,0,0,0};
float NumberArrayFadeInValue[6]={0.0f,0.0f,0.0f,0.0f,0.0f,0.0f};
float NumberArrayFadeOutValue[6]={5.0f,5.0f,5.0f,5.0f,5.0f,5.0f};

const byte N_NIXIES = 6;

static const unsigned int nixies[N_NIXIES] = {Anode_S1, Anode_S10, Anode_M1, Anode_M10, Anode_H1, Anode_H10};
bool Anode_sett[N_NIXIES] = {Anode_S1_sett, Anode_S10_sett, Anode_M1_sett, Anode_M10_sett, Anode_H1_sett, Anode_H10_sett};
static volatile byte unsigned nixie_val[N_NIXIES] = {0};
static byte unsigned nixie_set[N_NIXIES] = {0};

// Posizione del numero nella nixie
static const byte unsigned nixie_level[10 + 1] = { 4, 1, 7, 5, 9, 3, 10, 0, 6, 2, 8};   

// Variabili Per Uscita Forzata Standby 
bool Trigger = false;
bool DTFdisplayOn = false;
unsigned long Counter = 0;
unsigned long SpentCounter = 0;

//=====================setup()====================================

void setup() {

  t0 = millis();
  Wire.begin();
  // Inizializzo RTC
  if (!rtc.begin()) {
    while (1){
      delay(500);
      digitalWrite(colons_1, HIGH);
      digitalWrite(colons_2, HIGH);
      delay(500);
      digitalWrite(colons_1, LOW);
      digitalWrite(colons_2, LOW);
      delay(500);
    }  
  }
  
  temp.begin();
  temp.setResolution(10);
    
  pinMode(k155id1_a, OUTPUT);      
  pinMode(k155id1_b, OUTPUT);      
  pinMode(k155id1_c, OUTPUT);      
  pinMode(k155id1_d, OUTPUT);    
  
  pinMode(Anode_S1, OUTPUT);      
  pinMode(Anode_S10, OUTPUT);
  pinMode(Anode_M1, OUTPUT); 
  pinMode(Anode_M10, OUTPUT);      
  pinMode(Anode_H1, OUTPUT);
  pinMode(Anode_H10, OUTPUT);
  
  pinMode(ROTCLKPIN, INPUT);
  pinMode(ROTDTPIN, INPUT);
  pinMode(ROTBTTNPIN, INPUT);
  digitalWrite(ROTBTTNPIN, HIGH);

  pinMode(colons_1, OUTPUT);
  pinMode(colons_2, OUTPUT);

  lastStateCLK = digitalRead(ROTCLKPIN);
  
  getDateTime();
  delay (100);   
  
  // Controllo Funzionamento Numeri Nixie
  const int timer_8s_delay = 8000;   
  long timer_delay = 0;       
  while (millis() < timer_8s_delay) {
    if (millis() > timer_delay) {
      timer_delay = millis() + (timer_8s_delay / 10);

      int i = millis() / (timer_8s_delay / 10);
      NumberArray[0] = i;
      NumberArray[1] = i;
      NumberArray[2] = i;
      NumberArray[3] = i;
      NumberArray[4] = i;
      NumberArray[5] = i;
   }
    writeToNixieRAW(NumberArray);
 }

  // Controllo Funzionamento Colonne Separatrici
  delay(1000);
  digitalWrite(colons_1, HIGH);
  digitalWrite(colons_2, HIGH);
 
  delay(1000);
  digitalWrite(colons_1, LOW);
  digitalWrite(colons_2, LOW);
  delay(1000);

  TransitionEffect();
  
}

//=====================end setup()=============================

// Funzione lettura encoder
int readRotEncoder(int counter) 
{
  // Leggo lo stato attuale
  currentStateCLK = digitalRead(ROTCLKPIN);
  
  // Vario il conteggio solo se c'è un doppio impulso
  if (currentStateCLK != lastStateCLK == 1)
  {
    // Se ruoto in senso antiorario decremento
    if (digitalRead(ROTDTPIN) != currentStateCLK)
    {
      counter--;
    }
    else
    {
      // // Se ruoto in senso orario incremento
      counter++;
    }
  }
  
  // Memorizzo l'ultima lettura
  lastStateCLK = currentStateCLK;
  return counter;
}

int checkButton() 
{
  int event = 0;
  buttonVal = digitalRead(ROTBTTNPIN);
  
  // Premo il tasto
  if (buttonVal == LOW && buttonLast == HIGH && (millis() - upTime) > debounce)
  {
    downTime = millis();
    ignoreUp = false;
    waitForUp = false;
    singleOK = true;
    holdEventPast = false;
    longHoldEventPast = false;
    if ((millis() - upTime) < DCgap && DConUp == false && DCwaiting == true)  DConUp = true;
    else  DConUp = false;
    DCwaiting = false;
  }
  // Tasto rilasciato
  else if (buttonVal == HIGH && buttonLast == LOW && (millis() - downTime) > debounce)
  {
    if (!ignoreUp)
    {
      upTime = millis();
      if (DConUp == false) DCwaiting = true;
      else
      {
        event = 2;
        DConUp = false;
        DCwaiting = false;
        singleOK = false;
      }
    }
  }
  // Controllo singola pressione tasto
  if ( buttonVal == HIGH && (millis() - upTime) >= DCgap && DCwaiting == true && DConUp == false && singleOK == true && event != 2)
  {
    event = 1;
    DCwaiting = false;
  }
  // Controllo se il tasto è ancora premuto
  if (buttonVal == LOW && (millis() - downTime) >= holdTime) {
    // Trigger "normal" hold
    if (!holdEventPast)
    {
      event = 3;
      waitForUp = true;
      ignoreUp = true;
      DConUp = false;
      DCwaiting = false;
      holdEventPast = true;
    }
    // Controllo se il tasto è premuto a lungo
    if ((millis() - downTime) >= longHoldTime)
    {
      if (!longHoldEventPast)
      {
        Trigger = !Trigger;         
        longHoldEventPast = true;
      }
    }
  }
   buttonLast = buttonVal;
   return event;
}

//Menù di selezione
int changeMode(int mode, int numOfModes) 
{
  int lastmode = mode;
  mode = readRotEncoder(mode);
  if (mode > numOfModes) 
  {
    mode = 0;
  }
  else if (mode < 0)
  {
    mode = numOfModes;
  }
  if (mode != lastmode)
  {
    waitForDisplay = millis();
    while (millis() - waitForDisplay < 500)
    {
      writeToNixie(mode, 255, 255);
      mode = changeMode(mode, numOfModes);
    }
  }
  return mode;
}

//==========================loop ()=============================================
void loop() {

  getDateTime();

  if (second % 2 == 0 && !colonOFF) 
   {
   
    PORTD &= B00111111;
    colonOFF = true;
    colonON = false;
   }
    else if (second % 2 == 1 && !colonON)
     {
    
    PORTD |= B11000000;
    colonON = true;
    colonOFF = false;
     }
 
  // Spengo Le nixie
  if ((hour >= offHour && hour > onHour && offHour > onHour || hour <= offHour && hour < onHour && offHour > onHour
  || hour >= offHour && hour < onHour) && !DTFdisplayOn && !Trigger)
  {   
    digitalWrite(colons_1, LOW);
    digitalWrite(colons_2, LOW);  
    displayOff = !displayOff;
    
    while (displayOff)
     {
       Anode_S1_sett = false;
       Anode_S10_sett = false;
       Anode_M1_sett = false;
       Anode_M10_sett = false;
       Anode_H1_sett = false;
       Anode_H10_sett = false;
       colons_1_sett = false;
       colons_2_sett = false;
   
       getDateTime();
       
      
    
       // Accendo Le Nixie
       if (hour == onHour && onHour != offHour && displayOff) 
        {
          displayOff = !displayOff;
        }
        
           if (checkButton())
          {
             DTFdisplayOn = !DTFdisplayOn;
             displayOff = !displayOff;
             Counter = millis();              
            }             
    }
     TransitionEffect(); 
  }   

 if (DTFdisplayOn)  
  {   
     SpentCounter = millis() - Counter;
   
   if (SpentCounter >= 15000)
    {
       DTFdisplayOn = false; 
       Counter = 0;
     }
   }
   
    else if (Trigger)  
     {   
       if (hour == onHour && onHour != offHour)
       {
         Trigger = false;
       }
     }
 
  Anode_S1_sett = true;
  Anode_S10_sett = true;
  Anode_M1_sett = true;
  Anode_M10_sett = true;
  Anode_H1_sett = true;
  Anode_H10_sett = true;
  colons_1_sett = true;
  colons_2_sett = true;  


  if (minute % 5 == 0 && second == 0 )
   {
     TransitionEffect();
   }

  // Entro nel menù solamente con la doppia pressione del tasto
  if (checkButton() == 2) 
   {
     settingsMenu();
   }
   
  displayMode = changeMode(displayMode, 3);
  
  switch (displayMode)
   {
   case 0:
     normalTimeMode();
     break;
   case 1:
     dateMode();
     break;
   case 2:
     rotateMode();
     break;
   case 3:
     temperatureMode();
     break;
   }

}


//=========================end loop()==========================================

void settingsMenu()
{
  if (settingsMenu)
   {
     digitalWrite(colons_1, LOW);
     digitalWrite(colons_2, LOW);
   }
  
  int settingsMode = 0;
  bool exitSettings = false;
  lastBlinkTime = millis();
  while (!exitSettings)
  {
    settingsMode = displayMode = changeMode(displayMode, 6);
    switch (settingsMode)
    {
    case 0: 
      writeToNixie(settingsMode, 255, 255);
      break;
    case 1: 
      if ((millis() - lastBlinkTime) < blinkTime)
      {
        writeToNixie(hour, minute, second);
      }
      else if (((millis() - lastBlinkTime) > blinkTime * 2))
      {
        writeToNixie(hour, minute, second);
        lastBlinkTime = millis(); 
      }
      else
      {
        writeToNixie(255, 255, 255);
      }
      break;
    case 2: 
      if ((millis() - lastBlinkTime) < blinkTime)
      {
        writeToNixie(day, month, year);
      }
      else if (((millis() - lastBlinkTime) > blinkTime * 2))
      {
        writeToNixie(day, month, year);
        lastBlinkTime = millis();
      }
      else
      {
        writeToNixie(255, 255, 255);
      }
      break;
    case 3: 
      writeToNixie(settingsMode, 255, rotatespeed);
      break;
    case 4: 
      if (twelveHourMode)
      {
        writeToNixie(settingsMode, 255, 12);
      }
      else
      {
        writeToNixie(settingsMode, 255, 24);
      }
      break;
    case 5: 
      writeToNixie(settingsMode, 255, TransMode);
      break;
    case 6: 
      writeToNixie(settingsMode, offHour, onHour);
      break;
    //case 7:
      //writeToNixie(settingsMode, poisonTimeStart, poisonTimeSpan);
    //  break;
    }

    if (checkButton())
    {
      switch (settingsMode)
      {
      case 0:
        exitSettings = true;
        break;
      case 1:
        setTime();
        break;
      case 2:
        setDate();
        break;
      case 3:
        setRotationSpeed();
        break;
      case 4:
        setHourDisplay();
        break;
      case 5:
        setTransitionMode();
        break;
      case 6:
        setOnOffTime();
        break;
      }
    }
  }
}

void normalTimeMode()
{
  
  switch (TransMode)
   {
      case 0:
      
        if (twelveHourMode)
        {
          if (hour > 12)
           {
             writeToNixie(hour - 12, minute, second);
           }
            else if (hour == 00)
             {
               writeToNixie(12, minute, second);
             }
             else
              {
                writeToNixie(hour, minute, second);
              }
        }
        else
         {
           writeToNixie(hour, minute, second);
         } 
         
        break;
      case 1:  
      
        fadeMax = 2.8f;
        fadeStep = 0.2f;
                  
        if (twelveHourMode)
        {
          if (hour > 12)
           {
             writeToNixieFadeMalfunction(hour - 12, minute, second);
           }
            else if (hour == 00)
             {
               writeToNixieFadeMalfunction(12, minute, second);
             }
             else
              {
                writeToNixieFadeMalfunction(hour, minute, second);
              }
        }
        else
         {
           writeToNixieFadeMalfunction(hour, minute, second);
         }       

        break;
        
      case 2:
      
        if (twelveHourMode)
        {
          if (hour > 12)
           {
             writeToNixieScroll(hour - 12, minute, second);
           }
            else if (hour == 00)
             {
               writeToNixieScroll(12, minute, second);
             }
             else
              {
                writeToNixieScroll(hour, minute, second);
              }
        }
        else
         {
           writeToNixieScroll(hour, minute, second);
         }          
         
        break;
      case 3:
      
        fadeMax = 2.8f;
        fadeStep = 0.1f;
       
         if (twelveHourMode)
        {
          if (hour > 12)
           {
             writeToNixieFade(hour - 12, minute, second);
           }
            else if (hour == 00)
             {
               writeToNixieFade(12, minute, second);
             }
             else
              {
                writeToNixieFade(hour, minute, second);
              }
        }
        else
         {
           writeToNixieFade(hour, minute, second);
         }    

        break;
      }
}

void dateMode()
{ 
  if (!colonOFF) 
   { 
    PORTD &= B00111111;
    colonOFF = true;
    colonON = false;
   }
	
  writeToNixie(day, month, year); 
}

void rotateMode()
{

  if (second >= 50 && second <= 50 + rotatespeed)
    {
    dateOrTime = false;
    }
  else dateOrTime = true;

   if (dateOrTime)
    {
      normalTimeMode();
     }
   else
   {
     dateMode();
   }
 }

void temperatureMode()
{   
	if(!colonON)
     {
    PORTD |= B01000000;
    colonON = true;
    colonOFF = false;
     }
	
    dt = millis() - t0;
    if ( dt >= INTERVALLO )getTemperature();
    
  writeToNixie(255, (int)tc, (int)decimal %100); 
}

void getTemperature()
{
    t0 = millis();
    temp.requestTemperatures();
    tc = temp.getTempCByIndex(0);
    decimal = tc*100;
}

void setTime()
{
  int lasthour, lastminute, lastsecond;
  int digitMod = 0;
  lastBlinkTime = millis();
  while (digitMod < 3)
  {
    lasthour = hour;
    lastminute = minute;
    lastsecond = second;
    
    if (checkButton())
    {
      lastBlinkTime = millis(); 
      digitMod++;
    }
    if ((millis() - lastBlinkTime) < blinkTime)
    {
      writeToNixie(hour, minute, second);
    }
    else if (((millis() - lastBlinkTime) > blinkTime * 2))
    {
      writeToNixie(hour, minute, second);
      lastBlinkTime = millis(); 
    }
    else
    {
      //switch off
      switch (digitMod)
      {
      case 0:
        writeToNixie(255, minute, second);
        break;
      case 1:
        writeToNixie(hour, 255, second);
        break;
      case 2:
        writeToNixie(hour, minute, 255);
        break;
      }
    }

    switch (digitMod)
    {
    case 0:
      hour = readRotEncoder(hour);
      if (hour != lasthour)
      {
        lastBlinkTime = millis(); 
        if (hour > 23)
        {
          hour = 0;
        }
        else if (hour < 0)
        {
          hour = 23;
        }
      }
      break;
    case 1:
      minute = readRotEncoder(minute);
      if (minute != lastminute)
      {
        lastBlinkTime = millis(); 
        if (minute > 59)
        {
          minute = 0;
        }
        else if (minute < 0)
        {
          minute = 59;
        }
      }
      break;
    case 2:
      second = readRotEncoder(second);
      if (second != lastsecond)
      {
        lastBlinkTime = millis(); 
        if (second > 59)
        {
          second = 0;
        }
        else if (second < 0)
        {
          second = 59;
        }
      }
      break;
    }
  }
  rtc.adjust(DateTime(year, month, day, hour, minute, second));
}
void setDate()
{
  int lastyear, lastmonth, lastday;
  int digitMod = 0;
  lastBlinkTime = millis();
  while (digitMod < 3)
  {
    lastyear = year;
    lastmonth = month;
    lastday = day;
 
    if (checkButton())
    {
      lastBlinkTime = millis(); 
      digitMod++;
    }
    if (digitMod == 2)
    {
      writeToNixie(255, year / 100, year % 100);
    }
    else
    {
      if ((millis() - lastBlinkTime) < blinkTime)
      {
        writeToNixie(day, month, year);
      }
      else if (((millis() - lastBlinkTime) > blinkTime * 2))
      {
        writeToNixie(day, month, year);
        lastBlinkTime = millis(); 
      }
      else
      {
        switch (digitMod)
        {
        case 0:
          writeToNixie(255, month, year); 
          break;
        case 1:
          writeToNixie(day, 255, year);
          break;
        }
      }
    }

    switch (digitMod)
    {
    case 0:
      day = readRotEncoder(day);
      if (day != lastday)
      {
        lastBlinkTime = millis(); 
        if (day > 31)
        {
          day = 1;
        }
        else if (day < 1)
        {
          day = 31;
        }
      }
      break;
    case 1:
      month = readRotEncoder(month);
      if (month != lastmonth)
      {
        lastBlinkTime = millis(); 
        if (month > 12)
        {
          month = 1;
        }
        else if (month < 1)
        {
          month = 12;
        }
      }
      break;
    case 2:
      year = readRotEncoder(year);
      if (year != lastyear)
      {
        lastBlinkTime = millis(); 
        if (year > 2100)
        {
          year = 2100;
        }
        else if (year < 2020)
        {
          year = 2020;
        }
      }
      break;
    }
  }
  DateTime now = rtc.now();
  hour = now.hour();
  minute = now.minute();
  second = now.second();
  rtc.adjust(DateTime(year, month, day, hour, minute, second));
}
void setHourDisplay()
{
  bool origBool = twelveHourMode;
  int counter = 0;
  if (twelveHourMode)
  {
    counter = 1;
  }
  while (!checkButton())
  {
    counter = readRotEncoder(counter);
    if (counter % 2 == 0)
    {
      twelveHourMode = false;
    }
    else
    {
      twelveHourMode = true;
    }
    if (twelveHourMode)
    { 
      writeToNixie(255, 255, 12);
    }
    else
    {
      writeToNixie(255, 255, 24);
    }
  }
  if (origBool != twelveHourMode) 
  {
    EEPROM.write(twelveHourModeADDRESS, (bool)twelveHourMode); 
  }
}
void setRotationSpeed()
{
  int origRotateSpeed = rotatespeed;
  while (!checkButton())
  {
    rotatespeed = readRotEncoder(rotatespeed);
    if (rotatespeed > 59)
    {
      rotatespeed = 59;
    }
    if (rotatespeed < 0)
    {
      rotatespeed = 0;
    }    
    writeToNixie(255, 255, rotatespeed);
  }
  if (origRotateSpeed != rotatespeed)
  {
    EEPROM.write(ROTATETIMEADDRESS, rotatespeed); 
  }
}

void setTransitionMode()  
{
  int origTransMode = TransMode;
  while (!checkButton ())
  {
    TransMode = readRotEncoder(TransMode);
    if (TransMode > 3)
    {
      TransMode = 0;
    }
    if (TransMode < 0)
    {
      TransMode = 3;
    }
    writeToNixie(255, 255, TransMode);
  }
  if (origTransMode != TransMode)
  {
    EEPROM.write(TRANSMODEADDRESS, TransMode); 
  }
}
void setOnOffTime() 
{
  int origOffHour = offHour; 
  int origOnHour = onHour;
  int lastOnHour, lastOffHour;
  int varMod = 0;
  lastBlinkTime = millis();
  while (varMod < 2)
  {
    lastOffHour = offHour;
    lastOnHour = onHour;
    if (checkButton())
    {
      varMod++;
    }
    if ((millis() - lastBlinkTime) < blinkTime)
    {
      writeToNixie(255, offHour, onHour);
    }
    else if (((millis() - lastBlinkTime) > blinkTime * 2))
    {
      writeToNixie(255, offHour, onHour);
      lastBlinkTime = millis(); 
    }
    else
    {
      switch (varMod)
      {
      case 0:
        writeToNixie(255, 255, onHour);
        break;
      case 1:
        writeToNixie(255, offHour, 255);
        break;
      }
    }
    switch (varMod)
    {
    case 0:
      offHour = readRotEncoder(offHour); 
      if (offHour != lastOffHour)
      {
        lastBlinkTime = millis(); 
        if (offHour > 23)
        {
          offHour = 0;
        }
        if (offHour < 0)
        {
          offHour = 23;
        }
      }
      break;
    case 1:
      onHour = readRotEncoder(onHour); 
      if (onHour != lastOnHour)
      {
        lastBlinkTime = millis(); 
        if (onHour > 23)
        {
          onHour = 0;
        }
        if (onHour < 0)
        {
          onHour = 23;
        }
      }
      break;
    }
  }
  if (origOffHour != offHour)
  {
    EEPROM.write(OFFHOURADDRESS, offHour); 
  }
  if (origOnHour != onHour)
  {
    EEPROM.write(ONHOURADDRESS, onHour); 
  }
}

void getDateTime()
{
  DateTime now = rtc.now();
  hour = now.hour();
  minute = now.minute();
  second = now.second();
  year = now.year();
  month = now.month();
  day = now.day();
}

void setNumber(int num)
{
  int a,b,c,d;
  
  // Carico i valori a, b, c, d da inviare al convertitore binario decimale k155id1/SN74141 IC
  switch(num)
  {
    case 0: a=0;b=0;c=0;d=0;break;
    case 1: a=1;b=0;c=0;d=0;break;
    case 2: a=0;b=1;c=0;d=0;break;
    case 3: a=1;b=1;c=0;d=0;break;
    case 4: a=0;b=0;c=1;d=0;break;
    case 5: a=1;b=0;c=1;d=0;break;
    case 6: a=0;b=1;c=1;d=0;break;
    case 7: a=1;b=1;c=1;d=0;break;
    case 8: a=0;b=0;c=0;d=1;break;
    case 9: a=1;b=0;c=0;d=1;break;
    default: a=1;b=1;c=1;d=1;
    break;
  }  
  
  // Abilito i pin con i dati
  digitalWrite(k155id1_d, d);
  digitalWrite(k155id1_c, c);
  digitalWrite(k155id1_b, b);
  digitalWrite(k155id1_a, a);
}

// Scrittura nixie
void writeToNixie(int hours, int minutes, int seconds)
{
  NumberArray[0] = seconds % 10;
  NumberArray[1] = (seconds / 10) % 10;
  NumberArray[2] = minutes % 10;
  NumberArray[3] = (minutes / 10) % 10;
  NumberArray[4] = hours % 10;
  NumberArray[5] = (hours / 10) % 10;

      if (seconds == 255)
  {
    Anode_S1_sett = false;
    Anode_S10_sett = false;
  }
  else
  {
    Anode_S1_sett = true;
    Anode_S10_sett = true;
  }

      if (minutes == 255)
  {
    Anode_M1_sett= false;
    Anode_M10_sett = false;
  }
  else
  {
    Anode_M1_sett = true;
    Anode_M10_sett = true;
  }

       if (hours == 255)
  {
    Anode_H1_sett = false;
    Anode_H10_sett = false;
  }
  else
  {
    Anode_H1_sett = true;
    Anode_H10_sett = true;
  }
  
 bool Anode_sett[N_NIXIES] = {Anode_S1_sett, Anode_S10_sett, Anode_M1_sett, Anode_M10_sett, Anode_H1_sett, Anode_H10_sett};

  for ( int i = 0 ; i < N_NIXIES ; i ++ )
  {
   setNumber(NumberArray[i]);
   digitalWrite(nixies[i], Anode_sett[i]);
   delayMicroseconds(multiplexDelay);
   digitalWrite(nixies[i], LOW);
   delayMicroseconds(delayoff);        
  }
}

// Scrittura nixie modalita RAW
void writeToNixieRAW(int NumberArray[5]) 
{
  for (int i = 0 ; i < N_NIXIES ; i ++)
  {
  setNumber(NumberArray[i]);   
  digitalWrite(nixies[i], Anode_sett[i]);   
  delayMicroseconds(multiplexDelay);   
  digitalWrite(nixies[i], LOW);
  delayMicroseconds(delayoff);   
  }
}  

// Scrittura nixie effetto dissolvenza
void writeToNixieFade(int hours, int minutes, int seconds)
{ 
  NumberArray[0] = seconds % 10;
  NumberArray[1] = (seconds / 10) % 10;
  NumberArray[2] = minutes % 10;
  NumberArray[3] = (minutes / 10) % 10;
  NumberArray[4] = hours % 10;
  NumberArray[5] = (hours / 10) % 10;

  setNumber(currNumberArray[0]);   
  digitalWrite(nixies[0], Anode_sett[0]);   
  delay(NumberArrayFadeOutValue[0]);
  setNumber(NumberArray[0]);   
  delay(NumberArrayFadeInValue[0]);
  digitalWrite(nixies[0], LOW);
  delayMicroseconds(delayoff); 

    setNumber(currNumberArray[1]);   
  digitalWrite(nixies[1], Anode_sett[1]);   
  delay(NumberArrayFadeOutValue[1]);
  setNumber(NumberArray[1]);   
  delay(NumberArrayFadeInValue[1]);
  digitalWrite(nixies[1], LOW);
  delayMicroseconds(delayoff);

    setNumber(currNumberArray[2]);   
  digitalWrite(nixies[2], Anode_sett[2]);   
  delay(NumberArrayFadeOutValue[2]);
  setNumber(NumberArray[2]);   
  delay(NumberArrayFadeInValue[2]);
  digitalWrite(nixies[2], LOW);
  delayMicroseconds(delayoff);

   setNumber(currNumberArray[3]);   
  digitalWrite(nixies[3], Anode_sett[3]);   
  delay(NumberArrayFadeOutValue[3]);
  setNumber(NumberArray[3]);   
  delay(NumberArrayFadeInValue[3]);
  digitalWrite(nixies[3], LOW);
  delayMicroseconds(delayoff);    


  setNumber(currNumberArray[4]);   
  digitalWrite(nixies[4], Anode_sett[4]);   
  delay(NumberArrayFadeOutValue[4]);
  setNumber(NumberArray[4]);   
  delay(NumberArrayFadeInValue[4]);
  digitalWrite(nixies[4], LOW);
  delayMicroseconds(delayoff);

  setNumber(currNumberArray[5]);   
  digitalWrite(nixies[5], Anode_sett[5]);   
  delay(NumberArrayFadeOutValue[5]);
  setNumber(NumberArray[5]);   
  delay(NumberArrayFadeInValue[5]);
  digitalWrite(nixies[5], LOW);
  delayMicroseconds(delayoff);
  
  for (int i = 0; i < N_NIXIES; i++)
  {

    // Aggiorna tutti gli array e le dissolvenze
    if (NumberArray[i] != currNumberArray[i])
    {
      NumberArrayFadeInValue[i] += fadeStep;      
      NumberArrayFadeOutValue[i] -= fadeStep;
  
      if (NumberArrayFadeInValue[i] >= fadeMax)
      {
        NumberArrayFadeInValue[i] = fadeStep;
        NumberArrayFadeOutValue[i] = fadeMax; 
        currNumberArray[i] = NumberArray[i];
      }
    }
  }                   
}

// Scrittura nixie efetto malfunzionamento
void writeToNixieFadeMalfunction(int hours, int minutes, int seconds)
{ 
  NumberArray[0] = seconds % 10;
  NumberArray[1] = (seconds / 10) % 10;
  NumberArray[2] = minutes % 10;
  NumberArray[3] = (minutes / 10) % 10;
  NumberArray[4] = hours % 10;
  NumberArray[5] = (hours / 10) % 10;

  for (int i = 0 ; i < N_NIXIES ; i ++)
  {
  setNumber(currNumberArray[i]);   
  digitalWrite(nixies[i], Anode_sett[i]);   
  delay(NumberArrayFadeInValue[i]);
  setNumber(NumberArray[i]);   
  delay(NumberArrayFadeOutValue[i]);
  digitalWrite(nixies[i], LOW);
  delayMicroseconds(delayoff);    

// Aggiorna tutti gli array e le dissolvenze
    if (NumberArray[i] != currNumberArray[i])    
    {
      NumberArrayFadeInValue[i] += fadeStep;
      NumberArrayFadeOutValue[i] -= fadeStep;
  
      if (NumberArrayFadeInValue[i] >= fadeMax)
      {
        NumberArrayFadeInValue[i] = fadeStep;
        NumberArrayFadeOutValue[i] = fadeMax; 
        currNumberArray[i] = NumberArray[i];
      }
    }
  }
}

// Scrittura nixie effetto scorrimento numeri
void writeToNixieScroll(int hours, int minutes, int seconds) 
{ 
  NumberArray[0] = seconds % 10;
  NumberArray[1] = (seconds / 10) % 10;
  NumberArray[2] = minutes % 10;
  NumberArray[3] = (minutes / 10) % 10;
  NumberArray[4] = hours % 10;
  NumberArray[5] = (hours / 10) % 10;


  for (int i = 0; i < N_NIXIES; i++)
  {
    setNumber(nixie_val[i]);
    digitalWrite(nixies[i], Anode_sett[i]);
    delayMicroseconds(multiplexDelay);
    digitalWrite(nixies[i], LOW);
    delayMicroseconds(delayoff);
  }
   animate();
}

static void animate()
{
  unsigned int cl = 0;
  unsigned int tl = 0;

  for (int i = 0; i < N_NIXIES; i++)
  {
    // I valori attualmente visualizzati
    cl = get_level(nixie_val[i]);
    
    // I valori che devono essere effettivamente visualizzati alla fine
    tl = get_level(NumberArray[i]);

    if (cl > tl)
    {
      // Scendi di un livello
      nixie_val[i] = nixie_level[cl - 1];
    }
    else if (cl < tl)
    {
      // Sali di un livello
      nixie_val[i] = nixie_level[cl + 1];
    }
  }
}

static unsigned int get_level(unsigned int v)
{
  unsigned int l = sizeof(nixie_level);
  
  while (--l && nixie_level[l] != v)
  {
  }
  return l;
}

void TransitionEffect()
{
  
 for (int i = 0; i < 45; i++)
    {   
        if(i < 30) NumberArray[0] = NumberArray[0] + 1;        
        if(NumberArray[0] >= 9) NumberArray[0] = 0;
        
        if(i >= 5 && i < 35) NumberArray[1] = NumberArray[1] + 1;  
        if(NumberArray[1] >= 9) NumberArray[1] = 0;  

        if(i >= 10 && i < 40) NumberArray[2] = NumberArray[2] + 1;  
        if(NumberArray[2] >= 9) NumberArray[2] = 0; 

        if(i >= 15) NumberArray[3] = NumberArray[3] + 1;  
        if(NumberArray[3] >= 9) NumberArray[3] = 0;
       
        if(i < 30) NumberArray[4] = NumberArray[4] + 1;        
        if(NumberArray[4] >= 9) NumberArray[4] = 0;
        
        if(i >= 5 && i < 35) NumberArray[5] = NumberArray[5] + 1;  
        if(NumberArray[5] >= 9) NumberArray[5] = 0;
        
         writeToNixieRAW(NumberArray);   
    }
 }
