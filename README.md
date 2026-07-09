# IIT Dharwad Internship Project

Overview of internship work from June 8 to July 9, 2026. Project focuses on three-phase grid-tied inverter design, Phase-Locked Loop (PLL) synchronization, and C2000 DSP microcontrollers.

## Folder Navigation

- **animation/**: Standalone interactive web animations demonstrating PLL behavior.
can also be accessed from <https://lonepo.github.io/blog/pll-visual.html>
- **References/**: Reference literature, books, and datasheets (ignored in Git).
- **Screenshots/**: Waveforms, test results, and simulation screenshots.
- **IIT_Internship.pdf / .pptx**: Final presentation summarizing the internship.
- **Week1_Progress_Latex.pdf**: Week 1 progress report.
- **PLL_inv_final.slx**: MATLAB/Simulink model of the PLL-based inverter system.
- **threephase_inverter_final.slx**: MATLAB/Simulink model of the three-phase inverter.
- **PLL_values.m**: MATLAB script containing calculations and parameter values for the PLL.
- **c2000projects**: All C2000 project files(PLL output of DAC comes in ADCINA0 and A1 physical pins 30 and 70)

## Hardware Summary

- **Controller**: Texas Instruments C2000 TMS320F28379D LaunchPad.
- **Power Stage**: Tolarix IGBT half-bridge modules.

## Weekly Progress (June 8 - July 9)

### Week 1 (June 8 - June 15)

- Conducted project overview and literature survey on PLL techniques.
- Modeled simple three-phase inverter in MATLAB/Simulink.
- Tested different PWM generation techniques, including sinusoidal PWM (SPWM) and complementary PWM with dead-band.
- Verified PWM generation outputs on oscilloscope.

### Week 2 (June 16 - June 23)

- Performed tuning and calculations of PI controller values for the PLL.
- Run iterations of PLL simulation under grid disturbances.
- Calculated LCL filter component specifications based on system specifications.

### Week 3 (June 24 - June 30)

- Explored C2000 DSP microcontroller hardware architecture using NPTEL lectures (Prof. Narasamma).
- Implemented C2000 program to output complementary SPWM with dead-band.
- Set up interface between C2000 microcontroller and Tolarix IGBT power stage.

### Week 4 (July 1 - July 9)

- Replicated full grid-tied inverter plant model in C2000 microcontroller code.
- Configured DAC pins to output inverter waveforms for oscilloscope monitoring.
- Prepared final project presentation slides and summarized experimental results.

## References

## Digital Controller for Power Applications

### Inverter Control and PLL Theory

- Bhim Singh, Ambrish Chandra, and Kamal Al-Haddad. *Power Quality Problems and Mitigation Techniques*. John Wiley & Sons, 2015.
- Xinbo Ruan, Xuehua Wang, Donghua Pan, Dongsheng Yang, Weiwei Li, and Chenlei Bao. *Control Techniques for LCL-Type Grid-Connected Inverters*. Springer, 2018.
- L. Umanand. *NPTEL Course on Power Electronics*.
- Wikipedia. *Direct-quadrature-zero transformation*. <https://en.wikipedia.org/wiki/Direct-quadrature-zero_transformation>

### Hardware Manuals

- Texas Instruments. *TMS320F28379D Real-Time Microcontrollers Technical Reference Manual*.
- Tolarix IGBT Half-Bridge Module Datasheets.

Done by:
Gautam
Under the guidance of Prof. Abhijit Kshirsagar
with help from Renuka Varma and Pradeep Kumar M
and Vivek Kumble
