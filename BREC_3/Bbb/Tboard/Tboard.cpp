//
//
// This source code is available under the "Simplified BSD license".
//
// Copyright (c) 2016, J. Kleiner
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, 
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright 
//    notice, this list of conditions and the following disclaimer in the 
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the original author nor the names of its contributors 
//    may be used to endorse or promote products derived from this software 
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
// OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "Tboard.h"

#include "Util/mcf.h"
#include "Util/gpioutil.h"

#include "Iboard/Iboard.h"
#include "Bdc/Bdc.h"

////////////////////////////////////////////////////////////////////////////////
Tboard::Tboard()
{
    mDbg = 0xffffffff;
}

////////////////////////////////////////////////////////////////////////////////
int
Tboard::Attach( void *lvl0, void *lvl1 )
{
    int portN;
    int err;

    printf("Tboard:Open Enter\n");

    // Port to use is second level designator
    portN = (unsigned int)lvl1;

    if( FindCapeByName( "brecFpru" ) || FindCapeByName( "brecFjtag" ) ){
       printf("Tboard:Open F board\n");

       Bdc     *bdc;

       bdc  = (Bdc*)lvl0;

       // IO1 = p7 = "ss2"   = SCL
       // IO2 = p8 = "stat"  = SDA
       err = mUI2C.configure( 
                  bdc->GetGpioPin(portN,7), 
                  bdc->GetGpioPin(portN,8)
             );
       printf("%s:mUI2C  configure err=0x%08x\n",__FILE__,err);

    }
    else{
       printf("Tboard:Open I board\n");

       Iboard        *ibrd;
       Gpio6PinGroup *g6pg;

       ibrd = (Iboard*)lvl0;

       g6pg = ibrd->AllocPort( portN ); 
       ibrd->EnablePort( portN, 1 );

       // IO1 = p7 = "ss2"   = SCL
       // IO2 = p8 = "stat"  = SDA
       err = mUI2C.configure( g6pg->GetSs2(), g6pg->GetStat() );
       printf("%s:mUI2C  configure err=0x%08x\n",__FILE__,err);

    }

    err = mDAC.Configure( &mUI2C );
    printf("%s:mDAC   configure err=0x%08x\n",__FILE__,err);

    err = mTUNER.Configure( &mUI2C );
    printf("%s:mTUNER configure err=0x%08x\n",__FILE__,err);

    return( 0 );
}

////////////////////////////////////////////////////////////////////////////////
void
Tboard::Detach()
{
}

////////////////////////////////////////////////////////////////////////////////
void
Tboard::Show()
{
}

////////////////////////////////////////////////////////////////////////////////
uint32_t
Tboard::DacSet( int value )
{
    return( mDAC.Set( 0xC6, value ) );
}

////////////////////////////////////////////////////////////////////////////////
uint32_t
Tboard::DacGet( int *vvalue )
{
    return( mDAC.Get( 0xC6, vvalue ) );
}

////////////////////////////////////////////////////////////////////////////////
double
Tboard::SetFreqHz( double hzTgt )
{
   return( mTUNER.SetFreqHz(hzTgt) );
}

////////////////////////////////////////////////////////////////////////////////
void
Tboard::ShowTuner()
{
   mTUNER.Show();
}