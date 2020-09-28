C_SRCFILES = $(wildcard *.c)

OBJFILES := $(patsubst %.c, %.o, $(C_SRCFILES))

all: eeprom_config

eeprom_config: $(OBJFILES)
	$(CC) $(LDFLAGS) -o $@ $^

%.o:%.c
	@echo CC $(<:.c=.o)
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	-rm $(OBJFILES) eeprom_config

