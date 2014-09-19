#include <EtherCard.h>
#include <Wire.h>
#include <SFE_BMP180.h>
#include <dht11.h>

//#define DEBUG

#ifdef DEBUG
#define LOG(str) {Serial.print((float)((float)millis() / 1000.0));Serial.print(": ");Serial.println(str);}
#define LOG2(str1, str2) {Serial.print((float)((float)millis() / 1000.0));Serial.print(": ");Serial.print(str1);Serial.println(str2);}
#else
#define LOG(str) ;
#define LOG2(str1, str2) ;
#endif

#define DHT11_PIN 4

static byte mymac[] = { 0x00,0x1A,0x4B,0x38,0x0C,0x5C};
// ethernet interface ip address
static byte myip[] = { 192,168,1,100 };
// gateway ip address
static byte gwip[] = { 192,168,1,1 };
char website[] PROGMEM = "narodmon.ru";
static byte hisip[] = { 94,19,113,221,80 }; //Здесь надо указать IP адрес сайта narodmon.ru

byte Ethernet::buffer[350];
static unsigned long timer_temperature;
static unsigned long timer_humidity;
static unsigned long timerPressure;


dht11 DHT;

unsigned long lastUpdate = 0;

SFE_BMP180 pressure;

double baseline; // baseline pressure


void setup () 
{
#ifdef DEBUG 
  Serial.begin(57600);
  Serial.println("Client Demo");
  Serial.println();
#endif

  if (!ether.begin(sizeof Ethernet::buffer, mymac, 10)) {
    LOG("Failed to access Ethernet controller");
    while(1);
  } 
  else {
    LOG("Ethernet controller initialized");
  }
  LOG("");
  
  ether.staticSetup(myip, gwip);
/*
  if (!ether.dhcpSetup()) {
    LOG("Failed to get configuration from DHCP");
    while(1);
  } 
  else {
    LOG("DHCP configuration done")
    }
    */

#ifdef DEBUG 
    ether.printIp("IP Address:\t", ether.myip);
  ether.printIp("Netmask:\t", ether.mymask);
  ether.printIp("Gateway:\t", ether.gwip);
  Serial.println();
#endif

  ether.copyIp(ether.hisip, hisip);

/*  if (!ether.dnsLookup(website)) {
    LOG("DNS failed");
    while(1);
  } 
  else {
    LOG("DNS resolution done"); 
  }*/
#ifdef DEBUG
  ether.printIp("SRV IP:\t", ether.hisip);
  Serial.println();
#endif

  if (pressure.begin()) {
    LOG("BMP180 init success");
  } 
  else  {

    LOG("BMP180 init fail (disconnected?)\n\n");
    while(1); // Pause forever.
  }//*/
  // после подачи питания ждём секунду до готовности сенсора к работе
  delay(1000);
  timer_humidity = 5000;
  timerPressure = 10000;
}

static void response_callback_temperature (byte status, word off, word len) {
#ifdef DEBUG
  Serial.println((const char*) Ethernet::buffer + off + 182);
  Serial.println();
#endif
} 

static void response_callback_humidity (byte status, word off, word len) {
#ifdef DEBUG
  Serial.println((const char*) Ethernet::buffer + off + 182);
  Serial.println();
#endif
} 

void loop() 
{
  char buff[256];
  char c[10];

  ether.packetLoop(ether.packetReceive());

  if (millis() > timer_temperature) {
    timer_temperature = millis() + 300000;
    dtostrf(getBMP180Temperature(), 1, 2, c);
    sprintf(buff, "post.php?ID=6C71D9B5DF1B&6C71D9B5DF1B04=%s", c);
    LOG(buff);
    ether.browseUrl(PSTR("/"), buff, website, response_callback_temperature);

  }

  if (millis() > timer_humidity) {
    timer_humidity = millis() + 300000;
    updateDHT();

    sprintf(buff, "post.php?ID=6C71D9B5DF1B&6C71D9B5DF1B01=%d",DHT.humidity);
    LOG(buff);
    ether.browseUrl(PSTR("/"), buff, website, response_callback_humidity);
  }
  
  if (millis() > timerPressure) {
    timerPressure = millis() + 300000;
    dtostrf(getBMP180Pressure(), 1, 2, c);
    sprintf(buff, "post.php?ID=6C71D9B5DF1B&6C71D9B5DF1B06=%s", c);
    LOG(buff);
    ether.browseUrl(PSTR("/"), buff, website, response_callback_temperature);

  }
}


void updateDHT()
{
  int chk;

  LOG("updateDHT");

  boolean ok = false;
  byte attempt = 10;
  if (millis() > lastUpdate) {

    LOG("update DHT sensor...");

    while(!ok && attempt) {

      chk = DHT.read(DHT11_PIN);    // READ DATA

      switch (chk){
      case DHTLIB_OK:  
        ok = true;      
        lastUpdate = millis() + 10*1000;
        break;
      case DHTLIB_ERROR_CHECKSUM: 
        delay(100); 
        break;
      case DHTLIB_ERROR_TIMEOUT: 
        delay(100);
        break;
      default: 
        delay(100);
        break;
      }

      attempt--;
    }
  }
}

byte getDHTTemperatureInt()
{
  LOG2("DTH Temperature = ", DHT.temperature);
  return DHT.temperature;
}

byte getDHTHumidityInt()
{
  LOG2("DTH Humidity = ", DHT.humidity);
  return DHT.humidity;
}

double getBMP180Temperature()
{
  char status;
  double T;

  status = pressure.startTemperature();
  if (status != 0)
  {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed temperature measurement:
    // Note that the measurement is stored in the variable T.
    // Function returns 1 if successful, 0 if failure.

    status = pressure.getTemperature(T);
    if (status != 0) {
      LOG2("BMP180 Temperature: ", T);
    } 
    else {
      LOG("error retrieving temperature measurement");
    }
  } 
  else {
    LOG("error starting temperature measurement");
  }

  return T;
}

double getBMP180Pressure()
{
  char status;
  double T,P;
  T = getBMP180Temperature();    
  
  status = pressure.startPressure(0);
  if (status != 0) {
    // Wait for the measurement to complete:
    delay(status);

    // Retrieve the completed pressure measurement:
    // Note that the measurement is stored in the variable P.
    // Note also that the function requires the previous temperature measurement (T).
    // (If temperature is stable, you can do one temperature measurement for a number of pressure measurements.)
    // Function returns 1 if successful, 0 if failure.
    
    
    status = pressure.getPressure(P,T);
    if (status != 0) {  
      P = P * 0.75006375541921; // Переводим в милиметры ртутного столба
      LOG2("BMP180 Pressure: ", P);
    } 
    else {
      LOG("error retrieving pressure measurement\n");
    }
  } 
  else {
    LOG("error starting pressure measurement")
  }
  return P;
}












