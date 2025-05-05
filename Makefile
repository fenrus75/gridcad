all: gridcad


OBJS := src/main.o src/scene.o src/color.o src/element.o

gridcad: $(OBJS) include/gridcad.h Makefile
	g++ $(CFLAGS) -O3 -Wall -march=native $(OBJS) -flto -o gridcad -lSDL2main -lSDL2_image -lSDL2_gfx -lSDL2 
	
.cpp.o:
	g++ $(CXXFLAGS) -Iinclude/ -O3 -Wall -march=native -flto -c $< -o $@

	
clean:
	rm -f gridcad */*.o
	
devdeps:
	sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-gfx-dev