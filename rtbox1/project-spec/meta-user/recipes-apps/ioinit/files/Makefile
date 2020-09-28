APP = ioinit.sh

# Add any other object files to this list below
APP_OBJS = ioinit.o

all:

install: 
	$(TARGETINST) -d -p 755 ioinit.sh /etc/init.d/ioinit.sh
	$(TARGETINST) -s /etc/init.d/ioinit.sh /etc/rcS.d/S04ioinit.sh


