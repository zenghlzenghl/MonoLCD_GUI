#ifndef __TYPE_H
#define __TYPE_H

/*============================================================================
 * 编译器抽象层：SDCC(mcs51) 与 Keil C51 共用同一套业务代码
 *
 *   ROM  常量/字库放 Flash
 *   XRAM 大数组（显存等）放内部扩展 RAM
 *   IRAM 高频变量放内部 RAM
 *==========================================================================*/

#if defined(__SDCC) || defined(SDCC) || defined(__SDCC_mcs51)
    #define ROM             __code
    #define XRAM            __xdata
    #define IRAM            __data
    #define BIT             __bit
    #define SFR_AT(a, n)    __sfr  __at (a) n
    #define SBIT_AT(a, n)   __sbit __at (a) n
    #define INTERRUPT_AT(v) __interrupt (v)
    #define CRITICAL        __critical
#else                                    /* Keil C51 */
    #define ROM             code
    #define XRAM            xdata
    #define IRAM            data
    #define BIT             bit
    #define SFR_AT(a, n)    sfr  n = a
    #define SBIT_AT(a, n)   sbit n = a
    #define INTERRUPT_AT(v) interrupt v
    #define CRITICAL
#endif

typedef unsigned char  uint8_t;
typedef signed   char  int8_t;
typedef unsigned int   uint16_t;
typedef signed   int   int16_t;
typedef unsigned long  uint32_t;
typedef signed   long  int32_t;
typedef unsigned char  bool_t;

#ifndef NULL
    #define NULL  ((void *)0)
#endif
#ifndef TRUE
    #define TRUE  1u
#endif
#ifndef FALSE
    #define FALSE 0u
#endif

#define NELEM(a)  (sizeof(a) / sizeof((a)[0]))

#endif /* __TYPE_H */
