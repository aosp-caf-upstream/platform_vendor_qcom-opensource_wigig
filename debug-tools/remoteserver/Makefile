-include $(TOPDIR)/rules.mk

CFLAGS := -fPIE -Wall -g -MMD -Wno-unused-function
LDFLAGS := -pie -fPIE -pthread -lwigig_pciaccess -lwigig_utils

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
PROG = wigig_remoteserver

INCLUDES = -I ../lib/WlctPciAcss \
           -I ../lib/inc \
           -I ../lib/utils \

LIBS = -L../lib/WlctPciAcss \
       -L../lib/utils \

all: $(PROG)

CPP_FILES = $(shell find . -type f -name '*.cpp')
C_FILES = $(shell find . -type f -name '*.c')
LEX_FILES = $(shell find . -type f -name '*.l')


GENERATED_C_FILES=$(LEX_FILES:.l=.c)
OBJ_FILES=  $(CPP_FILES:.cpp=.o)
OBJ_FILES += $(C_FILES:.c=.o)
OBJ_FILES += $(GENERATED_C_FILES:.c=.o)

.PRECIOUS: $(GENERATED_C_FILES)

$(PROG): $(OBJ_FILES)
	$(CXX) -o $@ $^ $(LIBS) $(LDFLAGS)

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ -c $<

%.c : %.l
	flex -o $@ $<



clean:
	rm -rf $(PROG) $(GENERATED_C_FILES)
	find . -type f \( -name "*.d" -o -name "*.o" \) -delete


-include $(OBJ_FILES:%.o=%.d)
