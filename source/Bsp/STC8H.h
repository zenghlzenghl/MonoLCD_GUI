#ifndef __STC8H_H
#define __STC8H_H

#include "Type.h"

/*============================================================================
 * STC8H 精简寄存器定义（只用工程涉及的寄存器，其余按需补充）
 *==========================================================================*/

/*------------------------------------------------------------ 端口模式 */
SFR_AT(0x92, P1M0);
SFR_AT(0x93, P1M1);
SFR_AT(0x94, P0M0);
SFR_AT(0x95, P0M1);
SFR_AT(0xB2, P3M0);
SFR_AT(0xB1, P3M1);

/*------------------------------------------------------------ 端口 */
SFR_AT(0x80, P0);
SFR_AT(0x90, P1);
SFR_AT(0xA0, P2);
SFR_AT(0xB0, P3);

/*------------------------------------------------------------ 系统/时钟 */
SFR_AT(0x8E, AUXR);         /* .0=S1ST2 .1=EXTRAM .7=T0x12 */
SFR_AT(0xA2, P_SW1);        /* SPI_S[1:0] = bit3:2 */
SFR_AT(0xBA, P_SW2);
SFR_AT(0xC1, WDT_CONTR);
SFR_AT(0x97, CLKSEL);

/*------------------------------------------------------------ 定时器 0 */
SFR_AT(0x88, TCON);
SBIT_AT(0x8C, TR0);
SBIT_AT(0x8D, TF0);
SFR_AT(0x89, TMOD);
SFR_AT(0x8A, TL0);
SFR_AT(0x8C, TH0);

/*------------------------------------------------------------ 中断 */
SFR_AT(0xA8, IE);
SBIT_AT(0xAF, EA);
SBIT_AT(0xA9, ET0);

/*------------------------------------------------------------ SPI */
SFR_AT(0xCD, SPSTAT);       /* .7=SPIF .6=WCOL */
SFR_AT(0xCE, SPCTL);        /* .7=SSIG .6=SPEN .5=DORD .4=MSTR */
SFR_AT(0xCF, SPDAT);

/*------------------------------------------------------------ ADC */
SFR_AT(0xBC, ADC_CONTR);
SFR_AT(0xBD, ADC_RES);
SFR_AT(0xBE, ADC_RESL);
SFR_AT(0xDE, ADCCFG);

#endif /* __STC8H_H */
