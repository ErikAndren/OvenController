#include "max6675.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>

const int thermoDO = 4;
const int thermoCS = 3;
const int thermoCLK = 2;

const int rightBtnPin = 11;
const int leftBtnPin = 12;

const int relayPin = 13;

LiquidCrystal lcd(5, 6, 7, 8, 9, 10);


const int noProfiles = 4;
const int maxSteps = 4;
const int stepSize = 2;


//expose internal timer
extern volatile unsigned long timer0_overflow_count;

struct step {
  unsigned int targetTemp;
  unsigned int duration;
};

char itocTable[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };

char MainScreen[3][16] = {{ "Load profile\0"   },
                          { "Create profile\0" },
                          { "Delete profile\0" }};

step profiles[noProfiles][maxSteps];

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

int menuPos = 0;
int curProf = 0;

enum state {ROOT_STATE, 
            LOAD_PROFILE_STATE,
            CREATE_PROFILE_STATE,
            DELETE_PROFILE_STATE,
            DISPLAY_PROFILE_STATE
            };
state s = ROOT_STATE;

enum profExecState {
    WAIT_TARGET_TEMP,
    TARGET_TEMP_REACHED
};

enum tempDir {
  TEMP_TO_GO_LOW,
  TEMP_TO_GO_HIGH
};


void setup() {
  
  // Setup the number of columns and rows that are available on the LCD. 
  lcd.begin(16, 2);
 
  pinMode(rightBtnPin, INPUT);
  //Turn on pullup
  digitalWrite(rightBtnPin, HIGH); 
  
  pinMode(leftBtnPin, INPUT);
    //Turn on pullup
  digitalWrite(leftBtnPin, HIGH); 
  
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  lcd.begin(16, 2);  
  lcd.setCursor(0, 0);
  
  if (EEPROM.read(0) == 0xEE) {
    for (int i = 0; i < noProfiles; i++) {
      for (int j = 0; j < maxSteps; j++) { 
        profiles[i][j] = EEPROM.get((i * noProfiles) + j + 1, profiles[i][j]);
      }
    }
  } else {
    EEPROM.write(0, 0xEE);
    for (int i = 1; i <= maxSteps * stepSize * noProfiles; i++) {
      EEPROM.write(i, 0);
    }
  }
  
  drawMainScreen(0);
}

void loop() {
  //Select button
  if (digitalRead(rightBtnPin) == LOW) {
    switch (s) {
       case LOAD_PROFILE_STATE:
         if (menuPos == noProfiles) {
           //Back selected
           s = ROOT_STATE;
           menuPos = 0;
           drawMainScreen(0);
         } else {
           s = DISPLAY_PROFILE_STATE;
           curProf = menuPos;
           
           menuPos = 0;
           drawProfileState(curProf, menuPos);
         }
         break;
         
       case DISPLAY_PROFILE_STATE:
         if (menuPos == maxSteps) {
           menuPos = 0;
           executeProfile(curProf);
           
           lcd.clear();
           lcd.print("Done!");
           delay(5000);
                      
         } else if (menuPos == maxSteps + 1) {
            s = LOAD_PROFILE_STATE;
            drawProfile(curProf);
            menuPos = curProf;
         }
         break;
         
       case DELETE_PROFILE_STATE:
         curProf = menuPos;
         if (curProf == noProfiles) {
           s = ROOT_STATE;
           menuPos = 0;
           drawMainScreen(0);   
         } else {
           for (int i = 0; i < maxSteps; i++) {
             profiles[curProf][i].targetTemp = 0;
             profiles[curProf][i].duration = 0;             
           }
           
           for (int i = curProf; i <= (maxSteps * stepSize) + curProf; i++) {
             EEPROM.write(i, 0);
           }
           lcd.clear();
           lcd.print("Profile ");
           lcd.print(curProf);
           lcd.setCursor(0, 1);
           lcd.print("deleted!");
           delay(2000);
           drawProfile(curProf);
         }
         break;
         
       case CREATE_PROFILE_STATE:
         if (menuPos == noProfiles) {
           s = ROOT_STATE;
           drawMainScreen(0);
           menuPos = 0;
         } else {
           curProf = menuPos;
           createProfile(curProf);
           drawProfile(curProf);
         }
         break;
          
      
       case ROOT_STATE:
         switch (menuPos) {
            case 0:
              s = LOAD_PROFILE_STATE;
              drawProfile(0);
              break;
             
            case 1:
              s = CREATE_PROFILE_STATE;
              drawProfile(0);
              break;
              
            case 2:
              s = DELETE_PROFILE_STATE;
              drawProfile(0);
              break;
         }
         menuPos = 0;
         break;
    }
    delay(500);
    
  //Move down button
  } else if (digitalRead(leftBtnPin) == LOW) {
    menuPos++;
    
    switch (s) {
      case ROOT_STATE:
        if (menuPos == 3) {
          menuPos = 0; 
        }

        drawMainScreen(menuPos);
        break;
        
      case LOAD_PROFILE_STATE:
        if (menuPos > noProfiles) {
          menuPos = 0;
        }
        drawProfile(menuPos);
        break;
        
      case DISPLAY_PROFILE_STATE:
        if (menuPos > maxSteps + 1) {
           menuPos = 0; 
        }
        drawProfileState(curProf, menuPos);
        break;
        
      case DELETE_PROFILE_STATE:
        if (menuPos > noProfiles) {
          menuPos = 0;
        }
        drawProfile(menuPos);
        break;
        
      case CREATE_PROFILE_STATE:
        if (menuPos > noProfiles) {
          menuPos = 0;
        }
        drawProfile(menuPos);
        break;
    }
    delay(500);
  }
}

void drawMainScreen(int pos) {
    int nextPos = pos + 1;
    if (nextPos == 3) {
      nextPos = 0;
    }
    lcd.clear();
  
    lcd.print(">");
    lcd.print(MainScreen[pos]);
    lcd.setCursor(1, 1);
    lcd.print(MainScreen[nextPos]);
}

void drawProfile(int pos) {
    lcd.clear();
    
    int nextPos = pos + 1;
    if (nextPos > noProfiles) {
       nextPos = 0; 
    }
    
    if (pos == noProfiles) {
      lcd.print(">Back");
    } else {
      lcd.print(">Profile ");
      lcd.print(itocTable[pos]);
    }
    
    lcd.setCursor(1, 1);
    
    if (nextPos == noProfiles) {
      lcd.print("Back");
    } else {
      lcd.print("Profile ");
      lcd.print(itocTable[nextPos]);
    }
}

void drawProfileState(int prof, int curStep) {
  lcd.clear();
  char buf[6];
  
  int nextStep = curStep + 1;
  if (nextStep > maxSteps + 1) {
    nextStep = 0; 
  }
  
  if (curStep == maxSteps) {
     lcd.print(">Go");
  } else if (curStep == maxSteps + 1) {
     lcd.print(">Back"); 
  } else {
     lcd.print(">");
     lcd.print(itocTable[curStep]);
     lcd.print(" ");
     itoa(profiles[prof][curStep].targetTemp, buf, 10);
     lcd.print(buf);
     lcd.print(" C ");
     itoa(profiles[prof][curStep].duration, buf, 10);
     lcd.print(buf);
     lcd.print(" s");
  }
  
  lcd.setCursor(1, 1);
  if (nextStep == maxSteps) {
    lcd.print("Go");
  } else if (nextStep == maxSteps + 1) {
    lcd.print("Back");
  } else {
     lcd.print(itocTable[nextStep]);
     lcd.print(" ");
     itoa(profiles[prof][nextStep].targetTemp, buf, 10);
     lcd.print(buf);
     lcd.print("C ");
     itoa(profiles[prof][nextStep].targetTemp, buf, 10);
     lcd.print(buf);
     lcd.print("s");     
  }
}

void createProfile(int prof) {
  //Loop here until user has released button
  while (digitalRead(rightBtnPin) == LOW) {}

  
  //Loop here until we've set temp and duration
  for (int step = 0; step < maxSteps; step++) {
    unsigned int c = 0;

    drawC(c, step);
     
    //Temp loop
    while(true) {
      if (digitalRead(rightBtnPin) == LOW) {
        profiles[prof][step].targetTemp = c;
        
        break;
      } else if (digitalRead(leftBtnPin) == LOW) {
        c++;
        if (c > 300) {
          c = 0;
        }
        drawC(c, step);
        delay(100);
      }
   }
   
   //Loop here until user has released button
   while (digitalRead(rightBtnPin) == LOW) {}
   
   //Duration loop
   c = 0;
   drawT(c, step);
   
   while(true) {
      if (digitalRead(rightBtnPin) == LOW) {
        profiles[prof][step].duration = c;
        
        break;
      } else if (digitalRead(leftBtnPin) == LOW) {
        c++;
        if (c > 300) {
          c = 0;
        }
       
        drawT(c, step);
        delay(100);
      }
   }
   
   //Loop here until user has released button
   while (digitalRead(rightBtnPin) == LOW) {}
  }
}

void drawC(unsigned int c, int s) {
  char buf[4];

  lcd.clear();
  lcd.print("C step ");
  lcd.print(itocTable[s]);
            
  lcd.setCursor(0, 1);
  lcd.print("T=");
  itoa(c, buf, 10);
  lcd.print(buf);
  lcd.print(" C");
}

void drawT(unsigned int t, int s) {
   char buf[4];

   lcd.clear();
   lcd.print("T step ");
   lcd.print(itocTable[s]);
    
   lcd.setCursor(0, 1);
   lcd.print("Duration=");
   itoa(t, buf, 10);
   lcd.print(buf);
   lcd.print(" m");
}

void executeProfile(int prof) {
  //Reset timer
  timer0_overflow_count = 0;
  
  for (int step = 0; step < maxSteps; step++) {
    double startTemp = thermocouple.readCelsius();
    unsigned long maintainTempMark;
    tempDir tDir; 
    char buf[4];
    bool targetTempReached = false;
    profExecState pState = WAIT_TARGET_TEMP;
    
    if (startTemp < profiles[prof][step].targetTemp) {
      tDir = TEMP_TO_GO_HIGH;
    } else {
      tDir = TEMP_TO_GO_LOW;      
    }

    //Execute step
    while(true) {
      double curTemp = thermocouple.readCelsius();
      bool doneWithStep = false;
      
      //Wait for temperature to rise to target temp
      if (curTemp < profiles[prof][step].targetTemp) {
        digitalWrite(relayPin, HIGH);
      } else {
        digitalWrite(relayPin, LOW);        
      }

      switch (pState) {
        case WAIT_TARGET_TEMP:
          if ((tDir == TEMP_TO_GO_HIGH && curTemp >= profiles[prof][step].targetTemp) ||
              (tDir == TEMP_TO_GO_LOW && curTemp <= profiles[prof][step].targetTemp)) {
            pState = TARGET_TEMP_REACHED;
            maintainTempMark = timer0_overflow_count;
          }
          break;
         
        case TARGET_TEMP_REACHED:
          if ((timer0_overflow_count - maintainTempMark) > profiles[prof][step].duration * 60 * 1E6) {
            doneWithStep = true;
          }
          break;
      }
      
      if (doneWithStep) {
        break; 
      }

      lcd.clear();
      lcd.print("Step ");
      lcd.print(itocTable[step]);
      lcd.print(" C=");
      itoa(curTemp, buf, 10);
      lcd.print(buf);
      lcd.setCursor(0, 1);
      lcd.print("T=");
      itoa(profiles[prof][step].targetTemp, buf, 10);
      lcd.print(buf);
      lcd.print(" ");
      lcd.print("RunT=");
              
      unsigned long hours = (((timer0_overflow_count / 1E6) / 60) / 60);
      unsigned long mins = (timer0_overflow_count - (hours * 60 * 60 * 1E6)) / 1E6 / 60;
      unsigned long secs = (timer0_overflow_count - (mins * 60 * 1E6)) / 1E6;
      
      itoa(hours, buf, 10);
      lcd.print(buf);
      lcd.print(":");
      itoa(mins, buf, 10);
      lcd.print(buf);
      itoa(secs, buf, 10);
      lcd.print(":");
      lcd.print(buf);
      
      delay(1000);
    }   
  }
}

