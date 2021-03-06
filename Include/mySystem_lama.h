/*********************************************************
Project : ADROIT AVR Rev.3
Version : 1
Date    : 3/13/2014
Author  : Eko Henfri Binugroho  
Company : ER2C

Code    : - System initialisation Routine 
          - Interrupt Service Routines 
*********************************************************/

#ifndef _mySystem_
#define _mySystem_

#include <mega128.h>
#include <delay.h>
#include "myGlobalVars.h"
#include "myMotor.h"
#include "myLCD.h"
#include "myI2C.h"
#include "myBuzzer.h"
#include "myLineSensor.h"
#if UseIMU==1
  #include "myIMU6050.h"
  #if UseCompass==1
    #include "myHMC5883L.h"
  #endif
#endif


register signed int     Enkoder1=0, Enkoder2=0;     // data rotary enkoder 1 dan 2 (up/down 16 bit --> untuk perhitungan posisi)
register unsigned char  SysTick = 0;                // data system tick setiam ~1ms
volatile unsigned char  dCounter1=0, dCounter2=0;   // data counter dari enkoder (up 8 bit --> untuk perhitungan kecepatan)
volatile unsigned char  WaktuEksekusi;              // variabel untuk menghitung waktu eksekusi dalam mili detik

#if UseIMU == 1
    // Variabel yang digunakan
    volatile short int  Ax, Ay, Az, Gx, Gy, Gz;         // data mentah dari gyroscope dan accelerometer    
    volatile float      GxOffset, GyOffset, GzOffset;   // offset dari gyroscope
    volatile float      Roll=0, Pitch=0, Yaw=0;         // data sudut putaran dalam sumbu x,y dan z dalam derajat
#endif

// Rotari Enkoder Motor 1
interrupt [EXT_INT6] void ext_int6_isr(void)
{ 
  if(P_Enkoder1A)
  {  if(P_Enkoder1B) {#asm("cli")  Enkoder1++; #asm("sei")}
     else            {#asm("cli")  Enkoder1--; #asm("sei")}
  }
  else
  {  if(P_Enkoder1B) {#asm("cli")  Enkoder1--; #asm("sei")}
     else            {#asm("cli")  Enkoder1++; #asm("sei")}
  }
  dCounter1++;        
}

// Rotari Enkoder Motor 2
interrupt [EXT_INT7] void ext_int7_isr(void)
{ if(P_Enkoder2A)
  {  if(P_Enkoder2B) {#asm("cli")  Enkoder2--; #asm("sei")}
     else            {#asm("cli")  Enkoder2++; #asm("sei")}
  }
  else
  {  if(P_Enkoder2B) {#asm("cli")  Enkoder2++; #asm("sei")}
     else            {#asm("cli")  Enkoder2--; #asm("sei")}
  }
  dCounter2++;        
}


// Interupsi dengan frekwensi 40KHz -- Timer 3 output compare C interrupt service routine
interrupt [TIM3_COMPC] void timer3_compc_isr(void)
{  static unsigned int ServoCounter=0;
   if (++ServoCounter<=80)      // data servo maksimalnya adalah 80 dari 800  (2ms dari 20ms)
   {    
        #if UseServo1 == 1     
            if ((unsigned char)ServoCounter==dServo1) {P_Servo1=0;}
        #endif
        #if UseServo2 == 1     
            if ((unsigned char)ServoCounter==dServo2) {P_Servo2=0;}
        #endif
        #if UseServo3 == 1     
            if ((unsigned char)ServoCounter==dServo3) {P_Servo3=0;}
        #endif
        #if UseServo4 == 1     
            if ((unsigned char)ServoCounter==dServo4) {P_Servo4=0;}
        #endif
        #if UseServo5 == 1     
            if ((unsigned char)ServoCounter==dServo5) {P_Servo5=0;}
        #endif
   }
   else if (ServoCounter>800)
   {    ServoCounter = 0;
        //P_Servo1_4T = P_Servo1_4B | 0xF0; 
        #if UseServo1 == 1     
            P_Servo1 = 1;    
        #endif
        #if UseServo2 == 1     
            P_Servo2 = 1;    
        #endif
        #if UseServo3 == 1     
            P_Servo3 = 1;    
        #endif
        #if UseServo4 == 1     
            P_Servo4 = 1;    
        #endif
        #if UseServo5 == 1     
            P_Servo5 = 1;    
        #endif
   }
}

#if ModeBalancing == 1

#define     CenterOffset    11          // sudut pitch dari robot pada kondisi setimbang
#define     KpB             6.0F        // konstanta Kp dari kendali balancing robot
#define     KdB             0.02F       // konstanta Kd dari kendali balancing robot
#define     KiB             4.0F        // konstanta Ki dari kendali balancing robot
#define     sMax            3           // batas maksimal (+/-) dari integral error kecepatan robot 

signed char TargetSpeedB;               // Setpoint dari kecepatan robot    (0-10)
int SteerB;                             // Steering dari robot              (0-200)

void BalancingControl(void)
{   float CenterB, Error, dError, U, ErrorSpeed;
    static float iErrorSpeed=0, LastError=0, iError=0;
    static int Rotary1=0, Rotary2=0;                   
    
    ErrorSpeed = TargetSpeedB-(float)((Enkoder1 - Rotary1)+(Enkoder1 - Rotary2))*0.5;
    Rotary1 = Enkoder1;
    Rotary2 = Enkoder2;
    iErrorSpeed+=ErrorSpeed*0.1;
    if((int)iErrorSpeed>sMax)          iErrorSpeed = sMax;
    else if((int)iErrorSpeed<-sMax)    iErrorSpeed = -sMax;
    CenterB = CenterOffset + ErrorSpeed*0.2 +  iErrorSpeed;
         
    if(((char)Pitch<45) && ((char)Pitch >-45))
        {   Error  = Pitch - CenterB;
            iError += (Error*0.3F);
            LastError = Error;
            if      (iError>80)     iError = 80;
            else if (iError<-80)    iError = -80; 
            dError = ((float)Gy - GyOffset);
            U = (Error + LastError) * KpB + dError * KdB + KiB * iError;
            if      (U>400)     U=400;
            else if (U<-400)    U=-400;  
            //if(U<20 && U>-20)   U*=1.5F;
            
            SetDataMotorPWM((int)U-SteerB,(int)U+SteerB);
            //LCD_GotoXY(0,0);    LCD_Angka3(mSpeed1); LCD_Data(' ');
            //LCD_GotoXY(6,0);    LCD_Angka3(ErrorSpeed); LCD_Data(' ');
            //LCD_GotoXY(12,0);   LCD_Angka3(CenterB);
        }
    else
        {   SetDataMotorPWM(0,0);
            iError = 0;
        }

}
#endif

// Timer1 input capture interrupt service routine
interrupt [TIM1_CAPT] void timer1_capt_isr(void)
{
// Place your code here
// karena frekwensi PWM untuk motor Servo adalah 50Hz (20ms), maka pulsa diaktifkan hanya 1 dalam 4 kali interrupt 
static uint8_t ServoCounter=0;
#asm("sei");                                        // mengaktifkan interrupt dari event yang lain
if(++ServoCounter>4)
{ ServoCounter = 0;
  //TCCR1A=0xFC;      // mode phase correct PWM dengan output inverted
  TCCR1A=0xAA;      // mode phase correct PWM dengan output non inverted
}
else 
{ TCCR1A=0x02;      // mode phase correct PWM dengan output disabled
  P_Servo6 = 0;     // output dibuat 0 ketika output PWM tidak diaktifkan 
  P_Servo7 = 0;     // output dibuat 0 ketika output PWM tidak diaktifkan
  P_Servo8 = 0;     // output dibuat 0 ketika output PWM tidak diaktifkan
}

}


// Interrupt setiap 5ms
interrupt [TIM1_OVF] void timer1_ovf_isr(void)
{
// Place your code here

// Kalkulasi IMU
#if UseIMU == 1
    unsigned char i;
    #asm("sei");                                        // mengaktifkan interrupt dari event yang lain
    if (ImuStart)                                       // program ini dijalankan hanya ketika IMU diaktifkan
    {   i=SysTick;                                      // variabel untuk menghitung tick (1 tick = 1 ms)
        BacaAccelerometer();                            // membaca data accelerometer 3 axis
        BacaGyroScope();                                // membaca data gyroscope 3 axis
        #if ModeBalancing == 1                          // jika mode balancing diaktifkan
          #if ImuFilter == KalmanFilter                 // jika menggunakan kalman filter
            HitungKalmanPitch();                        // menghitung sudut pitch dari robot menggunakan kalman filter
          #elif ImuFilter == ComplementaryFilter        // jika mengggunakan complementary filter
            HitungComplementaryPitch();                 // menghitung sudut pitch dari robot menggunakan complementary filter
          #endif
          HitungYaw();                                  // menghitung sudut yaw 
          BalancingControl();                           // perhitungan kendali balancing robot
        #else                                           // jika mode balancing diaktifkan 
          #if ImuFilter == KalmanFilter
            HitungKalmanRoll();                         // menghitung sudut roll menggunakan kalman filter
            HitungKalmanPitch();                        // menghitung sudut pitch menggunakan kalman filter
            HitungYaw();                                // menghitung sudut yaw
          #elif ImuFilter == ComplementaryFilter
            HitungComplementaryRoll();                  // menghitung sudut roll menggunakan complementary filter   
            HitungComplementaryPitch();                 // menghitung sudut pitch menggunakan complementary filter
            HitungYaw();                                // menghitung sudut yaw
          #elif ImuFilter == MahonyFilter
            HitungMahonyFilter();                       // menghitung sudut roll, pitch dan yaw dengan mahony filter
          #endif
        #endif                                          // menghitung jumlah tick dari total waktu eksekusi
    //    if(SysTick>i)    WaktuEksekusi = SysTick - i;                
    //    else             WaktuEksekusi = 0xFF - i + SysTick + 1;
    }   
#endif
}

// Interupsi setiap 1 ms
interrupt [TIM2_COMP] void timer2_comp_isr(void)
{   static unsigned char LaguTick=0, _dCounter1=0, _dCounter2=0;
    static unsigned char TempoTick=0, Timer2Tick=0;
    SysTick++;
    #asm("sei");                                        // mengaktifkan interrupt dari event yang lain
    if(++Timer2Tick==4)           // menghitung data rotari motor 1   
    {   if(dCounter1>_dCounter1)    dSpeed1 = dCounter1 - _dCounter1;               // hitung selisih saat tidak overflow 
        else                        dSpeed1 = 0xFF - _dCounter1 + dCounter1 + 1;    // hitung selisih saat overflow
        _dCounter1 = dCounter1;
    }
    else if (Timer2Tick==5)       
    {   
    #if UsePIDmotor == 1
        if(PIDMotorOn) PIDmotor1(); // menghitung PID motor 1 jika mode PID motor diaktifkan
    #endif
    }
    else if(Timer2Tick==9)          // menghitung data rotari motor 2
    {   if(dCounter2>_dCounter2)    dSpeed2 = dCounter2 - _dCounter2;               // hitung selisih saat tidak overflow
        else                        dSpeed2 = 0xFF - _dCounter2 + dCounter2 + 1;    // hitung selisih saat overflow
        _dCounter2 = dCounter2;
    }
    else if (Timer2Tick>=10)
    {   
    #if UsePIDmotor == 1
        if(PIDMotorOn) PIDmotor2(); // menghitung PID motor 2 jika mode PID motor diaktifkan
    #endif
        Timer2Tick=0;
    }  
    
    if(LaguOn)                      // menyalakan lagu pada buzzer
    {   if(++TempoTick>=Tempo[LaguTick])            // Apakah hitungan tempo terpenuhi 
        {   TempoTick = 0;                          // reset hitungan tempo        
            FBuzzer(Melodi[LaguTick]);              // mengeluarkan nada ke buzzer
            if (++LaguTick>=JumlahNada) LaguTick=0; // reset kembali deretan nada ketika lagu selesai
        }        
    }
}

#pragma used+
void SystemInit(void)
{   
 //-------------- Inisialisasi Port -----------------------------------
    // Port A  = Koneksi ke LCD --> semua bit difungsikan sebagai output 
    PORTA=0x00; DDRA=0xFF;
    
    // Port B0   = Input Pull-Up (Enkoder1B)
    // Port B1   = Input (SCK - ISP programing)
    // Port B2   = Input Pull-Up (Enkoder2B)
    // Port B3   = Output (Servo 5)
    // Port B4   = Output (Buzzer)
    // Port B5-7 = Output (Servo 6-8)
    PORTB=0x05; DDRB=0xF8;

    // Port C0-3 = Output (LED Active High)
    // Port C4-7 = Input Pull-Up (Push Button Active Low)
    PORTC=0xFF; DDRC=0xF0;

    // Port D0,1 = Input (I2C)
    // Port D2,3 = Input (USART1)
    // Port D2,3 = Input (USART1)
    // Port D4-7 = Output(Servo1-4)
    PORTD=0x00; DDRD=0xF0;

    // Port E0,1 = Input (USART0, ISP programming)
    // Port E2   = Output (DirM2)
    // Port E3   = Output (PwmM2)
    // Port E4   = Output (PwmM1)
    // Port E5   = Output (DirM2)
    // Port E6,7 = Input Pull-Up (Enkoder1A,2A)
    PORTE=0xC0; DDRE=0x3C;

    // Port F  = Input (Input ADC)
    PORTF=0x00; DDRF=0x00;

    // Port G  = Input Pull-Up (Saklar Mode Active Low)
    PORTG=0x17;
    DDRG=0x00;
 
 //-------------- Inisialisasi Timer -----------------------------------
    // Timer/Counter 0 initialization
    // Clock source: System Clock
    // Clock value: 250,000 kHz
    // Mode: CTC top=OCR0
    // OC0 output: Toggle on compare match
    ASSR=0x00;
    TCCR0=0x00; // sementara dimatikan (diaktifkan ketika fungsi FBuzzer(Frek) dipanggil)
    TCNT0=0x00;
    OCR0=0x00;

    
    // Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: 2000,000 kHz
// Mode: Ph. correct PWM top=ICR1
// OC1A output: Non-Inv.
// OC1B output: Non-Inv.
// OC1C output: Non-Inv.
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer1 Overflow Interrupt: On
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
// Compare C Match Interrupt: Off
TCCR1A=0xAA;
TCCR1B=0x12;
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x27; ICR1L=0x10; // interrupt setiap 10ms
ICR1H=0x13; ICR1L=0x88; // interrupt setiap 5ms
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;
OCR1CH=0x00;
OCR1CL=0x00;

/*
// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: 2000,000 kHz
// Mode: Ph. & fr. cor. PWM top=ICR1
// OC1A output: Inverted
// OC1B output: Inverted
// OC1C output: Inverted
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer1 Overflow Interrupt: On
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
// Compare C Match Interrupt: Off
TCCR1A=0xFC;
TCCR1B=0x12;
TCNT1H=0x00;
TCNT1L=0x00;
ICR1H=0x13;     ICR1L=0x88; // interrupt setiap 5ms
OCR1AH=0x13;    OCR1AL=0x88;
OCR1BH=0x13;    OCR1BL=0x88;
OCR1CH=0x13;    OCR1CL=0x88;
*/


    // Timer/Counter 2 initialization
    // Clock source: System Clock
    // Clock value: 62,500 kHz
    // Mode: CTC top=OCR2
    // OC2 output: Disconnected
    //TCCR2=0x0C;       // 64,500 kHz        -- interupsi 4 ms
    TCCR2=0x0B;         // 250,000 kHz       -- interupsi 1 ms
    TCNT2=0x00;
    OCR2=0xFA;

    // Timer/Counter 3 initialization
    // Clock source: System Clock
    // Clock value: 16000,000 kHz
    // Mode: Ph. correct PWM top=ICR3
    // Noise Canceler: Off
    // Input Capture on Falling Edge
    // OC3A output: Inverted
    // OC3B output: Inverted
    // OC3C output: Discon.
    // Timer 3 Overflow Interrupt: Off
    // Input Capture Interrupt: Off
    // Compare A Match Interrupt: Off
    // Compare B Match Interrupt: Off
    // Compare C Match Interrupt: On
    TCCR3A=0xF2;
    TCCR3B=0x11;
    TCNT3H=0x00;
    TCNT3L=0x00;
    ICR3H=0x01;  // 400
    ICR3L=0x90;
    OCR3AH=0x00;
    OCR3AL=0x00;
    OCR3BH=0x00;
    OCR3BL=0x00;
    OCR3CH=0x00;
    OCR3CL=0xC8; // 200 
    
// External Interrupt(s) initialization
// INT0: Off
// INT1: Off
// INT2: Off
// INT3: Off
// INT4: Off
// INT5: Off
// INT6: On
// INT6 Mode: Any change
// INT7: On
// INT7 Mode: Any change
EICRA=0x00;
EICRB=0x50;
EIMSK=0xC0;
EIFR=0xC0;

// Timer(s)/Counter(s) Interrupt(s) initialization
//TIMSK=0x84; //
TIMSK=0xA4;     // T2 compare match(bit-7), T1 input capture (bit-5), T1 Overvlow((bit-2))
ETIMSK=0x02;    // T3 compare match C(bit-1) 

// ------------------------ Inisialisasi USART1 ----------------------
    // Communication Parameters: 8 Data, 1 Stop, No Parity
    // USART1 Receiver: On
    // USART1 Transmitter: On
    // USART1 Mode: Asynchronous
    // USART1 Baud rate: 9600
    UCSR1A=0x00;
    UCSR1B=0x98;
    UCSR1C=0x06;
    UBRR1H=0x00;
    UBRR1L=0x67;  

// ------------  Inisialisasi Analog Comparator -----------------------
    // Analog Comparator: Off
    // Analog Comparator Input Capture by Timer/Counter 1: Off
    ACSR=0x80;
    SFIOR=0x00;
                                                                       
// ------------ Inisialisasi Peripheral Board ------------------------

    LCD_Init(); // -------------------------------------- LCD Text 16x2
    Stop();     // memastikan motor roda mati
    
// -------------------------------Reset Sudut Servo
 #if UseServo1 == 1      
    SudutServo1(SudutAwalServo1);
 #endif    
 #if UseServo2 == 1      
    SudutServo2(SudutAwalServo2);
 #endif    
 #if UseServo3 == 1      
    SudutServo3(SudutAwalServo3);
 #endif    
 #if UseServo4 == 1      
    SudutServo4(SudutAwalServo4);
 #endif    
 #if UseServo5 == 1      
    SudutServo5(SudutAwalServo5);
 #endif    
    SudutServo6(SudutAwalServo6);
    SudutServo7(SudutAwalServo7);
    SudutServo8(SudutAwalServo8);
    #asm("cli");
    LCD_TulisKiri(0,"LCD TEST........");
    LCD_TulisKiri(1,"1234567890123456");
                
    I2C_Init();
    
    #if UseIMU == 1
        delay_ms(100);
        MPU6050_Init();   
        LCD_BackLight = 1;
        if (MPU6050_TestConnection())    
            {   LCD_TulisKiri(0,"IMU Test OK >>>>");
                LCD_TulisKiri(1,"Wait 4 Gyroscope");
                Nada1();
            }
        else
            {   LCD_TulisKiri(0,"IMU Test Fail >>");
                LCD_TulisKiri(1,"Check Hardware!!"); 
                Nada2();
            }
        delay_ms(800);
        KalibrasiGyroScope();
        LCD_TulisF(1,"Gyro Callibrated");
        Nada3();
        #if UseCompass == 1
        HMC5883L_Init();
        if (HMC5883L_TestConnection())    
            {   LCD_TulisKiri  (0,"Magneto Test OK.");
                LCD_TulisTengah(1,"<<<<<<<<>>>>>>>>");
                Nada1();
            }
        else
            {   LCD_TulisKiri  (0,"Mageto Test Fail");
                LCD_TulisTengah(1,"<<<<<<<<>>>>>>>>");
                Nada1();
            }
        #endif    
    #endif   
    #asm("sei");
    
// -------------- Global enable interrupts ----------------------------
    #asm("sei") 
}
#pragma used-

#endif
