all: lsiio iio_event_monitor iio_generic_buffer

lsiio: lsiio.o iio_utils.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

iio_event_monitor: iio_event_monitor.o iio_utils.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

iio_generic_buffer: iio_generic_buffer.o iio_utils.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	-rm -f lsiio iio_event_monitor iio_generic_buffer *.elf *.gdb *.o


