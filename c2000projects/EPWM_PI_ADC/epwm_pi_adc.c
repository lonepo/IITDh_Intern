//###########################################################################
// FILE:   epwm_pi_adc.c
// TITLE:  SRF-PLL Implementation for F2837xD.
//###########################################################################

#include "F28x_Project.h"
#include <math.h>  


void ConfigureADC(void);
void ConfigureEPWM(void);
void SetupADCEpwm(Uint16 channel);
interrupt void adca1_isr(void);


#define GRID_FREQ_HZ   50.0f          
#define MATH_PI        3.1415926535f
#define MATH_2PI       6.2831853071f
#define GAIN_2_OVER_3  0.6666666667f
#define GAIN_1_OVER_SQRT3 0.5773502691f


const float Ts = 0.00004096f; 

// --- PLL Global Variables ---
volatile float Va = 0.0f, Vb = 0.0f, Vc = 0.0f;    
volatile float Valpha = 0.0f, Vbeta = 0.0f;      
volatile float Vd = 0.0f, Vq = 0.0f;          

volatile float PLL_Setpoint = 0.0f;              
volatile float PLL_Error    = 0.0f;
volatile float PLL_Integral = 0.0f;
volatile float w_nominal    = MATH_2PI * GRID_FREQ_HZ; 
volatile float w_detected   = 0.0f;           
volatile float theta        = 0.0f;              

// PI Tuning Gains for PLL
volatile float Kp_pll = 42.4f;
volatile float Ki_pll = 900.0f;

#define BUFFER_SIZE 500
float data_buffer[BUFFER_SIZE]; 
uint16_t buffer_index = 0;



void main(void)
{

    InitSysCtrl();

    InitGpio(); 
    
    CpuSysRegs.PCLKCR2.bit.EPWM1 = 1; 
    InitEPwm1Gpio(); 


    DINT;
    InitPieCtrl();

    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    EALLOW;
    PieVectTable.ADCA1_INT = &adca1_isr; 
    EDIS;


    ConfigureADC();
    ConfigureEPWM();
    SetupADCEpwm(1); 


    IER |= M_INT1; 
    EINT;  
    ERTM;  

    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;


    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;


    EALLOW;
    EPwm1Regs.ETSEL.bit.SOCAEN = 1;  
    EPwm1Regs.TBCTL.bit.CTRMODE = 0; 
    EDIS;

    while(1)
    {
       
    }
}

void ConfigureADC(void)
{
    EALLOW;
    AdcaRegs.ADCCTL2.bit.PRESCALE = 6;
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1; 
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;  
    DELAY_US(1000); 
    EDIS;
}

void ConfigureEPWM(void)
{
    EALLOW;
    EPwm1Regs.ETSEL.bit.SOCAEN    = 0;  
    EPwm1Regs.ETSEL.bit.SOCASEL    = 4;  
    EPwm1Regs.ETPS.bit.SOCAPRD     = 1;  

    EPwm1Regs.TBPRD = 4095;            
    EPwm1Regs.CMPA.bit.CMPA = 2048;    
    EPwm1Regs.TBCTL.bit.CTRMODE = 3;     
    
    // Action Qualifier (Defines PWM shape)
    EPwm1Regs.AQCTLA.bit.ZRO = 2;     
    EPwm1Regs.AQCTLA.bit.CAU = 1;    
    EDIS;
}

void SetupADCEpwm(Uint16 channel)
{
    Uint16 acqps = 14; 

    EALLOW;

    AdcaRegs.ADCSOC0CTL.bit.CHSEL   = 1; 
    AdcaRegs.ADCSOC0CTL.bit.ACQPS   = acqps;
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 5; 

    AdcaRegs.ADCSOC1CTL.bit.CHSEL   = 2; 
    AdcaRegs.ADCSOC1CTL.bit.ACQPS   = acqps;
    AdcaRegs.ADCSOC1CTL.bit.TRIGSEL = 5; 

    // SOC2 -> Phase C (ADCINA3 - Pin 72)
    AdcaRegs.ADCSOC2CTL.bit.CHSEL   = 3;
    AdcaRegs.ADCSOC2CTL.bit.ACQPS   = acqps;
    AdcaRegs.ADCSOC2CTL.bit.TRIGSEL = 5; 

    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 2; 
    AdcaRegs.ADCINTSEL1N2.bit.INT1E   = 1;
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    EDIS;
}

interrupt void adca1_isr(void)
{

    Va = ((float)AdcaResultRegs.ADCRESULT0 - 2048.0f) * (3.3f / 4095.0f);
    Vb = ((float)AdcaResultRegs.ADCRESULT1 - 2048.0f) * (3.3f / 4095.0f);
    Vc = ((float)AdcaResultRegs.ADCRESULT2 - 2048.0f) * (3.3f / 4095.0f);

    Valpha = GAIN_2_OVER_3 * (Va - (0.5f * Vb) - (0.5f * Vc));
    Vbeta  = GAIN_2_OVER_3 * (GAIN_1_OVER_SQRT3 * (Vb - Vc));

    float sin_theta = sinf(theta);
    float cos_theta = cosf(theta);

    Vd =  Valpha * cos_theta + Vbeta * sin_theta;
    Vq = -Valpha * sin_theta + Vbeta * cos_theta;

    PLL_Error = 0.0f - Vq;

    float P_term = Kp_pll * PLL_Error;

    PLL_Integral += Ki_pll * PLL_Error * Ts;

    float w_limit = MATH_2PI * 10.0f; 
    if(PLL_Integral > w_limit)       PLL_Integral = w_limit;
    else if(PLL_Integral < -w_limit) PLL_Integral = -w_limit;

    float delta_w = P_term + PLL_Integral;

    w_detected = w_nominal + delta_w;

    theta += w_detected * Ts; 

    if (theta >= MATH_2PI)
    {
        theta -= MATH_2PI;
    }
    else if (theta < 0.0f)
    {
        theta += MATH_2PI;
    }

    if (buffer_index < BUFFER_SIZE) {
        data_buffer[buffer_index++] = theta;
    }


    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    if(1 == AdcaRegs.ADCINTOVF.bit.ADCINT1)
    {
        AdcaRegs.ADCINTOVFCLR.bit.ADCINT1 = 1;
        AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    }
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}
