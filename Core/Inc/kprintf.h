/**
 ******************************************************************************
 *	File		:	kprintf.h
 *	Brief		:	Header file of kprintf function.
 *	Created on	:	Sep 17, 2021
 *	Author		:	William, An.
 *	Email		:	ponytail2k@gmail.com
 ******************************************************************************
 *
 * Copyright (c) 2021 Lee & An Partners Co., Ltd. All rights reserved.
 *
 * This software component is licensed by Lee & An Partners under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/*
 * Copyright (C) 2001-2004 by egnite Software GmbH. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * For additional information see http://www.ethernut.de/
 */

#ifndef INC_CRT_KPRINTF_H_
#define INC_CRT_KPRINTF_H_

#define FMTFLG_ZERO     0x01    /*!< \brief Set, if zero padding required */
#define FMTFLG_SIGNED   0x02    /*!< \brief Set, if signed value */
#define FMTFLG_PLUS     0x04    /*!< \brief Set to force sign */
#define FMTFLG_MINUS    0x08    /*!< \brief Set to force left justification */
#define FMTFLG_CAPITAL  0x10    /*!< \brief Set for capital letter digits */

extern int kprintBinary(const char *data, int len);
extern int kprintString(const char *str);
extern int kprintInteger(unsigned long val, unsigned char radix, unsigned char width, unsigned char flags);
extern int kprintf(const char *fmt, ...);

#endif /* INC_CRT_KPRINTF_H_ */
