#include "F28x_Project.h"
#include "math.h"

extern interrupt void cpu_epwm1_isr(void);
void InitEPwm1(void);
void InitEPwm2(void);
void InitEPwm3(void);
void InitDaca(void);
void InitDacb(void);
void Inverter_3Ph(void);
//variable declaration
float Line_freq = 50;
float theta = 0, Sampling_time = 0.0001;
float L=30e-3, r_L=0.01;
float V_m = 325, Vd = 0, Vq = 0, Id = 0, Iq = 0, Pdq = 0, Qdq = 0;
float v_a = 0, v_b = 0, v_c = 0, i_a = 0, i_b = 0, i_c = 0;
float PLL_theta=0, PLL_kp=0.687, PLL_ki=77.47, del_f=0, PLL_theta1=0, PLL_theta1_pre=0, v_q_pre=0;
float del_f_pre=0, del_f_er = 0, del_f_er_pre = 0;
int epwm_prd = 5000, epwm_cmpa = 2500;
float B0, B1, B2, B3, B4, B5, B6, B7, B8, B9;
float sinevalue, ref_a=0, ref_b=0, ref_c=0;
float V_dc=800, V_ao=0, V_bo=0, V_co=0, v_no=0, v_La=0, v_Lb=0, v_Lc=0, v_La_pre=0, v_Lb_pre=0, v_Lc_pre=0;
float P_ref = 0, Q_ref = 0, omegal;
float Error_P=0, Error_Q=0, Error_Id=0, Error_Iq=0, Id_ref=0, Iq_ref=0, Ud=0, Uq=0;
float Error_P_pre=0, Error_Q_pre=0, Error_Id_pre=0, Error_Iq_pre=0;
float kp_p=2.08e-3, ki_p=0.1312, kp_q=2.08e-3, ki_q=0.1312, kp_I=199.52, ki_I=62.68;
float md, mq, ma, mb, mc;
Uint16 DAC1 = 0;
Uint16 DAC2 = 0;
float sim_time = 0.0f;

void main(void)
{
    InitSysCtrl();
    InitGpio();
    InitDaca();
    InitDacb();

    EALLOW;
    CpuSysRegs.PCLKCR2.bit.EPWM1=1;
    CpuSysRegs.PCLKCR2.bit.EPWM2=1;
    CpuSysRegs.PCLKCR2.bit.EPWM3=1;
    EDIS;

    InitEPwm1Gpio();
    InitEPwm2Gpio();
    InitEPwm3Gpio();

    EALLOW;
    ClkCfgRegs.PERCLKDIVSEL.bit.EPWMCLKDIV = 1;
    EDIS;

    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    EALLOW;
    PieVectTable.EPWM1_INT = &cpu_epwm1_isr;
    EDIS;

    IER |= M_INT3;
    PieCtrlRegs.PIEIER3.bit.INTx1 = 1;
    EINT;
    ERTM;

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 0;
    EDIS;

    InitEPwm1();
    InitEPwm2();
    InitEPwm3();

    EALLOW;
    CpuSysRegs.PCLKCR0.bit.TBCLKSYNC = 1;
    EDIS;
//initialising constants for BIlinear transformation
    B0 = (2 * PLL_kp + PLL_ki * Sampling_time)/2;
    B1 = (PLL_ki * Sampling_time - 2 * PLL_kp)/2;

    B2 = (2*L + r_L*Sampling_time)/Sampling_time;
    B3 = (r_L*Sampling_time - 2*L)/Sampling_time;

    B4 = (2 * kp_p + ki_p * Sampling_time)/2;
    B5 = (ki_p * Sampling_time - 2 * kp_p)/2;

    B6 = (2 * kp_q + ki_q * Sampling_time)/2;
    B7 = (ki_q * Sampling_time - 2 * kp_q)/2;

    B8 = (2 * kp_I + ki_I * Sampling_time)/2;
    B9 = (ki_I * Sampling_time - 2 * kp_I)/2;

    omegal = 2 * M_PI * Line_freq * L;

    while(1)
    {
    }
}
//inverter 3phase
void Inverter_3Ph(void)
{
    V_ao = V_dc*ref_a;
    V_bo = V_dc*ref_b;
    V_co = V_dc*ref_c;

    v_no = (V_ao + V_bo + V_co) / 3;

    v_La = V_ao - v_a - v_no;
    v_Lb = V_bo - v_b - v_no;
    v_Lc = V_co - v_c - v_no;

    i_a = (-B3*i_a + (v_La + v_La_pre)) / B2;
    i_b = (-B3*i_b + (v_Lb + v_Lb_pre)) / B2;
    i_c = (-B3*i_c + (v_Lc + v_Lc_pre)) / B2;

    v_La_pre = v_La;
    v_Lb_pre = v_Lb;
    v_Lc_pre = v_Lc;
}

void InitEPwm1()
{
    EPwm1Regs.TBPRD = epwm_prd;
    EPwm1Regs.TBPHS.bit.TBPHS = 0x0000;
    EPwm1Regs.TBCTR = 0x0000;
    EPwm1Regs.CMPA.bit.CMPA = epwm_cmpa;
    EPwm1Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm1Regs.TBCTL.bit.PHSEN = TB_DISABLE;
    EPwm1Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm1Regs.TBCTL.bit.CLKDIV = TB_DIV1;

    EPwm1Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm1Regs.AQCTLA.bit.CAD = AQ_SET;
    EPwm1Regs.AQCTLB.bit.CAU = AQ_SET;
    EPwm1Regs.AQCTLB.bit.CAD = AQ_CLEAR;

    EPwm1Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm1Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm1Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

    EPwm1Regs.TBCTL.bit.SYNCOSEL = 1;
    EPwm1Regs.ETSEL.bit.INTSEL = 0;
    EPwm1Regs.ETSEL.bit.INTEN = 1;
    EPwm1Regs.ETSEL.bit.INTSEL = 2;
    EPwm1Regs.ETPS.bit.INTPRD = 1;
    EPwm1Regs.ETCLR.bit.INT = 1;
}

void InitEPwm2(void)
{
    EPwm2Regs.TBPRD = epwm_prd;
    EPwm2Regs.TBPHS.bit.TBPHS = 0x0000;
    EPwm2Regs.TBCTR = 0x0000;
    EPwm2Regs.CMPA.bit.CMPA = 2500;
    EPwm2Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm2Regs.TBCTL.bit.PHSEN = TB_DISABLE;
    EPwm2Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm2Regs.TBCTL.bit.CLKDIV = TB_DIV1;

    EPwm2Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm2Regs.AQCTLA.bit.CAD = AQ_SET;
    EPwm2Regs.AQCTLB.bit.CAU = AQ_SET;
    EPwm2Regs.AQCTLB.bit.CAD = AQ_CLEAR;

    EPwm2Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm2Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm2Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm2Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;
    EPwm2Regs.TBCTL.bit.SYNCOSEL = 0;
}

void InitEPwm3(void)
{
    EPwm3Regs.TBPRD = epwm_prd;
    EPwm3Regs.TBPHS.bit.TBPHS = 0x0000;
    EPwm3Regs.TBCTR = 0x0000;
    EPwm3Regs.CMPA.bit.CMPA = 2500;
    EPwm3Regs.TBCTL.bit.CTRMODE = TB_COUNT_UPDOWN;
    EPwm3Regs.TBCTL.bit.PHSEN = TB_DISABLE;
    EPwm3Regs.TBCTL.bit.HSPCLKDIV = TB_DIV1;
    EPwm3Regs.TBCTL.bit.CLKDIV = TB_DIV1;

    EPwm3Regs.AQCTLA.bit.CAU = AQ_CLEAR;
    EPwm3Regs.AQCTLA.bit.CAD = AQ_SET;
    EPwm3Regs.AQCTLB.bit.CAU = AQ_SET;
    EPwm3Regs.AQCTLB.bit.CAD = AQ_CLEAR;

    EPwm3Regs.CMPCTL.bit.SHDWAMODE = CC_SHADOW;
    EPwm3Regs.CMPCTL.bit.SHDWBMODE = CC_SHADOW;
    EPwm3Regs.CMPCTL.bit.LOADAMODE = CC_CTR_ZERO;
    EPwm3Regs.CMPCTL.bit.LOADBMODE = CC_CTR_ZERO;

    EPwm3Regs.TBCTL.bit.SYNCOSEL = 0;
}

void InitDaca(void)
{
    EALLOW;
    DacaRegs.DACCTL.bit.DACREFSEL = 1;
    DacaRegs.DACCTL.bit.LOADMODE = 0;
    DacaRegs.DACOUTEN.bit.DACOUTEN = 1;
    DacaRegs.DACVALS.bit.DACVALS = 100;
    DELAY_US(10);
    EDIS;
}

void InitDacb(void)
{
    EALLOW;
    DacbRegs.DACCTL.bit.DACREFSEL = 1;
    DacbRegs.DACCTL.bit.LOADMODE = 0;
    DacbRegs.DACOUTEN.bit.DACOUTEN = 1;
    DacbRegs.DACVALS.bit.DACVALS = 100;
    DELAY_US(10);
    EDIS;
}

interrupt void cpu_epwm1_isr(void)
{
    PieCtrlRegs.PIEACK.all = PIEACK_GROUP3;

    sim_time += Sampling_time;
//grid fluctuations
    float phi = 0.0f;
    if (sim_time < 0.2f) {
        Line_freq = 50.0f;
        V_m = 325.0f;
    } else if (sim_time < 0.4f) {
        Line_freq = 50.0f + 10.0f * (sim_time - 0.2f);
        V_m = 325.0f;
    } else if (sim_time < 0.6f) {
        Line_freq = 100.0f;
        V_m = 325.0f * 0.85f;
    } else if (sim_time < 0.8f) {
        Line_freq = 50.0f;
        V_m = 325.0f;
        phi = 15.0f * M_PI / 180.0f;
    } else if (sim_time < 1.0f) {
        Line_freq = 48.0f;
        V_m = 325.0f;
        phi = 0.0f;
    } else {
        Line_freq = 50.0f;
        V_m = 325.0f;
        phi = 0.0f;
    }
//inverter fluctuations
    if (sim_time < 0.1f) {
        V_dc = 700.0f;
    } else if (sim_time < 0.3f) {
        V_dc = 730.0f;
    } else if (sim_time < 0.5f) {
        V_dc = 730.0f + 350.0f * (sim_time - 0.3f);
    } else if (sim_time < 0.7f) {
        V_dc = 640.0f;
    } else if (sim_time < 0.9f) {
        V_dc = 640.0f + 300.0f * (sim_time - 0.7f);
    } else if (sim_time < 1.1f) {
        V_dc = 670.0f;
    } else if (sim_time < 1.3f) {
        V_dc = 750.0f;
    } else {
        V_dc = 700.0f;
    }
    sim_time += Sampling_time;

    if(sim_time >= 1.4f){
        sim_time = 0.0f;}

    omegal = 2.0f * M_PI * Line_freq * L;

    theta += 2.0f * M_PI * Line_freq * Sampling_time;
    if (theta >= 2.0f * M_PI) theta -= 2.0f * M_PI;
//updating grid parameters
    float theta1 = theta + phi;
    v_a = V_m * __sin(theta1);
    v_b = V_m * __sin(theta1 - 2.0f * M_PI / 3.0f);
    v_c = V_m * __sin(theta1 - 4.0f * M_PI / 3.0f);

    if (sim_time >= 1.0f && sim_time < 1.2f) {
        float h = 0.05f * V_m;
        v_a += h * __sin(5.0f * theta1);
        v_b += h * __sin(5.0f * (theta1 - 2.0f * M_PI / 3.0f));
        v_c += h * __sin(5.0f * (theta1 - 4.0f * M_PI / 3.0f));
    }
//update inverter currents based on new 
    Inverter_3Ph();

    Vd = (2.0f/3.0f) * (__sin(PLL_theta)*v_a + __sin(PLL_theta - 2*M_PI/3)*v_b + __sin(PLL_theta - 4*M_PI/3)*v_c);
    Vq = (2.0f/3.0f) * (__cos(PLL_theta)*v_a + __cos(PLL_theta - 2*M_PI/3)*v_b + __cos(PLL_theta - 4*M_PI/3)*v_c);

    del_f = del_f_pre + B0 * Vq + B1 * v_q_pre;
    v_q_pre = Vq;
    del_f_pre = del_f;
    del_f_er = del_f + Line_freq;

    PLL_theta1 = (M_PI * Sampling_time) * (del_f_er + del_f_er_pre) + PLL_theta1_pre;
    del_f_er_pre = del_f_er;

    PLL_theta = PLL_theta1;
    while (PLL_theta >= 2.0f * M_PI) PLL_theta -= 2.0f * M_PI;
    while (PLL_theta < 0.0f)          PLL_theta += 2.0f * M_PI;
    PLL_theta1_pre = PLL_theta1;

    Id = (2.0f/3.0f) * (__sin(PLL_theta)*i_a + __sin(PLL_theta - 2*M_PI/3)*i_b + __sin(PLL_theta - 4*M_PI/3)*i_c);
    Iq = (2.0f/3.0f) * (__cos(PLL_theta)*i_a + __cos(PLL_theta - 2*M_PI/3)*i_b + __cos(PLL_theta - 4*M_PI/3)*i_c);
//current loop with power 
    Pdq = 1.5f * Vd * Id;
    Qdq = 1.5f * Vd * Iq;

    Error_P = P_ref - Pdq;
    Id_ref += B4 * Error_P + B5 * Error_P_pre;
    Error_P_pre = Error_P;

    Error_Q = Q_ref - Qdq;
    Iq_ref += B6 * Error_Q + B7 * Error_Q_pre;
    Error_Q_pre = Error_Q;

    Error_Id = Id_ref - Id;
    Ud += B8 * Error_Id + B9 * Error_Id_pre;
    Error_Id_pre = Error_Id;

    Error_Iq = Iq_ref - Iq;
    Uq += B8 * Error_Iq + B9 * Error_Iq_pre;
    Error_Iq_pre = Error_Iq;

    md = ((Ud - Iq * omegal) + Vd) * 2.0f / V_dc;
    mq = ((Uq + Id * omegal) + Vq) * 2.0f / V_dc;

    ma = __sin(PLL_theta) * md + __cos(PLL_theta) * mq;
    mb = __sin(PLL_theta - 2*M_PI/3) * md + __cos(PLL_theta - 2*M_PI/3) * mq;
    mc = __sin(PLL_theta - 4*M_PI/3) * md + __cos(PLL_theta - 4*M_PI/3) * mq;

    ref_a = (ma + 1.0f) / 2.0f;
    ref_b = (mb + 1.0f) / 2.0f;
    ref_c = (mc + 1.0f) / 2.0f;
//limit from 0 to 1
    if (ref_a > 1.0f) ref_a = 1.0f; if (ref_a < 0.0f) ref_a = 0.0f;
    if (ref_b > 1.0f) ref_b = 1.0f; if (ref_b < 0.0f) ref_b = 0.0f;
    if (ref_c > 1.0f) ref_c = 1.0f; if (ref_c < 0.0f) ref_c = 0.0f;

    EPwm1Regs.CMPA.bit.CMPA = (Uint16)(ref_a * epwm_prd);
    EPwm2Regs.CMPA.bit.CMPA = (Uint16)(ref_b * epwm_prd);
    EPwm3Regs.CMPA.bit.CMPA = (Uint16)(ref_c * epwm_prd);
//DAC for Oscilloscope reading
    DAC1 = (Uint16)(((v_a + 325.0f)/(2.0f*325.0f))*4095.0f);
    DAC1 = (Uint16)((PLL_theta/(2*M_PI))*4095);
    DacaRegs.DACVALS.bit.DACVALS = DAC1;
    DacbRegs.DACVALS.bit.DACVALS = DAC2;

    EPwm1Regs.ETCLR.bit.INT = 1;
}