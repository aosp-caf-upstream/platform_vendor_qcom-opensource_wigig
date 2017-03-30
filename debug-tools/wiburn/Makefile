-include $(TOPDIR)/rules.mk

CFLAGS := -fPIE -Wall -g -MMD -fPIE -Wno-reorder
LDFLAGS := -pie -fPIE -pthread -lwigig_flashaccess -lwigig_pciaccess -lwigig_utils

ifneq ($(CONFIG_TARGET_ipq)$(CONFIG_TARGET_ipq806x),)
is_ipq806x = 1
endif

ifeq ($(is_ipq806x), 1)
ifneq ($(strip $(TOOLPREFIX)),)
CROSS:=$(TOOLPREFIX)
endif
endif

CC := $(CROSS)gcc
CXX := $(CROSS)g++

.DEFAULT_GOAL = all
PROG = wigig_wiburn

INCLUDES += -I ../lib/WlctPciAcss \
            -I ../lib/FlashAcss \
            -I ../lib/inc \
            -I ../lib/utils \

LIBS = -L../lib/FlashAcss \
       -L../lib/WlctPciAcss \
       -L../lib/utils \

all: $(PROG)

CPP_FILES = wiburn.cpp \
            translation_map.cpp \
            ParameterParser.cpp \
            template_inst.cpp \
            CCRC32.cpp \

OBJ_FILES = $(CPP_FILES:.cpp=.o)

$(PROG): $(OBJ_FILES)
	$(CXX) -o $@ $^ $(LIBS) $(LDFLAGS)

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ -c $<

clean:
	rm -rf $(PROG)
	find . -type f \( -name "*.d" -o -name "*.o" \) -delete


-include $(OBJ_FILES:%.o=%.d)

