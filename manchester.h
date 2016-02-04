/*
 * @brief Common types used in NXP Quick_Jack Manchester functions
 *
 * @par
 * Copyright (c) 2010 The Regents of the University of Michigan, NXP Semiconductors 2014
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the
 *  distribution.
 * - Neither the name of the copyright holder nor the names of
 *  its contributors may be used to endorse or promote products derived
 *  from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Original author: Thomas Schmid.
 * Modified by NXP Semiconductors (2014).
 */

#ifndef __NXP_QUICK_JACK_MANCHESTER_H_
#define __NXP_QUICK_JACK_MANCHESTER_H_

/** @defgroup QUICKJACK MANCHESTER FUNCTION: NXP Quick_Jack manchester initialize, decode & encode function
 * @ingroup QUICKJACK
 * @{
 */

/**
 * @brief NXP Quick_Jack manchester function - NXP Quick_Jack manchester hardware function
 */

/*!< NXP Quick_Jack audio clock definition */
#define QUICKJACKAUDIOCLK										(44100)
/*!< NXP Quick_Jack communication clock definition = HjAudioClk/32 = 1378.125Hz = 1378Hz */
#define QUICKJACKCOMMUNICATIONCLK			  		(QUICKJACKAUDIOCLK >> 0x5)
/*!< NXP Quick_Jack receive clock sample definition */
#define QUICKJACKRXCLKSAMPLEBITS			  			(32)
/*!< NXP Quick_Jack receive clcok timing min value */
#define QUICKJACKRXCOUNTERMIN			  				(QUICKJACKRXCLKSAMPLEBITS/2 + QUICKJACKRXCLKSAMPLEBITS/4)
/*!< NXP Quick_Jack receive clcok timing max value */
#define QUICKJACKRXCOUNTERMAX			  				(QUICKJACKRXCLKSAMPLEBITS + QUICKJACKRXCLKSAMPLEBITS/2)

/* NXP Quick_Jack decode & encode states. */
typedef enum
{
    /*!< NXP Quick_Jack status */
    QUICKJACK_STARTBIT	= 0U,			/*!< NXP Quick_Jack start bit status */
    QUICKJACK_STARTBIT_FALL,			/*!< NXP Quick_Jack detect falling edge status */
    QUICKJACK_DECODE,							/*!< NXP Quick_Jack decode status */
    QUICKJACK_STOPBIT,						/*!< NXP Quick_Jack stop bit status */
    QUICKJACK_BYTE,								/*!< NXP Quick_Jack send byte status */
    QUICKJACK_IDLE,								/*!< NXP Quick_Jack idle status */
    QUICKJACK_STARTBIT0,					/*!< NXP Quick_Jack start bit 0 status */
    QUICKJACK_STARTBIT1,					/*!< NXP Quick_Jack start bit 1 status */
    QUICKJACK_STARTBIT2,					/*!< NXP Quick_Jack start bit 2 status */
    QUICKJACK_SENDBIT,						/*!< NXP Quick_Jack send bit status */
} QUICKJACK_STATE_T;

/**
 * @brief NXP Quick_Jack public parameter
 */
/*!< NXP Quick_Jack PhoneData: data from Mobile Phone to MCU */
extern volatile uint8_t	PhoneData;

/*!< NXP Quick_Jack PhoneData: data from Mobile Phone to MCU */
extern volatile uint8_t	DataRecvFlag;			// 1 -- new data arrived
/*!< NXP Quick_Jack Receive timeing counter */
extern volatile uint32_t QuickJackRxCounter;

/**
 * @brief NXP Quick_Jack Manchester initialize function
 */
void QuickJack_Manchester_Init(void);

/**
 * @brief NXP Quick_Jack Manchester decode function
 */
void QuickJack_Manchester_Decode(uint32_t DataTime);

/**
 * @brief NXP Quick_Jack Manchester encode function
 */
void QuickJack_Manchester_Encode(void);

/**
 * @brief MCU send data to Mobile phone function
 */
uint8_t	QuickJack_Data_Tx(uint8_t Data);

/**
 * @}
 */

#endif /* __NXP_QUICK_JACK_MANCHESTER_H_ */
