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
#ifndef __F_BOARD__
#define __F_BOARD__

#include "Util/mcf.h"
#include "Util/gpioutil.h"

class Fboard {

private:
    int                      mDbg;
    int                      mUsePru;

    volatile unsigned short *mPtrPruSamples;
    volatile unsigned char  *mPtrPruSram;

    GpioUtil                 mProgbGpio;
    GpioUtil                 mDoneGpio;
    GpioUtil                 mInitbGpio;

public:
    Fboard();
    int     Open();
    void    Close();
    void    Reset();
    void    Show();

    int                      PruStart();
    int                      PruIsAvail();
    volatile unsigned char  *PruGetSramPtr();
    volatile unsigned short *PruGetDramPtr();

    void    SpiXferStream8( unsigned char  *bf, int bfCount );
    void    SpiXferArray16( unsigned short *bf, int bfCount );
    void    SpiXferArray16x2( unsigned short *bf, int bfCount );
};

#endif
