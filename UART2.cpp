/* UART2.cpp
 * Melinda and Emily
 * Date: 04/06/2026
 * PA22 UART2 Rx from other microcontroller PA8 IR output<br>
 */


#include <ti/devices/msp/msp.h>
#include "UART2.h"
#include "../inc/Clock.h"
#include "../inc/LaunchPad.h"
#include "../inc/FIFO2.h"

uint32_t LostData;
uint32_t RxCounter;
Queue FIFO2;

// power Domain PD0
// for 80MHz bus clock, UART2 clock is ULPCLK 40MHz
// initialize UART2 for 2375 baud rate
// no transmit, interrupt on receive timeout
void UART2_Init(void){
   // RSTCLR to GPIOA and UART2 peripherals
  // write this
  LostData = 0;
  UART2->GPRCM.RSTCTL = 0xB1000003; // reset UART2
  UART2->GPRCM.PWREN = 0x26000001; // activate UART2
  Clock_Delay(24); // time for uart to activate
  //bit 7 PC connected
  //bits 5-0=2 for UART2_Tx
  IOMUX->SECCFG.PINCM[PA22INDEX] = 0x00040082;
  //bit 18 INENA input enable
  //bit 7 PC connected
  //bits 5-0=2 for UART0_Rx
  UART2->CLKSEL = 0x08; // bus clock
  UART2->CLKDIV = 0x00; // no divide
  UART2->CTL0 &= ~0x01; // disable UART2
  UART2->CTL0 = 0x00020018; // enable fifos, tx and rx
  // 40000000/16 = 2,500,000, 2,500,000/2375= 1052.
  UART2->IBRD = 1052;
  UART2->FBRD = 40;
  UART2->LCRH = 0x00000030; // 8bit, 1 stop, no parity

  UART2->CPU_INT.IMASK = (1 << 10) | (1 << 0);
  // bit 11 TXINT
  // bit 10 RXINT
  // bit 0  Receive timeout
  UART2->IFLS = 0x0400;

  NVIC->ICPR[0] = 1<<14; // UART2 is IRQ 14
  NVIC->ISER[0] = 1<<14;
  NVIC->IP[3] = (NVIC->IP[3] & ~0x00C00000) | (2 << 22);  // priority (bits 23,22)

  UART2->CTL0 |= 0x01; // enable UART2
}
//------------UART2_InChar------------
// Get new serial port receive data from FIFO2
// Input: none
// Output: Return 0 if the FIFO2 is empty
//         Return nonzero data from the FIFO1 if available
char UART2_InChar(void){
  char out;
  while(FIFO2.IsEmpty()){

  }
  FIFO2.Get(&out);
  return out;
  
  // char out;
  // while (FIFO2.Get(&out) == false) {}
  // return out;
}

extern "C" void UART2_IRQHandler(void);
void UART2_IRQHandler(void){
  uint32_t status;
  status = UART2->CPU_INT.IIDX; // reading clears interrupt
  if(status == 0x01){            // 0x01 = RTOUT receive timeout
    GPIOB->DOUTTGL31_0 = BLUE;  // toggle 1
    GPIOB->DOUTTGL31_0 = BLUE;  // toggle 2
    // while((UART2->STAT & (1 << 2)) == 0){ // while RXFE=0 (data available)
    //   if(FIFO2.Put(UART2->RXDATA) == false){
    //     LostData++;
    //   }
    // }
    char letter;
    while((UART2->STAT & (1<<2)) == 0){
      letter = UART2->RXDATA;
      if(FIFO2.Put(letter) == false){
        LostData++;
      }
    }

    RxCounter++;
    UART2->CPU_INT.ICLR = 0x01; // clear RTOUT flag
    GPIOB->DOUTTGL31_0 = BLUE;  // toggle 3
  }
  //return;
}
