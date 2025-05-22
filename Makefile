all: gridcad wiretest subdirs


SUBDIRS := fonts

MODELS :=  src/model_logic2.o src/model_nand.o src/model_and.o src/model_not.o src/model_toggle.o src/model_nor.o src/model_or.o src/model_output.o src/model_xor.o
OBJS := src/main.o src/document.o src/canvas.o src/scene.o src/color.o src/element.o src/wire.o src/connector.o src/port.o src/model_zero.o src/model_one.o src/iconbar.o lib/wirepath.o src/factory.o $(MODELS) 

gridcad: $(OBJS) include/gridcad.h Makefile Makefile.deps
	g++ $(CFLAGS) -O3 -Wall -march=native $(OBJS) -flto=4 -o gridcad -lSDL2_image -lSDL2_gfx -lSDL2_ttf -lSDL2 
	
	
wiretest: $(OBJS) include/gridcad.h Makefile test/wiretest.o
	g++ $(CFLAGS) -O3 -Wall -march=native lib/wirepath.o test/wiretest.o -flto -g -o wiretest -lSDL2_image -lSDL2_gfx -lSDL2_ttf -lSDL2 
	
.cpp.o:
	g++ $(CXXFLAGS) -fvisibility=hidden -Iinclude/ -O3 -Wall -march=native -flto -g -c $< -o $@

	
clean:	
	rm -f gridcad */*.o wiretest *~ */*~
	
devdeps:
	sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-gfx-dev nlohmann-json3-dev xutils-dev
	

subdirs:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done
	
depend:
	echo "# DO NOT DELETE THIS LINE -- make depend depends on it." > Makefile.deps
	makedepend -f Makefile.deps -- $(CFLAGS) -Iinclude/ -- src/*cpp test/*cpp include/*.h
	
.PHONY: all subdirs

-include Makefile.deps

