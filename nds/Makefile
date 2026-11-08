#---------------------------------------------------------------------------------
# Sky Dash für den Nintendo DS  –  baut skydash.nds
# Braucht devkitPro mit devkitARM + libnds + ndstool (siehe LIESMICH.txt)
#---------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Bitte DEVKITARM setzen, z. B.: export DEVKITARM=/opt/devkitpro/devkitARM")
endif

include $(DEVKITARM)/ds_rules

TARGET   := skydash
BUILD    := build
SOURCES  := source
DATA     :=
INCLUDES := source

GAME_TITLE     := Sky Dash
GAME_SUBTITLE1 := Sky Games
GAME_SUBTITLE2 := RD-GlitchedGuy

ARCH     := -march=armv5te -mtune=arm946e-s -mthumb
CFLAGS   := -g -Wall -O2 -fomit-frame-pointer -ffunction-sections -fdata-sections $(ARCH)
CFLAGS   += $(INCLUDE) -DARM9
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS  := -g $(ARCH)
LDFLAGS   = -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS     := -lnds9 -lm
LIBDIRS  := $(LIBNDS)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH  := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))

ifeq ($(strip $(CPPFILES)),)
	export LD := $(CC)
else
	export LD := $(CXX)
endif

export OFILES  := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean levels

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds

# Level aus levels/*.txt neu in source/levels.h übersetzen
levels:
	g++ -O2 -std=c++17 tools/leveltool.cpp -o tools/leveltool
	tools/leveltool levels --nds source/levels.h

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).nds : $(OUTPUT).elf
$(OUTPUT).elf : $(OFILES)

-include $(DEPENDS)

endif
