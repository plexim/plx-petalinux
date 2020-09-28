CROSS            ?= "/Users/Shared/oliver/Development/HIL_Framework/2.0.2/PLECS_RT_Box/bin/mac/arm-none-eabi/bin/arm-none-eabi-"
CFLAGS           := -Wall -O3 -g  -c -fmessage-length=0 -DEXTERNAL_MODE=0 -funsafe-math-optimizations -fopt-info-vec --param max-completely-peeled-insns=1000 --param max-completely-peel-times=100
CFLAGS           := -Wall -O2 -g  -c -fmessage-length=0 -DEXTERNAL_MODE=0 \
   -fgcse-after-reload \
   -finline-functions \
   -fipa-cp-clone \
   -floop-interchange \
   -floop-unroll-and-jam \
   -fpeel-loops \
   -fpredictive-commoning \
   -fsplit-paths \
   -ftree-loop-distribute-patterns \
   -ftree-loop-distribution \
   -ftree-loop-vectorize \
   -ftree-partial-pre \
   -ftree-slp-vectorize \
   -funswitch-loops \
   -fvect-cost-model \
   -fno-fast-math \
   -mtune=cortex-a9 -mcpu=cortex-a9 \
   -Wno-unused-variable \
   -funsafe-math-optimizations -fopt-info-vec --param max-completely-peeled-insns=1000 --param max-completely-peel-times=100
CC               = $(CROSS)gcc
STRIP            = $(CROSS)strip
INCLUDE          := -I"/Users/Shared/oliver/Development/HIL_Framework/2.0.2/PLECS_RT_Box/include/rtbox"
LIBS             := -L"/Users/Shared/oliver/Development/HIL_Framework/2.0.2/PLECS_RT_Box/lib/rtbox" -L"/Users/Shared/oliver/Development/HIL_Framework/2.0.2/PLECS_RT_Box/lib/rtbox/OpenAmp" \
                    -L"/Users/Shared/oliver/Development/HIL_Framework/2.0.2/PLECS_RT_Box/lib/rtbox/Xilinx" -lplexim_hil -lopen_amp -lbaremetal_remote \
                    -lm -Wl,--start-group,-lxil,-lgcc,-lc,--end-group
LFLAGS           := -Wl,-T -Wl,"/Users/Shared/oliver/Development/HIL_Framework/2.0.2/PLECS_RT_Box/build/rtbox.ld"

undefine GCC_EXEC_PREFIX

ifeq ($(OS),Windows_NT)
 SHELL            := cmd.exe
 RM               := DEL /F
 CP               := COPY /Y
else
 RM               := rm -f
 CP               := cp
endif

APP = testApp.elf

HEADERS += emptyCode.h
C_SRCFILES += main.c emptyCode.c

OBJFILES := $(patsubst %.c, %.o, $(C_SRCFILES))
DEPFILES := $(patsubst %.c, %.d, $(C_SRCFILES))

all: $(APP)

$(APP): $(OBJFILES)
	@echo Linking $@
	$(CC) -o $@ $(LFLAGS) $(OBJFILES) $(LIBS)
	$(STRIP) $@

%.o:%.c $(HEADERS)
	@echo Compiling $(<:.c=.o)
	$(CC) $(CFLAGS) $(ARCH_CFLAGS) $(INCLUDE) -c $< -o $@

clean:
	-$(RM) $(LIB) $(OBJFILES)

FORCE:

PHONY: all clean
