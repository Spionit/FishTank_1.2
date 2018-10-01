/*  Gestion d'aquarium v1.2
    par Marc Delalande
    bac 600 litres
    - Mesure et affichage de la température de l'eau
    - Alerte niveau d'eau osmosée
    - Ajouts automatiques
    - Chauffage
    - Ventilos
    
    à venir :
    intégration TFT
    serveur web
    communication par sms
*/

#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal.h>
#include <Time.h>
#include <OneWire.h>

#define DS18B20 0x28     // Adresse 1-Wire du DS18B20

//  Les Pins (4 11 12 13 réservés pour shield Ethernet)
const int PINTEMPEAU = 8; // Broche utilisée pour le bus 1-Wire
const int PINNIVEAU = 28;  // pin capteur niveau d'eau osmosée
const int PINBOUTONMOINS = 25;  // pin bouton -
const int PINBOUTONPLUS = 23;  // pin bouton +
const int PINBOUTONRESET = 27;  // pin bouton reset
const int POMPECAR = 30;  // pin pompe Carbonat
const int POMPECAL = 32;  // pin pompe Calcium
const int POMPEMIN = 34;  // pin pompe Mineral Pro
const int POMPEMG = 31;  // pin pompe Magnesium
const int POMPEA = 33;  // pin pompe A
const int POMPEB = 35;  // pin pompe B
const int POMPEC = 36;  // pin pompe C
const int POMPEM = 37;  // pin pompe M
const int PINSELECMODE = 42;  // interrupteur pour sélection du mode (normal ou manuel)
const int PINBUZZER = 9; // pin buzzer
const int PINPOTARAJOUT = A9;  // pin potar sélection ajout
const int HEAT1 = 41;  // pin chauffage 1
const int HEAT2 = 40;  // pin chauffage 2
const int VENTILOS = 39;  // pin ventilos

const int AFFAUTO = 9;  // Nombre d'affichages en mode AUTO
const int UNML = 1200;  // temps de mise en marche de la pompe en ms pour délivrance de 1 ml
const int HCAR = 9;  // heure pour Carbonat
const int HCAL = 13;  // heure pour Calcium
const int HMIN = 17;  // heure pour Mineral
const int HMG = 21;  // heure pour MG
const int HA = 10;  // heure pour A
const int HB = 14;  // heure pour B
const int HC = 16;  // heure pour C
const int HM = 19;  // heure pour M

float doseCar = 150;  // ajouts en millilitres par jour
float doseCal = 150;
float doseMin = 60;
float doseMG = 90;
float doseABCM = 9;  // ajouts en millilitres par jour
float tEauMin;  // Température mini
float tEauMax;  // Température maxi
float tVoulue = 26;  // Température voulue
float deltaT = 0.3;  // Delta Température
float tMaxVoulue = 27;  // Température maxi voulue

int resteCar = 1000;  // plein Carbonat en ml
int resteCal = 1000;  // plein Calcium en ml
int resteMin = 1000;  // plein Mineral Pro en ml
int resteMG = 1000;  // plein MG en ml
int resteABCM = 500;  // plein ABCM en ml
int mStopCar;
int sStopCar;
int mStopCal;
int sStopCal;
int mStopMin;
int sStopMin;
int mStopMG;
int sStopMG;
int mStopABCM;
int sStopABCM;
int h;  // heure actuelle
int m;  // minute actuelle
int s;  // seconde actuelle
int caseAuto;  // Pour Switch Case en mode AUTO

unsigned long comptMesureTempEau;
unsigned long millisMesure;
unsigned long currentMillis;

OneWire ds(PINTEMPEAU);  // Création de l'objet OneWire ds

boolean heat;
boolean pompe;
boolean getTemperature(float *tEau){
  byte data[9], addr[8];  // data : Données lues depuis le scratchpad - addr : adresse du module 1-Wire détecté

  if (!ds.search(addr)) {  // Recherche un module 1-Wire
    ds.reset_search();     // Réinitialise la recherche de module
    return false;          // Retourne une erreur
  }
 
  if (OneWire::crc8(addr, 7) != addr[7])  // Vérifie que l'adresse a été correctement reçue
    return false;                         // Si le message est corrompu on retourne une erreur
 
  if (addr[0] != DS18B20)  // Vérifie qu'il s'agit bien d'un DS18B20
    return false;          // Si ce n'est pas le cas on retourne une erreur
 
  ds.reset();              // On reset le bus 1-Wire
  ds.select(addr);         // On sélectionne le DS18B20
  
  ds.write(0x44, 1);       // On lance une prise de mesure de température
  delay(800);              // Et on attend la fin de la mesure *************************PREMIERE VALEUR 800
   
  ds.reset();              // On reset le bus 1-Wire
  ds.select(addr);         // On sélectionne le DS18B20
  ds.write(0xBE);          // On envoie une demande de lecture du scratchpad
 
  for (byte i = 0; i < 9; i++)  // On lit le scratchpad
    data[i] = ds.read();        // Et on stock les octets reçus
   
  *tEau = ((data[1] << 8) | data[0]) * 0.0625;  // Calcul de la température en degrés Celsius 
   
  return true;  // Pas d'erreur
}

RTC_DS1307 RTC;

LiquidCrystal lcd(2, 3, 4, 5, 6, 7); 

void setup() {

//  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  lcd.begin(16,2);
  lcd.noCursor();
  lcd.print("Bac v1.2 init");
  delay(3000);
  lcd.clear();
  delay(500);
  
//  RTC.adjust(DateTime(2018,7,18,22,0,0));    // ************* AJUSTER DATE ET HEURE *************

  pinMode(POMPECAR,OUTPUT);
  pinMode(POMPECAL,OUTPUT);
  pinMode(POMPEMIN,OUTPUT);
  pinMode(POMPEMG,OUTPUT);
  pinMode(POMPEA,OUTPUT);
  pinMode(POMPEB,OUTPUT);
  pinMode(POMPEC,OUTPUT);
  pinMode(POMPEM,OUTPUT);
  pinMode(HEAT1,OUTPUT);
  pinMode(HEAT2,OUTPUT);
  pinMode(VENTILOS,OUTPUT);
  pinMode(PINBUZZER,OUTPUT);
  pinMode(PINNIVEAU,INPUT_PULLUP);
  pinMode(PINBOUTONMOINS,INPUT_PULLUP);
  pinMode(PINBOUTONPLUS,INPUT_PULLUP);
  pinMode(PINBOUTONRESET,INPUT_PULLUP);
  pinMode(PINSELECMODE,INPUT_PULLUP);
  pinMode(PINPOTARAJOUT,INPUT);

  digitalWrite(POMPECAR,HIGH);
  digitalWrite(POMPECAL,HIGH);
  digitalWrite(POMPEMIN,HIGH);
  digitalWrite(POMPEMG,HIGH);
  digitalWrite(POMPEA,HIGH);
  digitalWrite(POMPEB,HIGH);
  digitalWrite(POMPEC,HIGH);
  digitalWrite(POMPEM,HIGH);
  digitalWrite(HEAT1,HIGH);
  digitalWrite(HEAT2,HIGH);
  digitalWrite(VENTILOS,HIGH);

  comptMesureTempEau = millis();
  millisMesure = millis();
  currentMillis = millis();

  heat = false;
  pompe = false;

//  int tEau = tVoulue;
}

void loop() {

  DateTime now = RTC.now();
  
  h = now.hour();
  m = now.minute();
  s = now.second();
  
  mStopCar = doseCar*UNML/60000;
  sStopCar = ((doseCar*UNML/60000)-mStopCar)*60;  // extraction des minutes et secondes pour l'arrêt de Carbo

  mStopCal = doseCal*UNML/60000;
  sStopCal = ((doseCal*UNML/60000)-mStopCal)*60;  // extraction des minutes et secondes pour l'arrêt de Calci

  mStopMin = doseMin*UNML/60000;
  sStopMin = ((doseMin*UNML/60000)-mStopMin)*60;  // extraction des minutes et secondes pour l'arrêt de Mineral
   
  mStopMG = doseMG*UNML/60000;
  sStopMG = ((doseMG*UNML/60000)-mStopMG)*60;  // extraction des minutes et secondes pour l'arrêt de la pompe MG

  mStopABCM = doseABCM*UNML/60000;
  sStopABCM = ((doseABCM*UNML/60000)-mStopABCM)*60;  // extraction des minutes et secondes pour l'arrêt des pompes ABCM
    

  boolean niveau = digitalRead(PINNIVEAU);
  boolean mode = digitalRead(PINSELECMODE);
  boolean moins = digitalRead(PINBOUTONMOINS);
  boolean plus = digitalRead(PINBOUTONPLUS);
  boolean reset = digitalRead(PINBOUTONRESET);
  int potar = analogRead(PINPOTARAJOUT);

// ******************************************
// ********** Affichage de l'heure **********
// ******************************************

  lcd.setCursor(11,0);
  if (h <= 9) {
    lcd.print("0");
  }
  lcd.print(h);
  lcd.print(":");
  if (m <= 9) {
    lcd.print("0");
  }
  lcd.print(m);

// *************************************
// ********** Température eau **********
// *************************************

  float tEau;
  if ((millis() - comptMesureTempEau) >= 60000) {  // mesure la température toutes les minutes
    if(getTemperature(&tEau)) {
      lcd.setCursor(0,0);
      lcd.print(tEau,1);
      lcd.print((char)223);
      
      comptMesureTempEau = millis();  // réinitialisation du compteur
    }
  }
  if (tEau < tEauMin) {
    tEauMin = tEau;
  }
  if (tEau > tEauMax) {
    tEauMax = tEau;
  }

// *******************************************
// ********** Chauffage et Ventilos **********
// *******************************************

  if (!heat) {
    if ((tEau + deltaT) <= tVoulue) {  // Chauffages ON
      digitalWrite(HEAT1,LOW);
      digitalWrite(HEAT2,LOW);
      heat = true;
    }
  }
  if (heat) {
    if (tEau >= tVoulue) {  // Chauffages OFF
      digitalWrite(HEAT1,HIGH);
      digitalWrite(HEAT2,HIGH);
      heat = false;
    }
  }
  if (tEau >= tMaxVoulue) {  // Ventilos ON
    digitalWrite(VENTILOS,LOW);
  }
  if (tEau <= tMaxVoulue - deltaT) {  // Ventilos OFF
    digitalWrite(VENTILOS,HIGH);
  }
  
// *****************************************
// ********** Alerte niveau d'eau **********
// *****************************************

  if (niveau) {
    noTone(PINBUZZER);
  }
  
  if (!niveau) {
    tone(PINBUZZER, 880);
  }
  
// **********************************************************
// ********** Réglage des ajouts, restes et minmax **********  Mode AUTO
// **********************************************************

if (!mode) {
  
  lcd.setCursor(6,0);
  lcd.print("AUTO");
  caseAuto = map(potar,0,1023,1,AFFAUTO);
  
 switch (caseAuto) {
  
  case 1 :  // potar position Carbonat
    lcd.setCursor(0,1);
    lcd.print("Carbo ");
    lcd.print(int(doseCar));
    lcd.print(" ml/j");
    if (doseCar < 10) {
      lcd.print("    ");
    }
    if (doseCar >= 10 && doseCar < 100) {
      lcd.print("   ");
    }
    if (doseCar >= 100) {
      lcd.print("  ");
    }
    if (!plus) {  // augmente de 5 ml
      doseCar += 5;
      delay(150);
    }
    if (!moins) {  // diminue de 5 ml
      doseCar -= 5;
      delay(150);
    }
  break;
  
  case 2 :  // potar position Calcium
    lcd.setCursor(0,1);
    lcd.print("Calci ");
    lcd.print(int(doseCal));
    lcd.print(" ml/j");
    if (doseCal < 10) {
      lcd.print("    ");
    }
    if (doseCal >= 10 && doseCal < 100) {
      lcd.print("   ");
    }
    if (doseCal >= 100) {
      lcd.print("  ");
    }
    if (!plus) {  // augmente de 5 ml
      doseCal += 5;
      delay(150);
    }
    if (!moins) {  // diminue de 5 ml
      doseCal -= 5;
      delay(150);
    }
  break;

  case 3 :  // potar position Mineral Pro
    lcd.setCursor(0,1);
    lcd.print("MinPro ");
    lcd.print(int(doseMin));
    lcd.print(" ml/j");
    if (doseMin < 10) {
      lcd.print("   ");
    }
    if (doseMin >= 10 && doseMin < 100) {
      lcd.print("  ");
    }
    if (doseMin >= 100) {
      lcd.print(" ");
    }
    if (!plus) {  // augmente de 5 ml
      doseMin += 5;
      delay(150);
    }
    if (!moins) {  // diminue de 5 ml
      doseMin -= 5;
      delay(150);
    }
  break;

  case 4 :  // potar position MG
    lcd.setCursor(0,1);
    lcd.print("MG ");
    lcd.print(int(doseMG));
    lcd.print(" ml/j");
    if (doseMG < 10) {
      lcd.print("        ");
    }
    if (doseMG >= 10 && doseMG < 100) {
      lcd.print("       ");
    }
    if (doseMG >= 100) {
      lcd.print("      ");
    }
    if (!plus) {  // augmente de 5 ml
      doseMG += 5;
      delay(150);
    }
    if (!moins) {  // diminue de 5 ml
      doseMG -= 5;
      delay(150);
    }
  break;
  
  case 5 :  // potar position ABCM
    lcd.setCursor(0,1);
    lcd.print("ABCM ");
    lcd.print(int(doseABCM));
    lcd.print(" ml/j");
    if (doseABCM < 10) {
      lcd.print("     ");
    }
    if (doseABCM >= 10 && doseABCM < 100) {
      lcd.print("    ");
    }
    if (doseABCM >= 100) {
      lcd.print("   ");
    }
    if (!plus) {  // augmente de 1 ml
      ++doseABCM;
      delay(150);
    }
    if (!moins) {  // diminue de 1 ml
      --doseABCM;
      delay(150);
    }
  break;
  
  case 6 :  // affichage des restes en jours
    lcd.setCursor(0,1);
    lcd.print(int(resteCar/doseCar));
    lcd.print(" ");
    lcd.print(int(resteCal/doseCal));
    lcd.print(" ");
    lcd.print(int(resteMin/doseMin));
    lcd.print(" ");
    lcd.print(int(resteMG/doseMG));
    lcd.print(" ");
    lcd.print(int(resteABCM/doseABCM));
    lcd.print(" ");
    if (!reset) {
      lcd.setCursor(0,1);
      lcd.print("                ");
    }
  break;
  
  case 7 :  // affichage des températures min et max 
    lcd.setCursor(0,1);
    lcd.print("MinMax ");
    lcd.print(tEauMin,1);
    lcd.print(" ");
    lcd.print(tEauMax,1);
    if (!reset) {
      tEauMin = tEau;
      tEauMax = tEau;
    }
  break;

  case 8 :  // Réglage température voulue
    lcd.setCursor(0,1);
    lcd.print("Tvoulue ");
    lcd.print(tVoulue,1);
    lcd.print("    ");
    if (!plus) {  // augmente de 0.1 degré
      tVoulue += 0.1;
      delay(150);
    }
    if (!moins) {  // diminue de 0.1 degré
      tVoulue -= 0.1;
      delay(150);
    }
  break;

  case 9 :  // Réglage delta température
    lcd.setCursor(0,1);
    lcd.print("deltaT ");
    lcd.print(deltaT,1);
    lcd.print("     ");
    if (!plus) {  // augmente de 0.1 degré
      deltaT += 0.1;
      delay(150);
    }
    if (!moins) {  // diminue de 0.1 degré
      deltaT -= 0.1;
      delay(150);
    }
  break;
  
 }
  
  // ************************************************************
  // ********** Déclenchements des ajouts automatiques **********
  // ************************************************************

  // ********** les STARTs **********
  
  if (h==HCAR && m==0 && (s==0 || s==1)) {  // ajout Carbonat
    digitalWrite(POMPECAR, LOW);  // pompe Carbonat ON
    pompe = true;
  }

  if (h==HCAL && m==0 && (s==0 || s==1)) {  // ajout Calcium
    digitalWrite(POMPECAL, LOW);  // pompe Calcium ON
    pompe = true;
  }

  if (h==HMIN && m==0 && (s==0 || s==1)) {  // ajout Mineral
    digitalWrite(POMPEMIN, LOW);  // pompe Mineral ON
    pompe = true;
  }
 
  if (h==HMG && m==0 && (s==0 || s==1)) {  // ajout MG
    digitalWrite(POMPEMG, LOW);  // pompe MG ON
    pompe = true;
  }

  if (h==HA && m==0 && (s==0 || s==1)) {  // ajout A
    digitalWrite(POMPEA, LOW);  // pompe A ON
    pompe = true;
  }

  if (h==HB && m==0 && (s==0 || s==1)) {  // ajout B
    digitalWrite(POMPEB, LOW);  // pompe B ON
    pompe = true;
  }

  if (h==HC && m==0 && (s==0 || s==1)) {  // ajout C
    digitalWrite(POMPEC, LOW);  // pompe C ON
    pompe = true;
  }

  if (h==HM && m==0 && (s==0 || s==1)) {  // ajout M
    digitalWrite(POMPEM, LOW);  // pompe M ON
    pompe = true;
  }

  // ********** les STOPs **********
  
  if (h==HCAR && m==mStopCar && (s==sStopCar || s==sStopCar+1)) {  // pompes Carbonat OFF
    digitalWrite(POMPECAR, HIGH);
    if (pompe) {
      resteCar -= doseCar;
      pompe = false;
    }
  }

  if (h==HCAL && m==mStopCal && (s==sStopCal || s==sStopCal+1)) {  // pompes Calcium OFF
    digitalWrite(POMPECAL, HIGH);
    if (pompe) {
      resteCal -= doseCal;
      pompe = false;
    }
  }

  if (h==HMIN && m==mStopMin && (s==sStopMin || s==sStopMin+1)) {  // pompes Calcium OFF
    digitalWrite(POMPEMIN, HIGH);
    if (pompe) {
      resteMin -= doseMin;
      pompe = false;
    }
  }
  
  if (h==HMG && m==mStopMG && (s==sStopMG || s==sStopMG+1)) {  // pompe MG OFF
    digitalWrite(POMPEMG, HIGH);
    if (pompe) {
      resteMG -= doseMG;
      pompe = false;
    }
  }
  
  if ((h==HA || h==HB || h==HC || h==HM) && m==mStopABCM && (s==sStopABCM || s==sStopABCM+1)) {  // pompes A, B, C et M OFF
    digitalWrite(POMPEA, HIGH);
    digitalWrite(POMPEB, HIGH);
    digitalWrite(POMPEC, HIGH);
    digitalWrite(POMPEM, HIGH);
    if (h==HM && m==mStopABCM && (s==sStopABCM || s==sStopABCM+1)) {
      if (pompe) {
        resteABCM -= doseABCM;
        pompe = false;
      }
    }
  }
}  // fin du mode AUTO

// ************************************************************
// ********** Mode manuel - amorçage - Remise à zéro **********
// ************************************************************

else {
  
  lcd.setCursor(6,0);
  lcd.print("MAN ");
  
  if (potar < 100) {  // potar position Carbonat - raz
    lcd.setCursor(0,1);
    lcd.print("Carbonat     RAZ");
    if (!plus) {
      digitalWrite(POMPECAR,LOW);
    }
    else {
      digitalWrite(POMPECAR,HIGH);
    }
    if (!reset) {
      resteCar = 1000;
    }

  }
  
  else if (potar >= 100 && potar < 200) {  // potar position Calcium - raz
    lcd.setCursor(0,1);
    lcd.print("Calcium      RAZ");
    if (!plus) {
      digitalWrite(POMPECAL,LOW);
    }
    else {
      digitalWrite(POMPECAL,HIGH);
    }
    if (!reset) {
      resteCal = 1000;
    }

  }
  
  else if (potar >= 200 && potar < 300) {  // potar position Mineral - raz
    lcd.setCursor(0,1);
    lcd.print("Mineral Pro  RAZ");
    if (!plus) {
      digitalWrite(POMPEMIN,LOW);
    }
    else {
      digitalWrite(POMPEMIN,HIGH);
    }
    if (!reset) {
      resteMin = 1000;
    }
  }
 
  else if (potar >= 300 && potar < 400) {  // potar position MG - raz
    lcd.setCursor(0,1);
    lcd.print("Magnesium    RAZ");
    if (!plus) {
      digitalWrite(POMPEMG,LOW);
    }
    else {
      digitalWrite(POMPEMG,HIGH);
    }
    if (!reset) {
      resteMG = 1000;
    }
  }
  
  else if (potar >= 400 && potar < 500) {  // potar position A
    lcd.setCursor(0,1);
    lcd.print("A               ");
    if (!plus) {
      digitalWrite(POMPEA,LOW);
    }
    else {
      digitalWrite(POMPEA,HIGH);
    }
  }
  
  else if (potar >= 500 && potar < 600) {  // potar position B
    lcd.setCursor(0,1);
    lcd.print("B               ");
    if (!plus) {
      digitalWrite(POMPEB,LOW);
    }
    else {
      digitalWrite(POMPEB,HIGH);
    }
  }
  
  else if (potar >= 600 && potar < 700) {  // potar position C
    lcd.setCursor(0,1);
    lcd.print("C               ");
    if (!plus) {
      digitalWrite(POMPEC,LOW);
    }
    else {
      digitalWrite(POMPEC,HIGH);
    }
  }
  
  else {  // potar position M - raz ABCM
    lcd.setCursor(0,1);
    lcd.print("M            RAZ");
    if (!plus) {
      digitalWrite(POMPEM,LOW);
    }
    else {
      digitalWrite(POMPEM,HIGH);
    }
    if (!reset) {
      resteABCM = 500;
    }
  }
}  // fin du mode manuel

}
