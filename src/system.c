#include <s3c44b0x.h>
#include <s3cev40.h>
#include <system.h>
#include <segs.h>
#include <uart.h>

#define USRMODE (0x10)
#define FIQMODE (0x11)
#define IRQMODE (0x12)
#define SVCMODE (0x13)
#define ABTMODE (0x17)
#define UNDMODE (0x1b)
#define SYSMODE (0x1f)

#define SET_OPMODE( mode )                 \
    asm volatile (                         \
        "mrs r0, cpsr               \n"    \
        "bic r0, r0, #0x1f          \n"    \
        "orr r0, r0, %0             \n"    \
        "msr cpsr_c, r0               "    \
        :                                  \
        : "i" (mode)                       \
        : "r0"                             \
    )

#define GET_OPMODE( mode_p )               \
    asm volatile (                         \
        "mrs r0, cpsr               \n"    \
        "and r0, r0, #0x1f          \n"    \
        "strb r0, %0                  "    \
        : "=m" (*mode_p)                   \
        :                                  \
        : "r0"                             \
    );

#define SET_IRQFLAG( value )               \
    asm volatile (                         \
        "mrs r0, cpsr               \n"    \
        "bic r0, r0, #0x80          \n"    \
        "orr r0, r0, %0             \n"    \
        "msr cpsr_c, r0               "    \
        :                                  \
        : "i" (value<<7)                   \
        : "r0"                             \
    )

#define SET_FIQFLAG( value )              \
    asm volatile (                        \
        "mrs r0, cpsr               \n"   \
        "bic r0, r0, #0x40          \n"   \
        "orr r0, r0, %0             \n"   \
        "msr cpsr_c, r0               "   \
        :                                 \
        : "i" (value<<6)                  \
        : "r0"                            \
    )

static inline void port_init(void);
static inline void install_dummy_isr(void);
static inline void show_sys_info(void);
static void sys_recovery(void);

void isr_SWI_dummy(void) __attribute__ ((interrupt ("SWI")));
void isr_UNDEF_dummy(void) __attribute__ ((interrupt ("UNDEF")));
void isr_IRQ_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_FIQ_dummy(void) __attribute__ ((interrupt ("FIQ")));
void isr_PABORT_dummy(void) __attribute__ ((interrupt ("ABORT")));
void isr_DABORT_dummy(void) __attribute__ ((interrupt ("ABORT")));
void isr_ADC_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_RTC_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_UTXD1_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_UTXD0_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_SIO_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_IIC_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_URXD1_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_URXD0_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_TIMER5_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_TIMER4_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_TIMER3_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_TIMER2_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_TIMER1_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_TIMER0_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_UERR01_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_WDT_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_BDMA1_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_BDMA0_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_ZDMA1_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_ZDMA0_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_TICK_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_PB_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_ETHERNET_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_TS_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_KEYPAD_dummy(void) __attribute__ ((interrupt ("IRQ")));
void isr_USB_dummy(void) __attribute__ ((interrupt ("IRQ")));

void sys_init(void) {
	uint8 mode;

	WTCON = 0;             // deshabilita el watchdog
	INTMSK = ~0;            // enmascara todas las interrupciones

	GET_OPMODE(&mode);    // lee el modo de ejecuci�n del procesador
	if (mode != SVCMODE)
		sys_recovery(); // si no es SVC (por una reejecuci�n de la aplicaci�n tras una excepci�n sin reset HW previo) recupera el modo SVC y restaura las pilas del sistema

	// Configuracion del gestor de reloj
	LOCKTIME = 0xFFF;         // estabilizacion del PLL = 512 us
	PLLCON = 0x38021;         // MCLK = 64MHz
	CLKSLOW = 0x8;         // MCLK_SLOW = 500 KHz
	CLKCON = 0x7FF8; // modo NORMAL y reloj distribuido a todos los controladores

	// Configuracion del arbitro de bus
	SBUSCON = 0x8000001B;  // prioridad fija por defecto LCD > ZDMA > BDMA > IRQ

	// Configuracion de cache
	SYSCFG = 0x0;           // deshabilitada

	// Configuacion del controlador de interrupciones
	I_PMST = 0x1F1B;           // prioridades fijas por defecto
	I_PSLV = 0x1B1B1B1B;
	INTMOD = 0x0;           // todas las interrupciones en modo IRQ
	install_dummy_isr(); // instala rutinas de tratamiento por defecto para todas las interrupciones
	EXTINTPND = 0xF; // borra las interrupciones externas pendientes por linea EINT7..4
	I_ISPC = 0x01E00024;           // borra todas las interrupciones pendientes
	INTCON = 0x1; // IRQ vectorizadas, linea IRQ activada, linea FIQ desactivada
	SET_IRQFLAG(0);       // Habilita IRQ en el procesador
	SET_FIQFLAG(1);       // Deshabilita FIQ en el procesador

	// Configuracion de puertos
	port_init();

	// Configuracion de dispositivos
	segs_init();

	uart0_init();

	show_sys_info();
}

static inline void port_init(void) {
	PDATA = ~0;
	PCONA = 0xFE;

	PDATB = ~0;
	PCONB = 0x14F;

	PDATC = ~0;
	PCONC = 0x5FF555FF;
	PUPC = 0x94FB;

	PDATD = ~0;
	PCOND = 0xAAAA;
	PUPD = 0xFF;

	PDATE = ~0;
	PCONE = 0x255A9;
	PUPE = 0x1FB;

	PDATF = ~0;
	PCONF = 0x251A;
	PUPF = 0x74;

	PDATG = ~0;
	PCONG = 0xF5FF;
	PUPG = 0x30;

	SPUCR = 0x7;

	EXTINT = 0x22000220;
}

static inline void install_dummy_isr(void) {
	pISR_SWI = (uint32) isr_SWI_dummy;
	pISR_PABORT = (uint32) isr_PABORT_dummy;
	pISR_DABORT = (uint32) isr_DABORT_dummy;
	pISR_IRQ = (uint32) isr_IRQ_dummy;
	pISR_FIQ = (uint32) isr_FIQ_dummy;
	pISR_ADC = (uint32) isr_ADC_dummy;
	pISR_RTC = (uint32) isr_RTC_dummy;
	pISR_UTXD1 = (uint32) isr_UTXD1_dummy;
	pISR_UTXD0 = (uint32) isr_UTXD0_dummy;
	pISR_SIO = (uint32) isr_SIO_dummy;
	pISR_IIC = (uint32) isr_IIC_dummy;
	pISR_URXD1 = (uint32) isr_URXD1_dummy;
	pISR_URXD0 = (uint32) isr_URXD0_dummy;
	pISR_TIMER5 = (uint32) isr_TIMER5_dummy;
	pISR_TIMER4 = (uint32) isr_TIMER4_dummy;
	pISR_TIMER3 = (uint32) isr_TIMER3_dummy;
	pISR_TIMER2 = (uint32) isr_TIMER2_dummy;
	pISR_TIMER1 = (uint32) isr_TIMER1_dummy;
	pISR_TIMER0 = (uint32) isr_TIMER0_dummy;
	pISR_UERR01 = (uint32) isr_UERR01_dummy;
	pISR_WDT = (uint32) isr_WDT_dummy;
	pISR_BDMA1 = (uint32) isr_BDMA1_dummy;
	pISR_BDMA0 = (uint32) isr_BDMA0_dummy;
	pISR_ZDMA1 = (uint32) isr_ZDMA1_dummy;
	pISR_ZDMA0 = (uint32) isr_ZDMA0_dummy;
	pISR_TICK = (uint32) isr_TICK_dummy;
	pISR_PB = (uint32) isr_PB_dummy;
	pISR_ETHERNET = (uint32) isr_ETHERNET_dummy;
	pISR_TS = (uint32) isr_TS_dummy;
	pISR_KEYPAD = (uint32) isr_KEYPAD_dummy;
	pISR_USB = (uint32) isr_USB_dummy;
}

void isr_SWI_dummy(void) {
	uart0_puts("\n error en isr_SWI_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_UNDEF_dummy(void) {
	uart0_puts("\n error en isr_UNDEF_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_IRQ_dummy(void) {
	uart0_puts("\n error en isr_IRQ_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_FIQ_dummy(void) {
	uart0_puts("\n error en isr_FIQ_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_PABORT_dummy(void) {
	uart0_puts("\n error en isr_PABORT_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_DABORT_dummy(void) {
	uart0_puts("\n error en isr_DABORT_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_ADC_dummy(void) {
	uart0_puts("\n error en isr_ADC_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_RTC_dummy(void) {
	uart0_puts("\n error en isr_RTC_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_UTXD1_dummy(void) {
	uart0_puts("\n error en isr_UTXD1_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_UTXD0_dummy(void) {
	uart0_puts("\n error en isr_UTXD0_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_SIO_dummy(void) {
	uart0_puts("\n error en isr_SIO_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_IIC_dummy(void) {
	uart0_puts("\n error en isr_IIC_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_URXD1_dummy(void) {
	uart0_puts("\n error en isr_URXD1_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_URXD0_dummy(void) {
	uart0_puts("\n error en isr_URXD0_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_TIMER5_dummy(void) {
	uart0_puts("\n error en isr_TIMER5_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_TIMER4_dummy(void) {
	uart0_puts("\n error en isr_TIMER4_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_TIMER3_dummy(void) {
	uart0_puts("\n error en isr_TIMER3_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_TIMER2_dummy(void) {
	uart0_puts("\n error en isr_TIMER2_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_TIMER1_dummy(void) {
	uart0_puts("\n error en isr_TIMER1_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_TIMER0_dummy(void) {
	uart0_puts("\n error en isr_TIMER0_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_UERR01_dummy(void) {
	uart0_puts("\n error en isr_UERR01_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_WDT_dummy(void) {
	uart0_puts("\n error en isr_WDT_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_BDMA1_dummy(void) {
	uart0_puts("\n error en isr_BDMA1_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_BDMA0_dummy(void) {
	uart0_puts("\n error en isr_BDMA0_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_ZDMA1_dummy(void) {
	uart0_puts("\n error en isr_ZDMA1_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_ZDMA0_dummy(void) {
	uart0_puts("\n error en isr_ZDMA0_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_TICK_dummy(void) {
	uart0_puts("\n\n*** ERROR FATAL: ejecutando isr_TICK_dummy");
	SEGS = 0x75;
	while (1)
		;
}

void isr_PB_dummy(void) {
	uart0_puts("\n error en isr_PB_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_ETHERNET_dummy(void) {
	uart0_puts("\n error en isr_ETHERNET_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_TS_dummy(void) {
	uart0_puts("\n error en isr_TS_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_KEYPAD_dummy(void) {
	uart0_puts("\n error en isr_KEYPAD_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

void isr_USB_dummy(void) {
	uart0_puts("\n error en isr_USB_dummy \n");
	SEGS = 0x75;
	while (1)
		;
}

static inline void show_sys_info(void) {
	uart0_puts("\n\n MENSAJE INFORMACI�N SISTEMA");
}

inline void sleep(void) {
	CLKCON |= (1 << 2);    // Pone a la CPU en estado IDLE
}

static void sys_recovery(void) // NO TOCAR
{
	uint8 mode;
	uint32 sp, fp;
	uint32 *addrSrc, *addrDst;
	uint32 diffStacks;

	asm volatile ( "str sp, %0" : "=m" (sp) : : );
	// lee el puntero a la cima de la pila de excepci�n (SP)
	asm volatile ( "str fp, %0" : "=m" (fp) : : );
	// lee el puntero al marco de activaci�n (FP) de sys_recovery() en la pila de excepci�n

	GET_OPMODE(&mode);    // lee el modo de ejecuci�n del procesador
	switch (mode) {
	case IRQMODE:
		diffStacks = IRQSTACK - SVCSTACK; // calcula la distancia entre la bases de la pila IRQ y la SVC
		addrSrc = (uint32 *) IRQSTACK;         // base de la pila IRQ
		break;
	case FIQMODE:
		diffStacks = FIQSTACK - SVCSTACK; // calcula la distancia entre la bases de la pila FIQ y la SVC
		addrSrc = (uint32 *) FIQSTACK;         // base de la pila FIQ
		break;
	case ABTMODE:
		diffStacks = ABTSTACK - SVCSTACK; // calcula la distancia entre la bases de la pila ABT y la SVC
		addrSrc = (uint32 *) ABTSTACK;         // base de la pila ABT
		break;
	case UNDMODE:
		diffStacks = UNDSTACK - SVCSTACK; // calcula la distancia entre la bases de la pila UND y la SVC
		addrSrc = (uint32 *) UNDSTACK;         // base de la pila UND
		break;
	case SYSMODE:
	case USRMODE:
		// Habr�a que hacer algo an�logo a lo anterior y adem�s para volver a modo SVC dado que no es v�lido SET_OPMODE( SVCMODE ), es necesario esto:
		// pISR_SWI = (uint32) isr_SWI;
		// SWI( 0 );
	default:
		while (1)
			;                           // aqu� no deber�a llegarse
		break;
	}

	asm volatile ( "ldr sp, %0" : "=m" (addrSrc) : : );
	// restaura el SP de excepci�n a su base para desechar su contenido y evitar su desbordamiento

	for (addrDst = (uint32 *) SVCSTACK; addrSrc > (uint32 *) sp;) // copia el contenido completo de la pila excepci�n en la pila SVC
		*(--addrDst) = *(--addrSrc);

	addrDst = (uint32 *) (fp - diffStacks); // carga el puntero al marco de activaci�n de sys_recovery() en la pila SVC
	addrDst--;                             // salta el PC apilado
	addrDst--;                             // salta el LR apilado
	*addrDst -= diffStacks; // actualiza SP apilado para que apunte a la pila SVC
	addrDst--;                             // salta el SP apilado
	*addrDst -= diffStacks; // actualiza el FP apilado para que apunte a la pila SVC

	addrDst = (uint32 *) (*addrDst); // carga el puntero al marco de activaci�n de sys_init()
	addrDst--;                             // salta el PC apilado
	addrDst--;                             // salta el LR apilado
	*addrDst -= diffStacks; // actualiza SP apilado para que apunte a la pila SVC
	addrDst--;                             // salta el SP apilado
	*addrDst -= diffStacks; // actualiza el FP apilado para que apunte a la pila SVC

	SET_OPMODE(SVCMODE);                           // cambia a modo SVC

	sp -= diffStacks;
	asm volatile ( "ldr sp, %0" : : "m" (sp) : );
	// actualiza SP_svc para que apunte a la cima de la pila SVC

	fp -= diffStacks;
	asm volatile ( "ldr fp, %0" : : "m" (fp) : );
	// actualiza FP para que apunte al marco de la pila SVC, debe ser siempre la �ltima sentencia
}

