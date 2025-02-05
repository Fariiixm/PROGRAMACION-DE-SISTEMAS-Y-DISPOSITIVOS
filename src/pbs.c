#include <s3c44b0x.h>
#include <s3cev40.h>
#include <pbs.h>
#include <timers.h>
#include <uart.h>

extern void isr_PB_dummy(void);

void pbs_init(void)
{
    timers_init();
}

uint8 pb_scan(void){
/*PDATG: PUERTO G:
 * PG[7] derecho
 *  PG[6] izquierdo
*/

    if ((PB_LEFT & PDATG) ==0)//Comprobamos si el bit de activacion del pulsador izquierdo esta activado en el puerto G
        return PB_LEFT;
    else if ((PB_RIGHT & PDATG) ==0)//Comprobamos si el bit de activacion del pulsador derecho esta activado en el puerto G
        return PB_RIGHT;
    else
        return PB_FAILURE;
}

uint8 pb_pressed(void){
if(pb_scan()!=PB_FAILURE) return 0;
	else return 1;

}
uint8 pb_getchar(void)
{
    uint8 scancode;
 while(((PDATG & 0X40)==0x40) && ((PDATG & 0x80)==0x80)) sw_delay_ms(PB_KEYDOWN_DELAY); //MBC

    scancode = pb_scan();

    while ((scancode & PDATG) == 0);
    sw_delay_ms(PB_KEYUP_DELAY);

    return scancode;
}

uint8 pb_getchartime(uint16 *ms)
{
    uint8 scancode;

    while(((PDATG & 0X40)==0x40) && ((PDATG & 0x80)==0x80));
    timer3_start();
    sw_delay_ms(PB_KEYDOWN_DELAY);
    scancode = pb_scan();

    while ((scancode & PDATG) == 0);
    *ms = timer3_stop() / 10;
    sw_delay_ms(PB_KEYUP_DELAY);

    return scancode;
}

uint8 pb_timeout_getchar(uint16 ms)
{
    uint8 scancode;
    timer3_start_timeout(ms);

    while (timer3_timeout() && ((PDATG & 0x40) == 0x40) && ((PDATG & 0x80) == 0x80));

    if (!timer3_timeout())
        return PB_TIMEOUT;
    else
    {
        sw_delay_ms(PB_KEYDOWN_DELAY);
        scancode = pb_scan();

        while ((scancode & PDATG) == 0);
        sw_delay_ms(PB_KEYUP_DELAY);

        return scancode;
    }
}

void pbs_open(void (*isr)(void))
{
    pISR_PB = (uint32)isr;
    EXTINTPND = 0xf;
    I_ISPC = BIT_EINT4567;
    INTMSK &= ~(BIT_GLOBAL | BIT_EINT4567);
}

void pbs_close(void)
{
    INTMSK |= BIT_EINT4567;
    pISR_PB = (uint32)isr_PB_dummy;
}
