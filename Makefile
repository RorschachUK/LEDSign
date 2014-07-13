CFLAGS=-Wall -O3 -g
CXXFLAGS=-Wall -O3 -g
OBJECTS=main.o gpio.o led-matrix.o thread.o Particle_Fixed.o Emitter_Spin.o Particle_Std.o Emitter_Side.o Emitter_Fixed.o Emitter_Fire.o ParticleSys.o Particle_Bounce.o Emitter_Fountain.o PartMatrix.o Particle_Attractor.o
BINARIES=led-matrix
LDFLAGS=-lrt -lm -lpthread

all : $(BINARIES)

led-matrix.o: led-matrix.cc led-matrix.h
main.o: led-matrix.h

led-matrix : $(OBJECTS)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJECTS) $(BINARIES)
