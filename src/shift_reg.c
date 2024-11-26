#include "stm32g4xx.h"
#include "stm32g441xx.h" // Suggestions
#include "stdint.h"
#include "pinout.h"
#include "delay.h"
#include "shift_reg.h"
#include "structs.h"

struct shift_reg_srtuct shift_reg;

void packDataToRegisters(void){
    unsigned char temp1, temp2;
    shift_reg.LE1 = 0; // init latches
    shift_reg.LE2 = 0;

    temp1 = shift_reg.Register1; //save last Reg. value
    shift_reg.Register1 = 0;
    shift_reg.Register1 |= (shift_reg.PLL_SA_RF_EN) |
                           (shift_reg.PLL_SA_LE << 1) |
                           (shift_reg.PLL_SA_DATA << 2) |
                           (shift_reg.PLL_SA_CLK << 3) |
                           (shift_reg.Att_SA_LE << 4) |
                           (shift_reg.Att_SA_CLK << 5) |
                           (shift_reg.Att_SA_DATA << 6);
    if(temp1 != shift_reg.Register1){
        shift_reg.LE1 = 1;   // data change for reg1
    }

    temp1 = shift_reg.Register2; //save last Reg. value
    shift_reg.Register2 = 0;
    shift_reg.Register2 |= (shift_reg.LED_Xtal_SEL) |
                           (shift_reg.LC_30k << 1) |
                           (shift_reg.LC_100k << 2) |
                           (shift_reg.LC_300k << 3) |
                           (shift_reg.LC_1M << 4) |
                           (shift_reg.LC_3M << 5)|
                           (shift_reg.LED_LO2_LOCK << 6)|
                           (shift_reg.LED_LC_SEL << 7);

    temp2 = shift_reg.Register3; //save last Reg. value
    shift_reg.Register3 = 0;
    shift_reg.Register3 |= (shift_reg.LED_LO1_LOCK) |
                           (shift_reg.Xtal_10k << 1) |
                           (shift_reg.Xtal_3k << 2) |
                           (shift_reg.Xtal_1k << 3) |
                           (shift_reg.Xtal_ON << 4) |
                           (shift_reg.LED_OK << 5) |
                           (shift_reg.LED_ERR << 6);


    if((temp1 != shift_reg.Register2) || (temp2 != shift_reg.Register3)){
        shift_reg.LE2 = 1;   // data change for reg2 or reg3
    }
}

void init_shift_reg(void){
    // reset comm lines
    SHIFT_DATA_OFF;
    SHIFT_CLK_OFF;
    SHIFT_LE1_OFF;
    SHIFT_LE2_OFF;

    shift_reg.PLL_SA_CLK = 0;
    shift_reg.PLL_SA_DATA = 0;
    shift_reg.PLL_SA_LE = 1;
    shift_reg.PLL_SA_RF_EN = 0;
    shift_reg.PLL_SA_MOSI = 0;

    /*shift_reg.PLL_TG_CLK = 0;
    shift_reg.PLL_TG_DATA = 0;
    shift_reg.PLL_TG_LE = 1;
    shift_reg.PLL_TG_RF_EN = 0;*/

    shift_reg.Xtal_ON = 0;
    shift_reg.Xtal_10k = 0;
    shift_reg.Xtal_3k = 0;
    shift_reg.Xtal_1k = 0;
    shift_reg.LED_Xtal_SEL = 0;

    shift_reg.LC_3M = 1;
    shift_reg.LC_1M = 0;
    shift_reg.LC_300k = 0;
    shift_reg.LC_100k = 0;
    shift_reg.LC_30k = 0;
    shift_reg.LED_LC_SEL = 1;

    shift_reg.Att_SA_CLK = 0;
    shift_reg.Att_SA_DATA = 0;
    shift_reg.Att_SA_LE = 0;

    /*shift_reg.Att_TG_CLK = 0;
    shift_reg.Att_TG_DATA = 0;
    shift_reg.Att_TG_LE = 0;*/


    shift_reg.LED_OK = 0;
    shift_reg.LED_ERR = 1;

    shift_reg.LE1 = 0; // init latches
    shift_reg.LE2 = 0;
}

void sendDataToShiftRegisters(void){
    packDataToRegisters(); // pack data into variables

    // reset comm lines
    SHIFT_DATA_OFF;
    SHIFT_CLK_OFF;
    SHIFT_LE1_OFF;
    SHIFT_LE2_OFF;

    if(shift_reg.LE2){
        // New data for 2/2 shift registers
        unsigned char temp = shift_reg.Register3;
        for(int i=0; i<8; i++){
            if(temp & 0b10000000){
                SHIFT_DATA_ON;
            }
            else{
                SHIFT_DATA_OFF;
            }
            SHIFT_CLK_ON;
            temp = temp << 1;
            //delay_us(1); // just to be an a safe side
            SHIFT_CLK_OFF;
        }
        temp = shift_reg.Register2;
        for(int i=0; i<8; i++){
            if(temp & 0b10000000){
                SHIFT_DATA_ON;
            }
            else{
                SHIFT_DATA_OFF;
            }
            SHIFT_CLK_ON;
            temp = temp << 1;
            //delay_us(1); // just to be an a safe side
            SHIFT_CLK_OFF;
        }
    }

    if(shift_reg.LE1){
        // new data dot 1/2 shift registers
        unsigned char temp = shift_reg.Register1;
        for(int i=0; i<8; i++){
            if(temp & 0b10000000){
                SHIFT_DATA_ON;
            }
            else{
                SHIFT_DATA_OFF;
            }
            SHIFT_CLK_ON;
            temp = temp << 1;
            //delay_us(1); // just to be an a safe side
            SHIFT_CLK_OFF;
        }
    }
    else{
        SHIFT_DATA_OFF; // send dummy end data
        for(int i=0; i<8; i++){
            SHIFT_CLK_ON;
            //delay_us(2); // just to be an a safe side
            SHIFT_CLK_OFF;
        }
    }
    // strobe
    if(shift_reg.LE1){
        SHIFT_LE1_ON;
    }
    if(shift_reg.LE2){
        SHIFT_LE2_ON;
    }
    //delay_us(2); // small delay
    SHIFT_LE1_OFF;
    SHIFT_LE2_OFF;
}

/*
BIT 0 - System OK
BIT 1 - System Error
BIT 2 - LO1 Lock
BIT 3 - LO2 Lock
BIT 4 - LC selected
BIT 5 - Xtal selected
*/
void LEDShiftRegister(unsigned char led_data){
    shift_reg.LED_OK = 0;
    shift_reg.LED_ERR = 0;
    shift_reg.LED_LO1_LOCK = 0;
    shift_reg.LED_LO2_LOCK = 0;
    shift_reg.LED_LC_SEL = 0;
    shift_reg.LED_Xtal_SEL = 0;
    if(led_data & 0b00000001) shift_reg.LED_OK = 1;
    if(led_data & 0b00000010) shift_reg.LED_ERR = 1;
    if(led_data & 0b00000100) shift_reg.LED_LO1_LOCK = 1;
    if(led_data & 0b00001000) shift_reg.LED_LO2_LOCK = 1;
    if(led_data & 0b00010000) shift_reg.LED_LC_SEL = 1;
    if(led_data & 0b00100000) shift_reg.LED_Xtal_SEL = 1;
    sendDataToShiftRegisters();
}
