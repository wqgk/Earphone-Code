#include <string.h>
#include "board.h"
#include "manchester.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/
#ifdef __ICCARM__
#define ALIGNSTR(x) # x
#define ALIGN(x) _Pragma(ALIGNSTR(data_alignment = ## x))
#else
#define ALIGN(x) __attribute__ ((aligned(x)))
#endif

/* DMA send string arrays. DMA buffers must remain in memory during the DMA
   transfer. */
/* Next available UART TX DMA descriptor and use counter */
static volatile int nextTXDesc, countTXDescUsed;

/* Number of UART RX descriptors used for DMA */
#define UARTRXDESC 1

/* Maximum size of each UART RX receive buffer */
#define UARTRXBUFFSIZE  20

/* UART RX receive buffers */
static uint8_t dmaRXBuffs[UARTRXBUFFSIZE];
/* UART receive buffer that is available and availa flag */
static volatile int uartRXBuff;
static volatile bool uartRxAvail = false;

/* DMA descriptors must be aligned to 16 bytes */
ALIGN(16) static DMA_CHDESC_T dmaRXDesc[UARTRXDESC];

#define UART_TX_PIN 18
#define UART_RX_PIN 0

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/
 
/*********************************************************************************************************
** Function name:       myDelay
** Descriptions:        软件延时
** input parameters:    ulTime:延时时间(ms)
** output parameters:   无
** Returned value:      无
*********************************************************************************************************/
void myDelay (uint32_t ulTime)
{
    uint32_t i;

    while (ulTime--) {
        for (i = 0; i < 2401; i++);
    }
}

static void Init_UART_PinMux(void)
{
    /* Enable the clock to the Switch Matrix */
    Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);

    Chip_Clock_SetUARTClockDiv(1);	/* divided by 1 */

    /* Disable the fixed pins assigned to 31 and 24 */
    Chip_SWM_DisableFixedPin(SWM_FIXED_ADC8);
    Chip_SWM_DisableFixedPin(SWM_FIXED_ACMP_I1);


    /* Disable debug USART */
    /* Connect the U0_TXD_O and U0_RXD_I signals to port pins (see define above) */
    Chip_SWM_MovablePinAssign(SWM_U0_TXD_O, UART_TX_PIN);
    Chip_SWM_MovablePinAssign(SWM_U0_RXD_I, UART_RX_PIN);

    /* Disable the clock to the Switch Matrix to save power */
    Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_SWM);
}

/* Setup DMA UART RX support, but do not queue descriptors yet */
static void dmaRXSetup(void)
{
    /* Setup UART 0 RX channel for the following configuration:
       - Peripheral DMA request (UART 0 RX channel)
       - Single transfer
       - Low channel priority */
    Chip_DMA_EnableChannel(LPC_DMA, DMAREQ_USART0_RX);
    Chip_DMA_EnableIntChannel(LPC_DMA, DMAREQ_USART0_RX);
    Chip_DMA_SetupChannelConfig(LPC_DMA, DMAREQ_USART0_RX,
                                (DMA_CFG_PERIPHREQEN | DMA_CFG_TRIGBURST_SNGL | DMA_CFG_CHPRIORITY(3)));
}

/* Queue up DMA descriptors and buffers for UART RX */
static void dmaRXQueue(void)
{
    int i;

    /* Linked list of descriptors that map to the 3 receive buffers */
    for (i = 0; i < UARTRXDESC; i++) {
        /* Setup next descriptor */
        if (i == (UARTRXDESC - 1)) {
            /* Wrap descriptors */
            dmaRXDesc[i].next = DMA_ADDR(&dmaRXDesc[0]);
        }
        else {
            dmaRXDesc[i].next = DMA_ADDR(&dmaRXDesc[i + 1]);
        }

        /* Create a descriptor for the data */
        dmaRXDesc[i].source = DMA_ADDR(&LPC_USART0->RXDATA) + 0;	/* Byte aligned */
        dmaRXDesc[i].dest = DMA_ADDR(dmaRXBuffs + UARTRXBUFFSIZE - 1);

        /* Setup transfer configuration */
        dmaRXDesc[i].xfercfg = DMA_XFERCFG_CFGVALID | DMA_XFERCFG_SETINTA |
                               DMA_XFERCFG_WIDTH_8 | DMA_XFERCFG_SRCINC_0 |
                               DMA_XFERCFG_DSTINC_1 | DMA_XFERCFG_RELOAD |
                               DMA_XFERCFG_XFERCOUNT(UARTRXBUFFSIZE);
    }

    /* Setup transfer descriptor and validate it */
    Chip_DMA_SetupTranChannel(LPC_DMA, DMAREQ_USART0_RX, &dmaRXDesc[0]);

    /* Setup data transfer */
    Chip_DMA_SetupChannelTransfer(LPC_DMA, DMAREQ_USART0_RX,
                                  dmaRXDesc[0].xfercfg);
    Chip_DMA_SetValidChannel(LPC_DMA, DMAREQ_USART0_RX);
    Chip_DMA_SWTriggerChannel(LPC_DMA, DMAREQ_USART0_RX);
}

/* Check and return UART RX data if it exists */
//static int checkRxData(uint8_t *buff)
//{
//    int bytesRec = 0;

//    if (uartRxAvail) {
//        uartRxAvail = false;

//        memcpy(buff, dmaRXBuffs, UARTRXBUFFSIZE);
//        uartRXBuff++;
//        if (uartRXBuff >= UARTRXDESC) {
//            uartRXBuff = 0;
//        }
//        bytesRec = UARTRXBUFFSIZE;
//    }

//    return bytesRec;
//}

/* Clear an error on a DMA channel */
static void dmaClearChannel(DMA_CHID_T ch)
{
    Chip_DMA_DisableChannel(LPC_DMA, ch);
    while ((Chip_DMA_GetBusyChannels(LPC_DMA) & (1 << ch)) != 0)
    {

    }
    Chip_DMA_AbortChannel(LPC_DMA, ch);
    Chip_DMA_ClearErrorIntChannel(LPC_DMA, ch);
}

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/**
 * @brief	DMA Interrupt Handler
 * @return	None
 */
void DMA_IRQHandler(void)
{
    uint32_t errors, pending;

    /* Get DMA error and interrupt channels */
    errors = Chip_DMA_GetErrorIntChannels(LPC_DMA);
    pending = Chip_DMA_GetActiveIntAChannels(LPC_DMA);

    /* Check DMA interrupts of UART 0 RX channel */
    if ((errors | pending) & (1 << DMAREQ_USART0_RX)) {
        /* Clear DMA interrupt for the channel */
        Chip_DMA_ClearActiveIntAChannel(LPC_DMA, DMAREQ_USART0_RX);

        /* Handle errors if needed */
        if (errors & (1 << DMAREQ_USART0_RX)) {
            /* DMA error, channel needs to be reset */
            dmaClearChannel(DMAREQ_USART0_RX);
            dmaRXSetup();
            dmaRXQueue();
        }
        else {
            uartRxAvail = true;
        }
    }
}

/**
 * @brief	MRT IRQ handler
 * @return	None
 */
void MRT_IRQHandler(void)
{
    uint32_t int_pend;
    /* Get interrupt pending status for all timers */
    int_pend = Chip_MRT_GetIntPending();
    /* Channel 1 is periodic, toggle on either interrupt */
    if (int_pend & MRT1_INTFLAG) {
        QuickJack_Manchester_Encode();
    }
    /* clear interrupt flag */
    Chip_MRT_ClearIntPending(int_pend);
}


/**
 * @brief	Main UART/DMA program body
 * @return	Does not exit
 */
int main(void)
{
    int i = 0;
    //uint8_t bytes = 0;
    
	//uint8_t buff[UARTRXBUFFSIZE];

    SystemCoreClockUpdate();
    Board_Init();

    Init_UART_PinMux();

    Chip_UART_Init(LPC_USART0);
    Chip_UART_ConfigData(LPC_USART0, UART_CFG_DATALEN_8 | UART_CFG_PARITY_NONE | UART_CFG_STOPLEN_1);
    Chip_Clock_SetUSARTNBaseClockRate((115200 * 16), true);
    Chip_UART_SetBaud(LPC_USART0, 2400);
    Chip_UART_Enable(LPC_USART0);
    Chip_UART_TXEnable(LPC_USART0);

    /* DMA initialization - enable DMA clocking and reset DMA if needed */
    Chip_DMA_Init(LPC_DMA);

    /* Enable DMA controller and use driver provided DMA table for current descriptors */
    Chip_DMA_Enable(LPC_DMA);
    Chip_DMA_SetSRAMBase(LPC_DMA, DMA_ADDR(Chip_DMA_Table));

    /* Setup UART 0 RX DMA support */
    dmaRXSetup();

    /* Enable the DMA IRQ */
    //NVIC_EnableIRQ(DMA_IRQn);

    Chip_MRT_Init();

    /* Set MRT timing parameter */
    Chip_MRT_SetInterval(LPC_MRT_CH1, ((SystemCoreClock / QUICKJACKCOMMUNICATIONCLK) >> 1));
    NVIC_EnableIRQ(MRT_IRQn);

    Chip_MRT_SetEnabled(LPC_MRT_CH1);

    /* Receive buffers are queued. The DMA interrupt will only trigger on a
       full DMA buffer receive, so if the UART is idle, but the DMA is only
       partially complete, the DMA interrupt won't fire. For UART data
       receive where data is not continuous, a timeout method will be
       required to flush the DMA when the DMA has pending data and no
       data has been received on the UART in a specified timeout */
    dmaRXQueue();
    for(i = 0; i < 20; i++)
    {
        dmaRXBuffs[i] = '\r';
    }
	QuickJack_Data_Tx(0xAA); /* HandShake Protocol AA 55 AA 55 */
	QuickJack_Data_Tx(0x55);
	QuickJack_Data_Tx(0xAA);
	QuickJack_Data_Tx(0x55);	
    while (1) {
        while(dmaRXBuffs[0] == '\r');
        myDelay(1000);
        //memcpy(rx_data1, dmaRXBuffs, UARTRXBUFFSIZE);
		i = 0;
		while(dmaRXBuffs[i] != '\r')
		{
			QuickJack_Data_Tx(dmaRXBuffs[i]);
			dmaRXBuffs[i] = '\r';
			i++;
		}
		dmaClearChannel(DMAREQ_USART0_RX);
        dmaRXSetup();
        dmaRXQueue();
    }
}
