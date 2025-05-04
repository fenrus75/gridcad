all: gridcad


OBJS := src/main.o src/scene.o src/color.o

gridcad: $(OBJS) include/gridcad.h Makefile
	g++ $(CFLAGS) -O3 -Wall -march=native $(OBJS) -flto -o gridcad -lSDL2 -lSDL2main
	
.cpp.o:
	g++ $(CXXFLAGS) -Iinclude/ -O3 -Wall -march=native -flto -c $< -o $@

	
clean:
	rm -f gridcad */*.o