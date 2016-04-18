// Include Hex File
#include <BOLIDE_Player.h>
#include <A1-16.h>
#include "BOLIDE.h"
#include "USER_MOTION.h"
#include <EEPROM.h>
#include <Wire.h>

// Declare The Parameters
BOLIDE_Player XYZrobot;
static int SN_packet[9] = {0}, SN_packet_index = 0, inByte = 0;
static int c, packet[7], temp_status_error, temp_status_detail, key, last_key, distance, LED_mode,ID ;
static unsigned char temp_system = 0x00; 
static boolean torque_release = false, last_key_update = false, blink_LED = true;
static int I2C_Address = 0x3B >> 1; 
static float ax, ay, az;

//========================= Set up =======================================
void setup()
{   
    Serial.begin(115200);
    Serial3.begin(115200);
    Buzzer_setup();
    LED_mode = 2;
    LED_task_on(); 
    Start_music(); 
    AIM_task_setup();
    RCU_task_setup();
    Initial_Pose_setup();
    Button_setup();
    Power_Detection_setup(); 
    delay(1000);
    LED_task_off();
    G_SENSOR_task_setup();
    WriteDataRAM2(A1_16_Broadcast_ID, RAM_Overload_PWM, 0x00FF);                           
    WriteDataRAM1(A1_16_Broadcast_ID, RAM_Overload_Dection_Period, 0x28);   
    Serial.println("Firmware Version:Bolide_Y-01_Default_v1.1");
    Serial.println("======================================================") ;
    Serial.println("Motor Status Checking:");   
}

//========================= Main =======================================
void loop()
{ 
//=== Button Command ===  
_0:  
  while( (c = Serial2.read() ) == -1 ) {

   Motor_Status_and_SN_task(); 
   Power_Detection_task();
   
   LED_mode = 2; 
   LED_task_on();
   
   BUTTON_task();
   if(last_key_update){
    if(last_key == key_mask_sw1) Action(RB_1);
    else if(last_key == key_mask_sw2) Action(RB_2);
    else if(last_key == key_mask_sw3) Action(RB_3);
    else if(last_key == key_mask_sw4) Action(RB_4);
    last_key_update = false;
   }  

   LED_task_off();
  }
  
//=== Read Bluetooth Packet === 
  if ( c == 0 ) goto _1 ;
  goto _0 ;
_1:  
  packet[0] = c ;
  while( (c = Serial2.read() ) == -1 ) ;
  if ( c == 0 ) goto _1 ; else packet[1] = c ;
  while( (c = Serial2.read() ) == -1 ) ;
  if ( c == 0 ) goto _1 ; else packet[2] = c ;
  while( (c = Serial2.read() ) == -1 ) ;
  if ( c == 0 ) goto _1 ; else packet[3] = c ;
  while( (c = Serial2.read() ) == -1 ) ;
  if ( c == 0 ) goto _1 ; else packet[4] = c ;
  while( (c = Serial2.read() ) == -1 ) ;
  if ( c == 0 ) goto _1 ; else packet[5] = c ;
  while( (c = Serial2.read() ) == -1 ) ;
  if ( c == 0 ) goto _1 ; else packet[6] = c ;
  Serial2.write( (c&0x00F0)>>4) ;
   
//==== RCU Command ==== 
  if(packet[1]!=255 && packet[2]!=1){
    
      LED_mode = 1;
      LED_task_on(); 
  
   if(packet[5] & 0x0010){        //Release Button
         A1_16_TorqueOff(254);
         cb() ;
       }
   else if(packet[5] & 0x0020){   //Bluetooth Button  
         cb() ;
       }      
   else if(packet[5] & 0x0040){   //Power Button 
         Action(2); 
         cb() ;
       }  
   else if(!(packet[5] & 0x0010)&&!(packet[5] & 0x0020)&&!(packet[5] & 0x0040)){    
       if((packet[1]>155&&packet[2]>155&&packet[1]>packet[2])|(packet[1]>155&&packet[2]<95&&(packet[1]-155)>(95-packet[2]))|(packet[1]>155&&packet[2]>=95&&packet[2]<=155)){ 
         //LeftJoystick_Rightside
         if (Falling_task()==5) Action(RCU_LJR);//WalkRightward  
         else Getup_task(Falling_task());
         cb() ;
       }
       else if((packet[1]<95&&packet[2]>155&&(95-packet[1])>(packet[2]-155))|(packet[1]<95&&packet[2]<155&&(95-packet[1])>(95-packet[2]))|(packet[1]<95&&packet[2]>=95&&packet[2]<=155)){
         //LeftJoystick_Leftside
         if (Falling_task()==5) Action(RCU_LJL);//WalkLeftward  
         else Getup_task(Falling_task());
         cb() ;
       }
       else if((packet[1]>155&&packet[2]>155&&packet[1]<packet[2])|(packet[1]<95&&packet[2]>155&&(95-packet[1])<(packet[2]-155))|(packet[2]>155&&packet[1]>=95&&packet[1]<=155)){
         //LeftJoystick_Upside
         if (Falling_task()==5){         
         if (IR_SENSOR_task() < 20){
          for(int i = 0; i<3; i++){
             tone(BUZZER_PIN,pgm_read_word_near(&obstacle_alarm_frq[i]));
             delay(250);
             noTone(BUZZER_PIN);}
        }   
        else{     
         Action(RCU_LJU);}} //WalkForward
         else Getup_task(Falling_task()); 
         cb() ;
       }
       else if((packet[1]>155&&packet[2]<95&&(packet[1]-155)<(95-packet[2]))|(packet[1]<95&&packet[2]<95&&(95-packet[1])<(155-packet[2]))|(packet[2]<95&&packet[1]>=95&&packet[1]<=155)){
         //LeftJoystick_Downside
         if (Falling_task()==5) Action(RCU_LJD);//WalkBackward 
         else Getup_task(Falling_task());
         cb();
       }
       else if((packet[3]>155&packet[4]>155&&packet[3]>packet[4])|(packet[3]>155&&packet[4]<95&&(packet[3]-155)>(95-packet[4]))|(packet[3]>155&&packet[4]>=95&&packet[4]<=155)){
         //RightJoystick_Rightside
         if (Falling_task()==5) Action(RCU_RJR);//TurnRight 
         else Getup_task(Falling_task());
         cb() ;
       }
       else if((packet[3]<95&&packet[4]>155&&(95-packet[3])>(packet[4]-155))|(packet[3]<95&&packet[4]<95&&(95-packet[3])>(95-packet[4]))|(packet[3]<95&&packet[4]>=95&&packet[4]<=155)){
         //RightJoystick_Leftside
         if (Falling_task()==5) Action(RCU_RJL); //TurnLeft 
         else Getup_task(Falling_task());
         cb() ;
       }
       else if((packet[3]>155&&packet[4]>155&&packet[3]<packet[4])|(packet[3]<95&&packet[4]>155&&(95-packet[3])<(packet[4]-155))|(packet[4]>155&&packet[3]>=95&&packet[3]<=155)){
         //RightJoystick_Upside        
         if (Falling_task()==5)  Action(RCU_RJU); 
         else Getup_task(Falling_task());
         cb() ; 
       }
       else if((packet[3]>155&&packet[4]<95&&(packet[3]-155)<(95-packet[4]))|(packet[3]<155&&packet[4]<95&&(95-packet[3])<(95-packet[4]))|(packet[4]<95&&packet[3]>=95&&packet[3]<=155)){
         //RightJoystick_Downside 
         if (Falling_task()==5)  Action(RCU_RJD); 
         else Getup_task(Falling_task());
         cb() ;
       }
       else if(packet[6] & 0x0001){
         //LeftButton_Upside-Front
         if (Falling_task()==5)  Action(RCU_L1); 
         else Getup_task(Falling_task());
         cb() ;
       }
       else if(packet[6] & 0x0002){
         //LeftButton_Upside-Back
         if (Falling_task()==5)  Action(RCU_L2); 
         else Getup_task(Falling_task());
         cb() ;
       }     
       else if(packet[6] & 0x0004){
         //LeftButton_Downside
         if (Falling_task()==5)  Action(RCU_L3); 
         else Getup_task(Falling_task());
         cb() ;
       }
       else if(packet[5] & 0x0001){
         //RightButton_Upside-Front
         if (Falling_task()==5)  Action(RCU_R1); 
         else Getup_task(Falling_task());
         cb() ;
       }
       else if(packet[5] & 0x0002){
         //RightButton_Upside-Back        
         if (Falling_task()==5)  Action(RCU_R2); 
         else Getup_task(Falling_task());
         cb() ;
       }     
       else if(packet[5] & 0x0004){
         //RightButton_Downside
         if (Falling_task()==5)  Action(RCU_R3); 
         else Getup_task(Falling_task());
         cb() ;
       }
   }
   LED_task_off();
 }

//==== App Command ==== 
 else if(packet[1]==255 && packet[2]==1){   
   LED_mode = 3;
   LED_task_on(); 
   
   if(packet[3] == 2){
    Action(2);
    cb(); 
   }
   else{    
    if (Falling_task()==5){
     if(packet[3] == 3){ //WalkForward 
       if (IR_SENSOR_task() < 20){
          for(int i = 0; i<3; i++){
             tone(BUZZER_PIN,pgm_read_word_near(&obstacle_alarm_frq[i]));
             delay(250);
             noTone(BUZZER_PIN);}
        }   
        else{     
         Action(packet[3]);} 
         cb() ;
       }
     else {  
     Action(packet[3]);}
    }
    else Getup_task(Falling_task());    
    cb();
    LED_task_off();
  }
 }
 cb();
}

//=========================== Function ================================
void AIM_task_setup(void){
  XYZrobot.setup(115200, 18);
}
//-----------------------------------------------------------
void RCU_task_setup(void){
  Serial2.begin(9600);
}
//-----------------------------------------------------------
void G_SENSOR_task_setup(void){
  Wire.begin();
  setReg(0x2D, 0xA);
}

void setReg(int reg, int data){
    Wire.beginTransmission(I2C_Address);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission();
}

int getData(int reg){
  static int Gsensor_timer;
  Gsensor_timer = 0;
  Wire.beginTransmission(I2C_Address);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(I2C_Address,1);
  if(Wire.available() <= 1 ) return Wire.read();
}

int Falling_task(void){
  int posture_index;
  ax = ((getData(0x33) << 8)  + getData(0x32)) / 256.0;
  ay = ((getData(0x35) << 8)  + getData(0x34)) / 256.0;
  az = ((getData(0x37) << 8)  + getData(0x36)) / 256.0;     

  if ((az) < -0.75) posture_index=1; //Frontside Getup
  else if ((az) > 0.75) posture_index=2; // Backside Getup    
  else if ((ax) < -0.75) posture_index=3; // Rightside Getup  
  else if ((ax) > 0.75) posture_index=4; // Leftside Getup
  else if ((az) <= 0.75 && (az) >= -0.75) posture_index=5;// Stand Status

  return posture_index;
}

void Getup_task(int posture_index){
 if (posture_index ==1) Action(23);
 else if(posture_index ==2) Action(24);
 else if(posture_index ==3) Action(96);
 else if(posture_index ==4) Action(96); 
}
//----------------------------------------------------------
int IR_SENSOR_task(void){
 distance = (6787/(analogRead(DISTANCE_SENSOR_PIN)-3))-4;
 return distance;
}
//-----------------------------------------------------------
void Buzzer_setup(void){
 pinMode(BUZZER_PIN, OUTPUT);
}

void Button_setup(void){
    pinMode(SW1_PIN, INPUT) ;
    pinMode(SW2_PIN, INPUT) ;
    pinMode(SW3_PIN, INPUT) ;
    pinMode(SW4_PIN, INPUT) ;
}

void BUTTON_task(void){
  static unsigned char button_timer;
  key = !digitalRead(SW1_PIN) + ((!digitalRead(SW2_PIN))<<1) + ((!digitalRead(SW3_PIN))<<2) + ((!digitalRead(SW4_PIN))<<3);
  if(key != last_key) button_timer++;
  else button_timer = 0;
  if(button_timer > 20){
    last_key = key;
    
    if(key != 0) last_key_update = true;
    button_timer = 0;
  }
}
//-----------------------------------------------------------
void Power_Detection_setup(void){
    pinMode(PWRDET_PIN, INPUT);
    analogReference(EXTERNAL); 
}

void Power_Detection_task(void){
  double PWR_Voltage = analogRead(PWRDET_PIN)/1024.0*5.108;
  PWR_Voltage = PWR_Voltage*16.49/6.49;
  if (PWR_Voltage < Power_Voltage_Alarm){
     tone(BUZZER_PIN,1000);
  }  
}
//-----------------------------------------------------------
void Initial_Pose_setup(void){
    XYZrobot.readPose() ;
    XYZrobot.playSeq(Initial);
    while(XYZrobot.playing)
    XYZrobot.play(); 
}
//-----------------------------------------------------------
//Motor Status Checking and SN Function 
void Motor_Status_and_SN_task(void){
    if (Serial.available()>0){  
     inByte = Serial.read();
      if (inByte == '1'){
       Serial.println("Single Motor Checking, Please Choose The Motor ID");
       while(Serial.available()<=0);   
       ID = Serial.parseInt();
       Serial.println("======================================================") ;
       Serial.print("Motor ID:");
       Serial.println(ID);   
       if(ID == 1|ID == 2|ID == 3|ID == 4|ID == 5|ID == 6|ID == 7|ID == 8|ID == 9|ID == 10|ID == 11|ID == 12|ID == 13|ID == 14|ID == 15|ID == 16|ID == 17|ID == 18){  
        temp_status_error = ReadDataRAM1(ID, RAM_Status_Error);
        temp_status_detail = ReadDataRAM1(ID, RAM_Status_Detail);
        if(temp_status_error & POT_ERROR_MASK) Serial.println("Motor is out of range !!");
        if(temp_status_error & VOLTAGE_ERROR_MASK) Serial.println("Supply Voltage is out of range !!");
        if(temp_status_error & TEMPERATURE_ERROR_MASK) Serial.println("Temperature is out of range !!");
        if(temp_status_error & OVERLOAD_ERROR_MASK) Serial.println("Motor is overload !!");
        if(temp_status_error & CHECKSUM_ERROR_MASK) Serial.println("Checksum Error !!");
        if(!(temp_status_error & (POT_ERROR_MASK|VOLTAGE_ERROR_MASK|TEMPERATURE_ERROR_MASK|OVERLOAD_ERROR_MASK|CHECKSUM_ERROR_MASK))){
         Serial.println("No Error") ; 
        } 
         Serial.println("======================================================") ;
        }
       }
      if (inByte == '2'){
       Serial.println("All Motor Checking");
       Serial.println("======================================================") ;
       for (int i=0;i<18;i++){
        ID = i+1; 
        temp_status_error = ReadDataRAM1(ID, RAM_Status_Error);
        temp_status_detail = ReadDataRAM1(ID, RAM_Status_Detail);
        Serial.print("Motor ID:");
        Serial.println(ID);         
        if(temp_status_error & POT_ERROR_MASK) Serial.println("Motor is out of range !!"); 
        if(temp_status_error & VOLTAGE_ERROR_MASK) Serial.println("Supply Voltage is out of range !!"); 
        if(temp_status_error & TEMPERATURE_ERROR_MASK) Serial.println("Temperature is out of range !!"); 
        if(temp_status_error & OVERLOAD_ERROR_MASK) Serial.println("Motor is overload !!"); 
        if(temp_status_error & CHECKSUM_ERROR_MASK) Serial.println("Checksum Error !!");
        if(!(temp_status_error & (POT_ERROR_MASK|VOLTAGE_ERROR_MASK|TEMPERATURE_ERROR_MASK|OVERLOAD_ERROR_MASK|CHECKSUM_ERROR_MASK))){
         Serial.println("No Error") ; 
        }
        Serial.println("------------------------------------------------------") ;
        } 
        Serial.println("======================================================") ;
       }     
      else {
       SN(inByte);
      }
     }
}

//SN Functon
void SN(int inByte){
  if (inByte == 255)
      SN_packet_index = 1;
    else if (inByte == 254)
    {
      if(SN_packet_index == 11)
      {
        //EE write
        for (int i = 0; i < SN_packet_index-1; i++)
          EEPROM.write(i, packet[i]);
      }
      else if (SN_packet_index == 15)
      {
        //EE write
        for (int i = 0; i < SN_packet_index-1; i++)
          EEPROM.write(i+10, packet[i]);        
      }
      else if (SN_packet_index == 2 && packet[0] == 111)
      {
        Serial.write(255);
        for (int i = 0; i < 10; i++)
        {
          Serial.write(EEPROM.read(i));
        }
        Serial.write(254);
      }
      else if (SN_packet_index == 2 && packet[0] == 112)
      {
        Serial.write(255);
        for (int j = 10; j < 24; j++)
        {
          Serial.write(EEPROM.read(j));
        }
        Serial.write(254);
      }
      SN_packet_index == 0;
    }
    else if (SN_packet_index > 0)
    {
      packet[SN_packet_index-1] = inByte;
      SN_packet_index++;
    }
}
//-----------------------------------------------------------
void Start_music(void){
  for(int i = 0 ; i<7; i++){
        tone(BUZZER_PIN,pgm_read_word_near(&start_music_frq[i]));
        delay(250);
        noTone(BUZZER_PIN);    
  }
}
//-----------------------------------------------------------
// LED Interrupt Function
void LED_task_on(){
  TCCR3A = 0x00;           
  TCCR3B |= _BV(CS32);      
  TCCR3B &= ~_BV(CS31);
  TCCR3B |= _BV(CS30);
  TIMSK3 |= _BV(TOIE3);
  TCNT3 = -15625;
  pinMode(LED_BLUE_PIN, OUTPUT) ;
  pinMode(LED_GREEN_PIN, OUTPUT) ;
  pinMode(LSA_LED_BLUE_PIN, OUTPUT);
  pinMode(LSA_LED_GREEN_PIN, OUTPUT);
  pinMode(LSA_LED_RED_PIN, OUTPUT);
  TCCR5A = _BV(COM5A1) | _BV(COM5B1) | _BV(COM5C1) | _BV(WGM51) | _BV(WGM50);  
  TCCR5B = _BV(WGM52) | _BV(CS52);  
  OCR5A = 0; OCR5B = 0; OCR5C = 0;
}

ISR(TIMER3_OVF_vect){
  static int R = 0, G = 0, B = 0;
  static int _R = 41, _G = 41, _B = 41;
  if (LED_mode == 1 ){
    if(blink_LED){
    digitalWrite(LED_BLUE_PIN, HIGH);
    digitalWrite(LED_GREEN_PIN, LOW);
    blink_LED = false;
    }
    else{
    digitalWrite(LED_BLUE_PIN, LOW);
    digitalWrite(LED_GREEN_PIN, HIGH);
    blink_LED = true;
    } 
    TCNT3 =-4500;
  }
  else if(LED_mode == 2 ){
  if(R < 40){R++; OCR5A = pgm_read_word_near(&log_light_40[R]);}
  else if(_R > 0){_R--; OCR5A = pgm_read_word_near(&log_light_40[_R]);}
  else if(G < 40){G++; OCR5B = pgm_read_word_near(&log_light_40[G]);digitalWrite(LED_BLUE_PIN, HIGH);digitalWrite(LED_GREEN_PIN, LOW);}
  else if(_G > 0){_G--; OCR5B = pgm_read_word_near(&log_light_40[_G]);}
  else if(B < 40){B++; OCR5C = pgm_read_word_near(&log_light_40[B]);digitalWrite(LED_BLUE_PIN, LOW);digitalWrite(LED_GREEN_PIN, HIGH);}
  else if(_B > 0){_B--; OCR5C = pgm_read_word_near(&log_light_40[_B]);}
  else{
    R = 0;G = 0;B = 0;
    _R = 41;_G = 41;_B = 41;
  }
  TCNT3 =-200;
  }
  else if (LED_mode == 3 ){
  if(R < 40){R++; OCR5A = pgm_read_word_near(&log_light_40[R]);}
  else if(_R > 0){_R--; OCR5A = pgm_read_word_near(&log_light_40[_R]);}
  else if(G < 40){G++; OCR5B = pgm_read_word_near(&log_light_40[G]);}
  else if(_G > 0){_G--; OCR5B = pgm_read_word_near(&log_light_40[_G]);}
  else if(B < 40){B++; OCR5C = pgm_read_word_near(&log_light_40[B]);}
  else if(_B > 0){_B--; OCR5C = pgm_read_word_near(&log_light_40[_B]);}
  else{
    R = 0;G = 0;B = 0;
    _R = 41;_G = 41;_B = 41;
  }
  TCNT3 =-200;
  }
}

void LED_task_off(){
  digitalWrite(LED_BLUE_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);  
  TIMSK3 &= ~_BV(TOIE3);
}
//-----------------------------------------------------------
// Music Playing Function
void MusicPlaying_wav_play(char song_name[]){
      Serial3.write(0);
      Serial3.print("P");
      Serial3.write(song_name); //set the filename of song : 0000 ~ 9999    
}
void MusicPlaying_wav_stop(){
      Serial3.write(0);
      Serial3.print("S0000");
}
void MusicPlaying_wav_volume(int volume){
      Serial3.write(0);
      Serial3.write('V');  
      Serial3.write(volume);// volume : 0x01 ~ 0x7F 
      Serial3.print("000");
}
//-----------------------------------------------------------
// Sample Motion Function
void Action(int N){
  if(N == 1) ;
  else if(N == 2) XYZrobot.playSeq(Initial);
  else if(N == 3) XYZrobot.playSeq(WalkForward);
  else if(N == 4) XYZrobot.playSeq(WalkBackward);
  else if(N == 5) XYZrobot.playSeq(WalkLeftward);
  else if(N == 6) XYZrobot.playSeq(WalkRightward);
  else if(N == 7) XYZrobot.playSeq(TurnLeft);
  else if(N == 8) XYZrobot.playSeq(TurnRight);
  else if(N == 9) XYZrobot.playSeq(WalkForwardTurnLeft);
  else if(N == 10) XYZrobot.playSeq(WalkForwardTurnRight);
  else if(N == 11) XYZrobot.playSeq(WalkBackwardTurnLeft);
  else if(N == 12) XYZrobot.playSeq(WalkBackwardTurnRight);
  else if(N == 13) XYZrobot.playSeq(FastWalkForward);
  else if(N == 14) XYZrobot.playSeq(FastWalkBackward);
  else if(N == 15) XYZrobot.playSeq(FastWalkLeftward);
  else if(N == 16) XYZrobot.playSeq(FastWalkRightward);
  else if(N == 17) XYZrobot.playSeq(FastTurnLeft);
  else if(N == 18) XYZrobot.playSeq(FastTurnRight);
  else if(N == 19) XYZrobot.playSeq(FastWalkForwardTurnLeft);
  else if(N == 20) XYZrobot.playSeq(FastWalkForwardTurnRight);
  else if(N == 21) XYZrobot.playSeq(FastWalkBackwardTurnLeft);
  else if(N == 22) XYZrobot.playSeq(FastWalkBackwardTurnRight);
  else if(N == 23) XYZrobot.playSeq(FrontGetUp);
  else if(N == 24) XYZrobot.playSeq(BackGetUp);
  else if(N == 25) XYZrobot.playSeq(PushUp);
  else if(N == 26) XYZrobot.playSeq(LeftHandPushUp);
  else if(N == 27) XYZrobot.playSeq(RightHandPushUp);
  else if(N == 28) XYZrobot.playSeq(Bow);
  else if(N == 29) XYZrobot.playSeq(WaveHand);
  else if(N == 30) XYZrobot.playSeq(SaluteRighthand);
  else if(N == 31) XYZrobot.playSeq(SaluteLefthand);
  else if(N == 32) {MusicPlaying_wav_play("0002"); XYZrobot.playSeq(TechDance);}  
  else if(N == 33) {MusicPlaying_wav_play("0001"); XYZrobot.playSeq(TaiwanDance);}
  else if(N == 34) {MusicPlaying_wav_play("0000"); XYZrobot.playSeq(KungFuDance);}
  else if(N == 35) {MusicPlaying_wav_play("0003"); XYZrobot.playSeq(ShowDance);}
  else if(N == 36) XYZrobot.playSeq(Dance1);
  else if(N == 37) XYZrobot.playSeq(Dance2);
  else if(N == 38) XYZrobot.playSeq(Dance3);
  else if(N == 39) XYZrobot.playSeq(Dance4);
  else if(N == 40) XYZrobot.playSeq(Dance5);
  else if(N == 41) XYZrobot.playSeq(Dance6);
  else if(N == 42) XYZrobot.playSeq(Dance7);
  else if(N == 43) XYZrobot.playSeq(Dance8);
  else if(N == 44) XYZrobot.playSeq(Dance9);
  else if(N == 45) XYZrobot.playSeq(Dance10);
  else if(N == 46) XYZrobot.playSeq(Dance11);
  else if(N == 47) XYZrobot.playSeq(Dance12);
  else if(N == 48) XYZrobot.playSeq(Dance13);
  else if(N == 49) XYZrobot.playSeq(Dance14);
  else if(N == 50) XYZrobot.playSeq(Dance15);
  else if(N == 51) XYZrobot.playSeq(Dance16);
  else if(N == 52) XYZrobot.playSeq(Dance17);
  else if(N == 53) XYZrobot.playSeq(Dance18);
  else if(N == 54) XYZrobot.playSeq(Dance19);
  else if(N == 55) XYZrobot.playSeq(Dance20);
  else if(N == 56) XYZrobot.playSeq(KungFu1);
  else if(N == 57) XYZrobot.playSeq(KungFu2);
  else if(N == 58) XYZrobot.playSeq(KungFu3);
  else if(N == 59) XYZrobot.playSeq(KungFu4);
  else if(N == 60) XYZrobot.playSeq(KungFu5);
  else if(N == 61) XYZrobot.playSeq(KungFu6);
  else if(N == 62) XYZrobot.playSeq(KungFu7);
  else if(N == 63) XYZrobot.playSeq(KungFu8);
  else if(N == 64) XYZrobot.playSeq(KungFu9);
  else if(N == 65) XYZrobot.playSeq(KungFu10);
  else if(N == 66) XYZrobot.playSeq(KungFu11);
  else if(N == 67) XYZrobot.playSeq(KungFu12);
  else if(N == 68) XYZrobot.playSeq(KungFu13);
  else if(N == 69) XYZrobot.playSeq(KungFu14);
  else if(N == 70) XYZrobot.playSeq(KungFu15);
  else if(N == 71) XYZrobot.playSeq(KungFu16);
  else if(N == 72) XYZrobot.playSeq(KungFu17);
  else if(N == 73) XYZrobot.playSeq(KungFu18);
  else if(N == 74) XYZrobot.playSeq(KungFu19);
  else if(N == 75) XYZrobot.playSeq(KungFu20);
  else if(N == 76) XYZrobot.playSeq(Fight1);
  else if(N == 77) XYZrobot.playSeq(Fight2);
  else if(N == 78) XYZrobot.playSeq(Fight3);
  else if(N == 79) XYZrobot.playSeq(Fight4);
  else if(N == 80) XYZrobot.playSeq(Fight5);
  else if(N == 81) XYZrobot.playSeq(Fight6);
  else if(N == 82) XYZrobot.playSeq(Fight7);
  else if(N == 83) XYZrobot.playSeq(Fight8);
  else if(N == 84) XYZrobot.playSeq(Fight9);
  else if(N == 85) XYZrobot.playSeq(Fight10);
  else if(N == 86) XYZrobot.playSeq(Fight11);
  else if(N == 87) XYZrobot.playSeq(Fight12);
  else if(N == 88) XYZrobot.playSeq(Fight13);
  else if(N == 89) XYZrobot.playSeq(Fight14);
  else if(N == 90) XYZrobot.playSeq(Fight15);
  else if(N == 91) XYZrobot.playSeq(Fight16);
  else if(N == 92) XYZrobot.playSeq(Fight17);
  else if(N == 93) XYZrobot.playSeq(Fight18);
  else if(N == 94) XYZrobot.playSeq(Fight19);
  else if(N == 95) XYZrobot.playSeq(Fight20);
  else if(N == 96) XYZrobot.playSeq(SideGetUp);
  else if(N == 97) XYZrobot.playSeq(Dance21);
  else if(N == 98) XYZrobot.playSeq(Dance22);
  else if(N == 99) XYZrobot.playSeq(Dance23);
  else if(N == 100) XYZrobot.playSeq(Dance24);
  else if(N == 101) XYZrobot.playSeq(Dance25);
  else if(N == 102) XYZrobot.playSeq(Dance26);
  else if(N == 103) XYZrobot.playSeq(Dance27);
  else if(N == 104) XYZrobot.playSeq(Dance28);
  else if(N == 105) XYZrobot.playSeq(Dance29);
  else if(N == 106) XYZrobot.playSeq(Dance30);
  else if(N == 107) XYZrobot.playSeq(KungFu21);
  else if(N == 108) XYZrobot.playSeq(KungFu22);
  else if(N == 109) XYZrobot.playSeq(KungFu23);
  else if(N == 110) XYZrobot.playSeq(KungFu24);
  else if(N == 111) XYZrobot.playSeq(Fight21);
  else if(N == 112) XYZrobot.playSeq(Fight22);
  else if(N == 113) XYZrobot.playSeq(Fight23);
  else if(N == 114) XYZrobot.playSeq(Fight24);
  else if(N == 115) XYZrobot.playSeq(New115);
  else if(N == 116) XYZrobot.playSeq(New116);
  else if(N == 117) XYZrobot.playSeq(New117); 
  else if(N == 118) XYZrobot.playSeq(New118);
  else if(N == 119) XYZrobot.playSeq(New119);
  else if(N == 120) XYZrobot.playSeq(New120);
  else if(N == 121) XYZrobot.playSeq(New121);
  else if(N == 122) XYZrobot.playSeq(New122); 
  else if(N == 123) XYZrobot.playSeq(New123);
  else if(N == 124) XYZrobot.playSeq(New124);
  else if(N == 125) XYZrobot.playSeq(New125);
  else if(N == 126) XYZrobot.playSeq(New126);
  else if(N == 127) XYZrobot.playSeq(New127); 
  else if(N == 128) XYZrobot.playSeq(New128);
  else if(N == 129) XYZrobot.playSeq(New129);
  else if(N == 130) XYZrobot.playSeq(New130);
  else if(N == 131) XYZrobot.playSeq(New131);
  else if(N == 132) XYZrobot.playSeq(New132); 
  else if(N == 133) XYZrobot.playSeq(New133);
  else if(N == 134) XYZrobot.playSeq(New134);
  else if(N == 135) XYZrobot.playSeq(New135);
  else if(N == 136) XYZrobot.playSeq(New136);
  else if(N == 137) XYZrobot.playSeq(New137); 
  else if(N == 138) XYZrobot.playSeq(New138);
  else if(N == 139) XYZrobot.playSeq(New139);
  else if(N == 140) XYZrobot.playSeq(New140);
  else if(N == 141) XYZrobot.playSeq(New141);
  else if(N == 142) XYZrobot.playSeq(New142); 
  else if(N == 143) XYZrobot.playSeq(New143);
  else if(N == 144) XYZrobot.playSeq(New144);
  else if(N == 145) XYZrobot.playSeq(New145);
  else if(N == 146) XYZrobot.playSeq(New146);
  else if(N == 147) XYZrobot.playSeq(New147); 
  else if(N == 148) XYZrobot.playSeq(New148);
  else if(N == 149) XYZrobot.playSeq(New149);
  else if(N == 150) XYZrobot.playSeq(New150);
  else if(N == 151) XYZrobot.playSeq(New151);
  else if(N == 152) XYZrobot.playSeq(New152); 
  else if(N == 153) XYZrobot.playSeq(New153);
  else if(N == 154) XYZrobot.playSeq(New154);
  else if(N == 155) XYZrobot.playSeq(New155);
  else if(N == 156) XYZrobot.playSeq(New156);
  else if(N == 157) XYZrobot.playSeq(New157); 
  else if(N == 158) XYZrobot.playSeq(New158);
  else if(N == 159) XYZrobot.playSeq(New159);
  else if(N == 160) XYZrobot.playSeq(New160);
  else if(N == 161) XYZrobot.playSeq(New161);
  else if(N == 162) XYZrobot.playSeq(New162); 
  else if(N == 163) XYZrobot.playSeq(New163);
  else if(N == 164) XYZrobot.playSeq(New164);
  else if(N == 165) XYZrobot.playSeq(New165);
  else if(N == 166) XYZrobot.playSeq(New166);
  else if(N == 167) XYZrobot.playSeq(New167); 
  else if(N == 168) XYZrobot.playSeq(New168);
  else if(N == 169) XYZrobot.playSeq(New169);
  else if(N == 170) XYZrobot.playSeq(New170);
  else if(N == 171) XYZrobot.playSeq(New171);
  else if(N == 172) XYZrobot.playSeq(New172); 
  else if(N == 173) XYZrobot.playSeq(New173);
  else if(N == 174) XYZrobot.playSeq(New174);
  else if(N == 175) XYZrobot.playSeq(New175);
  else if(N == 176) XYZrobot.playSeq(New176);
  else if(N == 177) XYZrobot.playSeq(New177); 
  else if(N == 178) XYZrobot.playSeq(New178);
  else if(N == 179) XYZrobot.playSeq(New179);
  else if(N == 180) XYZrobot.playSeq(New180);
  else if(N == 181) XYZrobot.playSeq(New181);
  else if(N == 182) XYZrobot.playSeq(New182); 
  else if(N == 183) XYZrobot.playSeq(New183);
  else if(N == 184) XYZrobot.playSeq(New184);
  else if(N == 185) XYZrobot.playSeq(New185);
  else if(N == 186) XYZrobot.playSeq(New186);
  else if(N == 187) XYZrobot.playSeq(New187); 
  else if(N == 188) XYZrobot.playSeq(New188);
  else if(N == 189) XYZrobot.playSeq(New189);
  else if(N == 190) XYZrobot.playSeq(New190);
  else if(N == 191) XYZrobot.playSeq(New191);
  else if(N == 192) XYZrobot.playSeq(New192); 
  else if(N == 193) XYZrobot.playSeq(New193);
  else if(N == 194) XYZrobot.playSeq(New194);
  else if(N == 195) XYZrobot.playSeq(New195);
  else if(N == 196) XYZrobot.playSeq(New196);
  else if(N == 197) XYZrobot.playSeq(New197); 
  else if(N == 198) XYZrobot.playSeq(New198);
  else if(N == 199) XYZrobot.playSeq(New199);
  else if(N == 200) XYZrobot.playSeq(New200);
  else if(N == 201) XYZrobot.playSeq(New201);
  else if(N == 202) XYZrobot.playSeq(New202); 
  else if(N == 203) XYZrobot.playSeq(New203);
  else if(N == 204) XYZrobot.playSeq(New204);
  else if(N == 205) XYZrobot.playSeq(New205);
  else if(N == 206) XYZrobot.playSeq(New206);
  else if(N == 207) XYZrobot.playSeq(New207); 
  else if(N == 208) XYZrobot.playSeq(New208);
  else if(N == 209) XYZrobot.playSeq(New209);
  else if(N == 210) XYZrobot.playSeq(New210);
  else if(N == 211) XYZrobot.playSeq(New211);
  else if(N == 212) XYZrobot.playSeq(New212); 
  else if(N == 213) XYZrobot.playSeq(New213);
  else if(N == 214) XYZrobot.playSeq(New214);
  else if(N == 215) XYZrobot.playSeq(New215);
  else if(N == 216) XYZrobot.playSeq(New216);
  else if(N == 217) XYZrobot.playSeq(New217); 
  else if(N == 218) XYZrobot.playSeq(New218);
  else if(N == 219) XYZrobot.playSeq(New219);
  else if(N == 220) XYZrobot.playSeq(New220);
  else if(N == 221) XYZrobot.playSeq(New221);
  else if(N == 222) XYZrobot.playSeq(New222); 
  else if(N == 223) XYZrobot.playSeq(New223);
  else if(N == 224) XYZrobot.playSeq(New224);
  else if(N == 225) XYZrobot.playSeq(New225);
  else if(N == 226) XYZrobot.playSeq(New226);
  else if(N == 227) XYZrobot.playSeq(New227); 
  else if(N == 228) XYZrobot.playSeq(New228);
  else if(N == 229) XYZrobot.playSeq(New229);
  else if(N == 230) XYZrobot.playSeq(New230);
  else if(N == 231) XYZrobot.playSeq(New231);
  else if(N == 232) XYZrobot.playSeq(New232); 
  else if(N == 233) XYZrobot.playSeq(New233);
  else if(N == 234) XYZrobot.playSeq(New234);
  else if(N == 235) XYZrobot.playSeq(New235);
  else if(N == 236) XYZrobot.playSeq(New236);
  else if(N == 237) XYZrobot.playSeq(New237); 
  else if(N == 238) XYZrobot.playSeq(New238);
  else if(N == 239) XYZrobot.playSeq(New239);
  else if(N == 240) XYZrobot.playSeq(New240);
  else if(N == 241) XYZrobot.playSeq(New241);
  else if(N == 242) XYZrobot.playSeq(New242); 
  else if(N == 243) XYZrobot.playSeq(New243);
  else if(N == 244) XYZrobot.playSeq(New244);
  else if(N == 245) XYZrobot.playSeq(New245);
  else if(N == 246) XYZrobot.playSeq(New246);
  else if(N == 247) XYZrobot.playSeq(New247); 
  else if(N == 248) XYZrobot.playSeq(New248);
  else if(N == 249) XYZrobot.playSeq(New249);
  else if(N == 250) XYZrobot.playSeq(New250);

  while((XYZrobot.playing) && !(Torque_Release_task())){
    XYZrobot.play();
  }
  if(torque_release) {
    A1_16_TorqueOff(A1_16_Broadcast_ID);
    MusicPlaying_wav_stop();
  }
}

boolean Torque_Release_task(void){
  static int temp_packet[7] = {0};
  static char _i = 0;
  if(Serial2.available() >= 7){
    if((temp_packet[0] = Serial2.read()) == 0) ; else {clear_buffer(); return false;}
    if((temp_packet[1] = Serial2.read()) == 0) {clear_buffer(); return false;}
    if((temp_packet[2] = Serial2.read()) == 0) {clear_buffer(); return false;}
    if((temp_packet[3] = Serial2.read()) == 0) {clear_buffer(); return false;}
    if((temp_packet[4] = Serial2.read()) == 0) {clear_buffer(); return false;}
    if((temp_packet[5] = Serial2.read()) == 0) {clear_buffer(); return false;}
    if((temp_packet[6] = Serial2.read()) == 0) {clear_buffer(); return false;}
    Serial2.write((temp_packet[6]&0x00F0)>>4);
    for(_i = 0;_i < 7 ;_i++) packet[_i] = temp_packet[_i];
    if(((temp_packet[5]&0x0010)>>3)) {
      torque_release = true;
      return true;
    }
    else{
      torque_release = false;
      return false;
    }
    cb();
  }
  return false;
}
//-----------------------------------------------------------
// CleanAllBuffer Function
void cb( void) {
  while( ( Serial2.read() ) != -1 ) ;
}

void clear_buffer(void){
  static int cb = 0x00;
  while(Serial2.available() > 0){
    if(Serial2.peek() == 0) return;
    cb = Serial2.read();
  }
}


