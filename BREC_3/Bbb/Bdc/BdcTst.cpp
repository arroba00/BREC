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
#include <unistd.h>
#include <math.h>
#include <fftw3.h>

#include "../Util/mcf.h"
#include "Bdc.h"


////////////////////////////////////////////////////////////////////////////////
void usage( int exit_code )
{
    printf("This utility uses the BDC fpga device interface software");
    printf("-echo Msg   echo Msg to output\n");
    printf("-usleep N   sleeps N microseconds\n");
    printf("-write  N   writes N to host spi port\n");
    printf("-gpio-loop  tests gpio pins looped between two ports\n");
    printf("-show       show state of sw and device\n");
    exit( exit_code );
}

////////////////////////////////////////////////////////////////////////////////
int
gpio_loop_test( Bdc *bdc )
{
   int            bit;
   unsigned short val;

   printf("NOTE: This test assumes a direct connect between GPIO ports.\n");

   // Make all pins in both ports inputs
   bdc->SpiRW16( BDC_GPIO0_DIR_WR | 0 );
   bdc->SpiRW16( BDC_GPIO1_DIR_WR | 0 );

   for( bit=0;bit<6;bit++ ){
      printf("Testing 0:%d -> 1:%d ... ",bit,bit);

      bdc->SpiRW16( BDC_GPIO0_DIR_WR | (1<<bit) );
      bdc->SpiRW16( BDC_GPIO0_OUT_WR | (1<<bit) );

      bdc->SpiRW16( BDC_GPIO1_INP_RD | (1<<bit) );
      val = bdc->SpiRW16(0);
     
      if( !(val & (1<<bit)) ){
          printf("FAIL mismatch on bit %d, read 0x%02x\n",bit,val);
      }
      else{
          printf("ok\n");
      }
   }

   // Make all pins in both ports inputs
   bdc->SpiRW16( BDC_GPIO0_DIR_WR | 0 );
   bdc->SpiRW16( BDC_GPIO1_DIR_WR | 0 );

   for( bit=0;bit<6;bit++ ){
      printf("Testing 1:%d -> 0:%d ... ",bit,bit);

      bdc->SpiRW16( BDC_GPIO1_DIR_WR | (1<<bit) );
      bdc->SpiRW16( BDC_GPIO1_OUT_WR | (1<<bit) );

      bdc->SpiRW16( BDC_GPIO0_INP_RD | (1<<bit) );
      val = bdc->SpiRW16(0);
     
      if( !(val & (1<<bit)) ){
          printf("FAIL mismatch on bit %d, read 0x%02x\n",bit,val);
      }
      else{
          printf("ok\n");
      }
   }

   // Make all pins in both ports inputs
   bdc->SpiRW16( BDC_GPIO0_DIR_WR | 0 );
   bdc->SpiRW16( BDC_GPIO1_DIR_WR | 0 );
}

////////////////////////////////////////////////////////////////////////////////
int
main( int argc, char *argv[] )
{
    int            idx;	  
    Bdc           *bdc;
    int            val;
    char          *end;

    bdc = new Bdc();
    bdc->Open();

    idx = 1;
    while( idx < argc ){

        if( 0==strcmp(argv[idx], "-help") ){
            usage(0);
        }

        else if( 0==strcmp(argv[idx], "-h") ){
            usage(0);
        }

        else if( 0==strcmp(argv[idx], "-echo") ){
            if( (idx+1) >= argc ){ usage(-1); }
            printf("%s\n",argv[idx+1]);
        } 

        else if( 0==strcmp(argv[idx], "-usleep") ){
            if( (idx+1) >= argc ){ usage(-1); }
            val = strtol(argv[idx+1],&end,0);
            us_sleep( val );
        } 

        else if( 0==strcmp(argv[idx], "-write") ){
            int rval; 
            if( (idx+1) >= argc ){ usage(-1); }
            val = strtol( argv[idx+1], &end, 0 );

            rval = bdc->SpiRW16( val );
            printf("BdcTst:w=0x%04hx r=0x%04hx (%hd)\n",val,rval,rval);
        }

        else if( 0==strcmp(argv[idx], "-gpio-loop") ){
            gpio_loop_test( bdc );
        }
    
        // Move to next argument for parsing
        idx++;
    }

}
