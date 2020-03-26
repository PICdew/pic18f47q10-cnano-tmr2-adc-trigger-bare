/**
    (c) 2020 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#pragma config WDTE = OFF               /* WDT operating mode->WDT Disabled */ 
#pragma config LVP = ON                 /* Low voltage programming enabled, RE3 pin is MCLR */ 

#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#define Timer2Period        0x2F        /* TMR2 Period is 100ms */
#define DesiredThreshold    300         /* Desired threshold value */
#define AnalogChannel       0x00        /* Use ANA0 as input for ADCC */

volatile uint16_t adcVal;

static void CLK_init(void);
static void PORT_init(void);
static void ADCC_init(void);
static void TMR2_init(void);
static void INTERRUPT_init(void);
static uint16_t ADCC_readValue(uint8_t);
static void ADCC_interrupt(void);

static void CLK_init(void)
{
    /* set HFINTOSC Oscillator */  
    OSCCON1 = _OSCCON1_NOSC1_MASK | _OSCCON1_NOSC2_MASK;
    /* set HFFRQ to 1 MHz */
    OSCFRQ = ~_OSCFREQ_HFFRQ_MASK;
}

static void PORT_init(void)
{
    /* Set RA0 pin as analog */
    ANSELA |= _ANSELA_ANSELA0_MASK;
    /* Set RA0 pin as input  */
    TRISA |= _TRISA_TRISA0_MASK;
    /* Set RE0 pin as output */
    TRISE &= ~_TRISE_TRISE0_MASK;
}

static void ADCC_init(void)
{
    /* ADACT Auto-Conversion Trigger Source is TMR2 */ 
    ADACT |= _ADACT_ADACT2_MASK;
    /* ADGO stop; ADFM right; ADON enabled; ADCONT disabled; ADCS FRC */
    ADCON0 = _ADCON0_ADON_MASK     /* Enable ADCC module */
           | _ADCON0_ADCS_MASK     /* Select FRC clock */
           | _ADCON0_ADFM_MASK;    /* Result right justified */
    /* Clear the ADCC interrupt flag */
    PIR1 &= ~_PIR1_ADIF_MASK;    
    /* Enabling ADCC interrupt flag */
    PIE1 |= _PIE1_ADIE_MASK;
}

static void TMR2_init(void)
{
    /* TMR2 Clock source, LFINTOSC (00100) has 31 kHz */
    T2CLKCON |= _T2CLKCON_CS2_MASK;
    /* T2PSYNC Not Synchronized; T2MODE Starts at T2ON = 1 and TMR2_ers = 0; T2CKPOL Rising Edge */
    T2HLT = 0; 
    /* TMR2ON on; T2CKPS Prescaler 1:64; T2OUTPS Postscaler 1:1 
       Minimum timer period is 31 kHz/64 = 2.064516 ms  */
    T2CON |= _T0CON1_T0CS_MASK;
    /* Set TMR2 period, PR2 to 100ms */
    T2PR = Timer2Period;
    /* Clear the TMR2 interrupt flag */
    PIR4 &= ~_PIR4_TMR2IF_MASK;
}

static void INTERRUPT_init(void)
{
    INTCON = _INTCON_GIE_MASK           /* Enable Global Interrupts */
           | _INTCON_PEIE_MASK;         /* Enable Peripheral Interrupts */   
}

static uint16_t ADCC_readValue(uint8_t channel)
{   
    ADPCH = channel;     /* Set the input channel for ADCC */
    /* TMR2 is trigger source for auto-conversion for ADCC */
    return ((uint16_t)((ADRESH << 8) + ADRESL));
}

static void __interrupt() INTERRUPT_InterruptManager(void)
{
    if (INTCON & _INTCON_PEIE_MASK)
    {
        if ((PIE1 & _PIE1_ADIE_MASK) && (PIR1 & _PIR1_ADIF_MASK))
        {
            ADCC_interrupt();
        } 
    }
}

static void ADCC_interrupt(void)
{
    /* Clear the ADCC interrupt flag */
    PIR1 &= ~_PIR1_ADIF_MASK;
    /* Toggle LED0 at the Timer2Period frequency */
    LATE ^= _LATE_LATE0_MASK;
    /* Get the conversion result from ADCC AnalogChannel */
    adcVal = ADCC_readValue(AnalogChannel);
}

void main(void)
{
    /* Initialize the device */
    CLK_init();             /* Oscillator init function */
    PORT_init();            /* Port init function */
    ADCC_init();            /* ADCC init function */
    TMR2_init();            /* TMR2 init function */
    INTERRUPT_init();       /* Interrupt init function */

    while (1)
    {
        if (adcVal > DesiredThreshold)
        {
            /* turn LED0 ON by writing pin RE0 to low */
            LATE &= ~_LATE_LATE0_MASK;
        }
    }
}

/**
 End of File
*/