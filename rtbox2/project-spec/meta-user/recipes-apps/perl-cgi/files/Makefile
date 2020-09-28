APP = perl-cgi

# Add any other object files to this list below
APP_OBJS = perl-cgi.o

all: build

build: $(APP)

$(APP): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(APP_OBJS) $(LDLIBS)

