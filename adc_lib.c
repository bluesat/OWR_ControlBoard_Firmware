/*
 * Date Started: 03/02/2016
 * Original Author: Harry J.E Day <harry@dayfamilyweb.com>
 * Editors: Simon Ireland (5/3/2016)) <simon.p.ireland@gmail.com>
 * Purpose: Provide the functions for using the ADC
 */

/* Device header file */
#if defined(__XC16__)
    #include <xc.h>
#elif defined(__C30__)
    #if defined(__dsPIC33E__)
    	#include <p33Exxxx.h>
    #elif defined(__dsPIC33F__)
    	#include <p33Fxxxx.h>
    #endif
#endif
#include <stdint.h>          /* For uint16_t definition                       */
#include <stdbool.h>         /* For true/false definition  */
#include "system.h"
#include "adc_lib.h"
//#include "srf02.h"


bool adc_ready = 0;

// ** Code to setup adc for reading potentiometers          ** //
// ** Uses the input scan select system to allow reading    ** //
// ** of multiple analog inputs within a single module      ** //

void setupADC1(void) {
    
    // Set appropriate pins as inputs (to read from the pots)
    TRISBbits.TRISB8 = 1;
    TRISBbits.TRISB10 = 1;
    TRISBbits.TRISB12 = 1;
    TRISBbits.TRISB15 = 1;
    TRISEbits.TRISE0 = 1; 
    TRISEbits.TRISE1 = 1;
    TRISEbits.TRISE2 = 1;
    TRISEbits.TRISE3 = 1;
    
    // Setup the 2 potentiometers connected to RE0 and RE1
    ANSELBbits.ANSB8 = 1;
    ANSELBbits.ANSB10 = 1;
    ANSELBbits.ANSB12 = 1;
    ANSELBbits.ANSB15 = 1;
    ANSELEbits.ANSE0 = 1;
    ANSELEbits.ANSE1 = 1;
    ANSELEbits.ANSE2 = 1;
    ANSELEbits.ANSE3 = 1;
    
    //setup register for arm feedback.
    //B10 for analog input
    TRISBbits.TRISB10 = 1;
    ANSELBbits.ANSB10 = 1;
    
    // Set the control registers to zero, these contain garbage after a reset
    // This also ensures the ADC module is OFF
    AD1CON1 = 0;
    AD1CON2 = 0;
    AD1CON3 = 0;
    
    // clear ADC1 control registers: CON4, CHS0, CHS123 and CHSSH/L
    AD1CON4 = 0;
    AD1CHS0 = 0;
    AD1CHS123 = 0;
    AD1CSSH = 0;
    AD1CSSL = 0;
    
    AD1CON1bits.AD12B = 1; // Activate 12 bit adc.
    
    // *** CLOCK SETTINGS *** //
    
    //Changes the ADC module clock period for both conversion ad sampling.
    
    // Tad must be greater than 117.6 ns (electrical specs, pg560), T_CY is 1/70Mhz 
    // Tad < T_CY * (ADCS + 1)
    // Tad/T_CY - 1 < ADCS
    // ADCS > (117.6*10^-9)*(70*10^6) - 1
    // ADCS > 7.232 ~ 8
    
    AD1CON3bits.ADCS = 0x0F; // T_AD = T_CY * (ADCS + 1)
    AD1CON3bits.SAMC = 0x1F; // Sampling for TAD * 14 (as required for 12-bit)
    
    // Auto-sampling, automatically end sampling and begin conversion
    AD1CON1bits.SSRC = 0b111;
    
    
    
    // Select the pins that will be cycled through via input scan select
    // NOTE: The ADC scans in ascending order of analog number, i.e.
    // if connecting an4, 9, 5, 12 -> the buffer will be filled:
    // 4, 5, 9, 12. Ensure any changes enforce this convention!
    
    AD1CON2bits.CSCNA = 1; // Activate channel scan select
    
    AD1CSSLbits.CSS8 =  1;
    AD1CSSLbits.CSS10 = 1;
    AD1CSSLbits.CSS12 = 1;
    AD1CSSLbits.CSS15 = 1;
    AD1CSSHbits.CSS24 = 1;
    AD1CSSHbits.CSS25 = 1;
    AD1CSSHbits.CSS26 = 1;
    AD1CSSHbits.CSS27 = 1;
    
    
    // Will need to interrupt after (N-1) sample/conversion sequences.
    // Where N = number of signals being read (e.g. an16 & an24 = 2 signals => SMPI = 1)
    AD1CON2bits.SMPI = 7; //interrupt on sample conversion
    
    //automatically begin sampling whenever last conversion finishes, SAMP bit will be set automatically
    AD1CON1bits.ASAM = 1;
    
    // Clear interupt flag, set priority
    _AD1IF = 0;
    _AD1IP = 3; 
    // Enable the interupt
    _AD1IE = 1; 
    
    //enable ADC1
    AD1CON1bits.ADON = 1;
    
}

// ADC interrupt serve routine (ISR). This sets a variable so that main
// knows that a conversion has finished and can read from buffer.
void __attribute__((__interrupt__, no_auto_psv)) _AD1Interrupt(void) { 
    _AD1IF = 0;
    adc_ready = 1;
}