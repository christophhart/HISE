# Automatically generated makefile, created by the Projucer
# Don't edit this file! Your changes will be overwritten when you re-save the Projucer project!

# (this disables dependency generation if multiple architectures are set)
DEPFLAGS := $(if $(word 2, $(TARGET_ARCH)), , -MMD)

ifndef STRIP
  STRIP=strip
endif

ifndef AR
  AR=ar
endif

ifndef CONFIG
  CONFIG=Debug
endif

ifeq ($(CONFIG),Debug)
  JUCE_BINDIR := build
  JUCE_LIBDIR := build
  JUCE_OBJDIR := build/intermediate/Debug
  JUCE_OUTDIR := build

  ifeq ($(TARGET_ARCH),)
    TARGET_ARCH := -march=native
  endif

  JUCE_CPPFLAGS := $(DEPFLAGS) -DLINUX=1 -DDEBUG=1 -D_DEBUG=1 -DUSE_IPP=0 -DJUCER_LINUX_MAKE_6D53C8B4=1 -DJUCE_APP_VERSION=0.99 -DJUCE_APP_VERSION_HEX=0x6300 $(shell pkg-config --cflags alsa freetype2 libcurl x11 xext xinerama) -pthread -I../../JuceLibraryCode -I../../../../../HISE -I../../../../JUCE/modules
  JUCE_CFLAGS += $(CFLAGS) $(JUCE_CPPFLAGS) $(TARGET_ARCH) -g -ggdb -O0
  JUCE_CXXFLAGS += $(CXXFLAGS) $(JUCE_CFLAGS) -std=c++11 -stdlib=libc++
  JUCE_LDFLAGS += $(LDFLAGS) $(TARGET_ARCH) -L$(JUCE_BIND IR) -L$(JUCE_LIBDIR) $(shell pkg-config --libs alsa freetype2 libcurl x11 xext xinerama) -lGL -ldl -lpthread -lrt 

  TARGET := HISE\ Standalone
  BLDCMD = $(CXX) -o $(JUCE_OUTDIR)/$(TARGET) $(OBJECTS) $(JUCE_LDFLAGS) $(RESOURCES) $(TARGET_ARCH)
  CLEANCMD = rm -rf $(JUCE_OUTDIR)/$(TARGET) $(JUCE_OBJDIR)
endif

ifeq ($(CONFIG),Release)
  JUCE_BINDIR := build
  JUCE_LIBDIR := build
  JUCE_OBJDIR := build/intermediate/Release
  JUCE_OUTDIR := build

  ifeq ($(TARGET_ARCH),)
    TARGET_ARCH := -march=native
  endif

  JUCE_CPPFLAGS := $(DEPFLAGS) -DLINUX=1 -DNDEBUG=1 -DUSE_IPP=0 -DJUCER_LINUX_MAKE_6D53C8B4=1 -DJUCE_APP_VERSION=0.99 -DJUCE_APP_VERSION_HEX=0x6300 $(shell pkg-config --cflags alsa freetype2 libcurl x11 xext xinerama) -pthread -I../../JuceLibraryCode -I../../../../../HISE -I../../../../JUCE/modules
  JUCE_CFLAGS += $(CFLAGS) $(JUCE_CPPFLAGS) $(TARGET_ARCH) -O3
  JUCE_CXXFLAGS += $(CXXFLAGS) $(JUCE_CFLAGS) -std=c++11
  JUCE_LDFLAGS += $(LDFLAGS) $(TARGET_ARCH) -L$(JUCE_BINDIR) -L$(JUCE_LIBDIR) -fvisibility=hidden $(shell pkg-config --libs alsa freetype2 libcurl x11 xext xinerama) -lGL -ldl -lpthread -lrt 

  TARGET := HISE\ Standalone
  BLDCMD = $(CXX) -o $(JUCE_OUTDIR)/$(TARGET) $(OBJECTS) $(JUCE_LDFLAGS) $(RESOURCES) $(TARGET_ARCH)
  CLEANCMD = rm -rf $(JUCE_OUTDIR)/$(TARGET) $(JUCE_OBJDIR)
endif

OBJECTS := \
  $(JUCE_OBJDIR)/DspUnitTests_8fd29654.o \
  $(JUCE_OBJDIR)/HiseEventBufferUnitTests_fc3efacf.o \
  $(JUCE_OBJDIR)/MainComponent_a6ffb4a5.o \
  $(JUCE_OBJDIR)/Main_90ebc5c2.o \
  $(JUCE_OBJDIR)/BinaryData_ce4232d4.o \
  $(JUCE_OBJDIR)/hi_backend_1cbc0b3f.o \
  $(JUCE_OBJDIR)/hi_core_bbc2a29e.o \
  $(JUCE_OBJDIR)/hi_dsp_library_a9241d68.o \
  $(JUCE_OBJDIR)/hi_modules_e5677ab2.o \
  $(JUCE_OBJDIR)/hi_scripting_d8d7e4e2.o \
  $(JUCE_OBJDIR)/juce_audio_basics_6b797ca1.o \
  $(JUCE_OBJDIR)/juce_audio_devices_a742c38b.o \
  $(JUCE_OBJDIR)/juce_audio_formats_5a29c68a.o \
  $(JUCE_OBJDIR)/juce_audio_processors_dea3173d.o \
  $(JUCE_OBJDIR)/juce_audio_utils_c7eb679f.o \
  $(JUCE_OBJDIR)/juce_core_75b14332.o \
  $(JUCE_OBJDIR)/juce_cryptography_6de2ebff.o \
  $(JUCE_OBJDIR)/juce_data_structures_72d3da2c.o \
  $(JUCE_OBJDIR)/juce_events_d2be882c.o \
  $(JUCE_OBJDIR)/juce_graphics_9c18891e.o \
  $(JUCE_OBJDIR)/juce_gui_basics_8a6da59c.o \
  $(JUCE_OBJDIR)/juce_gui_extra_4a026f23.o \
  $(JUCE_OBJDIR)/juce_opengl_cd70b4c2.o \
  $(JUCE_OBJDIR)/juce_tracktion_marketplace_51662d82.o \

.PHONY: clean

$(JUCE_OUTDIR)/$(TARGET): check-pkg-config $(OBJECTS) $(RESOURCES)
	@echo Linking HISE Standalone
	-@mkdir -p $(JUCE_BINDIR)
	-@mkdir -p $(JUCE_LIBDIR)
	-@mkdir -p $(JUCE_OUTDIR)
	@$(BLDCMD)

check-pkg-config:
	@command -v pkg-config >/dev/null 2>&1 || { echo >&2 "pkg-config not installed. Please, install it."; exit 1; }
	@pkg-config --print-errors alsa freetype2 libcurl x11 xext xinerama

clean:
	@echo Cleaning HISE Standalone
	@$(CLEANCMD)

strip:
	@echo Stripping HISE Standalone
	-@$(STRIP) --strip-unneeded $(JUCE_OUTDIR)/$(TARGET)

$(JUCE_OBJDIR)/DspUnitTests_8fd29654.o: ../../../../hi_scripting/scripting/api/DspUnitTests.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling DspUnitTests.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/HiseEventBufferUnitTests_fc3efacf.o: ../../../../hi_core/hi_core/HiseEventBufferUnitTests.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling HiseEventBufferUnitTests.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/MainComponent_a6ffb4a5.o: ../../Source/MainComponent.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling MainComponent.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/Main_90ebc5c2.o: ../../Source/Main.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling Main.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/BinaryData_ce4232d4.o: ../../JuceLibraryCode/BinaryData.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling BinaryData.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/hi_backend_1cbc0b3f.o: ../../JuceLibraryCode/hi_backend.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling hi_backend.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/hi_core_bbc2a29e.o: ../../JuceLibraryCode/hi_core.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling hi_core.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/hi_dsp_library_a9241d68.o: ../../JuceLibraryCode/hi_dsp_library.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling hi_dsp_library.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/hi_modules_e5677ab2.o: ../../JuceLibraryCode/hi_modules.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling hi_modules.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/hi_scripting_d8d7e4e2.o: ../../JuceLibraryCode/hi_scripting.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling hi_scripting.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_audio_basics_6b797ca1.o: ../../JuceLibraryCode/juce_audio_basics.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_audio_basics.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_audio_devices_a742c38b.o: ../../JuceLibraryCode/juce_audio_devices.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_audio_devices.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_audio_formats_5a29c68a.o: ../../JuceLibraryCode/juce_audio_formats.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_audio_formats.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_audio_processors_dea3173d.o: ../../JuceLibraryCode/juce_audio_processors.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_audio_processors.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_audio_utils_c7eb679f.o: ../../JuceLibraryCode/juce_audio_utils.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_audio_utils.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_core_75b14332.o: ../../JuceLibraryCode/juce_core.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_core.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_cryptography_6de2ebff.o: ../../JuceLibraryCode/juce_cryptography.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_cryptography.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_data_structures_72d3da2c.o: ../../JuceLibraryCode/juce_data_structures.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_data_structures.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_events_d2be882c.o: ../../JuceLibraryCode/juce_events.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_events.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_graphics_9c18891e.o: ../../JuceLibraryCode/juce_graphics.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_graphics.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_gui_basics_8a6da59c.o: ../../JuceLibraryCode/juce_gui_basics.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_gui_basics.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_gui_extra_4a026f23.o: ../../JuceLibraryCode/juce_gui_extra.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_gui_extra.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_opengl_cd70b4c2.o: ../../JuceLibraryCode/juce_opengl.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_opengl.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

$(JUCE_OBJDIR)/juce_tracktion_marketplace_51662d82.o: ../../JuceLibraryCode/juce_tracktion_marketplace.cpp
	-@mkdir -p $(JUCE_OBJDIR)
	@echo "Compiling juce_tracktion_marketplace.cpp"
	@$(CXX) $(JUCE_CXXFLAGS) -o "$@" -c "$<"

-include $(OBJECTS:%.o=%.d)
