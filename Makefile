all: gridcad


OBJS := src/main.o src/scene.o

gridcad: $(OBJS) include/gridcad.h Makefile
	g++ $(CFLAGS) -O3 -Wall -march=native $(OBJS) -o gridcad -lSDL2 -lSDL2main
	
.cpp.o:
	g++ $(CXXFLAGS) -Iinclude/ -O3 -Wall -march=native -c $< -o $@

	
clean:
	rm -f gridcad */*.o