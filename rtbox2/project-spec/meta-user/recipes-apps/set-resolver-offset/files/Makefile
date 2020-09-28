APP = set-resolver-offset

# Add any other object files to this list below
APP_OBJS = set-resolver-offset.o

all: build

build: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

clean:
	-rm $(APP_OBJS)
	-rm $(APP)
