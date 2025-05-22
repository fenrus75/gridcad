all: gridcad wiretest subdirs


SUBDIRS := fonts

MODELS :=  src/model_logic2.o src/model_nand.o src/model_and.o src/model_not.o src/model_toggle.o src/model_nor.o src/model_or.o src/model_output.o src/model_xor.o
OBJS := src/main.o src/canvas.o src/scene.o src/color.o src/element.o src/wire.o src/connector.o src/port.o src/model_zero.o src/model_one.o src/iconbar.o lib/wirepath.o src/factory.o $(MODELS) 

gridcad: $(OBJS) include/gridcad.h Makefile
	g++ $(CFLAGS) -O3 -Wall -march=native $(OBJS) -flto=4 -o gridcad -lSDL2_image -lSDL2_gfx -lSDL2_ttf -lSDL2 
	
	
wiretest: $(OBJS) include/gridcad.h Makefile test/wiretest.cpp
	g++ $(CFLAGS) -O3 -Wall -march=native lib/wirepath.cpp test/wiretest.cpp -flto -g -o wiretest -lSDL2_image -lSDL2_gfx -lSDL2_ttf -lSDL2 
	
.cpp.o:
	g++ $(CXXFLAGS) -fvisibility=hidden -Iinclude/ -O3 -Wall -march=native -flto -g -c $< -o $@

	
clean:	
	rm -f gridcad */*.o wiretest *~ */*~
	
devdeps:
	sudo apt install libsdl2-dev libsdl2-image-dev libsdl2-gfx-dev nlohmann-json3-dev
	

subdirs:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir; \
	done
	
depend:
	makedepend -f Makefile -- $(CFLAGS) -Iinclude/-- src/*cpp include/*.h


.PHONY: all subdirs

# DO NOT DELETE THIS LINE -- make depend depends on it.

src/canvas.o: /usr/include/SDL2/SDL.h /usr/include/SDL2/SDL_main.h
src/canvas.o: /usr/include/SDL2/SDL_stdinc.h /usr/include/SDL2/SDL_config.h
src/canvas.o: /usr/include/SDL2/SDL_platform.h /usr/include/SDL2/begin_code.h
src/canvas.o: /usr/include/SDL2/close_code.h /usr/include/sys/types.h
src/canvas.o: /usr/include/features.h /usr/include/features-time64.h
src/canvas.o: /usr/include/bits/wordsize.h /usr/include/bits/timesize.h
src/canvas.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
src/canvas.o: /usr/include/bits/long-double.h /usr/include/gnu/stubs.h
src/canvas.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
src/canvas.o: /usr/include/bits/time64.h /usr/include/bits/types/clock_t.h
src/canvas.o: /usr/include/bits/types/clockid_t.h
src/canvas.o: /usr/include/bits/types/time_t.h
src/canvas.o: /usr/include/bits/types/timer_t.h
src/canvas.o: /usr/include/bits/stdint-intn.h /usr/include/endian.h
src/canvas.o: /usr/include/bits/endian.h /usr/include/bits/endianness.h
src/canvas.o: /usr/include/bits/byteswap.h /usr/include/bits/uintn-identity.h
src/canvas.o: /usr/include/sys/select.h /usr/include/bits/select.h
src/canvas.o: /usr/include/bits/types/sigset_t.h
src/canvas.o: /usr/include/bits/types/__sigset_t.h
src/canvas.o: /usr/include/bits/types/struct_timeval.h
src/canvas.o: /usr/include/bits/types/struct_timespec.h
src/canvas.o: /usr/include/bits/pthreadtypes.h
src/canvas.o: /usr/include/bits/thread-shared-types.h
src/canvas.o: /usr/include/bits/pthreadtypes-arch.h
src/canvas.o: /usr/include/bits/atomic_wide_counter.h
src/canvas.o: /usr/include/bits/struct_mutex.h
src/canvas.o: /usr/include/bits/struct_rwlock.h /usr/include/stdio.h
src/canvas.o: /usr/include/bits/libc-header-start.h
src/canvas.o: /usr/include/bits/types/__fpos_t.h
src/canvas.o: /usr/include/bits/types/__mbstate_t.h
src/canvas.o: /usr/include/bits/types/__fpos64_t.h
src/canvas.o: /usr/include/bits/types/__FILE.h /usr/include/bits/types/FILE.h
src/canvas.o: /usr/include/bits/types/struct_FILE.h
src/canvas.o: /usr/include/bits/types/cookie_io_functions_t.h
src/canvas.o: /usr/include/bits/stdio_lim.h /usr/include/bits/floatn.h
src/canvas.o: /usr/include/bits/floatn-common.h /usr/include/stdlib.h
src/canvas.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
src/canvas.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h
src/canvas.o: /usr/include/string.h /usr/include/bits/types/locale_t.h
src/canvas.o: /usr/include/bits/types/__locale_t.h /usr/include/strings.h
src/canvas.o: /usr/include/wchar.h /usr/include/bits/wchar.h
src/canvas.o: /usr/include/bits/types/wint_t.h
src/canvas.o: /usr/include/bits/types/mbstate_t.h /usr/include/inttypes.h
src/canvas.o: /usr/include/stdint.h /usr/include/bits/stdint-uintn.h
src/canvas.o: /usr/include/bits/stdint-least.h /usr/include/ctype.h
src/canvas.o: /usr/include/math.h /usr/include/bits/math-vector.h
src/canvas.o: /usr/include/bits/libm-simd-decl-stubs.h
src/canvas.o: /usr/include/bits/flt-eval-method.h /usr/include/bits/fp-logb.h
src/canvas.o: /usr/include/bits/fp-fast.h
src/canvas.o: /usr/include/bits/mathcalls-macros.h
src/canvas.o: /usr/include/bits/mathcalls-helper-functions.h
src/canvas.o: /usr/include/bits/mathcalls.h
src/canvas.o: /usr/include/bits/mathcalls-narrow.h
src/canvas.o: /usr/include/bits/iscanonical.h /usr/include/SDL2/SDL_assert.h
src/canvas.o: /usr/include/SDL2/SDL_atomic.h /usr/include/SDL2/SDL_audio.h
src/canvas.o: /usr/include/SDL2/SDL_error.h /usr/include/SDL2/SDL_endian.h
src/canvas.o: /usr/include/SDL2/SDL_mutex.h /usr/include/SDL2/SDL_thread.h
src/canvas.o: /usr/include/SDL2/SDL_rwops.h /usr/include/SDL2/SDL_clipboard.h
src/canvas.o: /usr/include/SDL2/SDL_cpuinfo.h /usr/include/SDL2/SDL_events.h
src/canvas.o: /usr/include/SDL2/SDL_video.h /usr/include/SDL2/SDL_pixels.h
src/canvas.o: /usr/include/SDL2/SDL_rect.h /usr/include/SDL2/SDL_surface.h
src/canvas.o: /usr/include/SDL2/SDL_blendmode.h
src/canvas.o: /usr/include/SDL2/SDL_keyboard.h
src/canvas.o: /usr/include/SDL2/SDL_keycode.h
src/canvas.o: /usr/include/SDL2/SDL_scancode.h /usr/include/SDL2/SDL_mouse.h
src/canvas.o: /usr/include/SDL2/SDL_joystick.h /usr/include/SDL2/SDL_guid.h
src/canvas.o: /usr/include/SDL2/SDL_gamecontroller.h
src/canvas.o: /usr/include/SDL2/SDL_sensor.h /usr/include/SDL2/SDL_quit.h
src/canvas.o: /usr/include/SDL2/SDL_gesture.h /usr/include/SDL2/SDL_touch.h
src/canvas.o: /usr/include/SDL2/SDL_filesystem.h
src/canvas.o: /usr/include/SDL2/SDL_haptic.h /usr/include/SDL2/SDL_hidapi.h
src/canvas.o: /usr/include/SDL2/SDL_hints.h /usr/include/SDL2/SDL_loadso.h
src/canvas.o: /usr/include/SDL2/SDL_log.h /usr/include/SDL2/SDL_messagebox.h
src/canvas.o: /usr/include/SDL2/SDL_metal.h /usr/include/SDL2/SDL_power.h
src/canvas.o: /usr/include/SDL2/SDL_render.h /usr/include/SDL2/SDL_shape.h
src/canvas.o: /usr/include/SDL2/SDL_system.h /usr/include/SDL2/SDL_timer.h
src/canvas.o: /usr/include/SDL2/SDL_version.h /usr/include/SDL2/SDL_locale.h
src/canvas.o: /usr/include/SDL2/SDL_misc.h /usr/include/SDL2/SDL_image.h
src/canvas.o: /usr/include/SDL2/SDL.h /usr/include/SDL2/SDL2_gfxPrimitives.h
src/canvas.o: /usr/include/SDL2/SDL_ttf.h
src/color.o: /usr/include/sys/time.h /usr/include/features.h
src/color.o: /usr/include/features-time64.h /usr/include/bits/wordsize.h
src/color.o: /usr/include/bits/timesize.h /usr/include/stdc-predef.h
src/color.o: /usr/include/sys/cdefs.h /usr/include/bits/long-double.h
src/color.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
src/color.o: /usr/include/bits/typesizes.h /usr/include/bits/time64.h
src/color.o: /usr/include/bits/types/time_t.h
src/color.o: /usr/include/bits/types/struct_timeval.h
src/color.o: /usr/include/sys/select.h /usr/include/bits/select.h
src/color.o: /usr/include/bits/types/sigset_t.h
src/color.o: /usr/include/bits/types/__sigset_t.h
src/color.o: /usr/include/bits/types/struct_timespec.h
src/color.o: /usr/include/bits/endian.h /usr/include/bits/endianness.h
src/element.o: /usr/include/assert.h /usr/include/features.h
src/element.o: /usr/include/features-time64.h /usr/include/bits/wordsize.h
src/element.o: /usr/include/bits/timesize.h /usr/include/stdc-predef.h
src/element.o: /usr/include/sys/cdefs.h /usr/include/bits/long-double.h
src/element.o: /usr/include/gnu/stubs.h /usr/include/math.h
src/element.o: /usr/include/bits/libc-header-start.h
src/element.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
src/element.o: /usr/include/bits/time64.h /usr/include/bits/math-vector.h
src/element.o: /usr/include/bits/libm-simd-decl-stubs.h
src/element.o: /usr/include/bits/floatn.h /usr/include/bits/floatn-common.h
src/element.o: /usr/include/bits/flt-eval-method.h
src/element.o: /usr/include/bits/fp-logb.h /usr/include/bits/fp-fast.h
src/element.o: /usr/include/bits/mathcalls-macros.h
src/element.o: /usr/include/bits/mathcalls-helper-functions.h
src/element.o: /usr/include/bits/mathcalls.h
src/element.o: /usr/include/bits/mathcalls-narrow.h
src/element.o: /usr/include/bits/iscanonical.h
src/iconbar.o: /usr/include/SDL2/SDL_image.h /usr/include/SDL2/SDL.h
src/iconbar.o: /usr/include/SDL2/SDL_version.h /usr/include/SDL2/SDL_stdinc.h
src/iconbar.o: /usr/include/SDL2/SDL_config.h
src/iconbar.o: /usr/include/SDL2/SDL_platform.h
src/iconbar.o: /usr/include/SDL2/begin_code.h /usr/include/SDL2/close_code.h
src/iconbar.o: /usr/include/sys/types.h /usr/include/features.h
src/iconbar.o: /usr/include/features-time64.h /usr/include/bits/wordsize.h
src/iconbar.o: /usr/include/bits/timesize.h /usr/include/stdc-predef.h
src/iconbar.o: /usr/include/sys/cdefs.h /usr/include/bits/long-double.h
src/iconbar.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
src/iconbar.o: /usr/include/bits/typesizes.h /usr/include/bits/time64.h
src/iconbar.o: /usr/include/bits/types/clock_t.h
src/iconbar.o: /usr/include/bits/types/clockid_t.h
src/iconbar.o: /usr/include/bits/types/time_t.h
src/iconbar.o: /usr/include/bits/types/timer_t.h
src/iconbar.o: /usr/include/bits/stdint-intn.h /usr/include/endian.h
src/iconbar.o: /usr/include/bits/endian.h /usr/include/bits/endianness.h
src/iconbar.o: /usr/include/bits/byteswap.h
src/iconbar.o: /usr/include/bits/uintn-identity.h /usr/include/sys/select.h
src/iconbar.o: /usr/include/bits/select.h /usr/include/bits/types/sigset_t.h
src/iconbar.o: /usr/include/bits/types/__sigset_t.h
src/iconbar.o: /usr/include/bits/types/struct_timeval.h
src/iconbar.o: /usr/include/bits/types/struct_timespec.h
src/iconbar.o: /usr/include/bits/pthreadtypes.h
src/iconbar.o: /usr/include/bits/thread-shared-types.h
src/iconbar.o: /usr/include/bits/pthreadtypes-arch.h
src/iconbar.o: /usr/include/bits/atomic_wide_counter.h
src/iconbar.o: /usr/include/bits/struct_mutex.h
src/iconbar.o: /usr/include/bits/struct_rwlock.h /usr/include/stdio.h
src/iconbar.o: /usr/include/bits/libc-header-start.h
src/iconbar.o: /usr/include/bits/types/__fpos_t.h
src/iconbar.o: /usr/include/bits/types/__mbstate_t.h
src/iconbar.o: /usr/include/bits/types/__fpos64_t.h
src/iconbar.o: /usr/include/bits/types/__FILE.h
src/iconbar.o: /usr/include/bits/types/FILE.h
src/iconbar.o: /usr/include/bits/types/struct_FILE.h
src/iconbar.o: /usr/include/bits/types/cookie_io_functions_t.h
src/iconbar.o: /usr/include/bits/stdio_lim.h /usr/include/bits/floatn.h
src/iconbar.o: /usr/include/bits/floatn-common.h /usr/include/stdlib.h
src/iconbar.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
src/iconbar.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h
src/iconbar.o: /usr/include/string.h /usr/include/bits/types/locale_t.h
src/iconbar.o: /usr/include/bits/types/__locale_t.h /usr/include/strings.h
src/iconbar.o: /usr/include/wchar.h /usr/include/bits/wchar.h
src/iconbar.o: /usr/include/bits/types/wint_t.h
src/iconbar.o: /usr/include/bits/types/mbstate_t.h /usr/include/inttypes.h
src/iconbar.o: /usr/include/stdint.h /usr/include/bits/stdint-uintn.h
src/iconbar.o: /usr/include/bits/stdint-least.h /usr/include/ctype.h
src/iconbar.o: /usr/include/math.h /usr/include/bits/math-vector.h
src/iconbar.o: /usr/include/bits/libm-simd-decl-stubs.h
src/iconbar.o: /usr/include/bits/flt-eval-method.h
src/iconbar.o: /usr/include/bits/fp-logb.h /usr/include/bits/fp-fast.h
src/iconbar.o: /usr/include/bits/mathcalls-macros.h
src/iconbar.o: /usr/include/bits/mathcalls-helper-functions.h
src/iconbar.o: /usr/include/bits/mathcalls.h
src/iconbar.o: /usr/include/bits/mathcalls-narrow.h
src/iconbar.o: /usr/include/bits/iscanonical.h
src/main.o: /usr/include/SDL2/SDL.h /usr/include/SDL2/SDL_main.h
src/main.o: /usr/include/SDL2/SDL_stdinc.h /usr/include/SDL2/SDL_config.h
src/main.o: /usr/include/SDL2/SDL_platform.h /usr/include/SDL2/begin_code.h
src/main.o: /usr/include/SDL2/close_code.h /usr/include/sys/types.h
src/main.o: /usr/include/features.h /usr/include/features-time64.h
src/main.o: /usr/include/bits/wordsize.h /usr/include/bits/timesize.h
src/main.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
src/main.o: /usr/include/bits/long-double.h /usr/include/gnu/stubs.h
src/main.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
src/main.o: /usr/include/bits/time64.h /usr/include/bits/types/clock_t.h
src/main.o: /usr/include/bits/types/clockid_t.h
src/main.o: /usr/include/bits/types/time_t.h
src/main.o: /usr/include/bits/types/timer_t.h /usr/include/bits/stdint-intn.h
src/main.o: /usr/include/endian.h /usr/include/bits/endian.h
src/main.o: /usr/include/bits/endianness.h /usr/include/bits/byteswap.h
src/main.o: /usr/include/bits/uintn-identity.h /usr/include/sys/select.h
src/main.o: /usr/include/bits/select.h /usr/include/bits/types/sigset_t.h
src/main.o: /usr/include/bits/types/__sigset_t.h
src/main.o: /usr/include/bits/types/struct_timeval.h
src/main.o: /usr/include/bits/types/struct_timespec.h
src/main.o: /usr/include/bits/pthreadtypes.h
src/main.o: /usr/include/bits/thread-shared-types.h
src/main.o: /usr/include/bits/pthreadtypes-arch.h
src/main.o: /usr/include/bits/atomic_wide_counter.h
src/main.o: /usr/include/bits/struct_mutex.h
src/main.o: /usr/include/bits/struct_rwlock.h /usr/include/stdio.h
src/main.o: /usr/include/bits/libc-header-start.h
src/main.o: /usr/include/bits/types/__fpos_t.h
src/main.o: /usr/include/bits/types/__mbstate_t.h
src/main.o: /usr/include/bits/types/__fpos64_t.h
src/main.o: /usr/include/bits/types/__FILE.h /usr/include/bits/types/FILE.h
src/main.o: /usr/include/bits/types/struct_FILE.h
src/main.o: /usr/include/bits/types/cookie_io_functions_t.h
src/main.o: /usr/include/bits/stdio_lim.h /usr/include/bits/floatn.h
src/main.o: /usr/include/bits/floatn-common.h /usr/include/stdlib.h
src/main.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
src/main.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h
src/main.o: /usr/include/string.h /usr/include/bits/types/locale_t.h
src/main.o: /usr/include/bits/types/__locale_t.h /usr/include/strings.h
src/main.o: /usr/include/wchar.h /usr/include/bits/wchar.h
src/main.o: /usr/include/bits/types/wint_t.h
src/main.o: /usr/include/bits/types/mbstate_t.h /usr/include/inttypes.h
src/main.o: /usr/include/stdint.h /usr/include/bits/stdint-uintn.h
src/main.o: /usr/include/bits/stdint-least.h /usr/include/ctype.h
src/main.o: /usr/include/math.h /usr/include/bits/math-vector.h
src/main.o: /usr/include/bits/libm-simd-decl-stubs.h
src/main.o: /usr/include/bits/flt-eval-method.h /usr/include/bits/fp-logb.h
src/main.o: /usr/include/bits/fp-fast.h /usr/include/bits/mathcalls-macros.h
src/main.o: /usr/include/bits/mathcalls-helper-functions.h
src/main.o: /usr/include/bits/mathcalls.h
src/main.o: /usr/include/bits/mathcalls-narrow.h
src/main.o: /usr/include/bits/iscanonical.h /usr/include/SDL2/SDL_assert.h
src/main.o: /usr/include/SDL2/SDL_atomic.h /usr/include/SDL2/SDL_audio.h
src/main.o: /usr/include/SDL2/SDL_error.h /usr/include/SDL2/SDL_endian.h
src/main.o: /usr/include/SDL2/SDL_mutex.h /usr/include/SDL2/SDL_thread.h
src/main.o: /usr/include/SDL2/SDL_rwops.h /usr/include/SDL2/SDL_clipboard.h
src/main.o: /usr/include/SDL2/SDL_cpuinfo.h /usr/include/SDL2/SDL_events.h
src/main.o: /usr/include/SDL2/SDL_video.h /usr/include/SDL2/SDL_pixels.h
src/main.o: /usr/include/SDL2/SDL_rect.h /usr/include/SDL2/SDL_surface.h
src/main.o: /usr/include/SDL2/SDL_blendmode.h
src/main.o: /usr/include/SDL2/SDL_keyboard.h /usr/include/SDL2/SDL_keycode.h
src/main.o: /usr/include/SDL2/SDL_scancode.h /usr/include/SDL2/SDL_mouse.h
src/main.o: /usr/include/SDL2/SDL_joystick.h /usr/include/SDL2/SDL_guid.h
src/main.o: /usr/include/SDL2/SDL_gamecontroller.h
src/main.o: /usr/include/SDL2/SDL_sensor.h /usr/include/SDL2/SDL_quit.h
src/main.o: /usr/include/SDL2/SDL_gesture.h /usr/include/SDL2/SDL_touch.h
src/main.o: /usr/include/SDL2/SDL_filesystem.h /usr/include/SDL2/SDL_haptic.h
src/main.o: /usr/include/SDL2/SDL_hidapi.h /usr/include/SDL2/SDL_hints.h
src/main.o: /usr/include/SDL2/SDL_loadso.h /usr/include/SDL2/SDL_log.h
src/main.o: /usr/include/SDL2/SDL_messagebox.h /usr/include/SDL2/SDL_metal.h
src/main.o: /usr/include/SDL2/SDL_power.h /usr/include/SDL2/SDL_render.h
src/main.o: /usr/include/SDL2/SDL_shape.h /usr/include/SDL2/SDL_system.h
src/main.o: /usr/include/SDL2/SDL_timer.h /usr/include/SDL2/SDL_version.h
src/main.o: /usr/include/SDL2/SDL_locale.h /usr/include/SDL2/SDL_misc.h
src/main.o: /usr/include/SDL2/SDL_ttf.h /usr/include/SDL2/SDL.h
src/main.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
src/main.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
src/main.o: /usr/include/bits/getopt_posix.h /usr/include/bits/getopt_core.h
src/main.o: /usr/include/bits/unistd_ext.h
src/model_logic2.o: /usr/include/sys/time.h /usr/include/features.h
src/model_logic2.o: /usr/include/features-time64.h
src/model_logic2.o: /usr/include/bits/wordsize.h /usr/include/bits/timesize.h
src/model_logic2.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
src/model_logic2.o: /usr/include/bits/long-double.h /usr/include/gnu/stubs.h
src/model_logic2.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
src/model_logic2.o: /usr/include/bits/time64.h
src/model_logic2.o: /usr/include/bits/types/time_t.h
src/model_logic2.o: /usr/include/bits/types/struct_timeval.h
src/model_logic2.o: /usr/include/sys/select.h /usr/include/bits/select.h
src/model_logic2.o: /usr/include/bits/types/sigset_t.h
src/model_logic2.o: /usr/include/bits/types/__sigset_t.h
src/model_logic2.o: /usr/include/bits/types/struct_timespec.h
src/model_logic2.o: /usr/include/bits/endian.h /usr/include/bits/endianness.h
src/model_logic2.o: /usr/include/math.h /usr/include/bits/libc-header-start.h
src/model_logic2.o: /usr/include/bits/math-vector.h
src/model_logic2.o: /usr/include/bits/libm-simd-decl-stubs.h
src/model_logic2.o: /usr/include/bits/floatn.h
src/model_logic2.o: /usr/include/bits/floatn-common.h
src/model_logic2.o: /usr/include/bits/flt-eval-method.h
src/model_logic2.o: /usr/include/bits/fp-logb.h /usr/include/bits/fp-fast.h
src/model_logic2.o: /usr/include/bits/mathcalls-macros.h
src/model_logic2.o: /usr/include/bits/mathcalls-helper-functions.h
src/model_logic2.o: /usr/include/bits/mathcalls.h
src/model_logic2.o: /usr/include/bits/mathcalls-narrow.h
src/model_logic2.o: /usr/include/bits/iscanonical.h
src/port.o: /usr/include/SDL2/SDL.h /usr/include/SDL2/SDL_main.h
src/port.o: /usr/include/SDL2/SDL_stdinc.h /usr/include/SDL2/SDL_config.h
src/port.o: /usr/include/SDL2/SDL_platform.h /usr/include/SDL2/begin_code.h
src/port.o: /usr/include/SDL2/close_code.h /usr/include/sys/types.h
src/port.o: /usr/include/features.h /usr/include/features-time64.h
src/port.o: /usr/include/bits/wordsize.h /usr/include/bits/timesize.h
src/port.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
src/port.o: /usr/include/bits/long-double.h /usr/include/gnu/stubs.h
src/port.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
src/port.o: /usr/include/bits/time64.h /usr/include/bits/types/clock_t.h
src/port.o: /usr/include/bits/types/clockid_t.h
src/port.o: /usr/include/bits/types/time_t.h
src/port.o: /usr/include/bits/types/timer_t.h /usr/include/bits/stdint-intn.h
src/port.o: /usr/include/endian.h /usr/include/bits/endian.h
src/port.o: /usr/include/bits/endianness.h /usr/include/bits/byteswap.h
src/port.o: /usr/include/bits/uintn-identity.h /usr/include/sys/select.h
src/port.o: /usr/include/bits/select.h /usr/include/bits/types/sigset_t.h
src/port.o: /usr/include/bits/types/__sigset_t.h
src/port.o: /usr/include/bits/types/struct_timeval.h
src/port.o: /usr/include/bits/types/struct_timespec.h
src/port.o: /usr/include/bits/pthreadtypes.h
src/port.o: /usr/include/bits/thread-shared-types.h
src/port.o: /usr/include/bits/pthreadtypes-arch.h
src/port.o: /usr/include/bits/atomic_wide_counter.h
src/port.o: /usr/include/bits/struct_mutex.h
src/port.o: /usr/include/bits/struct_rwlock.h /usr/include/stdio.h
src/port.o: /usr/include/bits/libc-header-start.h
src/port.o: /usr/include/bits/types/__fpos_t.h
src/port.o: /usr/include/bits/types/__mbstate_t.h
src/port.o: /usr/include/bits/types/__fpos64_t.h
src/port.o: /usr/include/bits/types/__FILE.h /usr/include/bits/types/FILE.h
src/port.o: /usr/include/bits/types/struct_FILE.h
src/port.o: /usr/include/bits/types/cookie_io_functions_t.h
src/port.o: /usr/include/bits/stdio_lim.h /usr/include/bits/floatn.h
src/port.o: /usr/include/bits/floatn-common.h /usr/include/stdlib.h
src/port.o: /usr/include/bits/waitflags.h /usr/include/bits/waitstatus.h
src/port.o: /usr/include/alloca.h /usr/include/bits/stdlib-float.h
src/port.o: /usr/include/string.h /usr/include/bits/types/locale_t.h
src/port.o: /usr/include/bits/types/__locale_t.h /usr/include/strings.h
src/port.o: /usr/include/wchar.h /usr/include/bits/wchar.h
src/port.o: /usr/include/bits/types/wint_t.h
src/port.o: /usr/include/bits/types/mbstate_t.h /usr/include/inttypes.h
src/port.o: /usr/include/stdint.h /usr/include/bits/stdint-uintn.h
src/port.o: /usr/include/bits/stdint-least.h /usr/include/ctype.h
src/port.o: /usr/include/math.h /usr/include/bits/math-vector.h
src/port.o: /usr/include/bits/libm-simd-decl-stubs.h
src/port.o: /usr/include/bits/flt-eval-method.h /usr/include/bits/fp-logb.h
src/port.o: /usr/include/bits/fp-fast.h /usr/include/bits/mathcalls-macros.h
src/port.o: /usr/include/bits/mathcalls-helper-functions.h
src/port.o: /usr/include/bits/mathcalls.h
src/port.o: /usr/include/bits/mathcalls-narrow.h
src/port.o: /usr/include/bits/iscanonical.h /usr/include/SDL2/SDL_assert.h
src/port.o: /usr/include/SDL2/SDL_atomic.h /usr/include/SDL2/SDL_audio.h
src/port.o: /usr/include/SDL2/SDL_error.h /usr/include/SDL2/SDL_endian.h
src/port.o: /usr/include/SDL2/SDL_mutex.h /usr/include/SDL2/SDL_thread.h
src/port.o: /usr/include/SDL2/SDL_rwops.h /usr/include/SDL2/SDL_clipboard.h
src/port.o: /usr/include/SDL2/SDL_cpuinfo.h /usr/include/SDL2/SDL_events.h
src/port.o: /usr/include/SDL2/SDL_video.h /usr/include/SDL2/SDL_pixels.h
src/port.o: /usr/include/SDL2/SDL_rect.h /usr/include/SDL2/SDL_surface.h
src/port.o: /usr/include/SDL2/SDL_blendmode.h
src/port.o: /usr/include/SDL2/SDL_keyboard.h /usr/include/SDL2/SDL_keycode.h
src/port.o: /usr/include/SDL2/SDL_scancode.h /usr/include/SDL2/SDL_mouse.h
src/port.o: /usr/include/SDL2/SDL_joystick.h /usr/include/SDL2/SDL_guid.h
src/port.o: /usr/include/SDL2/SDL_gamecontroller.h
src/port.o: /usr/include/SDL2/SDL_sensor.h /usr/include/SDL2/SDL_quit.h
src/port.o: /usr/include/SDL2/SDL_gesture.h /usr/include/SDL2/SDL_touch.h
src/port.o: /usr/include/SDL2/SDL_filesystem.h /usr/include/SDL2/SDL_haptic.h
src/port.o: /usr/include/SDL2/SDL_hidapi.h /usr/include/SDL2/SDL_hints.h
src/port.o: /usr/include/SDL2/SDL_loadso.h /usr/include/SDL2/SDL_log.h
src/port.o: /usr/include/SDL2/SDL_messagebox.h /usr/include/SDL2/SDL_metal.h
src/port.o: /usr/include/SDL2/SDL_power.h /usr/include/SDL2/SDL_render.h
src/port.o: /usr/include/SDL2/SDL_shape.h /usr/include/SDL2/SDL_system.h
src/port.o: /usr/include/SDL2/SDL_timer.h /usr/include/SDL2/SDL_version.h
src/port.o: /usr/include/SDL2/SDL_locale.h /usr/include/SDL2/SDL_misc.h
src/port.o: /usr/include/SDL2/SDL_image.h /usr/include/SDL2/SDL.h
src/scene.o: /usr/include/stdio.h /usr/include/bits/libc-header-start.h
src/scene.o: /usr/include/features.h /usr/include/features-time64.h
src/scene.o: /usr/include/bits/wordsize.h /usr/include/bits/timesize.h
src/scene.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
src/scene.o: /usr/include/bits/long-double.h /usr/include/gnu/stubs.h
src/scene.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
src/scene.o: /usr/include/bits/time64.h /usr/include/bits/types/__fpos_t.h
src/scene.o: /usr/include/bits/types/__mbstate_t.h
src/scene.o: /usr/include/bits/types/__fpos64_t.h
src/scene.o: /usr/include/bits/types/__FILE.h /usr/include/bits/types/FILE.h
src/scene.o: /usr/include/bits/types/struct_FILE.h
src/scene.o: /usr/include/bits/types/cookie_io_functions_t.h
src/scene.o: /usr/include/bits/stdio_lim.h /usr/include/bits/floatn.h
src/scene.o: /usr/include/bits/floatn-common.h
src/wire.o: /usr/include/sys/time.h /usr/include/features.h
src/wire.o: /usr/include/features-time64.h /usr/include/bits/wordsize.h
src/wire.o: /usr/include/bits/timesize.h /usr/include/stdc-predef.h
src/wire.o: /usr/include/sys/cdefs.h /usr/include/bits/long-double.h
src/wire.o: /usr/include/gnu/stubs.h /usr/include/bits/types.h
src/wire.o: /usr/include/bits/typesizes.h /usr/include/bits/time64.h
src/wire.o: /usr/include/bits/types/time_t.h
src/wire.o: /usr/include/bits/types/struct_timeval.h
src/wire.o: /usr/include/sys/select.h /usr/include/bits/select.h
src/wire.o: /usr/include/bits/types/sigset_t.h
src/wire.o: /usr/include/bits/types/__sigset_t.h
src/wire.o: /usr/include/bits/types/struct_timespec.h
src/wire.o: /usr/include/bits/endian.h /usr/include/bits/endianness.h
include/gridcad.o: include/wirepath.h /usr/include/SDL2/SDL.h
include/gridcad.o: /usr/include/SDL2/SDL_main.h
include/gridcad.o: /usr/include/SDL2/SDL_stdinc.h
include/gridcad.o: /usr/include/SDL2/SDL_config.h
include/gridcad.o: /usr/include/SDL2/SDL_platform.h
include/gridcad.o: /usr/include/SDL2/begin_code.h
include/gridcad.o: /usr/include/SDL2/close_code.h /usr/include/sys/types.h
include/gridcad.o: /usr/include/features.h /usr/include/features-time64.h
include/gridcad.o: /usr/include/bits/wordsize.h /usr/include/bits/timesize.h
include/gridcad.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
include/gridcad.o: /usr/include/bits/long-double.h /usr/include/gnu/stubs.h
include/gridcad.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
include/gridcad.o: /usr/include/bits/time64.h
include/gridcad.o: /usr/include/bits/types/clock_t.h
include/gridcad.o: /usr/include/bits/types/clockid_t.h
include/gridcad.o: /usr/include/bits/types/time_t.h
include/gridcad.o: /usr/include/bits/types/timer_t.h
include/gridcad.o: /usr/include/bits/stdint-intn.h /usr/include/endian.h
include/gridcad.o: /usr/include/bits/endian.h /usr/include/bits/endianness.h
include/gridcad.o: /usr/include/bits/byteswap.h
include/gridcad.o: /usr/include/bits/uintn-identity.h
include/gridcad.o: /usr/include/sys/select.h /usr/include/bits/select.h
include/gridcad.o: /usr/include/bits/types/sigset_t.h
include/gridcad.o: /usr/include/bits/types/__sigset_t.h
include/gridcad.o: /usr/include/bits/types/struct_timeval.h
include/gridcad.o: /usr/include/bits/types/struct_timespec.h
include/gridcad.o: /usr/include/bits/pthreadtypes.h
include/gridcad.o: /usr/include/bits/thread-shared-types.h
include/gridcad.o: /usr/include/bits/pthreadtypes-arch.h
include/gridcad.o: /usr/include/bits/atomic_wide_counter.h
include/gridcad.o: /usr/include/bits/struct_mutex.h
include/gridcad.o: /usr/include/bits/struct_rwlock.h /usr/include/stdio.h
include/gridcad.o: /usr/include/bits/libc-header-start.h
include/gridcad.o: /usr/include/bits/types/__fpos_t.h
include/gridcad.o: /usr/include/bits/types/__mbstate_t.h
include/gridcad.o: /usr/include/bits/types/__fpos64_t.h
include/gridcad.o: /usr/include/bits/types/__FILE.h
include/gridcad.o: /usr/include/bits/types/FILE.h
include/gridcad.o: /usr/include/bits/types/struct_FILE.h
include/gridcad.o: /usr/include/bits/types/cookie_io_functions_t.h
include/gridcad.o: /usr/include/bits/stdio_lim.h /usr/include/bits/floatn.h
include/gridcad.o: /usr/include/bits/floatn-common.h /usr/include/stdlib.h
include/gridcad.o: /usr/include/bits/waitflags.h
include/gridcad.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
include/gridcad.o: /usr/include/bits/stdlib-float.h /usr/include/string.h
include/gridcad.o: /usr/include/bits/types/locale_t.h
include/gridcad.o: /usr/include/bits/types/__locale_t.h
include/gridcad.o: /usr/include/strings.h /usr/include/wchar.h
include/gridcad.o: /usr/include/bits/wchar.h /usr/include/bits/types/wint_t.h
include/gridcad.o: /usr/include/bits/types/mbstate_t.h
include/gridcad.o: /usr/include/inttypes.h /usr/include/stdint.h
include/gridcad.o: /usr/include/bits/stdint-uintn.h
include/gridcad.o: /usr/include/bits/stdint-least.h /usr/include/ctype.h
include/gridcad.o: /usr/include/math.h /usr/include/bits/math-vector.h
include/gridcad.o: /usr/include/bits/libm-simd-decl-stubs.h
include/gridcad.o: /usr/include/bits/flt-eval-method.h
include/gridcad.o: /usr/include/bits/fp-logb.h /usr/include/bits/fp-fast.h
include/gridcad.o: /usr/include/bits/mathcalls-macros.h
include/gridcad.o: /usr/include/bits/mathcalls-helper-functions.h
include/gridcad.o: /usr/include/bits/mathcalls.h
include/gridcad.o: /usr/include/bits/mathcalls-narrow.h
include/gridcad.o: /usr/include/bits/iscanonical.h
include/gridcad.o: /usr/include/SDL2/SDL_assert.h
include/gridcad.o: /usr/include/SDL2/SDL_atomic.h
include/gridcad.o: /usr/include/SDL2/SDL_audio.h
include/gridcad.o: /usr/include/SDL2/SDL_error.h
include/gridcad.o: /usr/include/SDL2/SDL_endian.h
include/gridcad.o: /usr/include/SDL2/SDL_mutex.h
include/gridcad.o: /usr/include/SDL2/SDL_thread.h
include/gridcad.o: /usr/include/SDL2/SDL_rwops.h
include/gridcad.o: /usr/include/SDL2/SDL_clipboard.h
include/gridcad.o: /usr/include/SDL2/SDL_cpuinfo.h
include/gridcad.o: /usr/include/SDL2/SDL_events.h
include/gridcad.o: /usr/include/SDL2/SDL_video.h
include/gridcad.o: /usr/include/SDL2/SDL_pixels.h
include/gridcad.o: /usr/include/SDL2/SDL_rect.h
include/gridcad.o: /usr/include/SDL2/SDL_surface.h
include/gridcad.o: /usr/include/SDL2/SDL_blendmode.h
include/gridcad.o: /usr/include/SDL2/SDL_keyboard.h
include/gridcad.o: /usr/include/SDL2/SDL_keycode.h
include/gridcad.o: /usr/include/SDL2/SDL_scancode.h
include/gridcad.o: /usr/include/SDL2/SDL_mouse.h
include/gridcad.o: /usr/include/SDL2/SDL_joystick.h
include/gridcad.o: /usr/include/SDL2/SDL_guid.h
include/gridcad.o: /usr/include/SDL2/SDL_gamecontroller.h
include/gridcad.o: /usr/include/SDL2/SDL_sensor.h
include/gridcad.o: /usr/include/SDL2/SDL_quit.h
include/gridcad.o: /usr/include/SDL2/SDL_gesture.h
include/gridcad.o: /usr/include/SDL2/SDL_touch.h
include/gridcad.o: /usr/include/SDL2/SDL_filesystem.h
include/gridcad.o: /usr/include/SDL2/SDL_haptic.h
include/gridcad.o: /usr/include/SDL2/SDL_hidapi.h
include/gridcad.o: /usr/include/SDL2/SDL_hints.h
include/gridcad.o: /usr/include/SDL2/SDL_loadso.h /usr/include/SDL2/SDL_log.h
include/gridcad.o: /usr/include/SDL2/SDL_messagebox.h
include/gridcad.o: /usr/include/SDL2/SDL_metal.h
include/gridcad.o: /usr/include/SDL2/SDL_power.h
include/gridcad.o: /usr/include/SDL2/SDL_render.h
include/gridcad.o: /usr/include/SDL2/SDL_shape.h
include/gridcad.o: /usr/include/SDL2/SDL_system.h
include/gridcad.o: /usr/include/SDL2/SDL_timer.h
include/gridcad.o: /usr/include/SDL2/SDL_version.h
include/gridcad.o: /usr/include/SDL2/SDL_locale.h
include/gridcad.o: /usr/include/SDL2/SDL_misc.h
include/gridcad.o: /usr/include/nlohmann/json.hpp
include/gridcad.o: /usr/include/nlohmann/adl_serializer.hpp
include/gridcad.o: /usr/include/nlohmann/detail/abi_macros.hpp
include/gridcad.o: /usr/include/nlohmann/detail/conversions/from_json.hpp
include/gridcad.o: /usr/include/nlohmann/detail/exceptions.hpp
include/gridcad.o: /usr/include/nlohmann/detail/value_t.hpp
include/gridcad.o: /usr/include/nlohmann/detail/macro_scope.hpp
include/gridcad.o: /usr/include/nlohmann/detail/meta/detected.hpp
include/gridcad.o: /usr/include/nlohmann/detail/meta/void_t.hpp
include/gridcad.o: /usr/include/nlohmann/thirdparty/hedley/hedley.hpp
include/gridcad.o: /usr/include/nlohmann/detail/string_escape.hpp
include/gridcad.o: /usr/include/nlohmann/detail/input/position_t.hpp
include/gridcad.o: /usr/include/nlohmann/detail/meta/cpp_future.hpp
include/gridcad.o: /usr/include/nlohmann/detail/meta/type_traits.hpp
include/gridcad.o: /usr/include/nlohmann/detail/iterators/iterator_traits.hpp
include/gridcad.o: /usr/include/nlohmann/detail/meta/call_std/begin.hpp
include/gridcad.o: /usr/include/nlohmann/detail/meta/call_std/end.hpp
include/gridcad.o: /usr/include/nlohmann/json_fwd.hpp
include/gridcad.o: /usr/include/nlohmann/detail/string_concat.hpp
include/gridcad.o: /usr/include/nlohmann/detail/meta/identity_tag.hpp
include/gridcad.o: /usr/include/nlohmann/detail/meta/std_fs.hpp
include/gridcad.o: /usr/include/nlohmann/detail/conversions/to_json.hpp
include/gridcad.o: /usr/include/nlohmann/detail/iterators/iteration_proxy.hpp
include/gridcad.o: /usr/include/nlohmann/byte_container_with_subtype.hpp
include/gridcad.o: /usr/include/nlohmann/detail/hash.hpp
include/gridcad.o: /usr/include/nlohmann/detail/input/binary_reader.hpp
include/gridcad.o: /usr/include/nlohmann/detail/input/input_adapters.hpp
include/gridcad.o: /usr/include/nlohmann/detail/input/json_sax.hpp
include/gridcad.o: /usr/include/nlohmann/detail/input/lexer.hpp
include/gridcad.o: /usr/include/nlohmann/detail/meta/is_sax.hpp
include/gridcad.o: /usr/include/nlohmann/detail/input/parser.hpp
include/gridcad.o: /usr/include/nlohmann/detail/iterators/internal_iterator.hpp
include/gridcad.o: /usr/include/nlohmann/detail/iterators/primitive_iterator.hpp
include/gridcad.o: /usr/include/nlohmann/detail/iterators/iter_impl.hpp
include/gridcad.o: /usr/include/nlohmann/detail/iterators/json_reverse_iterator.hpp
include/gridcad.o: /usr/include/nlohmann/detail/json_custom_base_class.hpp
include/gridcad.o: /usr/include/nlohmann/detail/json_pointer.hpp
include/gridcad.o: /usr/include/nlohmann/detail/json_ref.hpp
include/gridcad.o: /usr/include/nlohmann/detail/output/binary_writer.hpp
include/gridcad.o: /usr/include/nlohmann/detail/output/output_adapters.hpp
include/gridcad.o: /usr/include/nlohmann/detail/output/serializer.hpp
include/gridcad.o: /usr/include/nlohmann/detail/conversions/to_chars.hpp
include/gridcad.o: /usr/include/nlohmann/ordered_map.hpp
include/gridcad.o: /usr/include/nlohmann/detail/macro_unscope.hpp
include/gridcad.o: /usr/include/nlohmann/thirdparty/hedley/hedley_undef.hpp
include/iconbar.o: include/gridcad.h include/wirepath.h
include/iconbar.o: /usr/include/SDL2/SDL.h /usr/include/SDL2/SDL_main.h
include/iconbar.o: /usr/include/SDL2/SDL_stdinc.h
include/iconbar.o: /usr/include/SDL2/SDL_config.h
include/iconbar.o: /usr/include/SDL2/SDL_platform.h
include/iconbar.o: /usr/include/SDL2/begin_code.h
include/iconbar.o: /usr/include/SDL2/close_code.h /usr/include/sys/types.h
include/iconbar.o: /usr/include/features.h /usr/include/features-time64.h
include/iconbar.o: /usr/include/bits/wordsize.h /usr/include/bits/timesize.h
include/iconbar.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
include/iconbar.o: /usr/include/bits/long-double.h /usr/include/gnu/stubs.h
include/iconbar.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
include/iconbar.o: /usr/include/bits/time64.h
include/iconbar.o: /usr/include/bits/types/clock_t.h
include/iconbar.o: /usr/include/bits/types/clockid_t.h
include/iconbar.o: /usr/include/bits/types/time_t.h
include/iconbar.o: /usr/include/bits/types/timer_t.h
include/iconbar.o: /usr/include/bits/stdint-intn.h /usr/include/endian.h
include/iconbar.o: /usr/include/bits/endian.h /usr/include/bits/endianness.h
include/iconbar.o: /usr/include/bits/byteswap.h
include/iconbar.o: /usr/include/bits/uintn-identity.h
include/iconbar.o: /usr/include/sys/select.h /usr/include/bits/select.h
include/iconbar.o: /usr/include/bits/types/sigset_t.h
include/iconbar.o: /usr/include/bits/types/__sigset_t.h
include/iconbar.o: /usr/include/bits/types/struct_timeval.h
include/iconbar.o: /usr/include/bits/types/struct_timespec.h
include/iconbar.o: /usr/include/bits/pthreadtypes.h
include/iconbar.o: /usr/include/bits/thread-shared-types.h
include/iconbar.o: /usr/include/bits/pthreadtypes-arch.h
include/iconbar.o: /usr/include/bits/atomic_wide_counter.h
include/iconbar.o: /usr/include/bits/struct_mutex.h
include/iconbar.o: /usr/include/bits/struct_rwlock.h /usr/include/stdio.h
include/iconbar.o: /usr/include/bits/libc-header-start.h
include/iconbar.o: /usr/include/bits/types/__fpos_t.h
include/iconbar.o: /usr/include/bits/types/__mbstate_t.h
include/iconbar.o: /usr/include/bits/types/__fpos64_t.h
include/iconbar.o: /usr/include/bits/types/__FILE.h
include/iconbar.o: /usr/include/bits/types/FILE.h
include/iconbar.o: /usr/include/bits/types/struct_FILE.h
include/iconbar.o: /usr/include/bits/types/cookie_io_functions_t.h
include/iconbar.o: /usr/include/bits/stdio_lim.h /usr/include/bits/floatn.h
include/iconbar.o: /usr/include/bits/floatn-common.h /usr/include/stdlib.h
include/iconbar.o: /usr/include/bits/waitflags.h
include/iconbar.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
include/iconbar.o: /usr/include/bits/stdlib-float.h /usr/include/string.h
include/iconbar.o: /usr/include/bits/types/locale_t.h
include/iconbar.o: /usr/include/bits/types/__locale_t.h
include/iconbar.o: /usr/include/strings.h /usr/include/wchar.h
include/iconbar.o: /usr/include/bits/wchar.h /usr/include/bits/types/wint_t.h
include/iconbar.o: /usr/include/bits/types/mbstate_t.h
include/iconbar.o: /usr/include/inttypes.h /usr/include/stdint.h
include/iconbar.o: /usr/include/bits/stdint-uintn.h
include/iconbar.o: /usr/include/bits/stdint-least.h /usr/include/ctype.h
include/iconbar.o: /usr/include/math.h /usr/include/bits/math-vector.h
include/iconbar.o: /usr/include/bits/libm-simd-decl-stubs.h
include/iconbar.o: /usr/include/bits/flt-eval-method.h
include/iconbar.o: /usr/include/bits/fp-logb.h /usr/include/bits/fp-fast.h
include/iconbar.o: /usr/include/bits/mathcalls-macros.h
include/iconbar.o: /usr/include/bits/mathcalls-helper-functions.h
include/iconbar.o: /usr/include/bits/mathcalls.h
include/iconbar.o: /usr/include/bits/mathcalls-narrow.h
include/iconbar.o: /usr/include/bits/iscanonical.h
include/iconbar.o: /usr/include/SDL2/SDL_assert.h
include/iconbar.o: /usr/include/SDL2/SDL_atomic.h
include/iconbar.o: /usr/include/SDL2/SDL_audio.h
include/iconbar.o: /usr/include/SDL2/SDL_error.h
include/iconbar.o: /usr/include/SDL2/SDL_endian.h
include/iconbar.o: /usr/include/SDL2/SDL_mutex.h
include/iconbar.o: /usr/include/SDL2/SDL_thread.h
include/iconbar.o: /usr/include/SDL2/SDL_rwops.h
include/iconbar.o: /usr/include/SDL2/SDL_clipboard.h
include/iconbar.o: /usr/include/SDL2/SDL_cpuinfo.h
include/iconbar.o: /usr/include/SDL2/SDL_events.h
include/iconbar.o: /usr/include/SDL2/SDL_video.h
include/iconbar.o: /usr/include/SDL2/SDL_pixels.h
include/iconbar.o: /usr/include/SDL2/SDL_rect.h
include/iconbar.o: /usr/include/SDL2/SDL_surface.h
include/iconbar.o: /usr/include/SDL2/SDL_blendmode.h
include/iconbar.o: /usr/include/SDL2/SDL_keyboard.h
include/iconbar.o: /usr/include/SDL2/SDL_keycode.h
include/iconbar.o: /usr/include/SDL2/SDL_scancode.h
include/iconbar.o: /usr/include/SDL2/SDL_mouse.h
include/iconbar.o: /usr/include/SDL2/SDL_joystick.h
include/iconbar.o: /usr/include/SDL2/SDL_guid.h
include/iconbar.o: /usr/include/SDL2/SDL_gamecontroller.h
include/iconbar.o: /usr/include/SDL2/SDL_sensor.h
include/iconbar.o: /usr/include/SDL2/SDL_quit.h
include/iconbar.o: /usr/include/SDL2/SDL_gesture.h
include/iconbar.o: /usr/include/SDL2/SDL_touch.h
include/iconbar.o: /usr/include/SDL2/SDL_filesystem.h
include/iconbar.o: /usr/include/SDL2/SDL_haptic.h
include/iconbar.o: /usr/include/SDL2/SDL_hidapi.h
include/iconbar.o: /usr/include/SDL2/SDL_hints.h
include/iconbar.o: /usr/include/SDL2/SDL_loadso.h /usr/include/SDL2/SDL_log.h
include/iconbar.o: /usr/include/SDL2/SDL_messagebox.h
include/iconbar.o: /usr/include/SDL2/SDL_metal.h
include/iconbar.o: /usr/include/SDL2/SDL_power.h
include/iconbar.o: /usr/include/SDL2/SDL_render.h
include/iconbar.o: /usr/include/SDL2/SDL_shape.h
include/iconbar.o: /usr/include/SDL2/SDL_system.h
include/iconbar.o: /usr/include/SDL2/SDL_timer.h
include/iconbar.o: /usr/include/SDL2/SDL_version.h
include/iconbar.o: /usr/include/SDL2/SDL_locale.h
include/iconbar.o: /usr/include/SDL2/SDL_misc.h
include/iconbar.o: /usr/include/nlohmann/json.hpp
include/iconbar.o: /usr/include/nlohmann/adl_serializer.hpp
include/iconbar.o: /usr/include/nlohmann/detail/abi_macros.hpp
include/iconbar.o: /usr/include/nlohmann/detail/conversions/from_json.hpp
include/iconbar.o: /usr/include/nlohmann/detail/exceptions.hpp
include/iconbar.o: /usr/include/nlohmann/detail/value_t.hpp
include/iconbar.o: /usr/include/nlohmann/detail/macro_scope.hpp
include/iconbar.o: /usr/include/nlohmann/detail/meta/detected.hpp
include/iconbar.o: /usr/include/nlohmann/detail/meta/void_t.hpp
include/iconbar.o: /usr/include/nlohmann/thirdparty/hedley/hedley.hpp
include/iconbar.o: /usr/include/nlohmann/detail/string_escape.hpp
include/iconbar.o: /usr/include/nlohmann/detail/input/position_t.hpp
include/iconbar.o: /usr/include/nlohmann/detail/meta/cpp_future.hpp
include/iconbar.o: /usr/include/nlohmann/detail/meta/type_traits.hpp
include/iconbar.o: /usr/include/nlohmann/detail/iterators/iterator_traits.hpp
include/iconbar.o: /usr/include/nlohmann/detail/meta/call_std/begin.hpp
include/iconbar.o: /usr/include/nlohmann/detail/meta/call_std/end.hpp
include/iconbar.o: /usr/include/nlohmann/json_fwd.hpp
include/iconbar.o: /usr/include/nlohmann/detail/string_concat.hpp
include/iconbar.o: /usr/include/nlohmann/detail/meta/identity_tag.hpp
include/iconbar.o: /usr/include/nlohmann/detail/meta/std_fs.hpp
include/iconbar.o: /usr/include/nlohmann/detail/conversions/to_json.hpp
include/iconbar.o: /usr/include/nlohmann/detail/iterators/iteration_proxy.hpp
include/iconbar.o: /usr/include/nlohmann/byte_container_with_subtype.hpp
include/iconbar.o: /usr/include/nlohmann/detail/hash.hpp
include/iconbar.o: /usr/include/nlohmann/detail/input/binary_reader.hpp
include/iconbar.o: /usr/include/nlohmann/detail/input/input_adapters.hpp
include/iconbar.o: /usr/include/nlohmann/detail/input/json_sax.hpp
include/iconbar.o: /usr/include/nlohmann/detail/input/lexer.hpp
include/iconbar.o: /usr/include/nlohmann/detail/meta/is_sax.hpp
include/iconbar.o: /usr/include/nlohmann/detail/input/parser.hpp
include/iconbar.o: /usr/include/nlohmann/detail/iterators/internal_iterator.hpp
include/iconbar.o: /usr/include/nlohmann/detail/iterators/primitive_iterator.hpp
include/iconbar.o: /usr/include/nlohmann/detail/iterators/iter_impl.hpp
include/iconbar.o: /usr/include/nlohmann/detail/iterators/json_reverse_iterator.hpp
include/iconbar.o: /usr/include/nlohmann/detail/json_custom_base_class.hpp
include/iconbar.o: /usr/include/nlohmann/detail/json_pointer.hpp
include/iconbar.o: /usr/include/nlohmann/detail/json_ref.hpp
include/iconbar.o: /usr/include/nlohmann/detail/output/binary_writer.hpp
include/iconbar.o: /usr/include/nlohmann/detail/output/output_adapters.hpp
include/iconbar.o: /usr/include/nlohmann/detail/output/serializer.hpp
include/iconbar.o: /usr/include/nlohmann/detail/conversions/to_chars.hpp
include/iconbar.o: /usr/include/nlohmann/ordered_map.hpp
include/iconbar.o: /usr/include/nlohmann/detail/macro_unscope.hpp
include/iconbar.o: /usr/include/nlohmann/thirdparty/hedley/hedley_undef.hpp
include/models.o: include/gridcad.h include/wirepath.h
include/models.o: /usr/include/SDL2/SDL.h /usr/include/SDL2/SDL_main.h
include/models.o: /usr/include/SDL2/SDL_stdinc.h
include/models.o: /usr/include/SDL2/SDL_config.h
include/models.o: /usr/include/SDL2/SDL_platform.h
include/models.o: /usr/include/SDL2/begin_code.h
include/models.o: /usr/include/SDL2/close_code.h /usr/include/sys/types.h
include/models.o: /usr/include/features.h /usr/include/features-time64.h
include/models.o: /usr/include/bits/wordsize.h /usr/include/bits/timesize.h
include/models.o: /usr/include/stdc-predef.h /usr/include/sys/cdefs.h
include/models.o: /usr/include/bits/long-double.h /usr/include/gnu/stubs.h
include/models.o: /usr/include/bits/types.h /usr/include/bits/typesizes.h
include/models.o: /usr/include/bits/time64.h
include/models.o: /usr/include/bits/types/clock_t.h
include/models.o: /usr/include/bits/types/clockid_t.h
include/models.o: /usr/include/bits/types/time_t.h
include/models.o: /usr/include/bits/types/timer_t.h
include/models.o: /usr/include/bits/stdint-intn.h /usr/include/endian.h
include/models.o: /usr/include/bits/endian.h /usr/include/bits/endianness.h
include/models.o: /usr/include/bits/byteswap.h
include/models.o: /usr/include/bits/uintn-identity.h
include/models.o: /usr/include/sys/select.h /usr/include/bits/select.h
include/models.o: /usr/include/bits/types/sigset_t.h
include/models.o: /usr/include/bits/types/__sigset_t.h
include/models.o: /usr/include/bits/types/struct_timeval.h
include/models.o: /usr/include/bits/types/struct_timespec.h
include/models.o: /usr/include/bits/pthreadtypes.h
include/models.o: /usr/include/bits/thread-shared-types.h
include/models.o: /usr/include/bits/pthreadtypes-arch.h
include/models.o: /usr/include/bits/atomic_wide_counter.h
include/models.o: /usr/include/bits/struct_mutex.h
include/models.o: /usr/include/bits/struct_rwlock.h /usr/include/stdio.h
include/models.o: /usr/include/bits/libc-header-start.h
include/models.o: /usr/include/bits/types/__fpos_t.h
include/models.o: /usr/include/bits/types/__mbstate_t.h
include/models.o: /usr/include/bits/types/__fpos64_t.h
include/models.o: /usr/include/bits/types/__FILE.h
include/models.o: /usr/include/bits/types/FILE.h
include/models.o: /usr/include/bits/types/struct_FILE.h
include/models.o: /usr/include/bits/types/cookie_io_functions_t.h
include/models.o: /usr/include/bits/stdio_lim.h /usr/include/bits/floatn.h
include/models.o: /usr/include/bits/floatn-common.h /usr/include/stdlib.h
include/models.o: /usr/include/bits/waitflags.h
include/models.o: /usr/include/bits/waitstatus.h /usr/include/alloca.h
include/models.o: /usr/include/bits/stdlib-float.h /usr/include/string.h
include/models.o: /usr/include/bits/types/locale_t.h
include/models.o: /usr/include/bits/types/__locale_t.h /usr/include/strings.h
include/models.o: /usr/include/wchar.h /usr/include/bits/wchar.h
include/models.o: /usr/include/bits/types/wint_t.h
include/models.o: /usr/include/bits/types/mbstate_t.h /usr/include/inttypes.h
include/models.o: /usr/include/stdint.h /usr/include/bits/stdint-uintn.h
include/models.o: /usr/include/bits/stdint-least.h /usr/include/ctype.h
include/models.o: /usr/include/math.h /usr/include/bits/math-vector.h
include/models.o: /usr/include/bits/libm-simd-decl-stubs.h
include/models.o: /usr/include/bits/flt-eval-method.h
include/models.o: /usr/include/bits/fp-logb.h /usr/include/bits/fp-fast.h
include/models.o: /usr/include/bits/mathcalls-macros.h
include/models.o: /usr/include/bits/mathcalls-helper-functions.h
include/models.o: /usr/include/bits/mathcalls.h
include/models.o: /usr/include/bits/mathcalls-narrow.h
include/models.o: /usr/include/bits/iscanonical.h
include/models.o: /usr/include/SDL2/SDL_assert.h
include/models.o: /usr/include/SDL2/SDL_atomic.h
include/models.o: /usr/include/SDL2/SDL_audio.h /usr/include/SDL2/SDL_error.h
include/models.o: /usr/include/SDL2/SDL_endian.h
include/models.o: /usr/include/SDL2/SDL_mutex.h
include/models.o: /usr/include/SDL2/SDL_thread.h
include/models.o: /usr/include/SDL2/SDL_rwops.h
include/models.o: /usr/include/SDL2/SDL_clipboard.h
include/models.o: /usr/include/SDL2/SDL_cpuinfo.h
include/models.o: /usr/include/SDL2/SDL_events.h
include/models.o: /usr/include/SDL2/SDL_video.h
include/models.o: /usr/include/SDL2/SDL_pixels.h /usr/include/SDL2/SDL_rect.h
include/models.o: /usr/include/SDL2/SDL_surface.h
include/models.o: /usr/include/SDL2/SDL_blendmode.h
include/models.o: /usr/include/SDL2/SDL_keyboard.h
include/models.o: /usr/include/SDL2/SDL_keycode.h
include/models.o: /usr/include/SDL2/SDL_scancode.h
include/models.o: /usr/include/SDL2/SDL_mouse.h
include/models.o: /usr/include/SDL2/SDL_joystick.h
include/models.o: /usr/include/SDL2/SDL_guid.h
include/models.o: /usr/include/SDL2/SDL_gamecontroller.h
include/models.o: /usr/include/SDL2/SDL_sensor.h /usr/include/SDL2/SDL_quit.h
include/models.o: /usr/include/SDL2/SDL_gesture.h
include/models.o: /usr/include/SDL2/SDL_touch.h
include/models.o: /usr/include/SDL2/SDL_filesystem.h
include/models.o: /usr/include/SDL2/SDL_haptic.h
include/models.o: /usr/include/SDL2/SDL_hidapi.h
include/models.o: /usr/include/SDL2/SDL_hints.h
include/models.o: /usr/include/SDL2/SDL_loadso.h /usr/include/SDL2/SDL_log.h
include/models.o: /usr/include/SDL2/SDL_messagebox.h
include/models.o: /usr/include/SDL2/SDL_metal.h /usr/include/SDL2/SDL_power.h
include/models.o: /usr/include/SDL2/SDL_render.h
include/models.o: /usr/include/SDL2/SDL_shape.h
include/models.o: /usr/include/SDL2/SDL_system.h
include/models.o: /usr/include/SDL2/SDL_timer.h
include/models.o: /usr/include/SDL2/SDL_version.h
include/models.o: /usr/include/SDL2/SDL_locale.h /usr/include/SDL2/SDL_misc.h
include/models.o: /usr/include/nlohmann/json.hpp
include/models.o: /usr/include/nlohmann/adl_serializer.hpp
include/models.o: /usr/include/nlohmann/detail/abi_macros.hpp
include/models.o: /usr/include/nlohmann/detail/conversions/from_json.hpp
include/models.o: /usr/include/nlohmann/detail/exceptions.hpp
include/models.o: /usr/include/nlohmann/detail/value_t.hpp
include/models.o: /usr/include/nlohmann/detail/macro_scope.hpp
include/models.o: /usr/include/nlohmann/detail/meta/detected.hpp
include/models.o: /usr/include/nlohmann/detail/meta/void_t.hpp
include/models.o: /usr/include/nlohmann/thirdparty/hedley/hedley.hpp
include/models.o: /usr/include/nlohmann/detail/string_escape.hpp
include/models.o: /usr/include/nlohmann/detail/input/position_t.hpp
include/models.o: /usr/include/nlohmann/detail/meta/cpp_future.hpp
include/models.o: /usr/include/nlohmann/detail/meta/type_traits.hpp
include/models.o: /usr/include/nlohmann/detail/iterators/iterator_traits.hpp
include/models.o: /usr/include/nlohmann/detail/meta/call_std/begin.hpp
include/models.o: /usr/include/nlohmann/detail/meta/call_std/end.hpp
include/models.o: /usr/include/nlohmann/json_fwd.hpp
include/models.o: /usr/include/nlohmann/detail/string_concat.hpp
include/models.o: /usr/include/nlohmann/detail/meta/identity_tag.hpp
include/models.o: /usr/include/nlohmann/detail/meta/std_fs.hpp
include/models.o: /usr/include/nlohmann/detail/conversions/to_json.hpp
include/models.o: /usr/include/nlohmann/detail/iterators/iteration_proxy.hpp
include/models.o: /usr/include/nlohmann/byte_container_with_subtype.hpp
include/models.o: /usr/include/nlohmann/detail/hash.hpp
include/models.o: /usr/include/nlohmann/detail/input/binary_reader.hpp
include/models.o: /usr/include/nlohmann/detail/input/input_adapters.hpp
include/models.o: /usr/include/nlohmann/detail/input/json_sax.hpp
include/models.o: /usr/include/nlohmann/detail/input/lexer.hpp
include/models.o: /usr/include/nlohmann/detail/meta/is_sax.hpp
include/models.o: /usr/include/nlohmann/detail/input/parser.hpp
include/models.o: /usr/include/nlohmann/detail/iterators/internal_iterator.hpp
include/models.o: /usr/include/nlohmann/detail/iterators/primitive_iterator.hpp
include/models.o: /usr/include/nlohmann/detail/iterators/iter_impl.hpp
include/models.o: /usr/include/nlohmann/detail/iterators/json_reverse_iterator.hpp
include/models.o: /usr/include/nlohmann/detail/json_custom_base_class.hpp
include/models.o: /usr/include/nlohmann/detail/json_pointer.hpp
include/models.o: /usr/include/nlohmann/detail/json_ref.hpp
include/models.o: /usr/include/nlohmann/detail/output/binary_writer.hpp
include/models.o: /usr/include/nlohmann/detail/output/output_adapters.hpp
include/models.o: /usr/include/nlohmann/detail/output/serializer.hpp
include/models.o: /usr/include/nlohmann/detail/conversions/to_chars.hpp
include/models.o: /usr/include/nlohmann/ordered_map.hpp
include/models.o: /usr/include/nlohmann/detail/macro_unscope.hpp
include/models.o: /usr/include/nlohmann/thirdparty/hedley/hedley_undef.hpp
