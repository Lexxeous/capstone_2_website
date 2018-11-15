//include necessary libraries
#include <EEPROM.h>
#include <LiquidCrystal.h>

//setup LCD
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//define LCD button values
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

//keep track of LCD button values
int lcd_key     = 0;
int adc_key_in  = 0;
int btnPressed = 1; //1 is btnPressed, 0 is btnNotPressed
int prevBtnPressed = 1; //1 is btnPressed, 0 is btnNotPressed
bool btnHeld = false; //default to false

//declare pin numbers for components
const int coil = 2;
const int starter = 3;
const int genStatus = 11; //if(genStatus == 0), then generator is on
const int redLight = 12;
const int backlight = 10;

//keep track of voltages and counters
int genStartIterations = 0; //number of starter iterations
int inactivityCounter = 0;
int rawVolts = 0;
int batMilliVolts = 0; //external battery voltage in mV
int mapValue = 31000;

//keep track of timers and delays
int timer2_3sec = 0;
int timerDelay = 0; //counter for the delay to reach "delayTime" value
int secondTimer = 0;
int starterTimer = 0;
const int mainDelay = 100; //universal delay for main loop

//keep track of the menuIndex
int menuIndex = 5;
#define changeMinVolt 1
#define changeMaxVolt 2
#define changeDelay 3
#define changeStartDelay 4

//default user specified system parameters
int minimumVoltage = 11500; //external battery minimum threshold
int maximumVoltage = 13500; //external battery maximum threshold
int delayTime = 300; //time between starter iterations
int genStartTime = 25; //time to crank generator starter

//keep track of EEPROM addresses
int addr = 0;

//keep track of generator signals
bool coilStatus = true;
bool starterStatus = true;
bool actualGenStatus;
bool voltageStatus = true;

//keep track of screen and automation status
char* automationStatus = "OFF";
char* screenStatus = "ON";

//--------------------------------------------------------------------------------------

void setup() //system setup
{
  //assign pin modes
  pinMode(coil, OUTPUT);
  pinMode(starter, OUTPUT);
  pinMode(genStatus, INPUT_PULLUP);
  pinMode(redLight, OUTPUT);
  pinMode(backlight, OUTPUT);

  //initialize baud rate and LCD
  Serial.begin(9600);
  lcd.begin(16, 2);

  //default "coil" and "starter" to off
  digitalWrite(coil, HIGH);
  digitalWrite(starter, HIGH);

  read_from_memory(); //get system parameters from EEPROM
}

//--------------------------------------------------------------------------------------

void loop() //main loop
{
  check_for_inactivity();

  if(menuIndex <= 4)
  {
    read_from_memory(); //get system parameters from EEPROM
    while(menuIndex <= 4)
    {
      disp_params(); //adjust system parameters
      delay(mainDelay); //main loop runs every 100ms
    }
    write_to_memory(); //put system parameters into EEPROM
  } 
  else 
  {
    read_volts(); //read external battery voltage
    delay(mainDelay); //main loop runs every 100ms
  }
}

//--------------------------------------------------------------------------------------

void read_volts()
{
  lcd.setCursor(0, 0);
  lcd_key = read_LCD_buttons();
  rawVolts = analogRead(A1); //read voltage from the generator
  batMilliVolts = map(rawVolts, 0, 1023, 0, mapValue); //read external battery in mV

  //dont allow rapid button changes
  adj_btn_sensitivity();
  if(btnHeld == true)
  {
    btnHeld = false;
    return;
  }

  //set "automationStatus"
  if((lcd_key == btnUP || lcd_key == btnDOWN) && screenStatus == "ON")
  {
    if(automationStatus == "ON")
      automationStatus = "OFF";
    else if(automationStatus == "OFF")
      automationStatus = "ON";
  }

  //adjust system parameters
  if(lcd_key == btnLEFT || lcd_key == btnRIGHT)
  {
    menuIndex = 1;
    disp_params();
  }
  
  actualGenStatus = !(digitalRead(genStatus));
  starterTimer++;
  secondTimer++;
  if (secondTimer == 10)
  {
    //display external battery voltage in V
    lcd.print("Battery = ");
    lcd.print(batMilliVolts / 1000);
    lcd.print(".");
    lcd.print((batMilliVolts % 1000) / 100);
    lcd.print("V  ");
    secondTimer = 0;
  }

  //display "automationStatus"
  lcd.setCursor (0, 1);
  lcd.print("Automation: ");
  lcd.print(automationStatus);
  lcd.print(" ");

  if(batMilliVolts > minimumVoltage && actualGenStatus == 0)
  {
    stop_generator();
  }

  //check "automationStatus"
  if(automationStatus == "OFF")
  {
    genStartIterations = 0;
    return;
  }

  //if the generator needs to be turned on
  if(batMilliVolts < minimumVoltage && actualGenStatus == 0)
  { 
    if(voltageStatus == true) //avoid jitter
    {
      voltageStatus = false;
      minimumVoltage += 200;
    }
     
    if(coilStatus == true)
    {
      digitalWrite(coil, LOW); //sets coil hot at 12 volts
      coilStatus = false;
    }

    //while still trying to turn on generator
    while(actualGenStatus == 0 && genStartIterations <= 2)
    { 
      actualGenStatus = !(digitalRead(genStatus)); //assign actual generator status
      digitalWrite(starter, LOW); //turn on starter
      starterStatus = false;
      
      if(timer2_3sec < genStartTime) //delay 2~3 seconds
      {
        timer2_3sec++;
        return;
      }
            
      digitalWrite(starter, HIGH); //turn off starter
      starterStatus = true;
      
      if(timerDelay < delayTime)
      {
        timerDelay++;
        return;
      }
      
      actualGenStatus = !(digitalRead(genStatus)); //read generator status
      if(actualGenStatus == 1)
      {
        genStartIterations = 0;
        return;
      }
      
      genStartIterations++; //go to next starter try
      
      //reset timer values
      timer2_3sec = 0;
      timerDelay = 0;
    }

    if(actualGenStatus == 0 && genStartIterations >= 3)
    {
      //failed to start generator
      stop_generator();
      starter_failure(); //stops the main loop
    }
    
    //reset starter iterations and timer
    genStartIterations = 0;
    starterTimer = 0;
  }
  else if(batMilliVolts >= maximumVoltage && actualGenStatus == 1)
  { 
    stop_generator();
  } 
  else if(batMilliVolts > minimumVoltage && actualGenStatus == 0)
  {
    stop_generator();
    if(voltageStatus == false) //avoid jitter
    {
      voltageStatus = true;
      minimumVoltage -= 200;
    }
  }
  else if(batMilliVolts > 14000) //failsafe
  {
    stop_generator();
  }

  actualGenStatus = !(digitalRead(genStatus)); //assign actual generator status
  if(actualGenStatus == 1) //if generator started
  {
    //prevent starter from getting stuck on
    digitalWrite(starter, HIGH); //turn off starter
    starterStatus = true;
    genStartIterations = 0; //reset starter iterations
  }
}

//--------------------------------------------------------------------------------------

void stop_generator()
{
    //reset counters and timers
    genStartIterations = 0;
    timer2_3sec = 0;
    timerDelay = 0;
    starterTimer = 0;

    //turn of generator
    digitalWrite(coil, HIGH);
    coilStatus = true;
    digitalWrite(starter, HIGH); 
    starterStatus = true;
}

//--------------------------------------------------------------------------------------

void starter_failure()
{
  while(1) //loop forever
  {    
    if(screenStatus == "ON")
    {
      //display error message
      digitalWrite(redLight, HIGH);
      digitalWrite(backlight, HIGH);
      lcd.display();
      screenStatus = "ON";
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Starter Failure!");
      lcd.setCursor(0, 1);
      lcd.print("Reset System...");
    }
    
    check_for_inactivity(); //dont display error forever
    delay(mainDelay); //delay 100ms
  }
}

//--------------------------------------------------------------------------------------

void disp_params()
{
  //reset and read LCD buttons
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd_key = read_LCD_buttons();
  stop_generator();

  //dont allow rapid button changes
  adj_btn_sensitivity();
  if(btnHeld == true)
  {
    btnHeld = false;
    return;
  }

  //exit if user presses "btnSELECT"
  if(lcd_key == btnSELECT)
  {
    menuIndex = 5;
  }
  
  if(menuIndex == changeMinVolt) //first system parameter
  {
    //display "minimumVoltage" parameter
    lcd.print("Min Volt: ");
    lcd.setCursor(0, 1);
    lcd.print(minimumVoltage / 1000);
    lcd.print(".");
    lcd.print((minimumVoltage % 1000) / 100);
    lcd.print("V");
    
    if(lcd_key == btnUP) //increase "minimumVoltage" by 100mV
    {
      minimumVoltage = minimumVoltage + 100;
      if(minimumVoltage > 12000) //ceiling
      {minimumVoltage = 12000;}      
    }
    else if (lcd_key == btnDOWN) //decrease "minimumVoltage" by 100mV
    {
      minimumVoltage = minimumVoltage - 100;
      if(minimumVoltage < 11000) //floor
      {minimumVoltage = 11000;}
    }
    else if (lcd_key == btnRIGHT) //go to next parameter ("maximumVoltage")
    {
      menuIndex++; //"menuIndex" = 2
    }
  }
  else if (menuIndex == changeMaxVolt) //second system parameter
  {
    //display "maximumVoltage" parameter
    lcd.print("Max Volt: ");
    lcd.setCursor(0, 1);
    lcd.print(maximumVoltage / 1000);
    lcd.print(".");
    lcd.print((maximumVoltage % 1000) / 100 );
    lcd.print("V");
    
    if(lcd_key == btnUP) //increases the "maximumVoltage" by 100mV
    {
      maximumVoltage = maximumVoltage + 100;
      if(maximumVoltage > 14000) //ceiling
      {maximumVoltage = 14000;}
    }
    else if (lcd_key == btnDOWN) //decrease "minimumVoltage" by 100mV
    {
      maximumVoltage = maximumVoltage - 100;
      if(maximumVoltage < 13000) //floor
      {maximumVoltage = 13000;}
    }
    else if (lcd_key == btnRIGHT) //go to next parameter ("delayTime")
    {
      menuIndex++; //"menuIndex" = 3
    }
    else if (lcd_key == btnLEFT) //go to previous parameter ("minimumVoltage")
    {
      menuIndex--; //"menuIndex" = 2
    }
  }
  else if(menuIndex == changeDelay)
  { 
    //display "delayTime" parameter
    lcd.print("Delay Time: ");
    lcd.setCursor(0, 1);
    lcd.print(delayTime / 10); //display "delayTime" in seconds
    lcd.print("s");
    
    if(lcd_key == btnUP) //increase "delayTime" by 1 second
    {
      delayTime = delayTime + 10;
      if(delayTime > 600) //ceiling
      {delayTime = 600;}
    }
    else if(lcd_key == btnDOWN) //decrease "delayTime" by 1 second
    {
      delayTime = delayTime - 10;
      if(delayTime < 200) //floor
      {delayTime = 200;}
    }
    else if(lcd_key == btnRIGHT) //go to next parameter ("genStartTime")
    {
      menuIndex++; //"menuIndex" = 4
      lcd.clear();
    }
    else if(lcd_key == btnLEFT) //go to previous parameter ("maximumVoltage")
    {
      menuIndex--;  //"menuIndex" = 3
    }
  }
  else if(menuIndex == changeStartDelay)
  {
    //display "genStartTime"
    lcd.print("Gen Start Time: ");
    lcd.setCursor(0, 1);
    lcd.print(genStartTime / 10);
    lcd.print(".");
    lcd.print(genStartTime % 10);
    lcd.print("s");
    
    if(lcd_key == btnUP) //increase "genStartTime" by 0.1 seconds
    {
      genStartTime = genStartTime + 1;
      if(genStartTime > 30) //ceiling 
      {genStartTime = 30;}
    }
    else if(lcd_key == btnDOWN) //decrease "genStartTime" by 0.1 seconds
    {
      genStartTime = genStartTime - 1;
      if(genStartTime < 20)//floor
      {genStartTime = 20;}
    }
    else if(lcd_key == btnRIGHT) //go to main menuIndex
    {
      menuIndex++; //"menuIndex" = 5
      lcd.clear();
    }
    else if(lcd_key == btnLEFT) //go to previous parameter ("delayTime")
    {
      menuIndex--; //"menuIndex" = 3
    }
  }
}

//--------------------------------------------------------------------------------------

void write_to_memory() //put system parameters into EEPROM
{
  int addr = 0;
  EEPROM.put(addr, (float)minimumVoltage / 1000);
  
  addr = 5;
  EEPROM.put(addr, (float)maximumVoltage / 1000);

  addr = 10;
  EEPROM.put(addr, (float)delayTime / 10);

  addr = 15;
  EEPROM.put(addr, (float)genStartTime / 10);
}

//--------------------------------------------------------------------------------------

void read_from_memory() //get system parameters from EEPROM
{
  float fminv = 11.9f;
  float fmaxv = 13.4f;
  float fdelt = 2.00f;
  float fgensd = 5.00f;
  int address = 0;
  
  address = 0; //address 0 has "minimumVoltage"
  EEPROM.get(address, fminv);
  minimumVoltage = (int)(fminv * 1000);
  
  address = 5;
  EEPROM.get(address, fmaxv);
  maximumVoltage = (int)(fmaxv * 1000);

  address = 10;
  EEPROM.get(address, fdelt);
  delayTime = (int)(fdelt * 10);

  address = 15;
  EEPROM.get(address, fgensd);
  genStartTime = (int)(fgensd * 10);
}

//--------------------------------------------------------------------------------------

int read_LCD_buttons() //read LCD buttons
{
 adc_key_in = analogRead(0); //read the value from the sensor 
 if (adc_key_in > 1000) return btnNONE;
 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 250)  return btnUP; 
 if (adc_key_in < 450)  return btnDOWN; 
 if (adc_key_in < 650)  return btnLEFT; 
 if (adc_key_in < 850)  return btnSELECT;  
 return btnNONE; //default to return btnNONE
}

//--------------------------------------------------------------------------------------

void adj_btn_sensitivity() //dont allow rapid button changes
{
  if(lcd_key == btnNONE)
  {
    prevBtnPressed = btnPressed;
    btnPressed = 1;
  } 
  else 
  {
    prevBtnPressed = btnPressed;
    btnPressed = 0;
  }
  
  if(prevBtnPressed == 0 && btnPressed == 0)
  {
   btnHeld = true;
  }
}

//--------------------------------------------------------------------------------------

void check_for_inactivity() //turn off LCD screen after 10 seconds of inactivity
{
  lcd_key = read_LCD_buttons();
  if(lcd_key == btnNONE && inactivityCounter < 100 && screenStatus == "ON")
  {
    //one loop of inactivity
    inactivityCounter++;
    
    if(inactivityCounter >= 100) //inactivity threshold reached
    {
      digitalWrite(backlight, LOW); //turn screen off
      lcd.noDisplay();
      screenStatus = "OFF";
      inactivityCounter = 0;
    }
    else
    {
      digitalWrite(backlight, HIGH); //turn screen on
      lcd.display();
      screenStatus = "ON";
    }
  }
  else if(lcd_key == btnSELECT)
  {
     digitalWrite(backlight, HIGH); //turn screen on
     lcd.display();
     screenStatus = "ON";
  }

  if(lcd_key != btnNONE)
  {
    inactivityCounter = 0; //reset inactivity timer
  }
}

//--------------------------------------------------------------------------------------
