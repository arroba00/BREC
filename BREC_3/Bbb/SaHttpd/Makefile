
include ../MakeCommon.mk

ifeq ($(TGT),arm)
    PRU_INC=/home/root/am335x_pru_package-master/pru_sw/app_loader/include
    PRU_LIB=/home/root/am335x_pru_package-master/pru_sw/app_loader/lib
    PASM=/home/root/am335x_pru_package-master/pru_sw/utils/pasm
    LIBS=-lprussdrv
else
    PRU_INC=../PruEmul/
    PRU_LIB=./
    LIBS=
endif

ifeq ($(TGT),arm)
    EMUL_OBJS=
else
    EMUL_OBJS=../PruEmul/$(TGT)/pruss_emul.o
endif

UTIL_OBJS=../Util/$(TGT)/mcf.o           \
          ../Util/$(TGT)/gpioutil.o      \
          ../Util/$(TGT)/SimpleTable.o

DEV_OBJS= \
         ../Fboard/$(TGT)/Fboard.o       \
         ../Bdc/$(TGT)/Bdc.o             \
         ../Ddc100/$(TGT)/Ddc100.o       \
         ../Iboard/$(TGT)/Iboard.o       \
         ../Mboard/$(TGT)/Mboard.o       \
         ../Adf4351/$(TGT)/Adf4351.o     \
         ../Tboard/$(TGT)/Tboard.o       \
         ../Ui2c/$(TGT)/ui2c.o           \
         ../Tboard/$(TGT)/mcp4725.o      \
         ../Tboard/$(TGT)/max2112.o      \

LOCAL_OBJS= $(TGT)/InstModel.o           \
            $(TGT)/Pse.o                 \
            $(TGT)/Device.o              \
            $(TGT)/DevSim.o              \
            $(TGT)/DevMtdf.o             

all	:
	make $(TGT)/SaHttpd
	make Js

$(TGT)/SaHttpd  : SaHttpd.cpp $(LOCAL_OBJS) $(UTIL_OBJS) $(DEV_OBJS)  | $(TGT)
	g++ $(CPPFLAGS) SaHttpd.cpp \
              -I../           \
              -I$(PRU_INC)    \
              -L$(PRU_LIB)    \
                $(LOCAL_OBJS) \
                $(DEV_OBJS)   \
                $(UTIL_OBJS)  \
                $(EMUL_OBJS)  \
                $(LIBS) -lm -lfftw3 -lpthread -lmicrohttpd \
                -o $(TGT)/SaHttpd

$(TGT)/Device.o	: Device.cpp Device.h | $(TGT)
	g++ -I../ $(CPPFLAGS) -c Device.cpp  -o $(TGT)/Device.o

$(TGT)/DevSim.o	: DevSim.cpp DevSim.h | $(TGT)
	g++ -I../ $(CPPFLAGS) -c DevSim.cpp  -o $(TGT)/DevSim.o

$(TGT)/DevMtdf.o	: DevMtdf.cpp DevMtdf.h | $(TGT)
	g++ -I../ $(CPPFLAGS) -c DevMtdf.cpp  -o $(TGT)/DevMtdf.o

$(TGT)/Pse.o	: Pse.cpp Pse.h | $(TGT)
	g++ -I../ $(CPPFLAGS) -c Pse.cpp  -o $(TGT)/Pse.o

$(TGT)/InstModel.o	: InstModel.cpp InstModel.h | $(TGT)
	g++ -I../ $(CPPFLAGS) -c InstModel.cpp  -o $(TGT)/InstModel.o

Js	: ../../Js
	ln -s ../../Js

clean	:
	rm -f $(TGT)/*
	rm -rf $(TGT)
	rm -f Js
	rm -f *~
	rm -f tmp

