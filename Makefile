all: gridcad wiretest


OBJS := src/main.o src/scene.o src/color.o src/element.o src/wire.o src/connector.o src/port.o src/twogate.o lib/wirepath.o 

gridcad: $(OBJS) include/gridcad.h Makefile
	g++ $(CFLAGS) -O3 -Wall -march=native $(OBJS) -flto -o gridcad -lSDL2main -lSDL2_image -lSDL2_gfx -lSDL2 
	
	
wiretest: $(OBJS) include/gridcad.h Makefile test/wiretest.o
	g++ $(CFLAGS) -O3 -Wall -march=native lib/wirepath.o test/wiretest.o -flto -g -o wiretest -lSDL2main -lSDL2_image -lSDL2_gfx -lSDL2 
	
.cpp.o:
	g++ $(CXXFLAGS) -Iinclude/ -O3 -Wall -march=native -flto -g -c $< -o $@

	
clean:
	rm -f gridcad */*.o wiretest
	
devdeps:
	sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-gfx-dev