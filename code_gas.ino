#include <LiquidCrystal.h>  //library for LCD Display
#include <Servo.h>  //library for servo dc motor which will turn on and off regulator
#include "DHT.h"    //library for temperature and humidity measurement
#include "HX711.h"  //library for load sensor


//Connect 3rd pin of LCD to GND
LiquidCrystal lcd(7,6,5,4,3,2);  // parameter: (RS, En, D4, D5, D6, D7)

int gas_pin = A0;

int buz_pin = 8;  //digital pin for buzzer, can use analog pin as well

Servo myservo;
int angle = 0;  //angle = 0  means regulator is off, angle = 135 means regulator is on
int servo_pin = 9;
boolean gas_on  = true;   //for test, true if regulator is ON, otherwise false

int dht_pin = A1; //analog pin for temp and humidity sensor
#define DHTTYPE DHT11   // DHT 11 -->sensor type
DHT dht(dht_pin, DHTTYPE);
// Connect a 10K resistor from pin "SIG" (data) to pin "VCC" (power) of the sensor

HX711 scale(A4, A3);  //DT | SCK
float min_threshold = 100.0;   //replace this value with actual threshold if load sensor works properly

void setup() {
  Serial.begin(9600);
  
  pinMode(gas_pin, INPUT);

  pinMode(buz_pin, OUTPUT);  //setting the mode of buz_pin

  // set up the LCD's number of columns and rows:
  lcd.begin(20,4);

  //use a switch, if user turns it on, set gas_on = true, if user turns it off, set gas_on = false
  //gas_on = true;  
  myservo.attach(servo_pin); // attaches the servo on pin 9 to the servo object
  if(gas_on){
    myservo.write(135);
  }
  else{
    myservo.write(0);
  }
  
  //Setting up the temp and humidity sensor
  dht.begin();

  //setting up the HX711 
  scale.set_scale(7050.0);     // this value is obtained by calibrating the scale with known weights; see the README for details
  scale.tare();                // reset the scale to 0
}
 
void loop() {
  int gas_ppm;   //ranges from 0 to 1024
  gas_ppm=analogRead(gas_pin);

  if(gas_ppm>300)
  {
    lcd.clear();
  // Print a message to the LCD.
    lcd.setCursor(0, 0);  //sets the cursor to 0th col and 0th row!!
    //while(analogRead(gas_pin)>300){
    Serial.print("Gas Leakage Detected!!!   ");  
    Serial.println(gas_ppm);

    lcd.print("Gas Leakage!!");
    
    digitalWrite(buz_pin, HIGH);   //buzzer make a beeping noise on leakage detection
   
    if(gas_on){
      myservo.write(0);
      gas_on = false;
    }
    lcd.setCursor(0,1);
    lcd.print("gasppm: ");
    lcd.print(gas_ppm);
    delay(5000);
   // }
  }
  else
  {
     lcd.clear();
  // Print a message to the LCD.
     lcd.setCursor(0, 0);  //sets the cursor to 0th col and 0th row!!
     Serial.print("No Gas Leakage!!"); 
     Serial.println(gas_ppm);

     lcd.print("No Gas Leakage!!");

     digitalWrite(buz_pin, LOW);
     float w = scale.get_units();
     if(w < 0)
     {
       w = -1*w; 
     }
     //w = w*16.4 - 1;
     w =(float) w*45.0;  //normalizing the weight in terms of gram
     lcd.setCursor(0,1);
     lcd.print("Weight: ");
     lcd.print(w);
     
    
  }
  


  float hum = dht.readHumidity();  //unit : RH (e.g. 40% humidity)
  float temp = dht.readTemperature(false);  //unit: "false==deg. Celcius  ||| true== farenheit
  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (isnan(temp) || isnan(hum)) 
  {
    Serial.println("Failed to read temp and humidity from DHT Sensor");
  } 
  else 
  {
        Serial.print("Humidity: "); 
        Serial.print(hum);
        Serial.print(" %\t");
        Serial.print("Temperature: "); 
        Serial.print(temp);
        Serial.println(" *C");

        lcd.setCursor(0,2);
        lcd.print("Humidity: ");
        lcd.print(hum);
        lcd.print(" %");
        lcd.setCursor(0,3);
        lcd.print("Temperature: "); 
        lcd.print(temp);
        lcd.print(" C");
  }

  //myservo.write(135);
  //gas_on = true;

  if(gas_on && temp > 60 ){   //it means food is being cooked
    lcd.clear();
    Serial.println("Gas is being used in cooking");
    float weight = scale.get_units();
    if(weight < 0){
      weight = -1*weight;
    }
   // init_weight = init_weight*16.4 - 1;
    weight = (float)weight*45.0;
    float init_weight = 2000.0 ;
    lcd.setCursor(0,1);
    lcd.print("init_weight: ");
    lcd.print(weight); 
    
    while(dht.readTemperature(false) > 60  ){
      if(analogRead(gas_pin) > 400){
        lcd.clear();
        Serial.println("Gas is being leaked");
        lcd.setCursor(0,0);
        lcd.print("Gas Leakage!!");
        digitalWrite(buz_pin, HIGH);
        if(gas_on){
          myservo.write(0);
          gas_on = false;
        }
        lcd.setCursor(0,1);
        lcd.print("gasppm: ");
        lcd.print(gas_ppm);
      }
      else{
        //scale.power_down();       // put the ADC in sleep mode
        //delay(1000);
        //scale.power_up();
        lcd.clear();
        float final_weight = scale.get_units();
        if(final_weight < 0){
          final_weight = -1*final_weight ;
        }
       // final_weight = final_weight*16.4 - 1;
        final_weight = (float)final_weight* 45.0;
        float gas_used = (float)(weight - final_weight);
        if(gas_used < 0 ){
          gas_used = -1.0*gas_used;
        }
        
        
        lcd.setCursor(0,0);
        lcd.print("No Gas Leakage!!");

        lcd.setCursor(0,1);
        lcd.print("init_weight: ");
        lcd.print(weight);

        lcd.setCursor(0,2);
        lcd.print("final Weight: ");
        lcd.print(final_weight);
        
        lcd.setCursor(0,3);
        lcd.print("Gas Used: ");
        lcd.print(gas_used);

        //scale.power_down(); 

        if(final_weight < min_threshold){
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Order New Cylinder");
          lcd.setCursor(0,1);
          lcd.print("weight: ");
          lcd.print(final_weight);
          digitalWrite(buz_pin, HIGH);
          delay(200);
          digitalWrite(buz_pin, LOW);
          delay(1000);
          digitalWrite(buz_pin, HIGH);
          delay(200);
          digitalWrite(buz_pin, LOW);
        }
        
      }
      
      delay(1000);
            
    }
 
  }
  //If amount of gas left is less than the threshold then it will notify user to book a new cylinder
  /*if(gas_on && dht.readTemperature(false) > 20){
    lcd.clear();
  
     float w = scale.get_units() ;
     if(w<0){
      w = -1.0*w;
     }
     w = w*45.0;
     if(w<min_threshold){
  
    lcd.setCursor(0,0);
    lcd.print("Order New Cylinder");
    lcd.setCursor(0,1);
    lcd.print("weight: ");
    lcd.print(w);
    digitalWrite(buz_pin, HIGH);
    delay(200);
    digitalWrite(buz_pin, LOW);
    delay(1000);
    digitalWrite(buz_pin, HIGH);
    delay(200);
    digitalWrite(buz_pin, LOW);
    
    //delay(2000);
  }
  }*/


  delay(1000);
}
