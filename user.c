/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

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
#include <stdbool.h>         /* For true/false definition                     */
#include "user.h"            /* variables/params used by user.c               */
#include <i2c.h>
#include "system.h"
#include "srf02.h"
#include "pwm_lib.h"
#include "i2c_lib.h"
#include "adc_lib.h"

#include <pps.h>            /* Useful for peripheral pin macros                     */
                            /* http://singularengineer.com/peripheral-pin-select-pps-using-xc16-and-mplab-x/  */
//the follwing two are somehow not defined in pps.h, so are defined here
#define IN_FN_PPS_IC5				RPINR9bits.IC5R		/* Assign Input Capture 5 (IC5) to the corresponding RPn pin*/
#define IN_FN_PPS_IC6				RPINR9bits.IC6R		/* Assign Input Capture 6 (IC6) to the corresponding RPn pin*/

/******************************************************************************/
/* User Functions                                                             */
/******************************************************************************/

/* <Initialize variables in user.h and insert code for user algorithms.> */

#define I2C_FREQ 70000L
#define I2C_CONFIG2 (((1/I2C_FREQ) * FCY) - 2)

#define BRGVAL(x) ((FCY)/(4*x))

void InitApp(void)
{
    /* TODO Initialize User Ports/Peripherals/Project here */
    TRISBbits.TRISB0 = 0;
    TRISBbits.TRISB1 = 0;
    
    //TRISBbits.TRISB4 = 1;
    //TRISEbits.TRISE2 = 1;
    LATBbits.LATB0 = 1;
    LATBbits.LATB1 = 1;
    /* Setup analog functionality and port direction */

    __builtin_write_OSCCONL(OSCCON & ~(1<<6)); 
    //__builtin_write_OSCCONL(OSCCON & ~(1<<1));
    /* Initialize peripherals */
    // UART enabled, 8N1, BRGH
    U1MODE = 0b1000000000001000;//(1 << 15) | (1 << 3);
    // TX interrupt, RX enabled, TX enabled
    U1STA = 0b1000010000000000;//(1 << 15) | (1 << 12) | (1 << 10);
    U1BRG = 911;//BRGVAL(19200); // ((7.37/2)*10^6)/(4*19200)
    RPINR18bits.U1RXR = 75; // Link rx pin
    RPOR0bits.RP65R = 1; // Link tx pin
    
    // UART 2
    // UART enabled, 8N1, BRGH
    U2MODE = 0b1000000000001000;//(1 << 15) | (1 << 3);
    // TX interrupt, RX enabled, TX enabled
    U2STA = 0b1000010000000000;//(1 << 15) | (1 << 12) | (1 << 10);
    U2BRG = 1822;//BRGVAL(38400); // ((7.37/2)*10^6)/(4*19200)
    RPINR19bits.U2RXR = 83; // Link rx pin
    //RPOR0bits.RP64R = 1; // Link tx pin
    //this sets which pins are on the ADC
    ANSELE = 0;
    ANSELB = 0;
    ANSELC = 0;
    ANSELG = 0;
    ANSELD = 0;
    //see IO ports in Pic Spec
    ANSELBbits.ANSB4 = 1; // SP4
    ANSELEbits.ANSE2 = 1; // SD5
    
    // Setup adc for arm potentiometers
    setupADC1();
     
    // Timer setup
    // URX receive timeout
    T1CON = 0b1000000000000000;
    TMR1 = 0;
    T1CONbits.TCKPS = 0b11;
    PR1 = 6400; // ~24ms 
    
    // PWM setup
    pwm_init_p17();
    pwm_init_p21();
    pwm_init_p15();
    pwm_init_p3();
    pwm_init_p42();
    pwm_init_p2();
    pwm_init_p4();
    pwm_init_p5();
    pwm_init_p16();
    pwm_init_p13();
    pwm_init_p24(); // Initialise pwm for lidar tilt
    
    // Configure Timer 2 (default timer for output compare)
    // For the hardware pwm dummy
    PR2 = 5469;             // Timer 2 period (20ms)
    T2CONbits.TCKPS = 0b11; // Timer 2 prescaler 1:64
    T2CONbits.TON = 1;      // Enable Timer 2
    
    IFS0bits.U1TXIF = 0;
    IFS0bits.U1RXIF = 0;
    IEC0bits.U1RXIE = 1;
    IPC2bits.U1RXIP = 6;
    IEC0bits.U1TXIE = 1;
    IPC3bits.U1TXIP = 3;
    
    IFS1bits.U2TXIF = 0;
    IFS1bits.U2RXIF = 0;
    IEC1bits.U2RXIE = 1;
    IPC7bits.U2RXIP = 6;
    IEC1bits.U2TXIE = 1;
    IPC7bits.U2TXIP = 3;
    
    IEC0bits.T1IE = 1;
    IPC0bits.T1IP = 3;
    
    // Setup I2C
    /* Baud rate is set for 100 kHz */
    unsigned int config2 = I2C_CONFIG2;
    // Configure I2C for 7 bit address mode
    unsigned int config1 = (I2C1_ON & I2C1_IDLE_CON & I2C1_CLK_HLD &
               I2C1_IPMI_DIS & I2C1_7BIT_ADD &
               I2C1_SLW_DIS & I2C1_SM_DIS &
               I2C1_GCALL_DIS & I2C1_STR_DIS &
               I2C1_NACK & I2C1_ACK_DIS & I2C1_RCV_DIS &
               I2C1_STOP_DIS & I2C1_RESTART_DIS &
               I2C1_START_DIS);
    OpenI2C1(config1,config2);
    IdleI2C1();
    __builtin_write_OSCCONL(OSCCON | (1<<6));  
}


/*
    // **** Encoders using Quadrature Encoder Interface (QEI) **** //
    // ****    This is for testing purposes for IC code       **** //
    // ****        See QEI in family reference manual         **** //
void InitQEI(void) {
    RPINR14 = QEI_INPUT_B & QEI_INPUT_A; // Configure QEI pins as digital inputs
    QEICONbits.QEIM = 0; // Disable QEI Module
    QEICONbits.CNTERR = 0; // Clear any count errors
    QEICONbits.QEISIDL = 0; // Continue operation during sleep
    QEICONbits.SWPAB = 0; // QEA and QEB not swapped
    QEICONbits.PCDOUT = 0; // Normal I/O pin operation
    QEICONbits.POSRES = 1; // Index pulse resets position counter
    DFLTCONbits.CEID = 1; // Count error interrupts disabled
    DFLTCONbits.QEOUT = 1; // Digital filters output enabled for QEn pins
    DFLTCONbits.QECK = 5; // 1:64 clock divide for digital filter for QEn
    DFLTCONbits.INDOUT = 1; // Digital filter output enabled for Index pin
    DFLTCONbits.INDCK = 5; // 1:64 clock divide for digital filter for Index
    POSCNT = 0; // Reset position counter
    QEICONbits.QEIM = 6; // X4 mode with position counter reset by Index
    return;
}
*/


    // **** Initialisation for all encoders done here **** //
    // Added on: 11/02/2016 by Simon Ireland

void InitEncoders(void) {
    //Ensure Channel A & B ports are inputs:
/*    TRISB |= B_ENCODER_BITS; // If I recall, mapping to IC, done below, does this but
    TRISF |= F_ENCODER_BITS; // best practice is to be certain
    
    //Ensure that the pins are set up for DIGITAL input, not analog, only needed for port B
    ANSELB &= 0x00FF; // Clear bits 8-15 inclusive    
    
    
    // *** Initialise Input Capture Modules and Timer_5 *** //
    
    
    //Encoder 0 Initialisation
    
    IPC0bits.IC1IP = ENC_PRIORITY; // Set interrupt priority
    IFS0bits.IC1IF = 0; // Clear interrupt flag
    IEC0bits.IC1IE = 1; // Enable IC Interrupts
    
    IC1CON1bits.ICSIDL = 0; //Continue to run Input capture in cpu idle mode
    IC1CON1bits.ICM = 0b000; // Disable the input capture module and clear buffer
    
    PPSUnLock;                                  // Remap the input to the input capture module
    PPSInput( IN_FN_PPS_IC1, IN_PIN_PPS_RPI40); // Need to unlock before changing the mapping
    PPSLock;                                    //
    
    IC1CON1bits.ICTSEL = TMR_5; // Select the timer to use, all input captures shall work off timer 5 (no need for timer interrupt)
    IC1CON1bits.ICI = 0;  // Interrupt on every capture event
    
    IC1CON2 = 0b0000000000000000; // Ensure control register 2 is clear
    
    IC1CON1bits.ICM = 0b100; // Reenable IC to capture every 4th rising edge (Prescaler Capture mode)
    
    
    
    //Encoder 1 Initialisation
    IPC1bits.IC2IP = ENC_PRIORITY;
    IFS0bits.IC2IF = 0;
    IEC0bits.IC2IE = 1;
    
    IC2CON1bits.ICSIDL = 0;
    IC2CON1bits.ICM = 0b000;
    
    PPSUnLock;
    PPSInput( IN_FN_PPS_IC2, IN_PIN_PPS_RPI42);
    PPSLock;
    
    IC2CON1bits.ICTSEL = TMR_5;
    IC2CON1bits.ICI = 0;
    
    IC2CON2 = 0b0000000000000000;
    
    IC2CON1bits.ICM = 0b100;
    

    
    
    //Encoder 2 Initialisation
    IPC9bits.IC3IP = ENC_PRIORITY;
    IFS2bits.IC3IF = 0;
    IEC2bits.IC3IE = 1;
    
    IC3CON1bits.ICSIDL = 0;
    IC3CON1bits.ICM = 0b000;
    
    PPSUnLock;
    PPSInput( IN_FN_PPS_IC3, IN_PIN_PPS_RPI44);
    PPSLock;
    
    IC3CON1bits.ICTSEL = TMR_5;
    IC3CON1bits.ICI = 0;
    
    IC3CON2 = 0b0000000000000000;
    
    IC3CON1bits.ICM = 0b100;
    

    
    //Encoder 3 Initialisation
    IPC9bits.IC4IP = ENC_PRIORITY;
    IFS2bits.IC4IF = 0;
    IEC2bits.IC4IE = 1;
    
    IC4CON1bits.ICSIDL = 0;
    IC4CON1bits.ICM = 0b000;
    
    PPSUnLock;
    PPSInput( IN_FN_PPS_IC4, IN_PIN_PPS_RPI46);
    PPSLock;
    
    IC4CON1bits.ICTSEL = TMR_5;
    IC4CON1bits.ICI = 0;
    
    IC1CON2 = 0b0000000000000000;
    
    IC4CON1bits.ICM = 0b100;
    

    
    //Encoder 4 Initialisation
    IPC9bits.IC5IP = ENC_PRIORITY;
    IFS2bits.IC5IF = 0;
    IEC2bits.IC5IE = 1;
    
    IC5CON1bits.ICSIDL = 0;
    IC5CON1bits.ICM = 0b000;
    
    PPSUnLock;
    PPSInput( IN_FN_PPS_IC5, IN_PIN_PPS_RP100);
    PPSLock;
    //RPINR9bits.IC5R = ENC4_IC;
    
    IC5CON1bits.ICTSEL = TMR_5;
    IC5CON1bits.ICI = 0;
    
    IC5CON2 = 0b0000000000000000;
    
    IC5CON1bits.ICM = 0b100;
    

    
    //Encoder 5 Initialisation
    IPC10bits.IC6IP = ENC_PRIORITY;
    IFS2bits.IC6IF = 0;
    IEC2bits.IC6IE = 1;
    
    IC6CON1bits.ICSIDL = 0;
    IC6CON1bits.ICM = 0b000;
    
    PPSUnLock;
    PPSInput( IN_FN_PPS_IC6, IN_PIN_PPS_RP98);
    PPSLock;
    
    IC6CON1bits.ICTSEL = TMR_5;
    IC6CON1bits.ICI = 0;
    
    IC6CON2 = 0b0000000000000000;
    
    IC6CON1bits.ICM = 0b100;
    
    
    
    //Timer 5 Initialisation
    T5CONbits.TON = 0; // Disable Timer
    T5CONbits.TCS = 0; // Internal instruction cylce clock
    T5CONbits.TGATE = 0; // Disable gated timer mode
    
    T5CONbits.TCKPS = 0b11;   // prescaler 1:256
    TMR5 = 0; //Clear timer_9 register
    PR5 = TIMER_5_PERIOD;
    
    IPC7bits.T5IP = 3;  // Set interrupt priority
    IFS1bits.T5IF = 0;  // Clear interupt flag
    IEC1bits.T5IE = 1;  // Enable the interrupt
    
    T5CONbits.TON = 1; //Starts Timer_9 (Timerx On bit)
*/
}

