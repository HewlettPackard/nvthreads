CFLAGS=-Wall -Wno-sign-compare  -lm -lpthread -lboost_program_options -lnuma

SOURCES=gibbsDNS-ArrayPartitioned.cpp

OBJECTS=gibbsDNS-ArrayPartitioned.o

EXECUTABLE=gibbsDNSApp

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	g++ $(CFLAGS)  -o $@  $(SOURCES)

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)
