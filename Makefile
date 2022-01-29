CXX = clang++
EXE = audio-visualizer
SOURCES = main.cpp converter.cpp fft.cpp spectrogram.cpp gl.c shader_utils.cpp plot3d.cpp plot_utils.cpp

IMGUI_DIR = lib/imgui
SOURCES += $(IMGUI_DIR)/imgui.cpp $(IMGUI_DIR)/imgui_demo.cpp $(IMGUI_DIR)/imgui_draw.cpp $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui_widgets.cpp
SOURCES += $(IMGUI_DIR)/backends/imgui_impl_sdl.cpp $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

IMPLOT_DIR = lib/implot
SOURCES += $(IMPLOT_DIR)/implot.cpp $(IMPLOT_DIR)/implot_items.cpp

TINYFD_DIR = lib/tinyfd
SOURCES += $(TINYFD_DIR)/tinyfiledialogs.c

OBJS = $(addsuffix .o, $(basename $(notdir $(SOURCES))))
LINUX_GL_LIBS = -lGL

CXXFLAGS = -g -Wall -Wformat -std=c++17 -fsanitize=address
CXXFLAGS += -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends
CXXFLAGS += -I$(TINYFD_DIR)
CXXFLAGS += -I$(IMPLOT_DIR)

LIBS = $(LINUX_GL_LIBS) -ldl `sdl2-config --libs` -lmpg123 -lfftw3 -lm

CXXFLAGS += `sdl2-config --cflags`
CFLAGS = $(CXXFLAGS)

%.o:%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMGUI_DIR)/backends/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(TINYFD_DIR)/%.c
	$(CXX) $(CXXFLAGS) -c -o $@ $<

%.o:$(IMPLOT_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS)