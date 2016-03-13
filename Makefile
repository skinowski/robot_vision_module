#
# Robot Vision Module Makefile
#
NAME=vision_module.out

#
# TODO: Do we need to link to all OpenCV libs? Reduce this
# 
OPENCV_LDFLAGS := $(shell pkg-config opencv --libs)
OPENCV_CPPFLAGS := $(shell pkg-config opencv --cflags)

CPP=g++
CPPFLAGS=-g -O2 -MMD -std=c++11 -pthread
LDFLAGS=-lrt -pthread

MODULES :=

SOURCES := $(wildcard *.cpp)

-include $(patsubst %, %/module.mk, $(MODULES))

OBJECTS := $(patsubst %.cpp, %.o, $(filter %.cpp,$(SOURCES)))

.PHONY: all
all : compile_all

%.o : %.cpp
	$(CPP) $(CPPFLAGS) $(OPENCV_CPPFLAGS) $(INCLUDES) -c -o $@ $<

$(NAME) : $(OBJECTS)
	$(CPP) -o $@ $^ $(LDFLAGS) $(OPENCV_LDFLAGS)

.PHONY: compile_all
compile_all: $(NAME)

.PHONY: clean
clean :
	@rm -f $(OBJECTS) $(NAME)
	@rm -f $(patsubst %.o, %.d, $(filter %.o,$(OBJECTS)))

-include $(OBJECTS:.o=.d)