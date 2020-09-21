/*
    FreeRTOS V7.6.0 - Copyright (C) 2013 Real Time Engineers Ltd. 
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that has become a de facto standard.             *
     *                                                                       *
     *    Help yourself get started quickly and support the FreeRTOS         *
     *    project by purchasing a FreeRTOS tutorial book, reference          *
     *    manual, or both from: http://www.FreeRTOS.org/Documentation        *
     *                                                                       *
     *    Thank you!                                                         *
     *                                                                       *
    ***************************************************************************

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    >>! NOTE: The modification to the GPL is included to allow you to distribute
    >>! a combined work that includes FreeRTOS without being obliged to provide
    >>! the source code for proprietary components outside of the FreeRTOS
    >>! kernel.

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available from the following
    link: http://www.freertos.org/a00114.html

    1 tab == 4 spaces!

    ***************************************************************************
     *                                                                       *
     *    Having a problem?  Start by reading the FAQ "My application does   *
     *    not run, what could be wrong?"                                     *
     *                                                                       *
     *    http://www.FreeRTOS.org/FAQHelp.html                               *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org - Documentation, books, training, latest versions,
    license and Real Time Engineers Ltd. contact details.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.OpenRTOS.com - Real Time Engineers ltd license FreeRTOS to High
    Integrity Systems to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/* BASIC INTERRUPT DRIVEN SERIAL PORT DRIVER.   
 * 
 * This file only supports UART 0
 */

/* Standard includes. */
#include <stdlib.h>

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/* Demo application includes. */
#include "irq.h"
#include "serial.h"
#include <uart/drv_uart.h>

extern u8 gOsFromISR;
u32       gOsInFatal = 0;

/* The queue used to hold received characters. */
static xQueueHandle			xRxedChars = NULL;

/* The queue used to hold characters waiting transmission. */
static xQueueHandle			xCharsForTx = NULL;

static volatile portSHORT	sTHREEmpty;

static volatile portSHORT	queueFail = pdFALSE;

void UART0_RxISR( void *custom_data );
void UART0_TxISR( void *custom_data );

/*-----------------------------------------------------------*/
xComPortHandle xSerialPortInitMinimal( unsigned portLONG ulWantedBaud, unsigned portBASE_TYPE uxQueueLength )
{
	(void) ulWantedBaud;
	
	/* Initialise the hardware. */
	portENTER_CRITICAL();
	{
		/* Create the queues used by the com test task. */
		xRxedChars = xQueueCreate( uxQueueLength, ( unsigned portBASE_TYPE ) sizeof(signed portCHAR) );
		xCharsForTx = xQueueCreate( uxQueueLength, ( unsigned portBASE_TYPE ) sizeof(signed portCHAR) );

		if( xRxedChars == 0 )
		{
			queueFail = pdTRUE;
		}

		if( xCharsForTx == 0 )
		{
			queueFail = pdTRUE;
		}

		sTHREEmpty = pdTRUE;
		/* Initialize UART asynchronous mode */
		// -F BGR0 = configCLKP1_CLOCK_HZ / ulWantedBaud;

		// -F SCR0 = 0x17;	/* 8N1 */
		// -F SMR0 = 0x0d;	/* enable SOT3, Reset, normal mode */
		// -F SSR0 = 0x02;	/* LSB first, enable receive interrupts */

		// -F PIER08_IE2 = 1; /* enable input */
		// -F DDR08_D2 = 0;	/* switch P08_2 to input */
		// -F DDR08_D3 = 1;	/* switch P08_3 to output */
		
		/* Register interrupt handler */
		irq_request(IRQ_UART0_TX, UART0_TxISR, (void *)NULL);
		irq_request(IRQ_UART0_RX, UART0_RxISR, (void *)NULL);
		drv_uart_enable_rx_int_0();		
	}
	portEXIT_CRITICAL();

	/* Unlike other ports, this serial code does not allow for more than one
	com port.  We therefore don't return a pointer to a port structure and can
	instead just return NULL. */
	return NULL;
}
/*-----------------------------------------------------------*/

signed portBASE_TYPE xSerialGetChar( xComPortHandle pxPort, signed portCHAR *pcRxedChar, portTickType xBlockTime )
{
	// In case of serial driver has not been initialized yet or invoked in interrupt.
	if (gOsFromISR || gOsInFatal || (xRxedChars == NULL))
		do {
			s32	ret = drv_uart_rx_0();
			if (ret == (-1))
				continue;
			*pcRxedChar = (signed portCHAR)ret;
			return pdTRUE;
		} while (1);

	/* Get the next character from the buffer.  Return false if no characters
	are available, or arrive before xBlockTime expires. */
	//drv_uart_tx_0('g');
	if( xQueueReceive(xRxedChars, pcRxedChar, xBlockTime) )
	{
		return pdTRUE;
	}
	else
	{
		return pdFALSE;
	}
}
/*-----------------------------------------------------------*/
signed portBASE_TYPE xSerialPutChar( xComPortHandle pxPort, signed portCHAR cOutChar, portTickType xBlockTime )
{
	signed portBASE_TYPE	xReturn;

	// In case of serial driver has not been initialized yet or invoked in interrupt.
	if (gOsFromISR || gOsInFatal || xCharsForTx == NULL)
	{
		do {
			if (drv_uart_is_tx_busy_0())
				continue;
			if (cOutChar == '\n')
			{
				drv_uart_tx_0(0x0a);
				drv_uart_tx_0(0x0d);
			}
			else
				drv_uart_tx_0(cOutChar);
			return pdPASS;
		} while (1);
	}

	/* Transmit a character. */
	portENTER_CRITICAL();
	{
		if (sTHREEmpty == pdTRUE)
		{
			/* If sTHREEmpty is true then the UART Tx ISR has indicated that 
			there are no characters queued to be transmitted - so we can
			write the character directly to the shift Tx register. */
			sTHREEmpty = pdFALSE;
			if (cOutChar == '\n')
			{
				drv_uart_tx_0(0x0a);
				drv_uart_tx_0(0x0d);
			}
			else
				drv_uart_tx_0(cOutChar);
			xReturn = pdPASS;
		}
		else
		{
			/* sTHREEmpty is false, so there are still characters waiting to be
			transmitted.  We have to queue this character so it gets 
			transmitted	in turn. */

			/* Return false if after the block time there is no room on the Tx 
			queue.  It is ok to block inside a critical section as each task
			maintains it's own critical section status. */
			
			//drv_uart_tx_0('p');
			if (cOutChar == '\n')
			{
				signed portCHAR return_seq_0 = (signed portCHAR)0x0a;
				signed portCHAR return_seq_1 = (signed portCHAR)0x0d;
				signed portBASE_TYPE ret = xQueueSend(xCharsForTx, &return_seq_0, xBlockTime);
				if (ret == pdTRUE)
					ret = xQueueSend(xCharsForTx, &return_seq_1, xBlockTime);
				xReturn = (ret == pdTRUE) ? pdPASS : pdFAIL;

			}
			else
			{
				if( xQueueSend(xCharsForTx, &cOutChar, xBlockTime) == pdTRUE )
				{
					xReturn = pdPASS;
				}
				else
				{
					xReturn = pdFAIL;
				}
			}
		}

		if( pdPASS == xReturn )
		{
			/* Turn on the Tx interrupt so the ISR will remove the character from the
			queue and send it.   This does not need to be in a critical section as
			if the interrupt has already removed the character the next interrupt
			will simply turn off the Tx interrupt again. */
			drv_uart_enable_tx_int_0();
		}
	}
	portEXIT_CRITICAL();

	return pdPASS;
}
/*-----------------------------------------------------------*/


void vSerialPutString( xComPortHandle pxPort, const signed char * const pcString, unsigned short usStringLength )
{
	signed portBASE_TYPE	xReturn;

	// In case of serial driver has not been initialized yet or invoked in interrupt.
	if (gOsFromISR || gOsInFatal || xCharsForTx == NULL)
	{
		unsigned short i;
		for (i = 0; i < usStringLength; i++) 
		{
			while (drv_uart_is_tx_busy_0()) {}
			if (pcString[i] == '\n')
			{
				drv_uart_tx_0(0x0a);
				drv_uart_tx_0(0x0d);
			}
			else
				drv_uart_tx_0(pcString[i]);
		}
		return;
	}

	/* Transmit a character. */
	portENTER_CRITICAL();
	{
		unsigned short usFIFOLength = drv_uart_get_uart_fifo_length_0();
		unsigned short usSendToFIFOLength = usFIFOLength;
		unsigned short usSendToQueueLength = usStringLength;
		unsigned short i;
		const signed char * pcChar = pcString;
		
		if (sTHREEmpty == pdTRUE)
		{
			if (usSendToFIFOLength >= usStringLength) 
			{
				usSendToFIFOLength = usStringLength;
				usSendToQueueLength = 0;
			}
			else
			{
				usSendToQueueLength = usStringLength - usSendToFIFOLength;
			}
		
			/* If sTHREEmpty is true then the UART Tx ISR has indicated that 
			there are no characters queued to be transmitted - so we can
			write the character directly to the shift Tx register. */
			sTHREEmpty = pdFALSE;
			for (i = 0; i < usSendToFIFOLength; i++, pcChar++)
			{
				if (*pcChar == '\n')
				{
					if (usSendToFIFOLength == usFIFOLength)
					{
						usSendToQueueLength++;
					}
					else
					{
						usSendToFIFOLength++;
					}
					i++;
					drv_uart_tx_0(0x0a);
					drv_uart_tx_0(0x0d);
				}
				else
					drv_uart_tx_0(*pcChar);
			}
			xReturn = pdPASS;
		}
		
		if (usSendToQueueLength > 0)
		{
			/* sTHREEmpty is false, so there are still characters waiting to be
			transmitted.  We have to queue this character so it gets 
			transmitted	in turn. */

			/* Return false if after the block time there is no room on the Tx 
			queue.  It is ok to block inside a critical section as each task
			maintains it's own critical section status. */
			
			//drv_uart_tx_0('s');
			for (i = 0; i < usSendToQueueLength; i++, pcChar++)
				if (*pcChar == '\n')
				{
					signed portCHAR return_seq_0 = (signed portCHAR)0x0a;
					signed portCHAR return_seq_1 = (signed portCHAR)0x0d;
					xQueueSend(xCharsForTx, &return_seq_0, portMAX_DELAY);
					xQueueSend(xCharsForTx, &return_seq_1, portMAX_DELAY);
				}
				else
					xQueueSend(xCharsForTx, pcChar, portMAX_DELAY);
			xReturn = pdPASS;
		}

		if( pdPASS == xReturn )
		{
			/* Turn on the Tx interrupt so the ISR will remove the character from the
			queue and send it.   This does not need to be in a critical section as
			if the interrupt has already removed the character the next interrupt
			will simply turn off the Tx interrupt again. */
			drv_uart_enable_tx_int_0();
		}
	}
	portEXIT_CRITICAL();
}


/*
 * UART RX interrupt service routine.
 */
void UART0_RxISR( void *custom_data )
{
	s32 ret;
	signed portCHAR	cChar;
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

	/* Get the character from the UART and post it on the queue of Rxed 
	characters. */
	
	//drv_uart_tx_0('R');
	do {
		portBASE_TYPE ret_woken = pdFALSE;
		ret = drv_uart_rx_0();
		if (ret == (-1))
			break;
		cChar = (signed portCHAR)ret;

		xQueueSendFromISR( xRxedChars, ( const void *const ) &cChar, &ret_woken );
		xHigherPriorityTaskWoken = (xHigherPriorityTaskWoken || ret_woken);
	} while (1);
#if 0
	if( xHigherPriorityTaskWoken )
	{
		/*If the post causes a task to wake force a context switch 
		as the woken task may have a higher priority than the task we have 
		interrupted. */
		portYIELD_FROM_ISR();
	}
#endif
}
/*-----------------------------------------------------------*/

/*
 * UART Tx interrupt service routine.
 */
void UART0_TxISR( void *custom_data )
{
signed portCHAR			cChar;
signed portBASE_TYPE	xTaskWoken = pdFALSE;

	/* The previous character has been transmitted.  See if there are any
	further characters waiting transmission. */
	//drv_uart_tx_0('t');
	if( xQueueIsQueueEmptyFromISR(xCharsForTx) == pdTRUE)
	{
		/* There were no other characters to transmit. */
		sTHREEmpty = pdTRUE;

		/* Disable transmit interrupts */
		drv_uart_disable_tx_int_0();
	}
	else
	{	
		//drv_uart_tx_0('T');
		unsigned portLONG ulNumCharToSend = drv_uart_get_uart_fifo_length_0();
		while (ulNumCharToSend--) {
			if( xQueueReceiveFromISR(xCharsForTx, &cChar, &xTaskWoken) == pdTRUE )
			{
				/* There was another character queued - transmit it now. */
				drv_uart_tx_0(cChar);
			}
			else
				break;
		}
	}
}
