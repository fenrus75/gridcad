all: gridcad wiretest


MODELS :=  src/model_logic2.o src/model_nand.o src/model_and.o src/model_not.o src/model_toggle.o src/model_nor.o src/model_or.o
OBJS := src/main.o src/scene.o src/color.o src/element.o src/wire.o src/connector.o src/port.o src/model_zero.o src/model_one.o src/iconbar.o lib/wirepath.o $(MODELS) 

gridcad: $(OBJS) include/gridcad.h Makefile
	g++ $(CFLAGS) -O0 -Wall -march=native $(OBJS) -flto -o gridcad -lSDL2main -lSDL2_image -lSDL2_gfx -lSDL2 
	
	
wiretest: $(OBJS) include/gridcad.h Makefile test/wiretest.o
	g++ $(CFLAGS) -O0 -Wall -march=native lib/wirepath.o test/wiretest.o -flto -g -o wiretest -lSDL2main -lSDL2_image -lSDL2_gfx -lSDL2 
	
.cpp.o:
	g++ $(CXXFLAGS) -Iinclude/ -O0 -Wall -march=native -flto -g -c $< -o $@

	
clean:
	rm -f gridcad */*.o wiretest *~ */*~
	
devdeps:
	sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-gfx-dev