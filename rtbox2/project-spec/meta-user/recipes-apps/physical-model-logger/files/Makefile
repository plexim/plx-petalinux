APP = physical-model-logger

# Add any other object files to this list below
APP_OBJS = physical-model-logger.o

all: build

build: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

