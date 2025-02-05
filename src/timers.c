#include <s3c44b0x.h>
#include <s3cev40.h>
#include <timers.h>


extern void isr_TIMER0_dummy(void);

static uint32 loop_ms = 0;
static uint32 loop_s = 0;
static void sw_delay_init( void );
/*
 ** Pone a 0 los registros de configuraci�n
 ** Pone a 0 todos los b�fferes y registros de cuenta y comparaci�n
 ** Para todos los temporizadores
 ** Inicializa las variables para retardos software
 */
void timers_init(void) {
	TCFG0 = 0;
	TCFG1 = 0;
	TCNTB0 = 0;
	TCNTB1 = 0;
	TCNTB2 = 0;
	TCNTB3 = 0;
	TCNTB4 = 0;
	TCNTB5 = 0;
	TCMPB0 = 0;
	TCMPB1 = 0;
	TCMPB2 = 0;
	TCMPB3 = 0;
	TCMPB4 = 0;
	TCON = 0;
	loop_ms = 487;
	loop_s = 487440;

	sw_delay_init();
}
static void sw_delay_init( void )
 {
 uint32 i;
 timer3_start();
 for( i=1000000; i; i--);
 loop_s = ((uint64)1000000*10000)/timer3_stop();
 loop_ms = loop_s / 1000;
 };

/*
 ** Realiza una espera de n milisegundos usando el timer3
 */
void timer3_delay_ms(uint16 n) {
	TCFG0 = (TCFG0 & ~(0xff << 8)) | (31 << 8);	// (31 +1) *2 / 64
	TCFG1 = (TCFG1 & ~(0xf << 12)) | (0 << 12);	// (N,D,C) = (31, 2, 1000)
	for (; n; n--) {
		TCNTB3 = 1000;
		TCON = (TCON & ~(0xf << 16)) | (1 << 17);
		TCON = (TCON & ~(0xf << 16)) | (1 << 16);
		while (!TCNTO3 )
			;
		while (TCNTO3 )
			;
	}
}

/*
 ** Realiza una espera de n milisegundos sin usar temporizadores
 */
void sw_delay_ms(uint16 n) {
	uint32 i;
	for (i = loop_ms * n; i; i--)
		;
}

/*
 ** Realiza una espera de n segundos usando el timer3
 */
void timer3_delay_s(uint16 n) {
	TCFG0 = (TCFG0 & ~(0xff << 8)) | (63 << 8);
	TCFG1 = (TCFG1 & ~(0xf << 12)) | (4 << 12);
	for (; n; n--) {
		TCNTB3 = 31250;
		TCON = (TCON & ~(0xf << 16)) | (1 << 17);
		TCON = (TCON & ~(0xf << 16)) | (1 << 16);
		while (!TCNTO3 )
			;
		while (TCNTO3 )
			;
	}
}

/*
 ** Realiza una espera de n segundos sin usar temporizadores
 */
void sw_delay_s(uint16 n) {
	uint32 i;
 for( i=loop_s*n; i; i-- );
}

/*
 ** Arranca el timer3 a una frecuencia de 0,01 MHz
 ** Permitir� medir tiempos con una resoluci�n de 0,1 ms (100 us) hasta un m�ximo de 6.55s
 */
void timer3_start(void) {
	TCFG0 = (TCFG0 & ~(0xff << 8)) | (199 << 8);
	TCFG1 = (TCFG1 & ~(0xf << 12)) | (4 << 12);
	TCNTB3 = 0xffff;
	TCON = (TCON & ~(0xf << 16)) | (1 << 17);
	TCON = (TCON & ~(0xf << 16)) | (1 << 16);
	while (!TCNTO3);
}

/*
 ** Detiene el timer3, devolviendo el n�mero de d�cimas de milisegundo transcurridas desde que arranc� hasta un m�ximo de 6.55s
 */
uint16 timer3_stop(void) {
	TCON &= ~(1 << 16);
	return 0xffff - TCNTO3 ;
}

/*
 ** Arranca el timer3 a una frecuencia de 0,01 MHz
 ** Permitir� contar n d�cimas de milisegundo (0,1 ms = 100 us) hasta un m�ximo de 6.55s
 */
void timer3_start_timeout(uint16 n) {
	TCFG0 = (TCFG0 & ~(0xff << 8)) | (199 << 8);
	TCFG1 = (TCFG1 & ~(0xf << 12)) | (4 << 12);
	TCNTB3 = n;
	TCON = (TCON & ~(0xf << 16)) | (1 << 17);
	TCON = (TCON & ~(0xf << 16)) | (1 << 16);
	while (!TCNTO3 )
		;
}

/*
 ** Indica si el timer3 ha finalizado su cuenta
 */
uint16 timer3_timeout(void) {
	return !TCNTO3 ;
}

/*
 ** Instala, en la tabla de vectores de interrupci�n, la funci�n isr como RTI de interrupciones del timer0
 ** Borra interrupciones pendientes del timer0
 ** Desenmascara globalmente las interrupciones y espec�ficamente las interrupciones del timer0
 ** Configura el timer0 para que genere tps interrupciones por segundo
 */
void timer0_open_tick(void (*isr)(void), uint16 tps) {
	pISR_TIMER0 = (uint32)isr;
	I_ISPC = BIT_TIMER0;
	INTMSK &= ~(BIT_GLOBAL | BIT_TIMER0);

	if (tps > 0 && tps <= 10) {

		TCFG0 = (TCFG0 & ~(0xff << 0)) | (199 << 0); //TCFG0=199.
		TCFG1 = (TCFG1 & ~(0xf << 0)) | (2 << 0); //TCFG1=2.

		/*
		 * (200*2)/0.04=25
		 *
		 */
		TCNTB0 = (40000U / tps); //40khz<-Permite obtener el número de ticks deseado.
		//40khz->

	} else if (tps > 10 && tps <= 100) {
		TCFG0 = (TCFG0 & ~(0xff << 0))  | (4 << 0);
		TCFG1 = (TCFG1 & ~(0xf << 0)) | (4 << 0);
		TCNTB0 = (400000U / (uint32) tps);//400khz

	} else if (tps > 100 && tps <= 1000) {
		TCFG0 = (TCFG0 & ~(0xff << 0)) | (1 << 0);
		TCFG1 = (TCFG1 & ~(0xf << 0)) | (2 << 0);
		TCNTB0 = (4000000U / (uint32) tps);//40MHz

	} else if (tps > 1000) {
		TCFG0 = (TCFG0 & ~(0xff << 0)) | (0 << 0);
		TCFG1 = (TCFG1 & ~(0xf << 0)) | (0 << 0);
		TCNTB0 = (32000000U / (uint32) tps); //32Mhz
	}
	TCON = (TCON & ~(0x7 << 0)) | (1<<3) | (1 << 1);
	TCON = (TCON & ~(0x7 << 0)) | (1<<3) | (1 << 0);
}

/*
 ** Instala, en la tabla de vectores de interrupci�n, la funci�n isr como RTI de interrupciones del timer0
 ** Borra interrupciones pendientes del timer0
 ** Desenmascara globalmente las interrupciones y espec�ficamente las interrupciones del timer0
 ** Configura el timer0 para que genere interrupciones en el modo y con la periodicidad indicadas
 ** Configura el timer0 para que genere interrupciones en el modo y con la periodicidad indicadas
 */
void timer0_open_ms(void (*isr)(void), uint16 ms, uint8 mode) {
	pISR_TIMER0 = (uint32)isr;
		I_ISPC = BIT_TIMER0;
		INTMSK &= ~(BIT_GLOBAL | BIT_TIMER0);
		TCFG0 = (TCFG0 & ~(0xff << 0)) | (199 << 0);
		TCFG1 = (TCFG1 & ~(0xf << 0)) | (4 << 0);
		TCNTB0 = 10 * ms;
		TCON = (TCON & ~(0xf << 0)) | (1 << 1);
		TCON = (TCON & ~(0xf << 0)) | (1 << 0);
}

/*
 ** Para y pone a 0 todos sus bufferes y registros del timer0
 ** Deshabilita las interrupciones del timer0
 ** Desinstala la RTI del timer0
 */
void timer0_close(void) {
	TCNTB0 = 0;
	TCMPB0 = 0;
	TCON = (TCON & ~(0xf << 0)) | (1 << 1);
		TCON = (TCON & ~(0xf << 0)) | (1 << 0);
	INTMSK |= BIT_TIMER0;
	pISR_TIMER0 = (uint32)isr_TIMER0_dummy;

}
