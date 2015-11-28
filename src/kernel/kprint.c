/*
 * alOS
 * Copyright (C) 2015 Alexandre Monti
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "kernel/kprint.h"
#include "kernel/kmutex.h"
#include "platform.h"
#include <stdarg.h>
#include <string.h>

///////////////////////////
//// Module parameters ////
///////////////////////////

// N/A

////////////////////////////////
//// Module's sanity checks ////
////////////////////////////////

// N/A

//////////////////////////////
//// Module's definitions ////
//////////////////////////////

// N/A

///////////////////////////////////////
//// Module's forward declarations ////
///////////////////////////////////////

// N/A

/////////////////////////////////////
//// Module's internal variables ////
/////////////////////////////////////

//! Mutex to protect the ITM debug port
static kmutex itm_mutex = 0;

/////////////////////////////////////
//// Module's internal functions ////
/////////////////////////////////////

//! Output a character to the ITM SWO port 0
//! \param c The character to output
//! \return 0 if the write succeeded, -1 otherwise
static int itm_putc(int c)
{
    if((ITM->TCR & (0x01 << 0)) && (ITM->TER & (0x01 << 0)))
    {
        kmutex_lock(&itm_mutex);

        while(ITM->PORT[0].u32 == 0)
            ;
        ITM->PORT[0].u8 = (uint8_t)c;

        kmutex_unlock(&itm_mutex);

        return 0;
    }

    return -1;
}

/////////////////////////////
//// Public module's API ////
/////////////////////////////

int kprint_init()
{
    CoreDebug->DEMCR |= (0x01 << 24); // TRCENA = 1

    *((unsigned int*)0xE0000FB0) = 0xC5ACCE55; // Unlock ITM registers

    ITM->TCR |= (0x01 << 0); // ITMENA = 1
    ITM->TCR |= (0x01 << 4); // SWOENA = 1
    ITM->TCR |= (0x01 << 3); // TXENA = 1

    ITM->TER |= (0x01 << 0); // Enable channel 0
    ITM->TPR |= (0x01 << 0); // Unmask channels 0:7

    return 0;
}

void __attribute__((format(printf, 1, 2))) kprint(const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);

    int len = strlen(fmt);
    for(int i = 0; i < len; ++i)
    {
        char c = fmt[i];

        if(c == '%')
        {
            if(++i >= len)
                break;
            c = fmt[i];

            if(c == 's')
            {
                const char* arg = va_arg(va, const char*);
                if(!arg)
                {
                    kprint("<null>");
                }
                else
                {
                    int arg_len = strlen(arg);
                    for(int j = 0; j < arg_len; ++j)
                        itm_putc(arg[j]);
                }
            }
            else
                itm_putc(c);
        }
        else
            itm_putc(c);
    }

    va_end(va);
}
