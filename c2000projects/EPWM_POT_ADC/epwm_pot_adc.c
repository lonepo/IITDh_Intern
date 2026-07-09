//###########################################################################
// FILE:   adc_soc_epwm_pot_control.c
// TITLE:  Varying PWM Duty Cycle via POT (ADC) for F2837xD.
//###########################################################################

#include "F28x_Project.h"

// Function Prototypes
void ConfigureADC(void);
void ConfigureEPWM(void);
void SetupADCEpwm(Uint16 channel);
interrupt void adca1_isr(void);

// Globals
volatile Uint16 POT_Value = 0;

void main(void)
{
    // Step 1. Initialize System Control (PLL, WatchDog, Peripheral Clocks)
    InitSysCtrl();

    // Step 2. Initialize GPIO
    InitGpio(); 
    
    // CRITICAL ADDITION: Map EPWM1A to its physical pin (typically GPIO0)
    CpuSysRegs.PCLKCR2.bit.EPWM1 = 1; // Ensure EPWM1 clock is active
    InitEPwm1Gpio(); 

    // Step 3. Clear all interrupts and initialize PIE vector table
    DINT;
    InitPieCtrl();

    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // Map ISR function
    EALLOW;
    PieVectTable.ADCA1_INT = &adca1_isr; 
    EDIS;

    // Configure peripherals
    ConfigureADC();
    ConfigureEPWM();
    SetupADCEpwm(0); // Pin A0 will read the POT

    // Enable global Interrupts
    IER |= M_INT1; 
    EINT;  
    ERTM;  

    // Enable PIE interrupt for ADC A1
    PieCtrlRegs.PIEIER1.bit.INTx1 = 1;

    // Sync ePWM time base clock
    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;

    // Start ePWM1 to trigger ADC and output PWM
    EALLOW;
    EPwm1Regs.ETSEL.bit.SOCAEN = 1;  // Enable SOCA generation
    EPwm1Regs.TBCTL.bit.CTRMODE = 0; // Start counter in Up-Count Mode
    EDIS;

    // Real-time background loop
    while(1)
    {
        // Your background code can go here. 
        // The PWM updating is entirely handled by the hardware and ISR!
    }
}

void ConfigureADC(void)
{
    EALLOW;
    AdcaRegs.ADCCTL2.bit.PRESCALE = 6; // Set ADCCLK divider to /4
    AdcSetMode(ADC_ADCA, ADC_RESOLUTION_12BIT, ADC_SIGNALMODE_SINGLE);
    AdcaRegs.ADCCTL1.bit.INTPULSEPOS = 1; // Late interrupt pulse
    AdcaRegs.ADCCTL1.bit.ADCPWDNZ = 1;     // Power up ADC
    DELAY_US(1000); // Wait for power-up
    EDIS;
}

void ConfigureEPWM(void)
{
    EALLOW;
    EPwm1Regs.ETSEL.bit.SOCAEN    = 0;    // Disable SOC on A group temporarily
    EPwm1Regs.ETSEL.bit.SOCASEL    = 4;   // Trigger SOCA when counter equals CMPA on up-count
    EPwm1Regs.ETPS.bit.SOCAPRD     = 1;   // Generate SOCA pulse on 1st event
    
    // PWM Period & Initial Duty Cycle Configuration
    EPwm1Regs.TBPRD = 4095;               // Period = 4095 counts (matches 12-bit ADC max range!)
    EPwm1Regs.CMPA.bit.CMPA = 2048;       // Start at 50% duty cycle
    EPwm1Regs.TBCTL.bit.CTRMODE = 3;      // Freeze counter initially
    
    // CRITICAL ADDITION: Action Qualifier (Defines PWM shape)
    // For an asymmetric Up-Count PWM:
    EPwm1Regs.AQCTLA.bit.ZRO = 2;         // Set EPWM1A High when counter handles 0
    EPwm1Regs.AQCTLA.bit.CAU = 1;         // Clear EPWM1A Low when counter matches CMPA
    
    EDIS;
}

void SetupADCEpwm(Uint16 channel)
{
    Uint16 acqps = 14; // 12-bit resolution minimum window (75ns)

    EALLOW;
    AdcaRegs.ADCSOC0CTL.bit.CHSEL = channel;  // SOC0 converts pin A0
    AdcaRegs.ADCSOC0CTL.bit.ACQPS = acqps;    // Sample window setup
    AdcaRegs.ADCSOC0CTL.bit.TRIGSEL = 5;      // Trigger SOC0 via ePWM1 SOCA
    AdcaRegs.ADCINTSEL1N2.bit.INT1SEL = 0;    // End of SOC0 sets INT1 flag
    AdcaRegs.ADCINTSEL1N2.bit.INT1E = 1;      // Enable INT1 flag
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;    // Clear flag
    EDIS;
}

// ADC Interrupt Service Routine - Executes every time a new reading is ready
interrupt void adca1_isr(void)
{
    // Read the current analog value from the POT (Range: 0 to 4095)
    POT_Value = AdcaResultRegs.ADCRESULT0;

    // THE MAGIC LINK: Feed the POT value directly into the PWM Compare Register
    EPwm1Regs.CMPA.bit.CMPA = POT_Value; 

    // Clear flags so the next interrupt can trigger
    AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    
    if(1 == AdcaRegs.ADCINTOVF.bit.ADCINT1)
    {
        AdcaRegs.ADCINTOVFCLR.bit.ADCINT1 = 1;
        AdcaRegs.ADCINTFLGCLR.bit.ADCINT1 = 1;
    }

    PieCtrlRegs.PIEACK.all = PIEACK_GROUP1;
}