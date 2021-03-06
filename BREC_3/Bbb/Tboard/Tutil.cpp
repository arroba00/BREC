/*
 * This is a simple stand alone utility to exercize a T board I2C interface.
 *
 * All communication is done using the gpio's on an I board port and
 * contained within this file.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "Util/mcf.h"
#include "Util/gpioutil.h"

#include "Iboard/Iboard.h"
#include "Bdc/Bdc.h"

#include "Tboard.h"

////////////////////////////////////////////////////////////////////////////////
void usage( int exit_code )
{
    printf("-usleep    <M>   sleeps script execution for <M> micro seconds\n");
    printf("-echo      <str> echo string <str>\n");
    printf("-dac_write <N>   write <N> to dac (can be hex e.g. 0x800)\n");
    printf("-dac_read        read and display current dac value\n");
    printf("-dac_mod         loop ramp dac between 0x000 and 0x800\n");
    printf("-tshow           show tuner values\n");
    printf("-tune      <M>   tune to <M> hertz\n");
    printf("-gain      <M>   gain to <M> db [ 0 .. 80 ] approximately \n");
    printf("-bw        <M>   3db bw to <M> Hz \n");
    printf("NOTE: bw must be set prior to tuning. tuning applies the bw\n");
    printf("\n");
    exit( exit_code );
}

int
main( int argc, char *argv[] )
{
    Tboard        *tbrd;
    int            idx;
    int            val;
    char          *end;
    int            err;

    tbrd = new Tboard();

    if( FindCapeByName( "brecFpru" ) || FindCapeByName( "brecFjtag" ) ){
       Bdc       *bdc;
       GpioGroup *gpg;

       bdc = new Bdc();
       bdc->Open();

       gpg = bdc->GetPinGroup(0);
       tbrd->Attach( (void*)(gpg), 0 );
    }
    else{
       Iboard        *ibrd;

       ibrd = new Iboard();
       ibrd->Open();

       tbrd->Attach( (void*)ibrd, 0 );
    }

    idx = 1;
    while( idx < argc ){

        if( 0==strcmp(argv[idx], "-help") ){
            usage(0);
        }

        else if( 0==strcmp(argv[idx], "-h") ){
            usage(0);
        }

        else if( 0==strcmp(argv[idx], "-usleep") ){
            if( (idx+1) >= argc ){ usage(-1); }
            val = strtol(argv[idx+1],&end,0);
            us_sleep( val );
        } 

        else if( 0==strcmp(argv[idx], "-echo") ){
            if( (idx+1) >= argc ){ usage(-1); }
            printf("%s\n",argv[idx+1]);
        } 

        else if( 0==strcmp(argv[idx], "-dac_write") ){
            if( (idx+1) >= argc ){ usage(-1); }

            val = strtol(argv[idx+1],&end,0);

            err = tbrd->DacSet( val );
            printf("%s:dac_write val = 0x%04x err = 0x%08x\n",__FILE__,val,err);
        }

        else if( 0==strcmp(argv[idx], "-dac_read") ){
            err = tbrd->DacGet( &val );
            printf("%s:dac_read  val = 0x%04x err = 0x%08x\n",__FILE__,val,err);
        }

        else if( 0==strcmp(argv[idx], "-dac_mod") ){
            int up = 1;
            val = 0x0;
            while( 1 ){
                tbrd->DacSet( val  );
                if( up ){ val = (val+32); }
                else    { val = (val-32); }
                if( val > 0x800 ) { up=0; val=0x800; }
                if( val < 0x100 ) { up=1; val=0x100; }
                
            }
        }

        else if( 0==strcmp(argv[idx], "-tshow") ){
            tbrd->ShowTuner();
        }

        else if( 0==strcmp(argv[idx], "-tune") ){
            double hzTgt,hzAct;

            if( (idx+1) >= argc ){ usage(-1); }
            hzTgt = atof(argv[idx+1]);
            hzAct = tbrd->SetFreqHz( hzTgt  );
            printf("HzTgt=%f, HzAct=%f\n",hzTgt,hzAct);
        }

        else if( 0==strcmp(argv[idx], "-gain") ){
            double dbTgt,dbAct;

            if( (idx+1) >= argc ){ usage(-1); }
            dbTgt = atof(argv[idx+1]);
            dbAct = tbrd->SetRfGainDb( dbTgt  );
            printf("dBTgt=%f, dBAct=%f\n",dbTgt,dbAct);
        }

        else if( 0==strcmp(argv[idx], "-bw") ){
            double hzTgt,hzAct;

            if( (idx+1) >= argc ){ usage(-1); }
            hzTgt = atof(argv[idx+1]);
            hzAct = tbrd->SetBwHz( hzTgt  );
            printf("HzTgt=%f, HzAct=%f\n",hzTgt,hzAct);
        }

        // Move to next command
        idx++;
    }
}
