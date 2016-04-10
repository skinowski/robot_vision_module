#
# Robot Vision Module Makefile
#
NAME=test_client

CPP=g++
CPPFLAGS=-g -O2 -MMD -std=c++11 -I..
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