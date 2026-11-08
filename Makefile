.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please install devkitPro/devkitARM and set DEVKITARM")
endif

include $(DEVKITARM)/ds_rules

# DSi launcher/banner information.
TARGET := sky_dash
GAME_TITLE := SKY DASH
GAME_SUBTITLE1 := SKY GAMES
GAME_SUBTITLE2 := OFFLINE RHYTHM RUNNER

BUILD := build
SOURCES := source
INCLUDES := include
DATA :=
GRAPHICS :=
AUDIO := assets/audio
ICON := data/icon.bmp
NITRO := nitrofs

ARCH := -march=armv5te -mtune=arm946e-s
CFLAGS := -g -Wall -Wextra -O2 -ffunction-sections -fdata-sections $(ARCH)
CFLAGS += $(INCLUDE) -DARM9
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=ds_arm9.specs -g $(ARCH) -Wl,--gc-sections

LIBS := -lnds9 -lm
ifneq ($(strip $(NITRO)),)
LIBS := -lfilesystem -lfat $(LIBS)
endif
ifneq ($(strip $(AUDIO)),)
LIBS := -lmm9 $(LIBS)
endif

LIBDIRS := $(LIBNDS) $(PORTLIBS)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                $(CURDIR)/data
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifneq ($(strip $(AUDIO)),)
export MODFILES := $(foreach file,$(notdir $(wildcard $(AUDIO)/*.*)),$(CURDIR)/$(AUDIO)/$(file))
ifneq ($(strip $(NITRO)),)
export SOUNDBANK := $(CURDIR)/$(NITRO)/soundbank.bin
else
export SOUNDBANK := soundbank.bin
BINFILES += $(SOUNDBANK)
endif
endif

ifneq ($(strip $(NITRO)),)
export NITRO_FILES := $(CURDIR)/$(NITRO)
endif

export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)
export HFILES := $(addsuffix .h,$(subst .,_,$(BINFILES)))

ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif

export INCLUDE := $(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export GAME_ICON := $(CURDIR)/$(BUILD)/$(notdir $(basename $(ICON))).grf

.PHONY: all clean $(BUILD)
all: $(BUILD)

$(BUILD):
	@mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo "clean ..."
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).ds.gba $(NITRO)/soundbank.bin

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).nds: $(OUTPUT).elf $(NITRO_FILES) $(GAME_ICON)
$(OUTPUT).elf: $(OFILES)

# The generated Maxmod header is available before level data is compiled.
$(OFILES_SOURCES): $(SOUNDBANK)
ifneq ($(strip $(SOUNDBANK)),)
$(SOUNDBANK): $(MODFILES)
	@mkdir -p $(dir $@)
	mmutil $^ -d -o$@ -hsoundbank.h
endif

%.bin.o %_bin.h: %.bin
	@echo "Embedding $(notdir $<)"
	@$(bin2o)

$(GAME_ICON): $(notdir $(ICON))
	@mkdir -p $(dir $@)
	@echo "Converting DSi banner icon $(notdir $<)"
	@grit $< -g -gt -gB4 -gT FF00FF -m! -p -pe 16 -fh! -ftr

-include $(DEPSDIR)/*.d

endif
