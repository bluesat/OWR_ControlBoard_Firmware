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


#include <stdint.h>        /* Includes uint16_t definition                    */
#include <stdbool.h>       /* Includes true/false definition                  */
#include <stdlib.h>
#include <string.h>

#include "system.h"        /* System funct/params, like osc/peripheral config */
#include "user.h"          /* User funct/params, such as InitApp              */
#include "serial.h"
#include "HMC5883L.h"
#include "srf02.h"
#include "MPU6050.h"
#include "pwm_lib.h"
#include "message.h"

/******************************************************************************/
/* Global Variable Declaration                                                */
/******************************************************************************/

/* i.e. uint16_t <variable_name>; */

/******************************************************************************/
/* Main Program                                                               */
/******************************************************************************/

int16_t main(void)
{

    /* Configure the oscillator for the device */
    ConfigureOscillator();

    /* Initialize IO ports and peripherals */
    InitApp();

    struct toControlMsg *msg;
    struct toNUCMsg sendMsg;
    GPSData gpsData;
    char *gpsString;
    //init_mpu();
    init_hmc();
    while(1)
    {
        if (msg = recieveMessage()) {
            pwm_set_p17(msg->lSpeed);
            pwm_set_p21(msg->lSpeed);
            pwm_set_p15(msg->lSpeed);
            pwm_set_p12(msg->rSpeed);
            pwm_set_p42(msg->rSpeed);
            pwm_set_p10(msg->rSpeed);
            
            pwm_set_p7(msg->armRotate);
            pwm_set_p19(msg->armTop);
            pwm_set_p25(msg->armBottom);
            pwm_set_p16(msg->clawRotate); // colorful
            pwm_set_p13(msg->clawGrip); // white and red
            
            pwm_set_p2(msg->cameraBottomRotate);
            pwm_set_p3(msg->cameraBottomTilt);
            pwm_set_p4(msg->cameraTopRotate);
            pwm_set_p5(msg->cameraTopTilt);
            
            //Set lidar tilt pwm
            pwm_set_p24(msg->lidarTilt);
            
            AD1CON1bits.SAMP = 0;
            while (!AD1CON1bits.DONE);
            AD1CON1bits.DONE = 0;
            
            AD2CON1bits.SAMP = 0;
            while (!AD2CON1bits.DONE);
            AD2CON1bits.DONE = 0;
            
            int s = ADC1BUF2;
            sendMsg.magic = MESSAGE_MAGIC;
            sendMsg.vbat = ADC1BUF0;
            sendMsg.gpsData = gpsData;
            sendMsg.magData = read_hmc();
            
            sendMsg.armLower = ADC2BUF0;
            sendMsg.armHigher = ADC2BUF1;
            
            //sendMsg.imuData = read_mpu();              
            sendMessage(&sendMsg);
        }
        if (gpsString = recieveGPS()) {
            // We need to check if the gps has no fix
            if (gpsString[0] != ',') {
                gpsData.fixValid = 0;
                gpsData.altitude = 0;
                gpsData.latitude = 0;
                gpsData.longitude = 0;
                gpsData.numSatelites = 0;
                gpsData.time = 0;
                continue; // No fix, skip parsing data
            }
            char *tok = strtok(gpsString, ",");
            if (tok != NULL) {
                // Time of fix
                gpsData.time = atoi(tok);
                tok = strtok(NULL, ",");
            }
            if (tok != NULL) {
                // Latitude
                gpsData.latitude = atof(tok) * 10000;
                tok = strtok(NULL, ",");
            }
            if (tok != NULL) {
                // Latitude N or S
                if (tok[0] == 'S') {
                    gpsData.latitude *= -1;
                }
                tok = strtok(NULL, ",");
            }
            if (tok != NULL) {
                // Longitude
                gpsData.longitude = atof(tok) * 10000;
                tok = strtok(NULL, ",");
            }
            if (tok != NULL) {
                // Longitude E or W
                if (tok[0] == 'E') {
                    gpsData.longitude *= -1;
                }
                tok = strtok(NULL, ",");
            }
            if (tok != NULL) {
                // Fix quality
                gpsData.fixValid = atoi(tok);
                tok = strtok(NULL, ",");
            }
            if (tok != NULL) {
                // Number of satellites
                gpsData.numSatelites = atoi(tok);
                tok = strtok(NULL, ",");
            }
            if (tok != NULL) {
                // Horizontal dilution of position
                // Not used
                tok = strtok(NULL, ",");
            }
            if (tok != NULL) {
                // Number of satellites
                gpsData.altitude = atof(tok);
                tok = strtok(NULL, ",");
            }
            // Rest of the data we don't care about
        }
        //uint16_t data = read_srf02(SRF02_DEFAULT_ADDR);
    }
}
