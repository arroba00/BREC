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
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <math.h>
#include <string.h>

#include "Fboard/Fboard.h"
#include "Ddc100.h"
#include "pruinc.h"

#include "prussdrv.h"
#include "pruss_intc_mapping.h"
#include "pru_images.h"

/**
 * NOTE: The interfaces here are not mutex safe.  Specifically the
 * get samples vs the flush.  In the case of I/Q data, it may misalign
 * the I/Q samples to I(n),Q(n+1) depending on when the flush occurs
 * relative to sample extraction.
 *
 * All callers must ensure mutex.
 *
 */

////////////////////////////////////////////////////////////////////////////////
/// Hardware definitions ///////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define GetSramWord( off )   ( *(unsigned int*  )((mPtrPruSram + (off))) )
#define SetSramWord( v,off ) ( *(unsigned int*  )((mPtrPruSram + (off))) = (v) )

#define GetSramShort( off )  ( *(unsigned short*)((mPtrPruSram + (off))) )
#define SetSramShort( v,off ) (*(unsigned short*)((mPtrPruSram + (off))) = (v) )

////////////////////////////////////////////////////////////////////////////////
/// External Methods ///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Ddc100::Ddc100()
{
    mTpg     = 4; 
    mFifoSrc = 0;
    mFsHz    = 10000000; // Function of the board
    mCSPS    = mFsHz;    // Function of board and channel selected
}

//------------------------------------------------------------------------------
void
Ddc100::Attach( Bdc *bdc )
{
   mBdc = bdc;
}

//------------------------------------------------------------------------------
int
Ddc100::Open()
{
    int ret;
    unsigned short fwVer;

    printf("Ddc100:Open enter\n");

    fwVer = GetFwVersion();
    printf("Ddc100::Open::fw ver = 0x%08x\n",fwVer);

    mFsHz    = 40000000; // Function of the board
    mCSPS    = mFsHz;    

    printf("Ddc100::Open::FsHz        = %d\n",mFsHz);
    printf("Ddc100::Open::CSPS        = %d\n",mCSPS);

    // Set startup default signal paramters
    SetLoFreqHz( 640000 );

    SetSource( 0 ); 
    SetTpg( 0 ); 
    SetChannelMatch(0,1.0,0,1.0);

    printf("Ddc100:Open exit\n");

    return(0);
}

//------------------------------------------------------------------------------
int
Ddc100::SetChannelMatch( int Ioff, double Igain, int Qoff, double Qgain )
{
    int Inum,Qnum;

    Inum = -128 * Igain;
    Qnum = -128 * Qgain;

    mBdc->SpiRW16(  BDC_REG_WR | BDC_REG_R20 | (Ioff&0xff) );
    mBdc->SpiRW16(  BDC_REG_WR | BDC_REG_R21 | (Inum&0xff) );
    mBdc->SpiRW16(  BDC_REG_WR | BDC_REG_R22 | (Qoff&0xff) );
    mBdc->SpiRW16(  BDC_REG_WR | BDC_REG_R23 | (Qnum&0xff) );

    printf("Ddc100:Match: I=[+ %d, X %d (%f) ] Q=[+ %d, X %d (%f)]\n",
                Ioff,Inum,Igain, 
                Qoff,Qnum,Qgain
    );
}

//------------------------------------------------------------------------------
int
Ddc100::SetTpg( int arg )
{
    printf("Ddc100:SetTpg=%d\n",arg);
    mTpg = arg;

    mBdc->SpiRW16(  BDC_REG_WR | BDC_REG_R19 | (mTpg&0xff) );

    return(0);
}
//------------------------------------------------------------------------------
int
Ddc100::IsComplexFmt()
{
  return( 1 );
}

//------------------------------------------------------------------------------
int
Ddc100::SetSource( int arg )
{
    printf("Ddc100:SetSource=%d (mTpg=%d)\n",arg,mTpg);
    mFifoSrc = arg;

    mBdc->SpiRW16(  BDC_REG_WR | BDC_REG_R16 | (mFifoSrc&0xff) );

    Show( "at set\n" );
    return( arg );
}

//------------------------------------------------------------------------------
int
Ddc100::GetFwVersion()
{
    int ver;

    mBdc->SpiRW16(  BDC_REG_RD | BDC_REG_R0 );
    ver = mBdc->SpiRW16( 0 );

    FlushSamples(); // Necessary to start streaming

    return(ver);
}

//------------------------------------------------------------------------------
double
Ddc100::SetLoFreqHz( double freqHz )
{
    unsigned int   pincLo,pincHi;
    long long      hzMod,pinc;
    double         actHz;

    printf("Ddc100:SetFreq=%f Hz\n",freqHz);

    hzMod = ((long long)freqHz) % mFsHz;
    pinc  = hzMod * 65536 / mFsHz;
    pincLo=       pinc & 0x00ff;
    pincHi=  (pinc>>8) & 0x00ff;

    printf("Ddc100: pinc = %lld 0x%08x (0x%04x 0x%04x)\n",
                   pinc,(unsigned int)pinc,pincHi,pincLo);

    mBdc->SpiRW16( BDC_REG_WR | BDC_REG_R17 | pincLo );
    mBdc->SpiRW16( BDC_REG_WR | BDC_REG_R18 | pincHi );

    FlushSamples(); // Necessary to continue streaming

    actHz     = pinc * mFsHz / 65536;
    mLoFreqHz = actHz;
    printf("Ddc100:SetFreq= actual %f Hz\n",actHz);

    return( actHz );
}

//------------------------------------------------------------------------------
int
Ddc100::FlushSamples()
{
    // printf("Ddc100:FlushSamples Enter\n");

    // Stop the fpga acquisition

    // Flush the fpga fifos

    // TODO: Wait for the DRAM to drain to a constant spot

    // Start the fpga acquisition

    // TODO: Tell the pru to go back to streaming

    // Flush and restart the fifo
    mBdc->SpiRW16( BDC_REG_WR | BDC_REG_R16 | 0x40 | (mFifoSrc&0xff) );
    mBdc->SpiRW16( BDC_REG_WR | BDC_REG_R16 | (mFifoSrc&0xff) );

    return(0);
}

//------------------------------------------------------------------------------
int
Ddc100::Get2kSamples( short *bf )
{
    int          p;
    int          srcIdx,idx;
    unsigned int wd;

    // Transfer 2k samples with single word cpu xfers for now

    // Wait until the threashold bit inidcates samples ready
    do{
       wd = mBdc->SpiRW16( BDC_REG_RD | BDC_REG_R61  );
       if( !wd ) us_sleep( 100 );
    }while( wd!=0 );

    // Read 2k of samples
    mBdc->SpiRW16( BDC_REG_RD | BDC_REG_R63  );
    for(idx=0;idx<2047;idx++){
        bf[idx] = mBdc->SpiRW16( BDC_REG_RD | BDC_REG_R63 );
    }
    bf[idx] = mBdc->SpiRW16( 0 ); // Fetch last value but do not init read

    return( p );
}

//------------------------------------------------------------------------------
int
Ddc100::StartPrus()
{
    // NOTE: the constants share with pru code are relative to how it
    // references its sram which is zero based, however, cpu accesses
    // globally so pru1 is +0x2000
    mPtrPruSram    = mBdc->PruGetSramPtr() + 0x2000;
    mPtrPruSamples = mBdc->PruGetDramPtr();

    SetSramWord(  prussdrv_get_phys_addr( (void*)mPtrPruSamples ),
                  SRAM_OFF_DRAM_PBASE 
               );

    SetSramWord(  0,
                  SRAM_OFF_DBG1 
               );

    SetSramWord(  1,
                  SRAM_OFF_DBG2 
               );

    SetSramShort(  PRU1_CMD_NONE,
                  SRAM_OFF_CMD 
               );

    prussdrv_pru_write_memory(PRUSS0_PRU1_IRAM,0,
                             (unsigned int*)pru_image01,sizeof(pru_image01) );
    prussdrv_pru_enable(1);

    return( 0 );
}

//------------------------------------------------------------------------------
int
Ddc100::SetSim( int sim )
{
    // This method is not applicable
    return( 0 );
}

//------------------------------------------------------------------------------
int
Ddc100::SetComplexSampleRate( int complexSamplesPerSecond )
{
    printf("Ddc100:SetComplexSampleRate %d CSPS\n",complexSamplesPerSecond);
    printf("Ddc100:SetComplexSampleRate %d CSPS\n",mCSPS);
    return(0);
}

//------------------------------------------------------------------------------
int
Ddc100::GetComplexSampleRate()
{
    switch( mFifoSrc ){
        case 0  : return( mFsHz ); // raw input
        case 1  : return( mFsHz ); // equalized
        case 2  : return( mFsHz ); // dds/nco
        case 3  : return( mFsHz ); // mixer output
        case 4  : return( (mFsHz/10) );  // first stage decimation
        case 5  : return( (mFsHz/200) ); // second stage decimation
        default : return( mFsHz );
    }
}

//------------------------------------------------------------------------------
// NOTE: this adc interface is not supported
int
Ddc100::SetGain( int gn )
{
   if( gn!=0 ){
       fprintf(stderr,"Ddc100:SetGain invoked w/o 0 - err \n");
   }
   return( 0 );
}

//------------------------------------------------------------------------------
void
Ddc100::Show(const char *title )
{
    int rg,val;
    printf("Ddc100: %s",title);
    for(rg=0;rg<21;rg++){
        mBdc->SpiRW16( BDC_REG_RD | ((rg&0x3f)<<8) );
        val = mBdc->SpiRW16( 0 );
        printf("r[%02d] = 0x%04x\n",rg,val);
    }

#if 0
    if( mFbrd.PruIsAvail() ){
        printf("    PRU1 dbg1     0x%08x\n",GetSramWord( SRAM_OFF_DBG1 ) );
        printf("    PRU1 dbg2     0x%08x\n",GetSramWord( SRAM_OFF_DBG2 ) );
        printf("    PRU1 cmd      0x%08x\n",GetSramShort( SRAM_OFF_CMD ) );
        printf("    PRU1 res      0x%08x\n",GetSramShort( SRAM_OFF_RES ) );
        printf("    PRU1 pbase    0x%08x\n",GetSramWord( SRAM_OFF_DRAM_PBASE) );
        printf("    PRU1 dram off 0x%08x\n",GetSramWord( SRAM_OFF_DRAM_OFF) );
    }
    else{
        printf("    PRU not available\n");
    }
#endif 
}

