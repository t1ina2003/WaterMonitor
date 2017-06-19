// -------------- 溫度標頭檔 --------------
#include <OneWire.h> 
#include <DallasTemperature.h>
// -------------- LCD標頭檔 --------------
#include <Wire.h>
#include "rgb_lcd.h"
// -------------- 上傳influxDB標頭檔 --------------
#include <Bridge.h>
#include <BridgeClient.h>
#include <BridgeServer.h>
// -------------- 馬達標頭檔 --------------
#include <Servo.h>
#include <Timer.h>

// -------------- 溫度參數 --------------連接於Arduino Digital Input 2 
#define ONE_WIRE_BUS 4
OneWire oneWire(ONE_WIRE_BUS); // 運用程式庫建立物件
DallasTemperature sensors(&oneWire);
static float Temperature;
DeviceAddress insideThermometer = { 0x28, 0xFF, 0x45, 0xA8, 0xA0, 0x16, 0x04, 0x17 }; // 固定溫度感測器位置

// -------------- pH值參數量 --------------連接於Arduino Analog Input 0 (A0)
#define pHSensorPin A2    //pH值pin腳
#define Offset -10.00       //誤差補償
static float pHValue,pHvoltage;

//-------------- 水深參數 --------------連接於Arduino Analog Input 1 (A1)
#define waterDepthPin A1    //水深pin腳
static float waterDepthValue;

//-------------- LCD參數 --------------
rgb_lcd lcd;
const int colorR = 120;
const int colorG = 0;
const int colorB = 255;

//-------------- 上傳influxDB參數 --------------
IPAddress server(192,168,43,140);
BridgeClient client;
String data ="";
#define uploadSpeed 1000    //上傳速率(毫秒)

//-------------- 馬達參數 --------------
Servo myservo;  // create servo object to control a servo
Timer tcb;
int pos = 0;    // variable to store the servo position
#define degree 120    //馬達轉角度
void servoTurn(){
  for (pos = 0; pos <= degree; pos += 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
  for (pos = degree; pos >= 0; pos -= 1) { // goes from 180 degrees to 0 degrees
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
  }
}

//-------------- 蜂鳴器參數 --------------
const int buzzer = 8; //pin腳設定

void setup(void)
{
//  SerialUSB.begin(9600);
//  while (!SerialUSB);
//  SerialUSB.println("Starting bridge...\n");
  Bridge.begin();  
  //-------------- 溫度初始化 --------------
  sensors.begin();
  
  //-------------- LCD初始化 --------------
  lcd.begin(16, 2);
  lcd.setRGB(colorR, colorG, colorB);

  //-------------- 馬達初始化 --------------
  myservo.attach(6);  
  tcb.every(180000, servoTurn); // 每經過1000毫秒，就會呼叫servoTurn
  
  //-------------- 蜂鳴器初始化 --------------
  pinMode(buzzer, OUTPUT); // Set buzzer - pin 9 as an output
}

void loop(void)
{
  
  // -------------- 溫度處理 --------------
  // 設定溫度測量精度
  sensors.setResolution(insideThermometer, 12);
  // 要求匯流排上的所有感測器進行溫度轉換
  sensors.requestTemperatures();
  // 取得溫度讀數（攝氏）並輸出，參數0代表匯流排上第0個1-Wire裝置 
  Temperature = sensors.getTempCByIndex(0);
//  SerialUSB.print(" Temperature: ");
//  SerialUSB.print(Temperature);

  // -------------- pH值處理 --------------
  //獲取電壓 0~1024 轉為 0~5V
  pHvoltage = analogRead(pHSensorPin)*5.0/1024;
  //計算pH值
  pHValue = 3.5*pHvoltage+Offset;
  //列印數據
//  SerialUSB.print(" ,Voltage: ");
//  SerialUSB.print(pHvoltage,2);
//  SerialUSB.print(" ,pH value: ");
//  SerialUSB.print(pHValue,2);
  
  // -------------- 水深處理 --------------
  waterDepthValue = analogRead(waterDepthPin)*5.0/1024;
//  SerialUSB.print(" ,waterDepthValue: ");
//  SerialUSB.print(waterDepthValue,2);
 
  // -------------- LCD處理 --------------
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.setCursor(2, 0);
  lcd.print(Temperature);
  lcd.setCursor(8, 0);
  lcd.print("pH:");
  lcd.setCursor(11, 0);
  lcd.print(pHValue);
  lcd.setCursor(0, 1);
  lcd.print("D:");
  lcd.setCursor(2, 1);
  lcd.print(waterDepthValue);

  // -------------- infulxDB處理 --------------
  if (client.connect(server, 8086)) 
  {
    delay(uploadSpeed);
    data="WaterMonitor,device=Temperature value="+String(Temperature)+
      "\nWaterMonitor,device=pHValue value="+String(pHValue)+
      "\nWaterMonitor,device=waterDepthValue value="+String(waterDepthValue);
    
    client.println("POST /write?db=Fish_IoT HTTP/1.1");
    client.print("Content-length:");
    client.println(data.length());
    client.println("Connection: Close");
    client.println("Host:ArduinoYun");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println();
    client.println(data);     //send data
  }else{

  }
  if (client.available()) {
    char c = client.read();
  }
  if (!client.connected()) {
    client.stop();
  }

  // -------------- 馬達處理 --------------
  tcb.update();

  // -------------- 蜂鳴器處理 --------------
  if(Temperature>28 || Temperature<25 || pHValue<6.8 || pHValue>7.4 || waterDepthValue<2){
    tone(buzzer, 1); // Send 1KHz sound signal...
  }else{
    noTone(buzzer);     // Stop sound...
  }
  delay(100);
}
