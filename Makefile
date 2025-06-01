all: gridcad subdirs


#LTO :=
LTO := -flto=4
SAN :=
# SAN := -fsanitize=address

SUBDIRS := fonts

NEST := src/nest/model_nest.o
TRUTH := src/truth/model_truth.o src/truth/truthcanvas.o src/truth/widget.o src/truth/label.o src/truth/tristate.o src/truth/button.o src/model_clock.o
MODELS :=  src/model_logic2.o src/model_nand.o src/model_and.o src/model_not.o src/model_toggle.o src/model_nor.o src/model_or.o src/model_output.o src/model_xor.o  $(TRUTH) $(NEST) src/model_4to1.o src/model_1to4.o src/model_8to4.o src/model_4to8.o src/model_datascope.o src/model_dflipflop.o src/model_delayline.o
OBJS := src/main.o src/document.o src/canvas.o src/scene.o src/color.o src/element.o src/wire.o src/connector.o src/port.o src/model_zero.o src/model_one.o src/iconbar.o lib/wirepath.o src/factory.o src/basecanvas.o $(MODELS) src/event.o src/uuid.o src/library.o src/queued.o src/contextmenu.o
PNG := pngs.o

gridcad: $(OBJS) $(PNG) include/gridcad.h Makefile Makefile.deps
	g++ $(CFLAGS) -O3 -Wall -march=native $(OBJS) $(LTO) $(SAN) -o gridcad -lSDL2_image -lSDL2_gfx -lSDL2_ttf -lSDL2 `pkg-config --libs sdl2`
	
	
wiretest: $(OBJS) include/gridcad.h Makefile test/wiretest.o
	g++ $(CFLAGS) -O3 -Wall -march=native lib/wirepath.o test/wiretest.o $(LTO) -g -o wiretest -lSDL2_image -lSDL2_gfx -lSDL2_ttf -lSDL2 
	
.cpp.o:
	g++ $(CXXFLAGS) -fvisibility=hidden -Iinclude/ -O3 -Wall -march=native -flto -g $(SAN) -std=c++20   `pkg-config --cflags sdl2`-c $< -o $@
	
pngs.o: pngs.cpp
	g++ $(CXXFLAGS) -O1 pngs.cpp -c -o pngs.o

pngs.cpp: $(wildcard assets/*.png assets/*/*png library/*png)
	python3 png2header.py assets/*png assets/*/*png library/*png> pngs.cpp
	
clean:	
	rm -f gridcad */*.o wiretest *~ */*~ */*/*.o */*/*~ pngs.cpp pngs.o
	
devdeps:
	sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-gfx-dev nlohmann-json3-dev xutils-dev
	

subdirs:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done
	
depend:
	echo "# DO NOT DELETE THIS LINE -- make depend depends on it." > Makefile.deps
	makedepend -f Makefile.deps -- $(CFLAGS) -Iinclude/ -- src/*cpp src/*/*cpp test/*cpp include/*.h
	
.PHONY: all subdirs

-include Makefile.deps

