#include <SoftwareSerial.h>
#include <UM7_BIN.h>

/*
  SPI header of UM7 goes to Arduino (Vin, gnd, tx, rx)
  Secondary header of UM7 goes to GPS (rx2, tx2)
 */
#define GC_NULL  0
#define GC_RESET 1
#define GC_STOP  2
#define GC_BEGIN 3
#define GC_START 4
#define GC_ROLL 5
#define GC_LINEAR 6

#define ME_NOSE_DOWN 0
#define ME_GLIDE_DOWN 1
#define ME_NOSE_UP 2
#define ME_GLIDE_UP 3
#define ME_PAUSE 5

UM7_BIN um7;

struct param_t {
  int linPos;                      // linpos
  int rotPos;                      // rotpos
  int tankPos;                     // tankpos
  int linRate;                     // linrate
  int rotRate;                      // rotrate
  int linFrontLimit;            
  int linBackLimit;
  int tankBackLimit;
  int tankFrontLimit;
  int rotLowLimit;
  int rotHighLimit;
  int desTime;
  int riseTime;
  int tankMid;
  int linMid;
  int rotMid;
  int allowedWorkTime;
  float linkp;
  float linki;
  float linkd;
  int linNoseDownTarget;
  int linNoseUpTarget;
  float rateScale;
  float rollAngle;
} 
param;
// linpos 850 rotpos 680 tankpos 400
// linpos 250 rotpos 600 tankpos 150
const int motAPWM = 10;
const int motAConf1 = 12;
const int motAConf2 = 8;
const int motStdby = 13;
const int motBPWM = 11;
const int motBConf1 = 7;
const int motBConf2 = 4;

const int pumpOn = 53;
const int pumpDir = 51;
const int suicide = 49;
const int pwrOn = 47;

//// LINEAR MASS LIMITS
const int linmid = 525;
const int linfrontlimit = 280;
const int linbacklimit = 770;

//// WATER TANK LIMITS
const int tankmid = 270;
const int tankbacklimit = 80;
const int tankfrontlimit = 500; //WAS 500

//// ROTARY MASS LIMITS
const int rotmid = 700;
const int rotlowlimit = rotmid - 150;
const int rothighlimit = rotmid + 150;

float I = 0;
float error_act = 0;
float error_prev = 0;

int tankLevel = A0;
int linPos = A1;
//int linFrontLimit = 140;
//int linBackLimit = 890;
int rotPos = A7;

char *help = "Commands: \n\tparams will show the current parameters and acceptable ranges \n\treset will center linear mass and water tank (for trimming)\n\tupdate [-rothighlimit|-rotlowlimit|-linfrontlimit|-linbacklimit|-tankhighlimit|-tanklowlimit|-linpos|-rotpos\n\t\t-tankpos|-linrate|-rotrate|-destime|-risetime|-tankmid|-linmid|-rotmid|-allowedWorkTime -linNoseUpTarget\n\t\t -linNoseDownTarget -linkp -linki -linkd -rateScale] [newValue]\n\tcurrentpos shows current position of actuators\n\tstart starts glide cycle\n\tstop stops glide cycle\n\tlinear toggles linear PID controller on/off\n\trollto [num]\tTurns Glider to [num]";

void setup()
{
  // setup imu
  um7.begin(&Serial3);
  Serial.begin(9600);
  Serial.setTimeout(10);
  Serial.println("```````````````````````````````````````````````````````````````````````````````\n   @@@@@'     @@@@        @@@@@@         ,@@@@@@`              @'@+,``:@,      \n   @@@@@@     @@@@       `@@@@@@        @@@@@@@@.            `@@@        @     \n   @@@@@@#    @@@@       @@@@@@@+      @@@@@@@@@:          `@             #    \n   @@@@@@@    @@@@      `@@@@@@@@      @@@@@:`,#+         #`              ,    \n   @@@@@@@@   @@@@      @@@@`@@@@      @@@@'            .+                 .   \n   @@@@@@@@`  @@@@     .@@@@ '@@@#     @@@@@`          #`                  :   \n   @@@@@@@@@  @@@@     @@@@.  @@@@      @@@@@:        @                    ,   \n   @@@@ @@@@. @@@@    ,@@@@,,,@@@@.      @@@@@#      @                     `   \n  `@@@# #@@@@ @@@#    @@@@@@@@@@@@@       @@@@@#                  ```     :    \n  .@@@#  @@@@'@@@+   ;@@@@@@@@@@@@@        @@@@@:,      ',,,   ;,,,,,,.   @    \n  ,@@@'  :@@@@@@@;   @@@@@@@@@@@@@@;        @@@@#.     `,,,,,  ;,,++,,,   .    \n  ;@@@;   @@@@@@@:  +@@@@      @@@@@  @@.  :@@@@#.     ;,,+,,  ;,, `,,.  #     \n  '@@@;   `@@@@@@,  @@@@@      ,@@@@  '@@@@@@@@@,`    `,,`',,  ',,,,,:   .     \n  #@@@:    @@@@@@. #@@@@        @@@@# :@@@@@@@@+,`    :,,,,,,, +,,++,,, @      \n  @@@@:     @@@@@. @@@@@        @@@@@ .@@@@@@@',,`   .,,'++',, +,,  ,,,        \n                                              +,,,,,,:,,   +,,`+,,,,,,`        \n                                              +''''';''`   :''`+'''''.         \n                                                                               \nNonlinear  and      Autonomous        Systems          Laboratory              \n");
  //Serial.println("RRRRRRRRRRRRRRRRR        OOOOOOOOO     UUUUUUUU     UUUUUUUU       GGGGGGGGGGGGGHHHHHHHHH     HHHHHHHHHIIIIIIIIIIEEEEEEEEEEEEEEEEEEEEEE            VVVVVVVV           VVVVVVVV 222222222222222   \nR::::::::::::::::R     OO:::::::::OO   U::::::U     U::::::U    GGG::::::::::::GH:::::::H     H:::::::HI::::::::IE::::::::::::::::::::E            V::::::V           V::::::V2:::::::::::::::22 \nR::::::RRRRRR:::::R  OO:::::::::::::OO U::::::U     U::::::U  GG:::::::::::::::GH:::::::H     H:::::::HI::::::::IE::::::::::::::::::::E            V::::::V           V::::::V2::::::222222:::::2 \nRR:::::R     R:::::RO:::::::OOO:::::::OUU:::::U     U:::::UU G:::::GGGGGGGG::::GHH::::::H     H::::::HHII::::::IIEE::::::EEEEEEEEE::::E            V::::::V           V::::::V2222222     2:::::2 \n  R::::R     R:::::RO::::::O   O::::::O U:::::U     U:::::U G:::::G       GGGGGG  H:::::H     H:::::H    I::::I    E:::::E       EEEEEE             V:::::V           V:::::V             2:::::2 \n  R::::R     R:::::RO:::::O     O:::::O U:::::D     D:::::UG:::::G                H:::::H     H:::::H    I::::I    E:::::E                           V:::::V         V:::::V              2:::::2 \n  R::::RRRRRR:::::R O:::::O     O:::::O U:::::D     D:::::UG:::::G                H::::::HHHHH::::::H    I::::I    E::::::EEEEEEEEEE                  V:::::V       V:::::V            2222::::2  \n  R:::::::::::::RR  O:::::O     O:::::O U:::::D     D:::::UG:::::G    GGGGGGGGGG  H:::::::::::::::::H    I::::I    E:::::::::::::::E                   V:::::V     V:::::V        22222::::::22   \n  R::::RRRRRR:::::R O:::::O     O:::::O U:::::D     D:::::UG:::::G    G::::::::G  H:::::::::::::::::H    I::::I    E:::::::::::::::E                    V:::::V   V:::::V       22::::::::222     \n  R::::R     R:::::RO:::::O     O:::::O U:::::D     D:::::UG:::::G    GGGGG::::G  H::::::HHHHH::::::H    I::::I    E::::::EEEEEEEEEE                     V:::::V V:::::V       2:::::22222        \n  R::::R     R:::::RO:::::O     O:::::O U:::::D     D:::::UG:::::G        G::::G  H:::::H     H:::::H    I::::I    E:::::E                                V:::::V:::::V       2:::::2             \n  R::::R     R:::::RO::::::O   O::::::O U::::::U   U::::::U G:::::G       G::::G  H:::::H     H:::::H    I::::I    E:::::E       EEEEEE                    V:::::::::V        2:::::2             \nRR:::::R     R:::::RO:::::::OOO:::::::O U:::::::UUU:::::::U  G:::::GGGGGGGG::::GHH::::::H     H::::::HHII::::::IIEE::::::EEEEEEEE:::::E ,,,,,,              V:::::::V         2:::::2       222222\nR::::::R     R:::::R OO:::::::::::::OO   UU:::::::::::::UU    GG:::::::::::::::GH:::::::H     H:::::::HI::::::::IE::::::::::::::::::::E ,::::,               V:::::V          2::::::2222222:::::2\nR::::::R     R:::::R   OO:::::::::OO       UU:::::::::UU        GGG::::::GGG:::GH:::::::H     H:::::::HI::::::::IE::::::::::::::::::::E ,::::,                V:::V           2::::::::::::::::::2\nRRRRRRRR     RRRRRRR     OOOOOOOOO           UUUUUUUUU             GGGGGG   GGGGHHHHHHHHH     HHHHHHHHHIIIIIIIIIIEEEEEEEEEEEEEEEEEEEEEE ,:::,,                 VVV            22222222222222222222\n                                                                                                                                       ,:::,\n                                                                                                                                                                                           ,,,,   \n");
  Serial.println(help);
  delay(1000); //Wait for all the printing

  // initial parameters
  param.linPos = 400; // Limits are 138 945
  param.rotPos = 600; // 600 to 680
  param.tankPos = 285; // 70 to 500
  param.linRate = 185;
  param.rotRate = 20;
  param.linFrontLimit = linfrontlimit;
  param.linBackLimit = linbacklimit;
  param.tankBackLimit = tankbacklimit;
  param.tankFrontLimit = tankfrontlimit;
  param.rotLowLimit = rotlowlimit;
  param.rotHighLimit = rothighlimit;
  param.desTime = 10000;
  param.riseTime = 10000;
  param.tankMid = 285;
  param.linMid = 515;
  param.rotMid = rotmid;
  param.allowedWorkTime = 30000;
  param.linNoseDownTarget = -30;
  param.linNoseUpTarget = 30;
  param.linkp = 10;
  param.linki = 0;
  param.linkd = 0;
  param.rateScale = 1;

  gliderStateMachine(GC_BEGIN);
  gliderStateMachine(GC_STOP);
}

void loop()
{
  const int BUFF_LEN = 80;
  char buff[BUFF_LEN];

  gliderStateMachine(GC_NULL);
  
  int ret = um7.refresh();  // refresh nav data
  if(ret >= 3) { // unexpected error detected
    Serial.print("<"); Serial.print(ret); Serial.println(">");
  }
  //Serial.print("flags ");
  //Serial.println(um7.roll);
  if(um7.updated_p) { // pose packet
//  if(1) {
      um7.updated_p = 0;
      //Serial.println("NEW IMU DATA");
      //Serial.print("t_p1 = "); Serial.print(um7.t_p1); Serial.println("; ");
      //Serial.print("roll = "); Serial.print(um7.roll); Serial.println("; ");
      //Serial.print("pitch = "); Serial.print(um7.pitch); Serial.println("; ");
      //Serial.print("yaw = "); Serial.print(um7.yaw); Serial.println("; ");
      //Serial.print("rolld = "); Serial.print(um7.rolld); Serial.println("; ");
      //Serial.print("pitchd = "); Serial.print(um7.pitchd); Serial.println("; ");
      //Serial.print("yawd = "); Serial.print(um7.yawd); Serial.println("; ");
      //Serial.print("north = "); Serial.print(um7.north); Serial.println("; ");
      //Serial.print("east = "); Serial.print(um7.east); Serial.println("; ");
      //Serial.print("up = "); Serial.print(um7.up); Serial.println("; ");
      //Serial.print("t_p2 = "); Serial.print(um7.t_p2); Serial.println("; ");
    }
  
  if(Serial.available()) // if command was read
  {
    char arg[3][80];
    int len = Serial.readBytesUntil('\r', buff, BUFF_LEN);
    buff[len] = '\0';

    Serial.print("@");
    Serial.print(millis());
    Serial.print(" > ");
    Serial.println(buff); // echo back with timestamp

    sscanf(buff, "%s %s %s", arg[0], arg[1], arg[2]); // parsing

    if(strcmp(arg[0], "reset") == 0) {
      Serial.println("GO!");
      gliderStateMachine(GC_RESET);

    }
    else if(strcmp(arg[0], "start") == 0) {
      gliderStateMachine(GC_START);
    }
    else if(strcmp(arg[0], "stop") == 0) {
      gliderStateMachine(GC_STOP);
    }
    else if(strcmp(arg[0], "update") == 0) {  // update parameter
      if(strcmp(arg[1], "-linpos") == 0) {
        param.linPos = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-rotpos") == 0) {
        param.rotPos = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-tankpos") == 0) {
        param.tankPos = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-linrate") == 0) {
        param.linRate = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-rotrate") == 0) {
        param.rotRate = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-destime") == 0) {
        param.desTime = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-risetime") == 0) {
        param.riseTime = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-linmid") == 0) {
        param.linMid = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-rotmid") == 0) {
        param.rotMid = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-tankmid") == 0) {
        param.tankMid = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-linfrontlimit") == 0) {
        param.linFrontLimit = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-linbacklimit") == 0) {
        param.linBackLimit = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-tankfrontlimit") == 0) {
        param.tankFrontLimit = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-tankbacklimit") == 0) {
        param.tankBackLimit = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-rotlowlimit") == 0) {
        param.rotLowLimit = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-rothighlimit") == 0) {
        param.rotHighLimit = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-allowedWorkTime") == 0) {
        param.allowedWorkTime = atoi(arg[2]);
      }
      if(strcmp(arg[1], "-linkp") == 0) {
        param.linkp = atoi(arg[2]);
        Serial.print("Linear Kp updated to: ");
        Serial.println(param.linkp);
      }      
      if(strcmp(arg[1], "-linki") == 0) {
        param.linki = atoi(arg[2]);        
        Serial.print("Linear Ki updated to: ");
        Serial.println(param.linki);
      }      
      if(strcmp(arg[1], "-linkd") == 0) {
        param.linkd = atoi(arg[2]);        
        Serial.print("Linear Kd updated to: ");
        Serial.println(param.linkd);
      }
      if(strcmp(arg[1], "-linNoseUpTarget") == 0) {
        param.linNoseUpTarget = atoi(arg[2]);
        Serial.print("PID Nose up target set to: ");
        Serial.println(param.linNoseUpTarget);
      }
      if(strcmp(arg[1], "-linNoseDownTarget") == 0) {
        param.linNoseUpTarget = atoi(arg[2]);
        Serial.print("PID Nose up target set to: ");
        Serial.println(param.linNoseDownTarget);
      }
      if(strcmp(arg[1], "-rateScale") == 0) {
        param.rateScale = atoi(arg[2]);
        Serial.print("Rate Scale updated to: ");
        Serial.println(param.rateScale);
      }
      else {
        Serial.println(help);
      }
    }
    else if(strcmp(arg[0], "currentpos") == 0) { // Current positions
      Serial.print("Linear Mass: ");
      Serial.println(getFiltAnalog(linPos));
      Serial.print("Rotational Mass: ");
      Serial.println(getFiltAnalog(rotPos));
      Serial.print("Water tank: ");
      Serial.println(getFiltAnalog(tankLevel));
    }
    else if(strcmp(arg[0], "linear") == 0) {
      gliderStateMachine(GC_LINEAR);
    }
    else if(strcmp(arg[0], "params") == 0) { // Current parameters
      Serial.print("Desired Rotational Mass Position: ");
      Serial.print(param.rotPos);
      Serial.print("  Acceptable Range:  ");
      Serial.print(param.rotLowLimit);
      Serial.print(" to ");
      Serial.println(param.rotHighLimit);
      
      Serial.print("Desired Linear Mass Position: ");
      Serial.print(param.linPos);
      Serial.print("  Acceptable Range:  ");
      Serial.print(param.linFrontLimit);
      Serial.print(" to ");
      Serial.println(param.linBackLimit);
      
      Serial.print("Desired Water Tank Position: ");
      Serial.print(param.tankPos);
      Serial.print("  Acceptable Range:  ");
      Serial.print(param.tankBackLimit);
      Serial.print(" to ");
      Serial.println(param.tankFrontLimit);
      
      Serial.print("Descent Time: ");
      Serial.println(param.desTime);
      Serial.print("Rise Time: ");
      Serial.println(param.riseTime);
      
      Serial.println("MIDDLE SETTINGS");
      Serial.print("Tank: ");
      Serial.print(param.tankMid);
      Serial.print("  Default: ");
      Serial.println(tankmid);
      Serial.print("Linear Mass: ");
      Serial.print(param.linMid);
      Serial.print("  Default: ");
      Serial.println(linmid);
      Serial.print("Rotational Mass: ");
      Serial.print(param.rotMid);
      Serial.print("  Default: ");
      Serial.println(rotmid);
      
      Serial.print("Maximum allowed work time: ");
      Serial.print(param.allowedWorkTime);
      Serial.println("...THIS DOES NOT APPLY TO THE RESET FUNCTION!");
      
      Serial.println("PID Settings");
      Serial.print("kp: ");
      Serial.println(param.linkp);
      Serial.print("ki: ");
      Serial.println(param.linki);
      Serial.print("kd: ");
      Serial.println(param.linkd);
      Serial.print("Rate scaling: ");
      Serial.println(param.rateScale);
    }
      
    else {
      Serial.println(help); // print help is no match was found
    }
  }

}

void gliderStateMachine(int cmd) {

  static int state = 0;        // machine state
  static bool enGlider = 0;    // enable cycle
  static bool entry;           // if it's first time executing the current state
  static bool pumpDone;        // If the pump is done pumping
  static bool linDone;         // If the linear mass is done moving
  static bool DoLinPID = 0;
  static unsigned long int t0; // initial time of current state
  
  if(cmd == GC_START) {
    enGlider = 1;
    state = ME_NOSE_DOWN;
    entry = 1;
  }
  if(cmd == GC_STOP) {
    enGlider = 0;
  }
  
  if(cmd == GC_NULL) { // continue normal state machine run
    //Serial.println(um7.roll);
    if(!enGlider) return;
    
    switch(state) { // select current state
    
      case ME_NOSE_DOWN:
        if(entry) {
          t0 = millis();
          if(DoLinPID) {
            I = 0;
            param.linRate = linMassRatePID(param.linNoseDownTarget);
            analogWrite(motAPWM, param.linRate);
          }
          else {
            moveLinMass(param.linFrontLimit, param.linRate);//sets direction then turns on
          }
          moveWater(param.tankBackLimit); //sets pump direction then turns pump on
          entry = 0;
          pumpDone = 0;
          linDone = 0;
        }
        
        // Turn pump off when it's time
        if(abs(getFiltAnalog(tankLevel)-param.tankBackLimit) < 20) {
          digitalWrite(pumpOn, LOW);
          digitalWrite(pwrOn, LOW);
          pumpDone = 1;
        }
        
        // Turn linear mass off when it's time
        if(DoLinPID) {
          int ret = um7.refresh();
          if(um7.updated_p) {
            um7.updated_p = 0;
          }
          if((getFiltAnalog(linPos) <= param.linFrontLimit) && (um7.roll > param.linNoseDownTarget)) {
            analogWrite(motAPWM, 0);
            digitalWrite(motStdby, LOW);
            linDone = 1;
          }
          else if((getFiltAnalog(linPos) >= param.linBackLimit) && (um7.roll < param.linNoseDownTarget)) {
            analogWrite(motAPWM, 0);
            digitalWrite(motStdby, LOW);
            linDone = 1;
          }
          else {
            param.linRate = linMassRatePID(param.linNoseDownTarget);
            analogWrite(motAPWM, param.linRate);
          }
        }
        else {
          if((abs(getFiltAnalog(linPos) - param.linFrontLimit)) < 20) {
            analogWrite(motAPWM, 0);
            digitalWrite(motStdby, LOW);
            linDone = 1;
          }
        }
        
        if(pumpDone) {
          if(DoLinPID) {
            entry = 1;
            state = ME_GLIDE_DOWN;
            Serial.println("Nose is down.");
          }
          else {
            if(linDone) {
              entry = 1;
              state = ME_GLIDE_DOWN;
              Serial.println("Nose is down.");
            }
          }
        }
        
        if(millis() - t0 > param.allowedWorkTime) { //Taking too long
          state = ME_PAUSE;
          entry = 1;
          Serial.println("Something was taking too long!");
        }
        
          break;
        
      case ME_GLIDE_DOWN:
          if(entry) {
            pumpOff();
            digitalWrite(pwrOn,LOW);
            t0 = millis();
            entry = 0;
          }
         
          if(DoLinPID) {
            int ret = um7.refresh();
            if(um7.updated_p) {
              um7.updated_p = 0;
            }
            if((getFiltAnalog(linPos) <= param.linFrontLimit) && (um7.roll > param.linNoseDownTarget)) {
              analogWrite(motAPWM, 0);
              digitalWrite(motStdby, LOW);
              linDone = 1;
            }
            else if((getFiltAnalog(linPos) >= param.linBackLimit) && (um7.roll < param.linNoseDownTarget)) {
              analogWrite(motAPWM, 0);
              digitalWrite(motStdby, LOW);
              linDone = 1;
            }
            else {
              param.linRate = linMassRatePID(param.linNoseDownTarget);
              analogWrite(motAPWM, param.linRate);
            }
          }
              
          if(millis() - t0 > param.desTime) { //exit condition
            state = ME_NOSE_UP;
            entry = 1;
          }
          break;
        
      case ME_NOSE_UP:
        if(entry) {
          t0 = millis();
          I = 0;
          if(DoLinPID) {
            param.linRate = linMassRatePID(param.linNoseUpTarget);
            analogWrite(motAPWM, param.linRate);
          }
          else {
            moveLinMass(param.linBackLimit, param.linRate);//set linmass direction and turn on
          }
          moveWater(param.tankFrontLimit);//set pump direction and turn on
          entry = 0;
          pumpDone = 0;
          linDone = 0;
        }
        
        //turn off pump when it's time
        if(abs(getFiltAnalog(tankLevel)-param.tankFrontLimit) < 20) {
          digitalWrite(pumpOn, LOW);
          digitalWrite(pwrOn, LOW);
          pumpDone = 1;
        }
        
        if(DoLinPID) {
          int ret = um7.refresh();
          if(um7.updated_p) {
            um7.updated_p = 0;
          }
          if((getFiltAnalog(linPos) >= param.linBackLimit) && (um7.roll < param.linNoseUpTarget)) {
            analogWrite(motAPWM, 0);
            digitalWrite(motStdby, LOW);
            linDone = 1;
          }
          else if((getFiltAnalog(linPos) <= param.linFrontLimit) && (um7.roll > param.linNoseUpTarget)) {
            analogWrite(motAPWM, 0);
            digitalWrite(motStdby, LOW);
            linDone = 1;
          }
          else {
            param.linRate = linMassRatePID(param.linNoseUpTarget);
            analogWrite(motAPWM, param.linRate);
          }
        }
        else {
          if(abs(getFiltAnalog(linPos) - param.linBackLimit) < 20) {
            analogWrite(motAPWM, 0);
            digitalWrite(motStdby, LOW);
            linDone = 1;
          }
        }
        
        if(pumpDone) {
          if(DoLinPID) {
            entry = 1;
            state = ME_GLIDE_UP;
            Serial.println("Nose is up.");
          }
          else {
            if(linDone) {
              entry = 1;
              state = ME_GLIDE_UP;
              Serial.println("Nose is up.");
            }
          }
        }
        
        if(millis() - t0 > param.allowedWorkTime) { //Taking too long
          state = ME_PAUSE;
          entry = 1;
          Serial.println("Something was taking too long!");
        }

        break;
        
      case ME_GLIDE_UP:
        if(entry) {
          t0 = millis();
          pumpOff();
          digitalWrite(pwrOn,LOW);
          entry = 0;
        }
        
        if(DoLinPID) {
          int ret = um7.refresh();
          if(um7.updated_p) {
            um7.updated_p = 0;
          }
          if((getFiltAnalog(linPos) >= param.linBackLimit) && (um7.roll < param.linNoseUpTarget)) {
            analogWrite(motAPWM, 0);
            digitalWrite(motStdby, LOW);
            linDone = 1;
          }
          else if((getFiltAnalog(linPos) <= param.linFrontLimit) && (um7.roll > param.linNoseUpTarget)) {
            analogWrite(motAPWM, 0);
            digitalWrite(motStdby, LOW);
            linDone = 1;
          }
          else {
            param.linRate = linMassRatePID(param.linNoseUpTarget);
            analogWrite(motAPWM, param.linRate);
          }
        }
        
        if(millis() - t0 > param.riseTime) {
          state = ME_NOSE_DOWN;
          entry = 1;
        }
        break;
        
      case ME_PAUSE:
        digitalWrite(pumpOn, LOW);
        digitalWrite(pwrOn, LOW);
        digitalWrite(motAPWM, 0);
        digitalWrite(motStdby, LOW);
      break;
    }
  }
  
  if(cmd == GC_BEGIN) { // execute once at beginning of test
    pinMode(motAPWM, OUTPUT);
    pinMode(motAConf1, OUTPUT);
    pinMode(motAConf2, OUTPUT);
    pinMode(motStdby, OUTPUT);
    pinMode(motBPWM, OUTPUT);
    pinMode(motAConf1, OUTPUT);
    pinMode(motAConf2, OUTPUT);

    pinMode(pumpOn, OUTPUT);
    pinMode(pumpDir, OUTPUT);
    pinMode(suicide, OUTPUT);
    pinMode(pwrOn, OUTPUT);

    digitalWrite(suicide, LOW);
    delay(10);
    digitalWrite(pumpOn, LOW);
    delay(10);
    digitalWrite(pumpDir, LOW);
    delay(10);
    digitalWrite(motAConf1, LOW);
    delay(10);
    digitalWrite(motAConf2, LOW);
    delay(10);
    digitalWrite(motBConf1, LOW);
    delay(10);
    digitalWrite(motBConf2, LOW);
    delay(10);
    digitalWrite(motStdby, LOW);
    delay(10);
    analogWrite(motAPWM, 0);
    delay(10);
    analogWrite(motBPWM, 0);
    delay(10);

  }
  
  if(cmd == GC_RESET) { // GC_RESET to trimming position
    //moveRotMass(param.rotPos, param.rotRate);
    moveWater(param.tankMid);
    
    //turn off pump when it's time
      while(abs(getFiltAnalog(tankLevel)-param.tankMid) > 20) {
      }
      digitalWrite(pumpOn, LOW);
      digitalWrite(pwrOn, LOW);
      Serial.println("Tank Reset");

    moveLinMass(param.linMid, param.linRate);
    // Turn linear mass off when it's time
      while(abs(getFiltAnalog(linPos)-param.linMid) > 20) {
      }
      digitalWrite(motAPWM, 0);
      digitalWrite(motStdby, LOW);
      Serial.println("Linear Mass Reset");
    
    enGlider = 0;
  }
  
  if(cmd == GC_STOP) { // stop pump and stay
    Serial.println("STOP");
    enGlider = 0;
    pumpOff();
    digitalWrite(motAPWM, 0);
    digitalWrite(motStdby, LOW);
  }
  
  if(cmd == GC_START) { //begin gliding cycle
    enGlider = 1;
    state = ME_NOSE_DOWN;
    entry = 1;
  }
  
  if(cmd == GC_LINEAR) {
    if(DoLinPID) {
      DoLinPID = 0;
      Serial.println("Linear PID off");
      error_act = 0;
      error_prev = 0;
      I = 0;
      param.linRate = 200;
    }
    else {
      DoLinPID = 1;
      Serial.println("Linear PID on");
    }
  }
  
}

// Equivalent to Serial.readBytesUntil(), except it trashes the buffer if timeout
// Use for chipkit or for softSerial
int ReadBytesUntil(char term, char* buff, int len)
{
  unsigned long int t0, timeout = 100;
  int n = 0;

  t0 = millis();
  while(millis() - t0 < timeout)
  {
    if(Serial.available())
    {
      buff[n] = Serial.read();
      if(buff[n] == term)
        return n;
      n++;
    }
  }
  return 0;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Functions
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/*
 
 */
int getFiltAnalog(int APIN)
{
  int val = 0;
  for(int a=0; a<10; a++) {
    val = val + analogRead(APIN);
  }
  val = val/10;
  return val;
}
void moveLinMass(int dest, int rate) {
  int currentPos = getFiltAnalog(linPos);
  if(currentPos == 0) {
    Serial.println("Draw wire (linmass) reads zero!");
    Serial.println("...and also Eric sucks.");
    return;
  }
  Serial.print("Linear Mass Start: ");
  Serial.println(currentPos);
  if(dest < linfrontlimit) {
    Serial.println("Cannot go that far forward");
    Serial.print("Setting destination to ");
    Serial.println(linfrontlimit);
    param.linFrontLimit = linfrontlimit;
    dest = param.linFrontLimit;
  }
  if(dest > linbacklimit) {
    Serial.println("Cannot go that far backward");
    Serial.print("Setting destination to ");
    Serial.println(linbacklimit);
    param.linBackLimit = linbacklimit;
    dest = param.linBackLimit;
  }
  if(abs(currentPos - dest) < 5) {
    Serial.println("Already there");
    return; // Already there
  }

  if(currentPos > dest) {
    digitalWrite(motAConf1, LOW);
    digitalWrite(motAConf2, HIGH);
  }
  else {
    digitalWrite(motAConf1, HIGH);
    digitalWrite(motAConf2, LOW);
  }
  digitalWrite(motStdby, HIGH);
  analogWrite(motAPWM, rate);

  return;
}
void moveRotMass(int dest, int rate) {
  int currentPos = getFiltAnalog(rotPos);
  Serial.print("Rotational Mass Start: ");
  Serial.println(currentPos);
  if(dest <= rotlowlimit) {
    Serial.println("Too far");
    param.rotLowLimit = rotlowlimit;
    dest = param.rotLowLimit;
  }
  if(dest >= rothighlimit) {
    Serial.println("Too far");
    param.rotHighLimit = rothighlimit;
    dest = param.rotHighLimit;
  }
  if(abs(currentPos - dest) < 5) {
    Serial.println("Already there");
    return; // Already there
  }

  if(currentPos > dest) {
    digitalWrite(motBConf1, HIGH);
    digitalWrite(motBConf2, LOW);
  }
  else {
    digitalWrite(motBConf1, LOW);
    digitalWrite(motBConf2, HIGH);
  }
  digitalWrite(motStdby, HIGH);
  while(abs(getFiltAnalog(rotPos)-dest) > 5) {
    digitalWrite(motBPWM, rate);
  }
  digitalWrite(motBPWM, 0);
  digitalWrite(motStdby, LOW);
  Serial.println("Rotational Mass Done");
  return;
}
void moveWater(int dest) {
  digitalWrite(pwrOn, HIGH);
  delay(100);
  int currentPos = getFiltAnalog(tankLevel);
  if(currentPos == 0) {
    Serial.println("Draw wire (water tank) reads zero!");
    Serial.println("...and also Eric sucks.");
    return;
  }
  Serial.print("Water Tank Start: ");
  Serial.println(currentPos);
  if(dest < tankbacklimit) {
    Serial.println("Cannot go that far back");
    Serial.print("Setting destination to ");
    Serial.println(tankbacklimit);
    param.tankBackLimit = tankbacklimit;
    dest = param.tankBackLimit;
  }
  if(dest > tankfrontlimit) {
    Serial.println("Cannot go that far forward");
    Serial.print("Setting destination to ");
    Serial.println(tankfrontlimit);
    param.tankFrontLimit = tankfrontlimit;
    dest = param.tankFrontLimit;
  }
  if(abs(currentPos - dest) < 10) {
    Serial.println("Already there");
    return; // Already there
  }

  if(currentPos > dest) {
    digitalWrite(pumpDir, HIGH);
  }
  else {
    digitalWrite(pumpDir, LOW);
  }
  digitalWrite(pumpOn, HIGH);
  return;
}
void pumpOff() {
  digitalWrite(pumpOn, LOW);
}

void moveWater2(int dest) {
  digitalWrite(pwrOn, HIGH);
  delay(100);
  int currentPos = getFiltAnalog(tankLevel);
  if(currentPos == 0) {
    Serial.println("Draw wire (water tank) reads zero!");
    return;
  }
  Serial.print("Water Tank Start: ");
  Serial.println(currentPos);
  if(dest <= tankbacklimit) {
    Serial.println("Too far back");
    param.tankBackLimit = tankbacklimit;
    dest = param.tankBackLimit;
  }
  if(dest >= tankfrontlimit) {
    Serial.println("Too far forward");
    param.tankFrontLimit = tankfrontlimit;
    dest = param.tankFrontLimit;
  }
  if(abs(currentPos - dest) < 5) {
    Serial.println("Already there");
    return; // Already there
  }

  if(currentPos > dest) {
    digitalWrite(pumpDir, HIGH);
  }
  else {
    digitalWrite(pumpDir, LOW);
  }
   
   while(abs(getFiltAnalog(tankLevel)-dest) > 5) {
      digitalWrite(pumpOn, HIGH);
    }
  digitalWrite(pumpOn, LOW);  
  digitalWrite(pwrOn, LOW);
  return;
}

float linMassRatePID(int dest) {
  float kp, kd, ki;//kp = 10, ki = 0, kd = 0 currently
  kp=param.linkp;//Values set in parameters or by calling update -linkp [number]
  ki=param.linki;//
  kd=param.linkd;//
  float P, D, rate;
  int currentPos = getFiltAnalog(linPos);//This is giving some funky positions. Maybe need to filter a bit harder.
  int linpitchlimit = 45;
  if(dest < -linpitchlimit) {//check bounds
    Serial.println("Cannot go that far forward");
    Serial.print("Setting destination to ");
    Serial.println(-linpitchlimit);
    dest = -linpitchlimit;
  }
  if(dest > linpitchlimit) {//check bounds
    Serial.println("Cannot go that far backward");
    Serial.print("Setting destination to ");
    Serial.println(linpitchlimit);
    dest = linpitchlimit;
  }
  
  error_prev = error_act;
//  error_act = dest - currentPitch;
  error_act = dest - um7.roll;
  P = error_act;
  I = I + error_prev;
  D = um7.pitchd;
  rate = (P*kp + I*ki + D*kd)/param.rateScale;
//  rate = P*kp/param.rateScale;
  rate = abs(rate);//make it positive rate so things don't get too weird.
  rate = constrain(rate, 0, 200);
  if(rate < 15) {
    rate = 0;
  }
  if(um7.roll > dest) {//if it has to go forward, set pins to do that
    digitalWrite(motAConf1, LOW);
    digitalWrite(motAConf2, HIGH);
  }
  else {
    digitalWrite(motAConf1, HIGH);//backward reverse pins
    digitalWrite(motAConf2, LOW);
  }
  digitalWrite(motStdby, HIGH);//turn on
  return rate;
}
