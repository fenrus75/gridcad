project('gridcad', 'cpp',
    version: '1.0.0',
    default_options: [
        'cpp_std=c++20',
        'default_library=static',
        'optimization=2',
        'warning_level=1',
    ],
    license: 'GPL-3.0',
)

# ============================================================================
# Compiler Settings
# ============================================================================

add_project_arguments([
    '-march=native',
    '-fvisibility=hidden',
], language: 'cpp')

# Link-time optimization
add_project_link_arguments([
    '-flto=4',
], language: 'cpp')

# Sanitizer (can be enabled via -Duse_sanitizer=true)
sanitizer = get_option('use_sanitizer')
if sanitizer == 'address'
    add_project_arguments('-fsanitize=address', language: 'cpp')
    add_project_link_arguments('-fsanitize=address', language: 'cpp')
elif sanitizer == 'leak'
    add_project_arguments('-fsanitize=leak', language: 'cpp')
    add_project_link_arguments('-fsanitize=leak', language: 'cpp')
endif

# ============================================================================
# Include Directories
# ============================================================================

inc_dirs = include_directories([
    'include',
    'imgui',
    'imgui_hex_editor',
])

# ============================================================================
# Dependencies
# ============================================================================

sdl2_dep = dependency('SDL2')
sdl2_image_dep = dependency('SDL2_image')
sdl2_gfx_dep = dependency('SDL2_gfx')
sdl2_ttf_dep = dependency('SDL2_ttf')

# ============================================================================
# Source Files
# ============================================================================

# Core source files (excluding main.cpp for wiretest)
core_sources = files(
    'src/document.cpp',
    'src/canvas.cpp',
    'src/scene.cpp',
    'src/color.cpp',
    'src/element.cpp',
    'src/wire.cpp',
    'src/connector.cpp',
    'src/port.cpp',
    'src/dialog.cpp',
    'src/buttonbar.cpp',
    'src/synth.cpp',
    'src/net.cpp',
    'src/basecanvas.cpp',
    'src/event.cpp',
    'src/uuid.cpp',
    'src/library.cpp',
    'src/queued.cpp',
    'src/contextmenu.cpp',
    'src/name.cpp',
    'src/hexcanvas.cpp',
    'src/factory.cpp',
    'src/model_zero.cpp',
    'src/model_one.cpp',
    'src/iconbar.cpp',
    'lib/wirepath.cpp',
)

# Main entry point
main_sources = files(
    'src/main.cpp',
)

# Model source files
model_sources = files(
    'src/model_buffer.cpp',
    'src/model_logic2.cpp',
    'src/model_nand.cpp',
    'src/model_and.cpp',
    'src/model_not.cpp',
    'src/model_toggle.cpp',
    'src/model_nor.cpp',
    'src/model_or.cpp',
    'src/model_output.cpp',
    'src/model_xor.cpp',
    'src/model_4to1.cpp',
    'src/model_1to4.cpp',
    'src/model_8to4.cpp',
    'src/model_4to8.cpp',
    'src/model_datascope.cpp',
    'src/model_dflipflop.cpp',
    'src/model_delayline.cpp',
    'src/model_8to16.cpp',
    'src/model_16to8.cpp',
    'src/model_memory.cpp',
    'src/model_label.cpp',
    'src/model_1to8.cpp',
    'src/model_8to1.cpp',
    'src/model_deflipflop.cpp',
    'src/model_clock.cpp',
)

# Sequencer sources
sequencer_sources = files(
    'src/sequencer/model_sequencer.cpp',
    'src/sequencer/seqcanvas.cpp',
)

# Truth table sources
truth_sources = files(
    'src/truth/model_truth.cpp',
    'src/truth/truthcanvas.cpp',
    'src/truth/widget.cpp',
    'src/truth/label.cpp',
    'src/truth/tristate.cpp',
    'src/truth/button.cpp',
    'src/truth/slider.cpp',
)

# Nest sources
nest_sources = files(
    'src/nest/model_nest.cpp',
)

# Project canvas sources
project_sources = files(
    'src/project/projcanvas.cpp',
)

# ============================================================================
# Dear ImGui (from submodule)
# ============================================================================

imgui_sources = files(
    'imgui/imgui.cpp',
    'imgui/imgui_demo.cpp',
    'imgui/imgui_draw.cpp',
    'imgui/imgui_tables.cpp',
    'imgui/imgui_widgets.cpp',
    'imgui/backends/imgui_impl_sdl2.cpp',
    'imgui/backends/imgui_impl_sdlrenderer2.cpp',
)

# ImGui hex editor (from submodule)
imgui_hex_sources = files(
    'imgui_hex_editor/imgui_hex.cpp',
)

# ============================================================================
# PNG Assets - Generated Source
# ============================================================================

# Custom target to generate pngs.cpp from all PNG files
png_gen = custom_target('generate_pngs',
    output: 'pngs.cpp',
    command: [
        'python3',
        meson.project_source_root() + '/png2header.py',
        meson.project_source_root() + '/assets/andgate.png',
        meson.project_source_root() + '/assets/nandgate.png',
        meson.project_source_root() + '/assets/norgate.png',
        meson.project_source_root() + '/assets/orgate.png',
        meson.project_source_root() + '/assets/xorgate.png',
        meson.project_source_root() + '/assets/inverter.png',
        meson.project_source_root() + '/assets/buffer.png',
        meson.project_source_root() + '/assets/clock_on.png',
        meson.project_source_root() + '/assets/clock_off.png',
        meson.project_source_root() + '/assets/toggle_on.png',
        meson.project_source_root() + '/assets/toggle_off.png',
        meson.project_source_root() + '/assets/zero.png',
        meson.project_source_root() + '/assets/one.png',
        meson.project_source_root() + '/assets/output_on.png',
        meson.project_source_root() + '/assets/output_off.png',
        meson.project_source_root() + '/assets/connector.png',
        meson.project_source_root() + '/assets/connector_green.png',
        meson.project_source_root() + '/assets/connector_red.png',
        meson.project_source_root() + '/assets/wire_green.png',
        meson.project_source_root() + '/assets/wire_red.png',
        meson.project_source_root() + '/assets/wire_black.png',
        meson.project_source_root() + '/assets/wire_white.png',
        meson.project_source_root() + '/assets/1to4.png',
        meson.project_source_root() + '/assets/4to1.png',
        meson.project_source_root() + '/assets/8to4.png',
        meson.project_source_root() + '/assets/4to8.png',
        meson.project_source_root() + '/assets/1to8.png',
        meson.project_source_root() + '/assets/8to1.png',
        meson.project_source_root() + '/assets/16to8.png',
        meson.project_source_root() + '/assets/8to16.png',
        meson.project_source_root() + '/assets/sequencer.png',
        meson.project_source_root() + '/assets/dflipflop.png',
        meson.project_source_root() + '/assets/deflipflop.png',
        meson.project_source_root() + '/assets/memory.png',
        meson.project_source_root() + '/assets/datascope.png',
        meson.project_source_root() + '/assets/delayline.png',
    ],
    capture: true,
)

# ============================================================================
# Build Targets
# ============================================================================

# Main executable
executable('gridcad',
    main_sources,
    core_sources,
    model_sources,
    sequencer_sources,
    truth_sources,
    nest_sources,
    project_sources,
    imgui_sources,
    imgui_hex_sources,
    png_gen,
    include_directories: inc_dirs,
    dependencies: [
        sdl2_dep,
        sdl2_image_dep,
        sdl2_gfx_dep,
        sdl2_ttf_dep,
    ],
    install: true,
)

# ============================================================================
# Test Executable
# ============================================================================

test_sources = files(
    'test/wiretest.cpp',
)

# Wire test executable
executable('wiretest',
    core_sources,
    model_sources,
    sequencer_sources,
    truth_sources,
    nest_sources,
    project_sources,
    imgui_sources,
    imgui_hex_sources,
    png_gen,
    test_sources,
    include_directories: inc_dirs,
    dependencies: [
        sdl2_dep,
        sdl2_image_dep,
        sdl2_gfx_dep,
        sdl2_ttf_dep,
    ],
)

# ============================================================================
# Installation
# ============================================================================

# Note: fonts/Roboto-Medium-webfont.ttf needs to be downloaded manually:
#   make -C fonts
# Or: curl -O -L https://storage.googleapis.com/google-code-archive-downloads/v2/code.google.com/roboto-webfont/Roboto-Medium-webfont.ttf -o fonts/Roboto-Medium-webfont.ttf

# Install library JSON files
# library_subdirs = ['add', 'logic4', 'logic8', 'mux', 'register']
# foreach subdir : library_subdirs
#     install_data('library/' + subdir,
#         install_dir: 'share/gridcad/library',
#         preserve_path: true,
#     )
# endforeach