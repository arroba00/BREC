//
//
// This source code is available under the "Simplified BSD license".
//
// Copyright (c) 2015, J. Kleiner
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

#include "../Util/gpioutil.h"
#include "Xboard.h"

#include "Xpru.h"
#include "prussdrv.h"
#include "pruss_intc_mapping.h"
#include "pru_images.h"

//------------------------------------------------------------------------------
#define XSPI_FIFO_EN     0x8000
#define XSPI_FIFO_RST    0x4000

#define XSPI_LED0        0x0100
#define XSPI_LED1        0x0200
#define XSPI_LED2        0x0400
#define XSPI_LED3        0x0800

#define XSPI_SEL_P1_CNTR 0x0000
#define XSPI_SEL_P1_FIFO 0x0010
#define XSPI_SEL_P1_R3   0x0030
#define XSPI_SEL_P1_R6   0x0060

#define XSPI_READ        0x0008
#define XSPI_WRITE       0x0004
#define XSPI_PORT1       0x0001
#define XSPI_PORT0       0x0000

#define XSPI_RP0   (XSPI_READE | XSPI_PORT0)
#define XSPI_WP0   (XSPI_WRITE | XSPI_PORT0)
#define XSPI_RP1   (XSPI_READ  | XSPI_PORT1)
#define XSPI_WP1   (XSPI_WRITE | XSPI_PORT1)

#define XSPI_STOP      (               XSPI_SEL_P1_FIFO | XSPI_LED0 | XSPI_WP0 )
#define XSPI_FLUSH     (XSPI_FIFO_RST| XSPI_SEL_P1_FIFO | XSPI_LED1 | XSPI_WP0 )
#define XSPI_START     (XSPI_FIFO_EN | XSPI_SEL_P1_FIFO | XSPI_LED2 | XSPI_WP0 )
#define XSPI_READ_SAMPLE   (  XSPI_RP1 )

// NOTE: Use this start to collect counter values rather than fifo samples
// this is usefull for testing the full path as a continuous counter is 
// provided allowing the pattern to be checked
// #define XSPI_START  (XSPI_FIFO_EN | XSPI_SEL_P1_CNTR | XSPI_LED2 | XSPI_WP0 )

#define GetSramWord( off )   ( *(unsigned int*  )((mPtrPruSram + (off))) )
#define SetSramWord( v,off ) ( *(unsigned int*  )((mPtrPruSram + (off))) = (v) )
#define GetSramShort( off )  ( *(unsigned short*)((mPtrPruSram + (off))) )

//------------------------------------------------------------------------------
void
Xboard::ShowPrus(const char *title )
{
    printf("-----  %s ----------------------\n",title);
    printf("sram head ptr   0x%08x\n",GetSramWord( SRAM_OFF_HEAD_PTR   ) );
    printf("sram tail ptr   0x%08x\n",GetSramWord( SRAM_OFF_TAIL_PTR ) );
    printf("dram base ptr   0x%08x\n",GetSramWord( SRAM_OFF_DRAM_PBASE ) );
    printf("dram head idx   0x%08x\n",GetSramWord( SRAM_OFF_DRAM_HEAD ) );
    printf("pru cur cmd     0x%08x\n",GetSramWord( SRAM_OFF_CMD ) );
    printf("pru last wr     0x%08x\n",GetSramWord( SRAM_OFF_WRITE_VAL ) );
    printf("pru last rd     0x%08x\n",GetSramWord( SRAM_OFF_READ_VAL ) );
    printf("dbg1            0x%08x\n",GetSramWord( SRAM_OFF_DBG1 ) );
    printf("dbg2            0x%08x\n",GetSramWord( SRAM_OFF_DBG2 ) );
    printf("sram16 [ 0 ]    0x%08x\n",GetSramShort( 0 ) );
    printf("sram16 [ 2 ]    0x%08x\n",GetSramShort( 2 ) );
    printf("sram16 [ 4 ]    0x%08x\n",GetSramShort( 4 ) );
    printf("sram16 [ 6 ]    0x%08x\n",GetSramShort( 6 ) );
}

//------------------------------------------------------------------------------
Xboard::Xboard()
{
   mCSPS = 0;
}

//------------------------------------------------------------------------------
int
Xboard::SetGain( int gn )
{
   if( gn!=0 ){
       fprintf(stderr,"Xboard:SetGain invoked w/o 0 - err \n");
   }
   return( 0 );
}

//------------------------------------------------------------------------------
/**
 * Enumeration of valid data sources within fpga
 */
#define FIFO_SRC_ADC    0
#define FIFO_SRC_NCO1   1
#define FIFO_SRC_NCO2   2
#define FIFO_SRC_I      3
#define FIFO_SRC_Q      4
#define FIFO_SRC_CIC_I  5
#define FIFO_SRC_CIC_Q  6
#define FIFO_SRC_CIC_IQ 7

int
Xboard::SetSource( int arg )
{
    printf("Xboard:SetSource=%d\n",arg);

    XspiWrite( XSPI_SEL_P1_R3     | XSPI_WP0 );
    XspiWrite( ((arg&0x000f)<<12) | XSPI_WP1 );
    XspiWrite( XSPI_SEL_P1_FIFO   | XSPI_WP0 );

    switch( arg ){
        case FIFO_SRC_ADC      :  // input = 12 bit unsigned
            mOutFmtShift = 0;
            mOutFmtAdd   = 0;
            break;
        case FIFO_SRC_NCO1     :  // input = 12 bit signed 
            mOutFmtShift = 0;
            mOutFmtAdd   = 2048;
            break;
        case FIFO_SRC_NCO2     :  // input = 12 bit signed 
            mOutFmtShift = 0;
            mOutFmtAdd   = 2048;
            break;
        case FIFO_SRC_I        :  // input = 16 bit signed
            mOutFmtShift = 0;
            mOutFmtAdd   = 32768;
            break;
        case FIFO_SRC_Q        :
            mOutFmtShift = 0;
            mOutFmtAdd   = 32768;
            break;
        case FIFO_SRC_CIC_I    :
            mOutFmtShift = 0;
            mOutFmtAdd   = 32768;
            break;
        case FIFO_SRC_CIC_Q    :
            mOutFmtShift = 0;
            mOutFmtAdd   = 32768;
            break;
        case FIFO_SRC_CIC_IQ   :
        default                :
            mOutFmtShift = 0;
            mOutFmtAdd   = 0; // 32768;
            break;
    }
    printf("Xboard:SetSource shift=%d add=%d\n",mOutFmtShift,mOutFmtAdd);
   

    Flush(); // Necessary to start streaming
    return(0);
}

//------------------------------------------------------------------------------
int
Xboard::SetLoFreq( int arg )
{
    printf("Xboard:SetLoFreq=%d\n",arg);
    XspiWrite( XSPI_SEL_P1_R6    | XSPI_WP0 );
    XspiWrite( ((arg&0x0fff)<<4) | XSPI_WP1 );
    XspiWrite( XSPI_SEL_P1_FIFO  | XSPI_WP0 );

    Flush(); // Necessary to start streaming
    return(0);
}

//------------------------------------------------------------------------------
int
Xboard::Open()
{
    int ret;

    printf("Xboard:Open enter\n");

    prussdrv_init();
    ret = prussdrv_open(PRU_EVTOUT_0);
    if( ret ){
        fprintf(stderr,"prussdrv_open failed\n");
        return(-1);
    }
    prussdrv_map_extmem( (void**)( &mPtrPruSamples ) ); 
    prussdrv_map_prumem( PRUSS0_PRU0_DATARAM, (void**)(&mPtrPruSram) );

    // Enable (power on) I board port 2
    mPortEnable.Define( 68 ); // gpio_2_4
    mPortEnable.Export();
    mPortEnable.SetDirInput(0);
    mPortEnable.Open();

    // Since we may just have powered on fpga let it load
    us_sleep( 100000 );

    // Set startup default signal paramters
    SetLoFreq( 500 );
    SetSource( 2 ); 

    printf("Xboard:Open exit\n");

    return(0);
}

//------------------------------------------------------------------------------
int
Xboard::Flush()
{
    // Stop the fpga acquisition
    XspiWrite( XSPI_STOP );

    // Flush the fpga fifos
    XspiWrite( XSPI_FLUSH );

    // Start the fpga acquisition
    XspiWrite( XSPI_START );

    // Reset the pru dram fifo
    mPidx = GetSramWord( SRAM_OFF_DRAM_HEAD )/2;

    // Tell the pru to go back to streaming
    SetSramWord( 2, SRAM_OFF_CMD );

    // ShowPrus("at flush");

    return(0);
}

//------------------------------------------------------------------------------
int 
Xboard::FlushSamples()
{
    Flush();
    return(0);
}

//------------------------------------------------------------------------------
// NOTE this is only for test purposes, it will not support high sample rates
int
Xboard::GetSamplePair( short *eo )
{
   *eo = 0;
   return( 0 );
}

//------------------------------------------------------------------------------
int
Xboard::Get2kSamples( short *bf )
{
    int          p;
    int          srcIdx,idx;

    // ShowPrus("at Get2kSamples Start");

    // Setup pause count and block mask
    p   = 0;

    // Loop until we have all of the samples
    idx = 0;
    while( idx<2048 ){

        // See where pru writer is
        srcIdx = GetSramWord( SRAM_OFF_DRAM_HEAD )/2;

        // If we are at same spot as pru writer then wait
        while( mPidx == srcIdx ){
            us_sleep( 5000 );
            p++;
            srcIdx = GetSramWord( SRAM_OFF_DRAM_HEAD )/2;
        }

        // Copy out samples until we hit pru or are done
        while( (mPidx!=srcIdx) && (idx<2048) ){
            bf[ idx ] = mPtrPruSamples[mPidx];

            // bf[ idx ]=( ( (unsigned short)mOutFmtAdd + bf[ idx ]) & 0xffff );
            bf[ idx ] = (mOutFmtAdd + bf[ idx ])<<mOutFmtShift;
            // bf[ idx ] = (bf[ idx ]+mOutFmtAdd)<<mOutFmtShift;
            // bf[ idx ] = (bf[ idx ]) + 2048; // works some

            idx   = idx+1;
            mPidx = (mPidx+1)%PRU_MAX_SHORT_SAMPLES;
        }

    }

    // ShowPrus("at Get2kSamples End");
    return( p );
}

//------------------------------------------------------------------------------
int
Xboard::StartPrus()
{
    tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

    printf("Xboard:StartPrus Enter\n");

    // Stop the prus
    prussdrv_pru_disable(0);
    prussdrv_pru_disable(1);

    // Init pru data
    prussdrv_pruintc_init(&pruss_intc_initdata);

    // Clear pru sram
    memset( (void*)mPtrPruSram, 0x0, 8192 );

    // Initialize required ram values
    *( (unsigned int*)(mPtrPruSram + SRAM_OFF_DRAM_PBASE) ) = 
                                prussdrv_get_phys_addr((void*)mPtrPruSamples);
    *( (unsigned int*)(mPtrPruSram + SRAM_OFF_CMD) ) = 0;

    // Write the instructions
    prussdrv_pru_write_memory(PRUSS0_PRU0_IRAM,0,
                             (unsigned int*)pru_image0,sizeof(pru_image0) );
    prussdrv_pru_write_memory(PRUSS0_PRU1_IRAM,0,
                             (unsigned int*)pru_image1,sizeof(pru_image1) );

    // Run/Enable prus
    prussdrv_pru_enable(0);
    prussdrv_pru_enable(1);

    // Configure fpga adc fifo to start
    XspiWrite( XSPI_STOP );
    XspiWrite( XSPI_FLUSH );
    XspiWrite( XSPI_START );

    printf("Xboard:StartPrus Exit\n");
    return( 0 );
}

//------------------------------------------------------------------------------
int
Xboard::SetSim( int sim )
{
    return( 0 );
}

//------------------------------------------------------------------------------
int
Xboard::SetComplexSampleRate( int complexSamplesPerSecond )
{
    mCSPS = complexSamplesPerSecond;
    printf("Xboard:SetComplexSampleRate %d CSPS\n",mCSPS);

    return(0);
}

//------------------------------------------------------------------------------
int
Xboard::GetComplexSampleRate()
{
    return( mCSPS );
}

//------------------------------------------------------------------------------
int Xboard::GetRms( int nSamples, short *aSamples, double *rrms )
{
    return( 0 );
}

//------------------------------------------------------------------------------
int
Xboard::XspiWrite( int wval )
{
    int rval,bsy,cnt;

    if( mXspiDbg ){
       printf("Xboard::XspiWrite: wval = 0x%04x\n",wval);
    }

    // Issue command to pru
    SetSramWord(    0, SRAM_OFF_READ_VAL );
    SetSramWord( wval, SRAM_OFF_WRITE_VAL );
    SetSramWord(    1, SRAM_OFF_CMD );

    // Wait for pru done with command
    bsy  = 1;
    cnt  = 100;
    while( bsy && 0!=cnt ){
        bsy = GetSramWord( SRAM_OFF_CMD );
        if( bsy ){
            us_sleep( 100 );
            cnt--;
        }
    }

    // Read the results
    if( 0!=cnt ){
        rval = GetSramWord( SRAM_OFF_READ_VAL );
    }
    else{
        rval = -1;
    }

    if( mXspiDbg ){
        printf("Xboard::XspiWrite: rval = 0x%04x\n",rval);
    }

    return(rval);
}
