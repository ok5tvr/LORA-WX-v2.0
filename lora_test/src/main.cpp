#include <Arduino.h>


void lora_tx();
void wind_m();

const byte lora_PNSS = 18;    // pin number where the NSS line for the LoRa device is connected.
const byte lora_PReset = 23;  // pin where LoRa device reset line is connected
//const byte lora_DIO0 = 26;
const byte interruptPinW = 14;


const byte PLED1 = 25;        // pin number for LED on Tracker
int cnt=1;
int cnt_1H=0;
int cnt_rain=0;
int cnt_wind=0;
int sekund = 1;
int calc=1;
int cnt_rain_cor = 0;
int adc_b = 0;
int adc_v2 = 0;
float windr = 0;
String wind_dir = "...";
String Outstring="";  

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <SPI.h>
#include <sensor.h>
#include <LoRaTX.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_HTU21DF.h>
#include <Adafruit_BMP085.h>
#include <PCF8583.h>


// deep sleep 
#define uS_TO_S_FACTOR 1000000  // conversion factor for micro seconds to seconds 
#define TIME_TO_SLEEP  99       // time ESP32 will go to sleep (in seconds)   - 99 for (about) 1% duty cycle 

#define OLED_SDA 21
#define OLED_SCL 22 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
Adafruit_BMP085 bmp;
Adafruit_HTU21DF htu;
PCF8583 counter(0xA0);

void IRAM_ATTR wind(){
  cnt_wind = cnt_wind + 1;
  digitalWrite(PLED1, HIGH);
  }

RTC_DATA_ATTR int bootCount = 29;
RTC_DATA_ATTR int tx_count = 0;
RTC_DATA_ATTR byte boot = 0;
RTC_DATA_ATTR int dest_24h_cnt = 0;
RTC_DATA_ATTR int cnt_24H = 1; 

void setup() {
Serial.begin(115200);
delay(500);
Serial.print("Start WX...");

//initialize OLED
Wire.begin(OLED_SDA, OLED_SCL);

if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
Serial.println(F("SSD1306 error"));
for(;;); // Don't proceed, loop forever
}
if (bmp.begin())
Serial.println("BMP180 OK");
else
  {
    Serial.println("Error BMP180");
    while(1); // bucle infinito
  }

if (!htu.begin()){
  Serial.println(F("htu - error"));
  while (1) 
  {
    Serial.println(F("htu - ok"));
  }
}  

if (boot == 0)  {
 
 delay(100);
 counter.setMode(MODE_EVENT_COUNTER);
 counter.setCount(0);

boot = 1;
 }
 
pinMode (interruptPinW, INPUT_PULLUP);

display.ssd1306_command(0xAF);

display.clearDisplay();
display.setTextColor(WHITE);
display.setTextSize(2);
display.setCursor(0,0);
display.print("OK1JRA-5");
display.setTextSize(1);
display.setCursor(5,27);
display.print("WX test APRS LoRa");
display.setCursor(5,36);
display.print("(c) OK5TVR");
display.setCursor(6,45);
display.print(String(bootCount));
display.display();
delay(3000);


  pinMode(lora_PReset, OUTPUT);     // RFM98 reset line
  digitalWrite(lora_PReset, LOW);   // Reset RFM98
  pinMode (lora_PNSS, OUTPUT);      // set the slaveSelectPin as an output:
  digitalWrite(lora_PNSS, HIGH);
  pinMode(PLED1, OUTPUT);                                                          // for shield LED
  
  SPI.begin();                                                                     // initialize SPI:
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setDataMode(SPI_MODE0);
  SPI.setBitOrder(MSBFIRST);
   
   lora_ResetDev();                                                                 // Reset the device
   lora_Setup();                                                                    // Do the initial LoRa Setup

// LoRa frequency calculation (sample for 434.4 MHz): 
// ------------------------------------
// 434400000/61.03515625 = 71172096
// 71172096 (DEC) = 6C 99 99 (HEX)  
// 6C 99 99 (HEX) = 108 153 153 (DEC)  

   lora_SetFreq(108, 113, 153);  //433.775 MHz                                     // Set the LoRa frequency, 434.400 Mhz
// lora_SetFreq(108, 153, 153);  //434.400 MHz                                     // Set the LoRa frequency, 434.400 Mhz

}

void loop() {

Serial.println("temp_in");
  Serial.println(bmp.readTemperature());
  Serial.println("press");
  Serial.println(bmp.readPressure());
  Serial.println("temp");
  Serial.println(htu.readTemperature());
  Serial.println("hum");
  Serial.println(htu.readHumidity());

  Serial.println("count0");
  Serial.println(counter.getCount());
  
 Serial.println("adc1");
  Serial.println(analogRead(36));
  Serial.println("adc2");
   Serial.println(analogRead(39));

  adc_v2 = analogRead(39);
   Serial.println(adc_v2);

  switch (adc_v2) // adc read --> convert to win direct
  {
case 3399 ... 3699:
  wind_dir = "000";
  break;
case 2019 ... 2389:
  wind_dir = "045";
  break;
case 414 ... 660:
  wind_dir = "090";
  break;
case 1094 ... 1132:
  wind_dir = "135";
  break;
case 1617 ... 1682:
  wind_dir = "180";
  break;
case 2903 ... 2945:
  wind_dir = "222";
  break;
case 3971 ... 4095:
  wind_dir = "270";
  break;
case 3700 ... 3967:
  wind_dir = "315";
  break;
}
Serial.println(wind_dir);

  //calc = 3600;  // 1800sec. = 30 Min.
//  calc = 900;   // 900sec.  = 15 Min.
//   calc = 600;   // 600sec.  = 10 Min.
//   calc = 300;   // 300sec.  = 5 Min.
//   calc = 60;    //  60sec.  = 1 Min.
calc = 30;    //  30sec.  = 30 Sec.

Outstring = "OK1JRA-5>APRS:!4976.00N/01332.20E-";
  Outstring += ("LORA solar power test by OK5TVR");
  Outstring += (" Bat.: ");
  float battery = 0;
  String battery_st = "";
  battery = (analogRead(39) * 1.149 ) / (4095);
  battery_st = String(battery,2);
  Outstring += battery_st;
  Outstring += (" V");
  Serial.println ("TX koment");
  Serial.println (Outstring);
 

bootCount++;
Serial.println("Boot number: " + String(bootCount));

if (bootCount == calc)
{
  cnt_wind = 0;
  wind_m();
tx_count ++;
  //  TX koment
  //  Outstring = "OK1JRA-5>APRS:!4945.84N/01319.33E-";
  //  Outstring += ("LORA solar power test by OK5TVR");
  //  Outstring += (" Bat.: ");
  //  float battery = 0;
  //  String battery_st = "";
  //  battery = (analogRead(39) * 1.149 ) / (4095);
  //  battery_st = String(battery,2);
  //  Outstring += battery_st;
  //  Outstring += (" V");
  //  Serial.println ("TX koment");
  //  Serial.println (Outstring);
  //  lora_tx(); 


  delay(2000);
//  TX telemtry
  Outstring = "OK1JRA-5>APRS:T#";
  Outstring += (tx_count);
  Outstring += (",");
  Outstring += String (analogRead(39));
  Outstring += (",");
  float press = (bmp.readPressure()/10)+380;
  if(press<10000) { Outstring += "0"; }
   Outstring += String(press,0);
  Outstring += (",");
  Outstring += (String(bmp.readTemperature()));
  Outstring += (",0.0,0.0,00000000");
    Serial.println ("TX telemtry");
Serial.println (Outstring);
  lora_tx(); 

 delay(2000);
 //  TX WX


float press1 = (bmp.readPressure()/10)+380;
float temp = (htu.readTemperature()*1.8);
temp = temp + 32;
float humi = htu.readHumidity();
float lumi = analogRead(36)/100;
float wind2 = windr*0.621;  // wind2 is in mph
float rain_total = 0;
float rain_1h = 0;
float dest_24h = 0;
String wind_dir = "000";
String rain_st ="000";
String rain_st1 ="000";
String wind_st = "000";
String temp_st = "";
rain_1h = (counter.getCount() * 1.181);
dest_24h_cnt = dest_24h_cnt + counter.getCount();
dest_24h = (dest_24h_cnt * 1.181) ;


adc_v2 = analogRead(39);
switch (adc_v2) // adc read --> convert to win direct
  {
case 3399 ... 3699:
  wind_dir = "000";
  break;
case 2019 ... 2389:
  wind_dir = "045";
  break;
case 414 ... 660:
  wind_dir = "090";
  break;
case 1094 ... 1132:
  wind_dir = "135";
  break;
case 1617 ... 1682:
  wind_dir = "180";
  break;
case 2903 ... 2945:
  wind_dir = "222";
  break;
case 3971 ... 4095:
  wind_dir = "270";
  break;
case 3700 ... 3967:
  wind_dir = "315";
  break;
}

Outstring = "OK1JRA-5>APRS:!4945.84N/01319.33E_";
Outstring += wind_dir;
Outstring += ("/");
  if(wind2<100) { Outstring += "0"; }
  if(wind2<10)  { Outstring += "0"; }
  wind2 = wind2 * 10;
  wind_st = String(wind2,0);
  wind_st.remove(wind_st.length()-1,1);
  if(wind2==0){ wind_st = "0"; }
Outstring += wind_st;

Outstring += ("g...");

Outstring += ("t");
   if(temp<100) { Outstring += "0"; }
   if(temp<10) { Outstring += "0"; }
   temp = temp * 10;
   temp_st += String(temp,0);
   temp_st.remove(temp_st.length()-1,1);
   if(temp_st==0){ temp_st = "0"; }
   if(temp_st==" "){ temp_st = "0"; }
   Outstring += temp_st;


Outstring += ("r");
  if(rain_1h<100) { Outstring += "0"; }
  if(rain_1h<10)  { Outstring += "0"; }
  rain_1h = rain_1h * 10;
  rain_st = String(rain_1h,0);
  rain_st.remove(rain_st.length()-1,1);
  if(rain_1h==0){ rain_st = "0"; }
  Outstring += rain_st;

  Outstring += ("p");
  if(dest_24h<100) { Outstring += "0"; }
  if(dest_24h<10)  { Outstring += "0"; }
  dest_24h = dest_24h * 10;
  rain_st1 = String(dest_24h,0);
  rain_st1.remove(rain_st1.length()-1,1);
  if(dest_24h==0){ rain_st1 = "0"; }
  Outstring += rain_st1;

Outstring += ("P...");

Outstring += ("h");
Outstring += String(humi,0);

Outstring += ("b");
   if(press1<10000) { Outstring += "0"; }
   Outstring += String(press1,0);

Outstring += ("L...");
 
Outstring += (" WX-Radcice by OK5TVR");
Serial.println ("TX WX");
Serial.println (Outstring);
lora_tx(); 

delay(2000);

if (cnt_24H == 24)
{
  cnt_24H = 1;
  dest_24h = 0;
  dest_24h_cnt = 0;
}
 cnt_24H = cnt_24H + 1;
 counter.setCount(0);
 bootCount = 0; 
}

    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print("OK1JRA-5");
    display.setTextSize(1);
    display.setCursor(5,10);
    display.print(String(bootCount));
    display.setTextSize(2);
    display.setCursor(15,15);
    display.print("sleep");
    display.display();  

display.ssd1306_command(0xAE);




esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
Serial.println("Going to sleep now");
Serial.flush();   // waits for the transmission of outgoing serial data to complete 
esp_deep_sleep_start();   // enters deep sleep with the configured wakeup options

if (sekund == calc)
{
    
  sekund = 1; 
}

if (cnt_1H == 3600) // time 1H
{
 
}

if (cnt_24H == 86400) // time 24H
{
     
  cnt_24H = 1;
}




sekund++;
cnt_1H++;
cnt_24H++;
delay(1000);

// put your main code here, to run repeatedly:
}

void lora_tx()
{
 byte i;
 byte ltemp;
 String text_tx = "";
 text_tx = "tx by ";
 text_tx = text_tx + calc;
 text_tx = text_tx + " s";

 ltemp = Outstring.length();
    lora_SetModem(lora_BW125, lora_SF12, lora_CR4_5, lora_Explicit, lora_LowDoptON);		// Setup the LoRa modem parameters
    lora_PrintModem();                                                                  // Print the modem parameters
    lora_TXStart = 0;
    lora_TXEnd = 0;
    for (i = 0; i <= ltemp; i++)
    {
    lora_TXBUFF[i] = Outstring.charAt(i);
    }
    i--;
    lora_TXEnd = i;    
    digitalWrite(PLED1, HIGH);  // LED ON on during sending
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0,0);
    display.print("ON AIR");
    display.setTextSize(1);
    display.setCursor(10,30);
    display.print(text_tx);
    display.display();  

    lora_Send(lora_TXStart, lora_TXEnd, 60, 255, 1, 10, 17);	
    digitalWrite(PLED1, LOW);   // LED OFF after sending
    lora_TXBuffPrint(0);
    Outstring = "";
}

void wind_m()
{
attachInterrupt (digitalPinToInterrupt(interruptPinW), wind, FALLING);
delay(3000); //Time for measure counts
detachInterrupt(digitalPinToInterrupt(interruptPinW));
windr = ((float)cnt_wind / (float)3 * 2.4) / 2; //Convert counts & time to km/h
}


