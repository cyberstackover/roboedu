/*********************************************************
Project : ADROIT AVR Rev.3
Version : 1
Date    : 3/13/2014
Author  : copyright (c) Davide Gironi, 2012, modified by Eko Henfri Binugroho @ ER2C  

References:
  - HMC5883L Triple Axis Magnetometer Arduino Library
    http://bildr.org/2012/02/hmc5883l_arduino/
 
*********************************************************
*/

#if UseCompass == 1

#ifndef HMC5883L_H_
#define HMC5883L_H_

//definitions
#define HMC5883L_ADDR (0x1E<<1) //device address

//registers
#define HMC5883L_CONFREGA       0x00
#define HMC5883L_CONFREGB       0x01
#define HMC5883L_MODEREG        0x02
#define HMC5883L_DATAREGBEGIN   0x03
#define HMC5883L_STATUSREG      0x09
#define HMC5883L_IDREG_A        0x0A
#define HMC5883L_IDREG_B        0x0B
#define HMC5883L_IDREG_C        0x0C

//setup measurement mode
#define HMC5883L_MEASURECONTINOUS   0x00
#define HMC5883L_MEASURESINGLESHOT  0x01
#define HMC5883L_MEASUREIDLE        0x03
#define HMC5883L_MEASUREMODE        HMC5883L_MEASURECONTINOUS

//setup scale
#define HMC5883L_SCALE088   1 //0.88
#define HMC5883L_SCALE13    2 //1.3
#define HMC5883L_SCALE19    3 //1.9
#define HMC5883L_SCALE25    4 //2.5
#define HMC5883L_SCALE40    5 //4.0
#define HMC5883L_SCALE47    6 //4.7
#define HMC5883L_SCALE56    7 //5.6
#define HMC5883L_SCALE81    8 //8.1
#define HMC5883L_SCALE      HMC5883L_SCALE13

#define HMC5883L_CALIBRATED 1 //enable this if this magn is calibrated

//calibration values
#if HMC5883L_CALIBRATED == 1
#define HMC5883L_OFFSETX    -99.7
#define HMC5883L_OFFSETY    -154.0
#define HMC5883L_OFFSETZ    -22.7
#define HMC5883L_GAINX1     0.952017
#define HMC5883L_GAINX2     0.00195895
#define HMC5883L_GAINX3     0.0139661
#define HMC5883L_GAINY1     0.00195895
#define HMC5883L_GAINY2     0.882824
#define HMC5883L_GAINY3     0.00760243
#define HMC5883L_GAINZ1     0.0139661
#define HMC5883L_GAINZ2     0.00760243
#define HMC5883L_GAINZ3     0.995365
#endif




//functions
#pragma used+
extern void HMC5883L_Init();
extern void HMC5883L_GetRawData(int16_t *mxraw, int16_t *myraw, int16_t *mzraw);
extern void HMC5883L_GetData(float *mx, float *my, float *mz);
extern int8_t HMC5883L_TestConnection(void); 
extern int8_t HMC5883L_GetRate(void);
extern void HMC5883L_SetRate(int8_t rate);
 
static float hmc5883l_scale = 0;

/*
 * init the hmc5883l
 */      
 
void HMC5883L_Init() {
    uint8_t regValue = 0x00;
    //set scale
    hmc5883l_scale = 0;
    #if HMC5883L_SCALE == HMC5883L_SCALE088
        regValue = 0x00;
        hmc5883l_scale = 0.73;
    #elif HMC5883L_SCALE == HMC5883L_SCALE13
        regValue = 0x01;
        hmc5883l_scale = 0.92;
    #elif HMC5883L_SCALE == HMC5883L_SCALE19
        regValue = 0x02;
        hmc5883l_scale = 1.22;
    #elif HMC5883L_SCALE == HMC5883L_SCALE25
        regValue = 0x03;
        hmc5883l_scale = 1.52;
    #elif HMC5883L_SCALE == HMC5883L_SCALE40
        regValue = 0x04;
		hmc5883l_scale = 2.27;
	#elif HMC5883L_SCALE == HMC5883L_SCALE47
		regValue = 0x05;
		hmc5883l_scale = 2.56;
	#elif HMC5883L_SCALE == HMC5883L_SCALE56
		regValue = 0x06;
		hmc5883l_scale = 3.03;
	#elif HMC5883L_SCALE == HMC5883L_SCALE81
		regValue = 0x07;
		hmc5883l_scale = 4.35;
	#endif

	//setting is in the top 3 bits of the register.
	regValue = regValue << 5;
    I2C_Start_Wait(HMC5883L_ADDR | I2C_WRITE);
    I2C_Write(HMC5883L_CONFREGB);
    I2C_Write(regValue);
    I2C_Stop();

	//set measurement mode
	I2C_Start_Wait(HMC5883L_ADDR | I2C_WRITE);
	I2C_Write(HMC5883L_MODEREG);
	I2C_Write(HMC5883L_MEASUREMODE);
	I2C_Stop();
}

// Update rate 
// Value | Typical Data Output Rate (Hz)
// ------+------------------------------
// 0     | 0.75
// 1     | 1.5
// 2     | 3
// 3     | 7.5
// 4     | 15 (Default)
// 5     | 30
// 6     | 75
// 7     | Not used

int8_t HMC5883L_GetRate(void)
{   int8_t rate;
    I2C_Start_Wait(HMC5883L_ADDR | I2C_READ);
	I2C_Write(HMC5883L_CONFREGA);
	rate = I2C_ReadNak();      // membaca isi ID register C --> isi seharusnya adalah karakter 3
	I2C_Stop();
	return (rate & 7);
}    

void HMC5883L_SetRate(int8_t rate)
{   int8_t buffer;
    I2C_Start_Wait(HMC5883L_ADDR | I2C_READ);
	I2C_Write(HMC5883L_CONFREGA);
	buffer = I2C_ReadNak();      // membaca isi ID register C --> isi seharusnya adalah karakter 3
	I2C_Stop();
	buffer = (buffer & 0xF8) | (rate & 7); 
    I2C_Start_Wait(HMC5883L_ADDR | I2C_WRITE);
	I2C_Write(HMC5883L_CONFREGA); 
    I2C_Write(buffer); 
    I2C_Stop();
}    




/*
 * get raw data
 */




void HMC5883L_GetRawData(int16_t *mxraw, int16_t *myraw, int16_t *mzraw) {
	int8_t i = 0;
	uint8_t buffer[6];

	I2C_Start_Wait(HMC5883L_ADDR | I2C_WRITE);
	I2C_Write(HMC5883L_DATAREGBEGIN);
	I2C_Stop();
	I2C_Start_Wait(HMC5883L_ADDR | I2C_READ);
	for(i=5; i>=0; i--) {
		if(i==0)    buffer[i] = I2C_ReadNak();
		else        buffer[i] = I2C_ReadAck();
	}
	I2C_Stop();                                    
    *myraw = peekw(&buffer[4]); *mzraw = peekw(&buffer[2]); *mxraw = peekw(&buffer[0]);

//	*mxraw = (int16_t)((buff[0] << 8) | buff[1]);
//	*mzraw = (int16_t)((buff[2] << 8) | buff[3]);
//	*myraw = (int16_t)((buff[4] << 8) | buff[5]);
}

/*
 * get scaled data
 */
void HMC5883L_GetData(float *mx, float *my, float *mz) {
	int16_t mxraw = 0;
	int16_t myraw = 0;
	int16_t mzraw = 0;
	#if HMC5883L_CALIBRATED == 1
	    float mxt = mxraw - HMC5883L_OFFSETX;
	    float myt = myraw - HMC5883L_OFFSETY;
	    float mzt = mzraw - HMC5883L_OFFSETZ;
	#endif
    HMC5883L_GetRawData(&mxraw, &myraw, &mzraw);

	#if HMC5883L_CALIBRATED == 1
	*mx = HMC5883L_GAINX1 * mxt + HMC5883L_GAINX2 * myt + HMC5883L_GAINX3 * mzt;
	*my = HMC5883L_GAINY1 * mxt + HMC5883L_GAINY2 * myt + HMC5883L_GAINY3 * mzt;
	*mz = HMC5883L_GAINZ1 * mxt + HMC5883L_GAINZ2 * myt + HMC5883L_GAINZ3 * mzt;
	#else
	*mx = mxraw * hmc5883l_scale;
	*my = myraw * hmc5883l_scale;
	*mz = mzraw * hmc5883l_scale;
	#endif
}

int8_t HMC5883L_TestConnection(void) 
{   uint8_t buffer[3];
    // meletakkan ponter register pada ID register A
    I2C_Start_Wait(HMC5883L_ADDR | I2C_WRITE);
	I2C_Write(HMC5883L_IDREG_A);
	I2C_Stop();
	// membaca data dari ID register A, B dan C
    I2C_Start_Wait(HMC5883L_ADDR | I2C_READ);
	buffer[0] = I2C_ReadAck();      // membaca isi ID register A --> isi seharusnya adalah karakter H
	buffer[1] = I2C_ReadAck();      // membaca isi ID register B --> isi seharusnya adalah karakter 4
	buffer[2] = I2C_ReadNak();      // membaca isi ID register C --> isi seharusnya adalah karakter 3
	I2C_Stop();                                    
    
    if(buffer[0]=='H' && buffer[1]=='4' && buffer[2]=='3')  {   return 1;   }
    else                                                    {   return 0;   }
    
}

#pragma used-

#endif
#endif
