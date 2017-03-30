-include $(TOPDIR)/rules.mk

CFLAGS := -fPIC -Wall -g -MMD
LDFLAGS := -shared -fPIC

LIB := libwigig_utils.so

.DEFAULT_GOAL = all


ifneq ($(CONFIG_TARGET_ipq)$(CONFIG_TARGET_ipq806x),)
is_ipq806x = 1
endif

ifeq ($(is_ipq806x), 1)
ifneq ($(strip $(TOOLPREFIX)),)
CROSS:=$(TOOLPREFIX)
endif
endif

CC = $(CROSS)gcc
CXX = $(CROSS)g++

INCLUDES = -I . \
	   -I ../inc/linux \
           -I ../inc \
           -I ./linux \


all: $(LIB)


CPP_FILES := $(shell find . -type f -name '*.cpp')
C_FILES := $(shell find . -type f -name '*.c')

OBJ_FILES :=  $(CPP_FILES:.cpp=.o)
OBJ_FILES +=  $(C_FILES:.c=.o)

$(LIB): $(OBJ_FILES)
	$(CXX) $(LDFLAGS) -o $(LIB) $(OBJ_FILES)

%.o : %.cpp
	$(CXX) $(CFLAGS) $(INCLUDES) -o $@ -c $<

%.o : %.c
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ -c $<




clean:
	rm -rf $(LIB)
	find . -type f \( -name "*.d" -o -name "*.o" \) -delete


-include $(OBJ_FILES:%.o=%.d)
