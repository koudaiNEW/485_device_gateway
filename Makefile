.SUFFIXES : .x .o .c .s

DD = 1
# CC := arm-linux-gnueabi-gcc 
CC := arm-linux-gcc 
# CC := /home/user/NUC970_Buildroot/output/host/usr/bin/arm-linux-gcc
# STRIP := arm-linux-gnueabi-strip
STRIP := arm-linux-strip 
# STRIP := /home/user/NUC970_Buildroot/output/host/usr/bin/arm-linux-strip

TARGET = tcp_uart_server
SRCS := *.c
LIBS = -luv  -lpthread -lc -lgcc -ldl
# STATIC = -static -mfloat-abi=soft 
CVERSION = -std=gnu99
GDB = -g

all:
ifeq ($(DD),1)
	$(CC)  $(STATIC) $(CVERSION) $(GDB) -o $(TARGET) $(SRCS) $(LIBS)
ifndef $(GDB)
		$(STRIP) $(TARGET)
endif
else
	@echo ' ROOTFS COPY PATH = $(DD) '
	$(CC) $(STATIC) $(CVERSION) $(GDB) -o $(TARGET) $(SRCS) $(LIBS)
ifndef $(GDB)
		$(STRIP) $(TARGET)
endif
	cp *.c $(DD)
endif

clean:
	rm -f *.o
	rm -f *.x
	rm -f *.flat
	rm -f *.map
	rm -f temp
	rm -f *.img
	rm -f $(TARGET)
	rm -f *.gdb

rebuild: clean all
