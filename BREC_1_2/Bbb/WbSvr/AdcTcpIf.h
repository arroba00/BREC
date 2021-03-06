//
//
// This source code is available under the "Simplified BSD license".
//
// Copyright (c) 2013, J. Kleiner
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
///
#ifndef __ADC_TCPIF_H__
#define __ADC_TCPIF_H__

#include "../Util/mcf.h"
#include "../Util/net.h"
#include "Devs.h"

class AdcTcpIf : public McF {

  private:
    int             mThreadExit;
    int             mIpPort;
    char           *mIpStr;
    int             mRun;

    TcpSvrCon      *mTsc;

    int             mPauseCount;
    unsigned int    mSampleCount;
    unsigned char  *mPktData;
    short          *mAdcData;
    int             mAdcSamplesAvail;
    unsigned short  mSeqNum;
    int             mSamplesPerSecond;
    double          mRateEst;
    int             mAdcSamplesToCapture;

    void            ConfigureHw();
    int             SendSamples( TcpSvrCon *tcl );
    void            AdcDataCapture();
    int             AdcDataSend( TcpSvrCon *tcl );

  public:
    AdcTcpIf();
    void          Main();
    void          RcvEvent( char *evtStr );
    void          SetSvcPort( int port );
};

#endif
