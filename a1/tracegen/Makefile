AS = 
CC = g++
C++ = g++
LD = g++

TARGET = tracegen

FILES = tracegen.c

OBJECTS = tracegen.o

ASMFLAGS = 
CFLAGS = 
CPPFLAGS =
LDFLAGS = -lz

all	: $(TARGET)

.s.o :
	$(AS) $(ASMFLAGS) -o $@ $<

.c.o :
	$(CC) -c $(CFLAGS) -o $@ $<

.cpp.o :
	$(C++) -c $(CPPFLAGS) -o $@ $<

$(TARGET) : $(OBJECTS)
	$(LD) -o $@ $(OBJECTS) $(LDFLAGS)

clean :
	rm *.o $(TARGET)

depend:
	$(CC) -MM $(CFLAGS) $(FILES)

