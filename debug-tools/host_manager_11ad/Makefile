-include $(TOPDIR)/rules.mk

CPPFLAGS := -Wall -g -MMD -std=c++0x -fPIE
LDFLAGS := -pthread -fPIE -pie

ifneq ($(CONFIG_TARGET_ipq)$(CONFIG_TARGET_ipq806x),)
is_ipq806x = 1
endif

ifeq ($(is_ipq806x), 1)
ifneq ($(strip $(TOOLPREFIX)),)
CROSS:=$(TOOLPREFIX)
endif
endif

CXX := $(CROSS)g++

.DEFAULT_GOAL = all
PROG = host_manager_11ad

INCLUDE_CFLAGS += -I access_layer_11ad \
                  -I access_layer_11ad/Unix \

all: $(PROG)

CPP_FILES = $(shell find . -type f -name '*.cpp')
OBJ_FILES=  $(CPP_FILES:.cpp=.o)

$(PROG): $(OBJ_FILES) $(LIBS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(LIBS)

%.o : %.cpp
	$(CXX) $(CPPFLAGS) $(INCLUDE_CFLAGS) -o $@ -c $<

clean:
	rm -rf $(PROG)
	find . -type f \( -name "*.d" -o -name "*.o" \) -delete

-include $(OBJ_FILES:%.o=%.d)
