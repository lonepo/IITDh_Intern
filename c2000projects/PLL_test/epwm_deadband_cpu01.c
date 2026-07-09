#include "F28x_Project.h"
#include <math.h>

// --- System & Control Defines ---
#define PI            3.14159265
#define PWM_FREQ      5000.0
#define FUND_FREQ     50.0
#define TS            (1.0 / PWM_FREQ)   // 200us control loop period

// --- Parameters from Prompt ---
#define VDC_REF       700.0
#define V_RMS         400.0
#define L1            3e-3
#define L2            1.5e-3
#define L_TOTAL       (L1 + L2)
#define I_MAX         50.0

// --- Gains ---
#define KP_PLL        42.42
#define KI_PLL        900.0
#define KP_CURR       14.14
#define KI_CURR       314.2

// --- Global Variables ---
volatile float Vdc_meas = 700.0;    // Simulated DC bus measurement
volatile float Vabc_sim[3];         // Simulated grid voltage
volatile float Iabc_sim[3];         // Simulated grid current
volatile float Iabc_ref[3];         // Final PWM modulation targets

// --- Control Loop Variables ---
volatile float theta_grid = 0.0;    // Actual grid phase
volatile float theta_pll = 0.0;     // Estimated PLL phase
volatile float omega_pll = 2.0 * PI * 50.0;

// --- PI Controller States ---
volatile float integral_pll = 0.0;
volatile float integral_id = 0.0;
volatile float integral_iq = 0.0;

// --- Referenced & Measured Values ---
volatile float Id_ref = 0.0, Iq_ref = 0.0;
volatile float Id_meas = 0.0, Iq_meas = 0.0;
volatile float Vd_meas = 0.0, Vq_meas = 0.0;
volatile float Vd_ref = 0.0, Vq_ref = 0.0;

// --- Function Prototypes ---
void InitEPwm1(void);
void InitEPwm2(void);
void InitEPwm3(void);
void Sensor_Simulation(void);
void Control_Loop(void);
void Update_PWM(void);

void main(void)
{
    InitSysCtrl();

    CpuSysRegs.PCLKCR2.bit.EPWM1 = 1;
    CpuSysRegs.PCLKCR2.bit.EPWM2 = 1;
    CpuSysRegs.PCLKCR2.bit.EPWM3 = 1;

    InitEPwm1Gpio();
    InitEPwm2Gpio();
    InitEPwm3Gpio();

    DINT;
    InitPieCtrl();

    IER = 0;
    IFR = 0;

    InitPieVectTable();

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;

    InitEPwm1();
    InitEPwm2();
    InitEPwm3();

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;

    // Initial condition for the simulated plant
    theta_grid = 0.0;
    Id_meas = 0.0; 
    Iq_meas = 0.0;

    while(1)
    {
        // 1. Simulate the physical plant (since you don't have sensors)
        Sensor_Simulation();

        // 2. Run the Closed Loop Control (PLL + Current Controller)
        Control_Loop();

        // 3. Convert reference voltages to PWM Duty Cycles
        Update_PWM();

        // 4. Synchronize to 5 kHz rate (200us)
        DELAY_US(200);
    }
}

// -----------------------------------------------
// Function: Sensor Simulation (Software Plant)
// -----------------------------------------------
void Sensor_Simulation(void)
{
    // Update the simulated grid angle
    theta_grid += 2.0 * PI * FUND_FREQ * TS;
    if(theta_grid >= 2.0 * PI) theta_grid -= 2.0 * PI;

    float Vpeak = V_RMS * sqrt(2.0); // 565.68 V

    // Generate a perfectly balanced 3-phase grid voltage
    Vabc_sim[0] = Vpeak * sin(theta_grid);
    Vabc_sim[1] = Vpeak * sin(theta_grid - 2.0 * PI / 3.0);
    Vabc_sim[2] = Vpeak * sin(theta_grid + 2.0 * PI / 3.0);

    // Simulate the LCL filter current response to the inverter's output voltage.
    // (Simplified model using L_total and a small parasitic R for stability)
    float R_parasitic = 0.1; 
    float Vd_inv = Vd_ref; 
    float Vq_inv = Vq_ref;
    float Vd_grid = Vd_meas; // PLL tracking this
    float Vq_grid = Vq_meas;

    // Euler integration of the RL filter dynamics (dI/dt = (Vinv - Vgrid - R*I)/L)
    Id_meas += ( (Vd_inv - Vd_grid) - R_parasitic * Id_meas ) / L_TOTAL * TS;
    Iq_meas += ( (Vq_inv - Vq_grid) - R_parasitic * Iq_meas ) / L_TOTAL * TS;

    // Clamp simulated current to Imax
    if(Id_meas > I_MAX) Id_meas = I_MAX;
    if(Id_meas < -I_MAX) Id_meas = -I_MAX;
    if(Iq_meas > I_MAX) Iq_meas = I_MAX;
    if(Iq_meas < -I_MAX) Iq_meas = -I_MAX;
}

// -----------------------------------------------
// Function: Main Control Loops (PLL & PI Controllers)
// -----------------------------------------------
void Control_Loop(void)
{
    // --- 1. SRF-PLL ---
    // Clarke Transform: Vabc -> Valpha/Vbeta
    float Valpha = Vabc_sim[0];
    float Vbeta = (Vabc_sim[0] + 2.0 * Vabc_sim[1]) / sqrt(3.0);

    // Park Transform: Valpha/Vbeta -> Vd/Vq (using estimated theta_pll)
    Vd_meas = Valpha * cos(theta_pll) + Vbeta * sin(theta_pll);
    Vq_meas = -Valpha * sin(theta_pll) + Vbeta * cos(theta_pll);

    // PLL PI Controller (tracks Vq to zero)
    float error_pll = 0.0 - Vq_meas;
    integral_pll += error_pll * TS;
    // Anti-windup clamp
    if(integral_pll > 100.0) integral_pll = 100.0; 
    if(integral_pll < -100.0) integral_pll = -100.0;

    omega_pll = 2.0 * PI * 50.0 + KP_PLL * error_pll + KI_PLL * integral_pll;
    theta_pll += omega_pll * TS;
    if(theta_pll >= 2.0 * PI) theta_pll -= 2.0 * PI;


    // --- 2. Outer DC Voltage Loop (Optional for testing. Using fixed Id_ref to test inner loops) ---
    // Since you want to test the PI loops, we will just command a fixed active current.
    // (You can uncomment below to enable a DC voltage PI loop if you add gains)
    // float error_vdc = VDC_REF - Vdc_meas;
    // integral_vdc += error_vdc * TS;
    // Id_ref = KPL * error_vdc + KIL * integral_vdc;
    Id_ref = I_MAX * 0.5; // Command 50% of max current (25A peak)
    Iq_ref = 0.0;         // Unity power factor (zero reactive current)


    // --- 3. Inner Current Loops (d/q axis) ---
    // d-axis Controller: Vd*
    float error_id = Id_ref - Id_meas;
    integral_id += error_id * TS;
    if(integral_id > 100.0) integral_id = 100.0; 
    if(integral_id < -100.0) integral_id = -100.0;
    float Vd_pi = KP_CURR * error_id + KI_CURR * integral_id;

    // q-axis Controller: Vq*
    float error_iq = Iq_ref - Iq_meas;
    integral_iq += error_iq * TS;
    if(integral_iq > 100.0) integral_iq = 100.0; 
    if(integral_iq < -100.0) integral_iq = -100.0;
    float Vq_pi = KP_CURR * error_iq + KI_CURR * integral_iq;

    // --- 4. Decoupling & Feed-forward ---
    float wL = 2.0 * PI * 50.0 * L_TOTAL; 
    Vd_ref = Vd_meas + Vd_pi - wL * Iq_meas;
    Vq_ref = Vq_meas + Vq_pi + wL * Id_meas;

    // Voltage Limiter (Prevent over-modulation > 1.0)
    float Vm = sqrt(Vd_ref * Vd_ref + Vq_ref * Vq_ref);
    if(Vm > (VDC_REF / 2.0)) 
    {
        Vd_ref = Vd_ref * (VDC_REF / 2.0) / Vm;
        Vq_ref = Vq_ref * (VDC_REF / 2.0) / Vm;
    }
}

// -----------------------------------------------
// Function: Calculate Duty cycles and Update EPWM
// -----------------------------------------------
void Update_PWM(void)
{
    // Inverse Park: Vdq -> Valpha/Vbeta (using estimated theta_pll)
    float Valpha_ref = Vd_ref * cos(theta_pll) - Vq_ref * sin(theta_pll);
    float Vbeta_ref = Vd_ref * sin(theta_pll) + Vq_ref * cos(theta_pll);

    // Inverse Clarke: Valpha/Vbeta -> Vabc references
    float Va_ref = Valpha_ref;
    float Vb_ref = -0.5 * Valpha_ref + 0.8660254 * Vbeta_ref;
    float Vc_ref = -0.5 * Valpha_ref - 0.8660254 * Vbeta_ref;

    // Calculate Modulation Index for SPWM (ModIndex = 2*Vm_ref / Vdc)
    float Ma = 2.0 * Va_ref / VDC_REF;
    float Mb = 2.0 * Vb_ref / VDC_REF;
    float Mc = 2.0 * Vc_ref / VDC_REF;

    // Clamp ModIndex between -1.0 and 1.0
    if(Ma > 1.0) Ma = 1.0; if(Ma < -1.0) Ma = -1.0;
    if(Mb > 1.0) Mb = 1.0; if(Mb < -1.0) Mb = -1.0;
    if(Mc > 1.0) Mc = 1.0; if(Mc < -1.0) Mc = -1.0;

    // Update PWM Compare Registers (Up-Down counter, Zero = 0, Period = TBPRD)
    EPwm1Regs.CMPA.bit.CMPA = (Uint16)((EPwm1Regs.TBPRD / 2.0) * (1.0 + Ma));
    EPwm2Regs.CMPA.bit.CMPA = (Uint16)((EPwm2Regs.TBPRD / 2.0) * (1.0 + Mb));
    EPwm3Regs.CMPA.bit.CMPA = (Uint16)((EPwm3Regs.TBPRD / 2.0) * (1.0 + Mc));
}

// -----------------------------------------------
// EPWM Initialization (Corrected TBPRD for 5kHz)
// -----------------------------------------------
void InitEPwm1(void)
{
    // SYSCLK = 100MHz. For 5kHz, TBPRD = 100e6 / (2 * 5000) = 10000
    EPwm1Regs.TBPRD = 10000; 

    EPwm1Regs.TBPHS.bit.TBPHS = 0;
    EPwm1Regs.TBCTR = 0;
    
    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;

    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;

    EPwm1Regs.CMPA.bit.CMPA = 5000; // Default 50% duty

    EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm1Regs.AQCTLA.bit.CAD = AQ_SET;

    EPwm1Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
    EPwm1Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC;
    EPwm1Regs.DBCTL.bit.IN_MODE = DBA_ALL;

    EPwm1Regs.DBRED.bit.DBRED = 500;
    EPwm1Regs.DBFED.bit.DBFED = 500;
}

void InitEPwm2(void)
{
    EPwm2Regs.TBPRD = 10000;

    EPwm2Regs.TBPHS.bit.TBPHS = 0;
    EPwm2Regs.TBCTR = 0;
    
    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm2Regs.TBCTL.bit.PHSEN = TB_DISABLE;
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1;

    EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;

    EPwm2Regs.CMPA.bit.CMPA = 5000;

    EPwm2Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm2Regs.AQCTLA.bit.CAD = AQ_SET;

    EPwm2Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
    EPwm2Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC;
    EPwm2Regs.DBCTL.bit.IN_MODE = DBA_ALL;

    EPwm2Regs.DBRED.bit.DBRED = 500;
    EPwm2Regs.DBFED.bit.DBFED = 500;
}

void InitEPwm3(void)
{
    EPwm3Regs.TBPRD = 10000;

    EPwm3Regs.TBPHS.bit.TBPHS = 0;
    EPwm3Regs.TBCTR = 0;
    
    EPwm3Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm3Regs.TBCTL.bit.PHSEN = TB_DISABLE;
    EPwm3Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm3Regs.TBCTL.bit.CLKDIV = TB_DIV1;

    EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;

    EPwm3Regs.CMPA.bit.CMPA = 5000;

    EPwm3Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm3Regs.AQCTLA.bit.CAD = AQ_SET;

    EPwm3Regs.DBCTL.bit.OUT_MODE = DB_FULL_ENABLE;
    EPwm3Regs.DBCTL.bit.POLSEL = DB_ACTV_HIC;
    EPwm3Regs.DBCTL.bit.IN_MODE = DBA_ALL;

    EPwm3Regs.DBRED.bit.DBRED = 500;
    EPwm3Regs.DBFED.bit.DBFED = 500;
}