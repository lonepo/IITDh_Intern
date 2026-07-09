//###########################################################################
// FILE:   main_inverter_simulation.c
// TITLE:  Full Core Emulator & Control Loop for TMS320F28379D
//###########################################################################

#include "F28x_Project.h"
#include <math.h>  

// --- Core Math Constants ---
#define MATH_PI           3.1415926535f
#define MATH_2PI          6.2831853071f
#define GAIN_2_OVER_3     0.6666666667f
#define GAIN_1_OVER_SQRT3 0.5773502691f
#define V_NOM_PEAK        326.5986f        // (400V / sqrt(3)) * sqrt(2)

// --- Sizing and Time Parameters ---
const float Ts = 0.00004096f;              // Execution cycle step (40.96 us)
volatile float virtual_time = 0.0f;        // Simulated system clock accumulator
volatile float virtual_theta_grid = 0.0f;  // Phase progression counter for grid source

// --- Internal Simulated Source States ---
volatile float Va = 0.0f, Vb = 0.0f, Vc = 0.0f; 
volatile float Vdc_meas = 700.0f;

// --- PLL Output/Variables ---
volatile float Valpha = 0.0f, Vbeta = 0.0f;     
volatile float Vd = 0.0f, Vq = 0.0f;               
volatile float PLL_Error = 0.0f, PLL_Integral = 0.0f;
volatile float w_nominal = MATH_2PI * 50.0f; 
volatile float theta_pll = 0.0f;                 // [wt] Output to system

// Tuning Gains for SRF-PLL
volatile float Kp_pll = 42.42f;
volatile float Ki_pll = 900.0f;

// --- Control Loop References (Placeholder entries for your current loops) ---
volatile float Vd_control_out = 250.0f;    // Mock output from d-axis current controller
volatile float Vq_control_out = 0.0f;      // Mock output from q-axis current controller

// --- Modulation Index References ---
volatile float Ma = 0.0f, Mb = 0.0f, Mc = 0.0f;
#define MOD_INDEX_LIMIT 0.85f

// Define Buffer Size (500 Samples)
#define BUFFER_SIZE 500

// Keep only necessary buffers
volatile float theta_buffer[BUFFER_SIZE];  // Crucial PLL output
volatile float vdc_buffer[BUFFER_SIZE];    // Crucial DC Link measurement
volatile uint16_t b_idx = 0;
volatile uint16_t downsample_cnt = 0;
#define DOWNSAMPLE_FACTOR 20  // Capture every 20th sample (50Hz / 4ms resolution)

// --- System Function Prototypes ---
void ConfigureADC(void);
void ConfigureEPWM(void);
void SetupADCEpwm(void);
interrupt void adca1_isr(void);

// =========================================================================
// Main Execution Entry Point
// =========================================================================
void main(void)
{
    // Step 1. Initialize System Control (PLL, WatchDog, Peripheral Clocks)
    InitSysCtrl();

    // Step 2. Initialize GPIO Architectures
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
    SetupADCEpwm(); 

    IER |= M_INT1; 
    EINT;  
    ERTM;  

    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;

    // Step 6. Fire up ePWM1 to trigger ADC timing loops
    EALLOW;
    EPwm1Regs.ETSEL.bit.SOCAEN = 1;  // Allow SOCA generation events
    EPwm1Regs.TBCTL.bit.CTRMODE = 0; // Launch counter in Up-Count Mode
    EDIS;

    // Infinite Background Loop
    while(1)
    {
        // Low-priority tasks, monitoring, or safety checks go here.
    }
}

// =========================================================================
// Peripheral Configuration Functions
// =========================================================================

void ConfigureADC(void)
{
    EALLOW;
    AdcaRegs.ADCCTL2.bit.PRESCALE = 6; // Set internal ADCCLK divider to /4
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1; // End-of-conversion interrupt timing
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;     // Apply operational power to ADC circuitry
    DELAY_US(1000); // Allow hardware charge rails to stabilize
    EDIS;
}

void ConfigureEPWM(void)
{
    EALLOW;
    EPwm1Regs.ETSEL.bit.SOCAEN    = 0;   
    // CHANGE FROM 4 (CMPA) TO 2 (ZRO - Zero Count)
    EPwm1Regs.ETSEL.bit.SOCASEL    = 2;   
    EPwm1Regs.ETPS.bit.SOCAPRD     = 1;   
    
    // Timer Base Period & Base Compare setup
    EPwm1Regs.TBPRD = 4095;               // 24.4 kHz sampling
    EPwm1Regs.CMPA.bit.CMPA = 2048;       // Initial duty
    EPwm1Regs.TBCTL.bit.CTRMODE = 3;      // Freeze timer
    
    // Action Qualifier
    EPwm1Regs.AQCTLA.bit.ZRO = 2;        
    EPwm1Regs.AQCTLA.bit.CAU = 1;         
    EDIS;
}

void SetupADCEpwm(void)
{
    Uint16 acqps = 14; 

    EALLOW;

    AdcaRegs.ADCSOC0CTL.bit.CHSEL   = 1;
    AdcaRegs.ADCSOC0CTL.bit.ACQPS   = acqps;
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 5; 

    AdcaRegs.ADCSOC1CTL.bit.CHSEL   = 2;  
    AdcaRegs.ADCSOC1CTL.bit.ACQPS   = acqps;
    AdcaRegs.ADCSOC1CTL.bit.TRIGSEL = 5; 

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

    if (virtual_time < 0.1f)       Vdc_meas = 700.0f;
    else if (virtual_time < 0.3f)  Vdc_meas = 730.0f;
    else if (virtual_time < 0.5f)  Vdc_meas = 730.0f + 350.0f * (virtual_time - 0.3f);
    else if (virtual_time < 0.7f)  Vdc_meas = 640.0f;
    else if (virtual_time < 0.9f)  Vdc_meas = 640.0f + 300.0f * (virtual_time - 0.7f);
    else if (virtual_time < 1.1f)  Vdc_meas = 670.0f;
    else if (virtual_time < 3.0f)  Vdc_meas = 750.0f;
    else                           Vdc_meas = 700.0f;

    float f_grid = 50.0f;
    float A_grid = V_NOM_PEAK;
    float phi_grid = 0.0f;

    if (virtual_time < 0.2f) {
        f_grid = 50.0f;
    } else if (virtual_time < 0.4f) {
        f_grid = 50.0f + 10.0f * (virtual_time - 0.2f); // +2Hz linear ramp
    } else if (virtual_time < 0.6f) {
        f_grid = 52.0f;
        A_grid = 0.85f * V_NOM_PEAK;                    // 15% Voltage Sag
    } else if (virtual_time < 0.8f) {
        f_grid = 50.0f;
        phi_grid = 15.0f * (MATH_PI / 180.0f);          // 15-Degree Phase Jump
    } else if (virtual_time < 1.0f) {
        f_grid = 48.0f;                                 // Frequency Drop
    }

    // Advance independent grid reference angle
    virtual_theta_grid += MATH_2PI * f_grid * Ts;
    if (virtual_theta_grid >= MATH_2PI) virtual_theta_grid -= MATH_2PI;
    
    float target_angle = virtual_theta_grid + phi_grid;

    // Calculate fundamental virtual phase waveforms
    Va = A_grid * sinf(target_angle);
    Vb = A_grid * sinf(target_angle - (MATH_2PI / 3.0f));
    Vc = A_grid * sinf(target_angle + (MATH_2PI / 3.0f));

    // Dynamic 5th Harmonic Distortion Injection Window
    if (virtual_time >= 1.0f && virtual_time < 1.2f) {
        float h_amp = 0.05f * A_grid;
        Va += h_amp * sinf(5.0f * target_angle);
        Vb += h_amp * sinf(5.0f * (target_angle - (MATH_2PI / 3.0f)));
        Vc += h_amp * sinf(5.0f * (target_angle + (MATH_2PI / 3.0f)));
    }

    Valpha = GAIN_2_OVER_3 * (Va - (0.5f * Vb) - (0.5f * Vc));
    Vbeta  = GAIN_2_OVER_3 * (GAIN_1_OVER_SQRT3 * (Vb - Vc));

    float sin_theta = sinf(theta_pll);
    float cos_theta = cosf(theta_pll);

    Vd =  Valpha * cos_theta + Vbeta * sin_theta;
    Vq = -Valpha * sin_theta + Vbeta * cos_theta;

    PLL_Error = 0.0f - Vq;
    PLL_Integral += Ki_pll * PLL_Error * Ts;
    
    float w_limit = MATH_2PI * 10.0f;
    if (PLL_Integral > w_limit)       PLL_Integral = w_limit;
    else if (PLL_Integral < -w_limit) PLL_Integral = -w_limit;

    float w_detected = w_nominal + (Kp_pll * PLL_Error) + PLL_Integral;

    theta_pll += w_detected * Ts;
    if (theta_pll >= MATH_2PI)      theta_pll -= MATH_2PI;
    else if (theta_pll < 0.0f)      theta_pll += MATH_2PI;

    float Ia_meas = (float)AdcaRegs.ADCRESULT0 * 0.000805f;
    float Ib_meas = (float)AdcaRegs.ADCRESULT1 * 0.000805f;
    float Ic_meas = -(Ia_meas + Ib_meas);
    float I_alpha = GAIN_2_OVER_3 * (Ia_meas - 0.5f * Ib_meas - 0.5f * Ic_meas);
    float I_beta  = GAIN_2_OVER_3 * (GAIN_1_OVER_SQRT3 * (Ib_meas - Ic_meas));

    float Id = I_alpha * cos_theta + I_beta * sin_theta;
    float Iq = -I_alpha * sin_theta + I_beta * cos_theta;

    static float Id_integral = 0.0f, Iq_integral = 0.0f;
    float Id_ref = 0.0f;
    float Iq_ref = 10.0f; 
    float Kp_i = 1.5f, Ki_i = 50.0f;

    float error_d = Id_ref - Id;
    float error_q = Iq_ref - Iq;
    Id_integral += Ki_i * error_d * Ts;
    Iq_integral += Ki_i * error_q * Ts;
    
    if (Id_integral > 100.0f) Id_integral = 100.0f; else if (Id_integral < -100.0f) Id_integral = -100.0f;
    if (Iq_integral > 100.0f) Iq_integral = 100.0f; else if (Iq_integral < -100.0f) Iq_integral = -100.0f;

    Vd_control_out = Kp_i * error_d + Id_integral;
    Vq_control_out = Kp_i * error_q + Iq_integral;
    float V_alpha_ref = Vd_control_out * cos_theta - Vq_control_out * sin_theta;
    float V_beta_ref  = Vd_control_out * sin_theta + Vq_control_out * cos_theta;

    float Va_ref = V_alpha_ref;
    float Vb_ref = -0.5f * V_alpha_ref + (1.7320508075f * 0.5f) * V_beta_ref; 
    float Vc_ref = -0.5f * V_alpha_ref - (1.7320508075f * 0.5f) * V_beta_ref;

    float inv_Vdc_half = 2.0f / (Vdc_meas > 100.0f ? Vdc_meas : 100.0f);
    
    Ma = Va_ref * inv_Vdc_half;
    Mb = Vb_ref * inv_Vdc_half;
    Mc = Vc_ref * inv_Vdc_half;

    if (Ma >  MOD_INDEX_LIMIT) Ma =  MOD_INDEX_LIMIT;
    if (Ma < -MOD_INDEX_LIMIT) Ma = -MOD_INDEX_LIMIT;
    if (Mb >  MOD_INDEX_LIMIT) Mb =  MOD_INDEX_LIMIT;
    if (Mb < -MOD_INDEX_LIMIT) Mb = -MOD_INDEX_LIMIT;
    if (Mc >  MOD_INDEX_LIMIT) Mc =  MOD_INDEX_LIMIT;
    if (Mc < -MOD_INDEX_LIMIT) Mc = -MOD_INDEX_LIMIT;

    float dutyA = (1.0f + Ma) * 0.5f;
    
    EPwm1Regs.CMPA.bit.CMPA = (Uint16)(dutyA * (float)EPwm1Regs.TBPRD);
    virtual_time += Ts;

    downsample_cnt++;
    if (downsample_cnt >= DOWNSAMPLE_FACTOR) {
        downsample_cnt = 0; // Reset counter
        
        theta_buffer[b_idx] = theta_pll;
        vdc_buffer[b_idx]   = Vdc_meas;
        

        b_idx++;
        if (b_idx >= BUFFER_SIZE) {
            b_idx = 0; 
        }
    }
}
