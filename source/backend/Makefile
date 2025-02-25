#!/usr/bin/make -f
# Makefile for carla-backend #
# -------------------------- #
# Created by falkTX
#

CWD=..
include Makefile.mk

# ---------------------------------------------------------------------------------------------------------------------

OBJS_standalone = \
	$(OBJDIR)/CarlaStandalone.cpp.o \
	$(OBJDIR)/CarlaStandaloneNSM.cpp.o

ifneq ($(STATIC_PLUGIN_TARGET),true)
TARGETS = \
	$(BINDIR)/libcarla_standalone2$(LIB_EXT)
endif

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += $(FLUIDSYNTH_FLAGS)

ifneq ($(HAIKU),true)
ifneq ($(WASM),true)
BUILD_CXX_FLAGS += -pthread
endif
endif

# ---------------------------------------------------------------------------------------------------------------------

STANDALONE_LIBS  = $(MODULEDIR)/carla_engine.a
STANDALONE_LIBS += $(MODULEDIR)/carla_plugin.a
STANDALONE_LIBS += $(MODULEDIR)/jackbridge.a

STANDALONE_LIBS += $(MODULEDIR)/audio_decoder.a
STANDALONE_LIBS += $(MODULEDIR)/lilv.a
STANDALONE_LIBS += $(MODULEDIR)/native-plugins.a
STANDALONE_LIBS += $(MODULEDIR)/rtmempool.a
STANDALONE_LIBS += $(MODULEDIR)/sfzero.a
STANDALONE_LIBS += $(MODULEDIR)/water.a
STANDALONE_LIBS += $(MODULEDIR)/zita-resampler.a

ifeq ($(HAVE_DGL),true)
STANDALONE_LIBS += $(MODULEDIR)/dgl.a
endif

ifeq ($(HAVE_HYLIA),true)
STANDALONE_LIBS += $(MODULEDIR)/hylia.a
endif

ifeq ($(USING_JUCE),true)
STANDALONE_LIBS += $(MODULEDIR)/carla_juce.a
STANDALONE_LIBS += $(MODULEDIR)/juce_audio_basics.a
ifeq ($(USING_JUCE_AUDIO_DEVICES),true)
STANDALONE_LIBS += $(MODULEDIR)/juce_audio_devices.a
endif
STANDALONE_LIBS += $(MODULEDIR)/juce_audio_processors.a
STANDALONE_LIBS += $(MODULEDIR)/juce_core.a
STANDALONE_LIBS += $(MODULEDIR)/juce_data_structures.a
STANDALONE_LIBS += $(MODULEDIR)/juce_events.a
STANDALONE_LIBS += $(MODULEDIR)/juce_graphics.a
STANDALONE_LIBS += $(MODULEDIR)/juce_gui_basics.a
STANDALONE_LIBS += $(MODULEDIR)/juce_gui_extra.a
endif

ifeq ($(USING_RTAUDIO),true)
STANDALONE_LIBS += $(MODULEDIR)/rtaudio.a
STANDALONE_LIBS += $(MODULEDIR)/rtmidi.a
endif

STANDALONE_LIBS += $(MODULEDIR)/ysfx.a

# ---------------------------------------------------------------------------------------------------------------------

STANDALONE_LINK_FLAGS  = $(HYLIA_LIBS)
STANDALONE_LINK_FLAGS += $(JACKBRIDGE_LIBS)
STANDALONE_LINK_FLAGS += $(LILV_LIBS)
STANDALONE_LINK_FLAGS += $(NATIVE_PLUGINS_LIBS)
STANDALONE_LINK_FLAGS += $(RTMEMPOOL_LIBS)
STANDALONE_LINK_FLAGS += $(WATER_LIBS)

STANDALONE_LINK_FLAGS += $(LIBLO_LIBS)
STANDALONE_LINK_FLAGS += $(MAGIC_LIBS)
STANDALONE_LINK_FLAGS += $(FLUIDSYNTH_LIBS)
STANDALONE_LINK_FLAGS += $(SDL_LIBS)
STANDALONE_LINK_FLAGS += $(X11_LIBS)

ifeq ($(USING_JUCE),true)
STANDALONE_LINK_FLAGS += $(JUCE_AUDIO_BASICS_LIBS)
ifeq ($(USING_JUCE_AUDIO_DEVICES),true)
STANDALONE_LINK_FLAGS += $(JUCE_AUDIO_DEVICES_LIBS)
endif
STANDALONE_LINK_FLAGS += $(JUCE_AUDIO_PROCESSORS_LIBS)
STANDALONE_LINK_FLAGS += $(JUCE_CORE_LIBS)
STANDALONE_LINK_FLAGS += $(JUCE_DATA_STRUCTURES_LIBS)
STANDALONE_LINK_FLAGS += $(JUCE_EVENTS_LIBS)
STANDALONE_LINK_FLAGS += $(JUCE_GRAPHICS_LIBS)
STANDALONE_LINK_FLAGS += $(JUCE_GUI_BASICS_LIBS)
STANDALONE_LINK_FLAGS += $(JUCE_GUI_EXTRA_LIBS)
endif

ifeq ($(USING_RTAUDIO),true)
STANDALONE_LINK_FLAGS += $(RTAUDIO_LIBS)
STANDALONE_LINK_FLAGS += $(RTMIDI_LIBS)
endif

STANDALONE_LINK_FLAGS += $(YSFX_GRAPHICS_LIBS)

ifeq ($(JACKBRIDGE_DIRECT),true)
STANDALONE_LINK_FLAGS += $(JACK_LIBS)
endif

# ---------------------------------------------------------------------------------------------------------------------

all: $(TARGETS)
	$(MAKE) -C utils

static: $(OBJS_standalone)

# ---------------------------------------------------------------------------------------------------------------------

clean:
	rm -f $(OBJS_standalone) $(TARGETS)
	$(MAKE) clean -C engine
	$(MAKE) clean -C plugin
	$(MAKE) clean -C utils

debug:
	$(MAKE) DEBUG=true

doxygen: CarlaBackend.doxygen
	doxygen $^

# ---------------------------------------------------------------------------------------------------------------------

$(BINDIR)/libcarla_standalone2$(LIB_EXT): $(OBJS_standalone) $(STANDALONE_LIBS)
	-@mkdir -p $(BINDIR)
	@echo "Linking libcarla_standalone2$(LIB_EXT)"
	$(SILENT)$(CXX) $(OBJS_standalone) $(BUILD_CXX_FLAGS) $(LIBS_START) $(STANDALONE_LIBS) $(LIBS_END) $(LINK_FLAGS) $(STANDALONE_LINK_FLAGS) $(SHARED) -o $@

# ---------------------------------------------------------------------------------------------------------------------

ifeq ($(MACOS),true)
$(OBJDIR)/CarlaStandalone.cpp.o: CarlaStandalone.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@
endif

$(OBJDIR)/%.cpp.o: %.cpp
	-@mkdir -p $(OBJDIR)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

-include $(OBJS_standalone:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
