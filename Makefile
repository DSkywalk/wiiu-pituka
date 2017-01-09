#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

# Use the compile script when no explicit target is named (it will just call make)
ifeq ($(strip $(COMPILE)),)
.PHONY: compile
compile:
	@./compile
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	wiituka
BUILD		:=	build
SOURCES		:=	source/wii source/caprice source/port source/ymlib source/wii/gfx source/wii/libpngu source/wii/grrlib source/wii/menu source/wii/wpad_leds source/wii/tcp source/wii/images source/wii/fonts source/wii/sound
DATA		:=	data  
INCLUDES	:=  

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------

CFLAGS_X	 = -ggdb -O2 -Wall $(MACHDEP) $(INCLUDE) -I$(LIBOGC_INC)/freetype2
LDFLAGS_X	 = -ggdb -O2 $(MACHDEP) -Wl,-Map,$(notdir $@).map

CFLAGS_D   = -ggdb -O1 -Wall $(MACHDEP) $(INCLUDE) -I$(LIBOGC_INC)/freetype2
LDFLAGS_D  = -ggdb -O1 $(MACHDEP) -Wl,-Map,$(notdir $@).map

CFLAGS = $(CFLAGS_X)
LDFLAGS = $(LDFLAGS_X)

CXXFLAGS = $(CFLAGS)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:=	 -lmad -lasnd -lpng -lz -lfat -lwiiuse -lmxml -lbte -logc -lm -lwiikeyboard -lfreetype

# -lSDL 
#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
TTFFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.ttf)))
PNGFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.png)))
MP3FILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.mp3)))
ymFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.ym)))
YMFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.YM)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) \
					$(sFILES:.s=.o) $(SFILES:.S=.o) \
					$(TTFFILES:.ttf=.ttf.o) $(PNGFILES:.png=.png.o) $(MP3FILES:.mp3=.mp3.o)

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES), -iquote $(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) \
					-I$(LIBOGC_INC) -I$(LIBOGC_INC)/SDL

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
					-L$(LIBOGC_LIB)

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol

#---------------------------------------------------------------------------------
run:
	wiiload $(TARGET).dol

commit:
	svn commit 

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

#---------------------------------------------------------------------------------
# This rule links in binary data with the data extensions
#---------------------------------------------------------------------------------
%.jpg.o	:	%.jpg
	@echo $(notdir $<)
	$(bin2o)
%.ttf.o : 	%.ttf
	@echo $(notdir $<)
	$(bin2o)
	
%.png.o : 	%.png
	@echo $(notdir $<)
	$(bin2o)

%.mp3.o :	%.mp3
	@echo $(notdir $<)
	$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
