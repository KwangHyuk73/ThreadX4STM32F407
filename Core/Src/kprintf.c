
/**
 ******************************************************************************
 *	File		:	kprintf.c
 *	Brief		:	C source file of kprintf function.
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

#include <stdarg.h>
#include <string.h>

#include "main.h"
#include "cmsis_os.h"

#include "kprintf.h"

extern UART_HandleTypeDef *console;

static unsigned char large[] = "0123456789ABCDEF";
static unsigned char small[] = "0123456789abcdef";

/*
 * Write a buffer to the debug output.
 */
int kprintBinary(const char *data, int len)
{
	int ret = 0;

	TX_INTERRUPT_SAVE_AREA

    /* Disable interrupts.  */
	TX_DISABLE

	while(len) {
		HAL_UART_Transmit(console, (void *)data, 1, 1);
	    data++;
	    ret++;
	    len--;
	}

    /* Restore interrupts.  */
    TX_RESTORE

	return ret;
}

/*
 * Print a string on the debug output.
 */
int kprintString(const char *str)
{
	return kprintBinary(str, strlen(str));
}

/*
 * Print a numeric value on the debug output.
 */
int kprintInteger(unsigned long val, unsigned char radix, unsigned char width, unsigned char flags)
{
	unsigned char *digits;
	unsigned char raw[16];
	int result;
	unsigned char rem;
	unsigned char sign;
	unsigned char cnt;

	if(radix < 2)
	    radix = 10;
	result = 0;

	sign = 0;
	if(flags & FMTFLG_PLUS)
	    sign = '+';
    if(flags & FMTFLG_SIGNED)
	    if((long)val < 0) {
	        sign = '-';
	        val = -(long)val;
	    }

	/*
	 * Fill scratch buffer with raw digits.
	 */
	digits = (flags & FMTFLG_CAPITAL) ? large : small;
	cnt = 0;
	do {
	    rem = (unsigned char)(val % (unsigned long)radix);
	    val = val / (unsigned long)radix;
	    raw[cnt++] = digits[rem];
	} while(val);

    /*
     * Calculate the remaining width for padding
     */
    if(width > cnt) {
        if(sign)
            width--;
        width -= cnt;
    }
    else
        width = 0;

    /*
     * If not zero padded and not left justified,
     * we put enough spaces in front.
     */
    if((flags & (FMTFLG_ZERO | FMTFLG_MINUS)) == 0) {
        while(width) {
        	kprintBinary(" ", 1);
            result++;
            width--;
        }
    }
    if(sign) {
    	kprintBinary((const char*)(&sign), 1);
        result++;
    }
    if((flags & (FMTFLG_ZERO | FMTFLG_MINUS)) == FMTFLG_ZERO) {
        result += width;
        while(width) {
        	kprintBinary("0", 1);
            width--;
        }
    }
    while(cnt-- > 0) {
    	kprintBinary((const char*)(&raw[cnt]), 1);
        result++;
    }
    while(width) {
    	kprintBinary(" ", 1);
        result++;
        width--;
    }

    return result;
}

/*
 * Print parameters using a format string.
 */
int kprintf(const char *fmt, ...)
{
    va_list ap;
    unsigned char *s;
    unsigned char *cp;
    int width;
    int result;
    unsigned short len;
    unsigned long val;
    unsigned char radix;
    unsigned char flags;
    unsigned char isize;

    va_start(ap, fmt);
    for(result = 0; *fmt; ++fmt) {
        if(*fmt != '%') {
            if(kprintBinary(fmt, 1) < 0)
                return -1;
            result++;
            continue;
        }

        flags = 0;
        while(1) {
            if(*++fmt == '-')
                flags |= FMTFLG_MINUS;
            else if(*fmt == '+')
                flags |= FMTFLG_PLUS;
            else if(*fmt == '0')
                flags |= FMTFLG_ZERO;
            else
                break;
        }

        if(*fmt == '*') {
            ++fmt;
            width = va_arg(ap, int);
            if(width < 0) {
                width = -width;
                flags |= FMTFLG_MINUS;
            }
        }
        else {
            width = 0;
            while(*fmt >= '0' && *fmt <= '9')
                width = width * 10 + (*fmt++ - '0');
        }

        isize = 's';
        if(*fmt == 'l') {
            isize = *fmt;
            ++fmt;
        }

        if(*fmt == 'c' || *fmt == 's') {
            s = 0;
            if(*fmt == 's') {
                if((s = ((unsigned char*)va_arg(ap, char *))) == 0) {
                    s = (unsigned char*)"(NULL)";
                    len = 6;
                }
                else {
                    len = 0;
                    cp = s;
                    while(*cp++)
                        len++;
                }
            }
            else
                len = 1;
            if((flags & FMTFLG_MINUS) == 0)
                while(width > len) {
                	kprintBinary(" ", 1);
                    result++;
                    width--;
                }
            if(s) {
            	kprintBinary((const char*)s, len);
                result += len;
            }
            else {
            	unsigned char ch = (unsigned char)va_arg(ap, int);
            	kprintBinary((const char*)&ch, 1);
                result++;
            }
            while(width > len) {
            	kprintBinary(" ", 1);
                result++;
                width--;
            }
            continue;
        }

        radix = 10;
        if(*fmt == 'o')
            radix = 8;
        else if(*fmt == 'X' || *fmt == 'x') {
            if(*fmt == 'X')
                flags |= FMTFLG_CAPITAL;
            radix = 16;
        }
        else if(*fmt == 'd')
            flags |= FMTFLG_SIGNED;
        else if(*fmt != 'u') {
            if(*fmt != '%') {
            	kprintBinary("%", 1);
                result++;
            }
            if(*fmt) {
            	kprintBinary(fmt, 1);
                result++;
            }
            else
                fmt--;
            continue;
        }
        if(isize == 'l') {
            if(flags & FMTFLG_SIGNED)
                val = va_arg(ap, long);
            else
                val = va_arg(ap, unsigned long);
        }
        else {
            if(flags & FMTFLG_SIGNED)
                val = va_arg(ap, int);
            else
                val = va_arg(ap, unsigned int);
        }
        result += kprintInteger(val, radix, width, flags);
    }
    va_end(ap);

    return result;}
