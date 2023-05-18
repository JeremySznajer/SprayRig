#include "msp.h"
#include "msp432.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <time.h>

int menu_option = 0;
bool spray_flag = false;
uint8_t result = 0;

void Pneumatic_Init(int);

void Coat_Bottles(int);


void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer


    P2->DIR |= 0x3F;                    // Set P2.0->P2.5 to output
    P2->OUT &= ~0xFF;

    // Buttons

    P3->DIR &= ~0xE1;                   // Set 3.0, 3.5, 3.6 and 3.7 to input
    P3->OUT |= 0xE1;
    P3->REN |= 0xE1;
    P3->IE  |= 0xE1;
    P3->IFG &= ~0xE1;

    P4->DIR &= ~0x06;                   // Set 4.2 and 4.3 to input
    P4->OUT |= 0x06;
    P4->REN |= 0x06;
    P4->IE  |= 0x06;
    P4->IES &= ~0x06;
    P4->IFG &= ~0x06;

    NVIC->ISER[1] = 1 << ((PORT3_IRQn)&31);
    NVIC->ISER[1] = 1 << ((PORT4_IRQn)&31);

    // enables the global interrupts
    __enable_irq();

    while(1)
        {
            switch(menu_option){
                  case 1:
                      // SPRAY TEST
                      P2->SEL0 &= ~0x08;                  // Set P2.3 to GPIO mode
                      P2->SEL1 &= ~0x08;                  // Set P2.3 to GPIO mode
                      P2->OUT |= 0x08;                    // set P2.3 high
                      __delay_cycles(700000);           //700000
                      //__delay_cycles(20000000);
                      P2->OUT &= ~0x08;                   // set P2.3 low
                      break;
                  case 2:
                      // MOVE TEST
                      Pneumatic_Init(2500);               //100%
                      __delay_cycles(3000000);
                      Pneumatic_Init(0);
                      __delay_cycles(3000000);

                      TA0CTL = TACLR;
                      break;
                  case 3:
                      //COAT BOTTLES
                      P2->OUT &= ~(BIT0 + BIT1 + BIT2);
                      Coat_Bottles(6);
                      TA0CTL = TACLR;
                      break;
                  case 4:
                      //COAT BOTTLES 1x
                      P2->OUT &= ~(BIT0 + BIT1 + BIT2);
                      Coat_Bottles(1);
                      TA0CTL = TACLR;
                      break;
            }
            menu_option = 0;
       }
}


void TA0_0_IRQHandler(void){
    TIMER_A0->CCTL[0] &= ~0x0001;
}

void PORT3_IRQHandler(void) // Hard stop valve
{
    result = P3->IFG;   // store IFG

    switch(result){
        case 0x20:
            menu_option = 1;
            break;
        case 0x40:
            menu_option = 2;
            break;
        case 0x80:
            menu_option = 3;
            break;
        case 0x01:
            menu_option = 4;
            break;
    }
    P3->IFG &= ~(result); // clear interrupts flags
}

void PORT4_IRQHandler(void) // Spray Control Mode
{

    uint8_t result = P4->IFG;   // store IFG
    if (result & BIT1)          // "Bottle" Light Gate triggered
    {
        P2->OUT ^= BIT0;
        if(spray_flag==true)
            P2->OUT |= 0x08;

    }
    if (result & BIT2)          // "Home" Light Gate triggered
    {
        P2->OUT ^= BIT1;
        if(spray_flag==true)
        {
            P2->OUT &= ~0x08;
            spray_flag = false;
        }
    }
    P4->IFG &= ~(result); // clear interrupts flags


}

void Coat_Bottles(int num_coatings)
{
    int i = 0;
    for(i=0;i<num_coatings;i++)
    {
        P2->SEL0 &= ~0x08;                  // Set P2.3 to GPIO mode
        P2->SEL1 &= ~0x08;                  // Set P2.3 to GPIO mode

        Pneumatic_Init(2500);               //100%
        __delay_cycles(3000000);           //quarter of a second
        //P2->OUT |= 0x08;
        spray_flag = true;
        Pneumatic_Init(0);
        __delay_cycles(1000000);           //quarter of a second

        TA0CTL = TACLR;
    }
}

void Pneumatic_Init(int pwm) // add interrupt flag value as variable so can choose if want or not
{

    TA0CCR0 = 2500;
    TA0CCR1 = 0;

    P2->SEL0 |= 0x10; // P2.4 TimerA0 functions
    P2->SEL1 &= ~0x10; // P2.4 TimerA0 functions

    // TIMER A Setup                       // TACCR0 interrupt enabled
    TA0CCR1 = pwm;
    TA0CTL = TASSEL_2 + MC_1 + ID_3;                    // SMCLK, Up Mode, divider 4
    TA0CCTL1 = 0x00C0;                         // CCI0 toggle/reset
}


