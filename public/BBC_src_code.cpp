#include <LiquidCrystal.h>
LiquidCrystal lcd(8,9,4,5,6,7);
// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

const int Coil = 2;
const int Starter = 3;
const int GenStatus = 11;//=0 ---> gen is on
const int RedLight = 12; //Red light 
bool ActualGenStatus;
int GenStartIterations = 0;//# of times the system tries to
//start the generator
int RawVolts = 0;
int BatteryMilliVolts = 0;
int mapValue = 31000;
int TimeDelay = 100;
int SecondTimer = 0;
int Starter_Timer = 0;
int Stop_Timer = 0;
int genStartTime = 10 * 10; //We want to change this value
//Timers
int timer200ms = 0;
int timer2sec = 0;
int timerDelay = 0; //Counter for the delay to reach the DelayTime value
//Keeps track of the menu
int Menu = 0;
#define ChangeMinVolt 1
#define ChangeMaxVolt 2
#define ChangeDelay 3
#define ChangeStartDelay 4
//Values to change
int MinimumVoltage = 11500;
int MaximumVoltage = 14000;
int DelayTime = 1 * 10; //The delay for the coils to start again ( seconds * 10)

//Keeps track of the status of the signals
bool CoilStatus = true;
bool StarterStatus = true; 




void setup() {
  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(12,OUTPUT); //Red LED
  pinMode(11,INPUT_PULLUP);
  Serial.begin(9600);
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print("Battery= ");
  lcd.setCursor(14,0);
  lcd.print("mV");
  lcd.setCursor(0,1);
  lcd.print("StartTime= ");
  lcd.print(genStartTime / 10);
  lcd.print("s");

  //Both Coil and Starter must be OFF at first
  digitalWrite(Coil, HIGH);
  digitalWrite(Starter, HIGH);

}

void loop() {

  
  //If device is not ready then menu
  if(Menu <= 4)
  {
    DisplayMenu();
  } else 
  {
    readVolts(); //Continuously reads the voltage
  }

 /* lcd_key = read_LCD_buttons();
  if(lcd_key == btnSELECT) //To change values while device is running
  {
    Menu = 1;
  }
  
  */
  delay(TimeDelay);
}



//Function for reading the voltage
void readVolts()
{
  //lcd.clear();
  lcd.setCursor(0,0);
  lcd_key = read_LCD_buttons();
  RawVolts = analogRead(A1); //Reads the voltage from the generator
  BatteryMilliVolts = map(RawVolts,0,1023,0,mapValue); //Value is assigned to the variable
  
  ActualGenStatus = !(digitalRead(GenStatus));
  Starter_Timer++;
  SecondTimer++;
  if (SecondTimer == 10)
   {
    
    lcd.print("Battery= ");
    lcd.setCursor(14,0);
    lcd.print("mV");
    lcd.setCursor(9,0);
    lcd.print(BatteryMilliVolts);

     SecondTimer = 0;
   }

  //additional feature to change the start time while the device is running
  if(CoilStatus == false and StarterStatus == false)
  {
   if(lcd_key == btnUP)
   {
    genStartTime += 10;
   } else if(lcd_key == btnDOWN)
   {
    genStartTime -= 10;
   }
  }
  lcd.setCursor (0, 1);
  lcd.print("Start Time: ");
  lcd.print(genStartTime /10);
  lcd.print("s     ");
  
  if(BatteryMilliVolts < MinimumVoltage && ActualGenStatus == 0) {
    // Start Generator

    if(CoilStatus == true)
    {
      digitalWrite(Coil,LOW);//sets coil hot at 12 Volts
      CoilStatus = false;
    }
    //delay 200 ms
    if(timer200ms < 2) //This should delay it by 200ms (Works)
    {
      timer200ms++;
      return; //Using Return since its a function and can be changed to "break" when its in the loop
    }
    
    
    //see if generator is on
    //by checking ActualGenStatus from the beginning
    
    while(ActualGenStatus == 0 && GenStartIterations <= 2){
      ActualGenStatus = !(digitalRead(GenStatus));//Checks the generator if its on

      digitalWrite(Starter,LOW); //turn on starter
      StarterStatus = false;
      //delay 2 seconds using timer2sec
      if(timer2sec < genStartTime) //Delay works
      {
        timer2sec++;//this delays 2 seconds for starter
        return; //Using Return since its a function and can be changed to "break" when its in the loop
      }
            
      digitalWrite(Starter,HIGH);//turn off starter
      StarterStatus = true;

      //digitalWrite(RedLight, HIGH);//Testing purposes
      
      
      //delay 15 seconds using timer30sec
      if(timerDelay < DelayTime)
      {
        timerDelay++;
        return;
      }
      ActualGenStatus = !(digitalRead(GenStatus));//read generator status (on or off)
      if(ActualGenStatus == 1){
        GenStartIterations = 0;
        return;
      }
      
      GenStartIterations++;
      
      timer200ms = 0; //Resets the value of 1st timer 
      timer2sec = 0;
      timerDelay = 0;
      
      
    }//bracket for while loop

    if(ActualGenStatus == 0 && GenStartIterations == 3){
    //KILL THE PROGRAM, END IT!!!//turn red light on!
    digitalWrite(Coil, HIGH); //Coil is off
    CoilStatus = true;
    digitalWrite(RedLight, HIGH); //Turns on Red LED
      while(1){ 
       
          StarterFailure();
        }; //Stops the program
    }
    
    
    GenStartIterations = 0;
    Starter_Timer = 0;
  }  else if(BatteryMilliVolts > MaximumVoltage && ActualGenStatus == 1) {
     GenStartIterations = 0;
     digitalWrite(Coil, HIGH);
     CoilStatus = true;
     digitalWrite(Starter, HIGH); 
     StarterStatus = true;
    // Stop Generator 
  }
    ActualGenStatus = !(digitalRead(GenStatus));
    if(ActualGenStatus == 1){
       digitalWrite(Starter, HIGH);
       StarterStatus = true;
       GenStartIterations = 0;
    }
}

//Displays a message to let the user know that the system failed
void StarterFailure()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Starter Failure!");
  delay(1000);
}

void DisplayMenu()
{
  lcd.clear();
  lcd.setCursor(0,0);

  lcd_key = read_LCD_buttons();

  if(Menu == 0)
  {
    lcd.print("Select to start!");
    if(lcd_key == btnSELECT) //When Select is pressed, then we can start the program
    {
      Menu++; //Goes to the next menu
    }
  } else if(Menu == ChangeMinVolt)
  {
    lcd.print("Min Volt: ");
    lcd.setCursor(0,1);
    lcd.print(MinimumVoltage);
    if(lcd_key == btnUP) //Increases the Minimum Voltage by 100
    {
      MinimumVoltage = MinimumVoltage + 100;
    } else if (lcd_key == btnDOWN) //Decreases the Minimum Voltage by 100
    {
      MinimumVoltage = MinimumVoltage - 100;
    } else if (lcd_key == btnRIGHT) //Goes to the next value to change
    {
      Menu++; //Menu is now equals to 2
    }
  } else if (Menu == ChangeMaxVolt)
  {
    lcd.print("Max Volt: ");
    lcd.setCursor(0,1);
    lcd.print(MaximumVoltage);
    if(lcd_key == btnUP) //Increases the Maximum Voltage by 100
    {
      MaximumVoltage = MaximumVoltage + 100;
    } else if (lcd_key == btnDOWN) //Decreases the Minimum Voltage by 100
    {
      MaximumVoltage = MaximumVoltage - 100;
    } else if (lcd_key == btnRIGHT) //Goes to the next value to change
    {
      Menu++; //Menu is now equals to 3
    } else if (lcd_key == btnLEFT) //Goes back to prev value to change
    {
      Menu--; //Menu is now equals to 2
    }
  } else if(Menu == ChangeDelay)
  {
    lcd.print("Delay Time: ");
    lcd.setCursor(0,1);
    lcd.print(DelayTime / 10); //Displays it in seconds
    if(lcd_key == btnUP)
    {
      DelayTime = DelayTime + 10;
    } else if(lcd_key == btnDOWN)
    {
      DelayTime = DelayTime - 10;
    } else if(lcd_key == btnRIGHT)
    {
      Menu++; //Menu is now equal to 4
      lcd.clear();
    } else if(lcd_key == btnLEFT)
    {
      Menu--; //Menu is now equal to 3
    }
  } else if(Menu == ChangeStartDelay)
  {
    lcd.print("Gen Start Time: ");
    lcd.setCursor(0,1);
    lcd.print(genStartTime / 10);
    if(lcd_key == btnUP)
    {
      genStartTime = genStartTime + 10;
    } else if(lcd_key == btnDOWN)
    {
      genStartTime = genStartTime - 10;
    } else if(lcd_key == btnRIGHT)
    {
      Menu++; //Menu is now equal to 4
      lcd.clear();
    } else if(lcd_key == btnLEFT)
    {
      Menu--; //Menu is now equal to 3
    }
  }
}

// read the buttons
int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // read the value from the sensor 
 if (adc_key_in > 1000) return btnNONE;
 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 250)  return btnUP; 
 if (adc_key_in < 450)  return btnDOWN; 
 if (adc_key_in < 650)  return btnLEFT; 
 if (adc_key_in < 850)  return btnSELECT;  

 return btnNONE;  // when all others fail, return this...
}