# Author: Shih-Hao Tseng <st688@cornell.edu>
SOURCE_DIR=sources
BUILD_DIR=build
MAIN_DIR=main
SHARED_SOURCE_DIR=../shared/sources
SHARED_BUILD_DIR=../shared/build
EXECUTABLE_DIR=executable

SOURCES=$(shell find $(SOURCE_DIR)/ -name '*.cc')
OBJECTS=$(SOURCES:$(SOURCE_DIR)/%.cc=$(BUILD_DIR)/%.o)
SOURCE_PARTS=$(shell find $(SOURCE_DIR)/ -name '*.cc_part')

SHARED_SOURCES=$(shell find $(SHARED_SOURCE_DIR)/ -name '*.cc')
SHARED_OBJECTS=$(SHARED_SOURCES:$(SHARED_SOURCE_DIR)/%.cc=$(SHARED_BUILD_DIR)/%.o)

SIM_SOURCES=$(shell find $(MAIN_DIR)/ -name '*.cc')
EXECUTABLES=$(SIM_SOURCES:$(MAIN_DIR)/%.cc=$(EXECUTABLE_DIR)/sim-%)

CC=g++
CFLAGS=-c -g -O3 -pg

prog_INCLUDE_DIRS=../shared/sources/ sources/
prog_LIBRARY_DIRS=
prog_RUNTIME_LIB_DIRS=
prog_LIBS=

CPPFLAGS += $(foreach includedir,$(prog_INCLUDE_DIRS),-I$(includedir))
LDFLAGS  += $(foreach librarydir,$(prog_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS  += $(foreach rt_libdir,$(prog_RUNTIME_LIB_DIRS),-Wl,-rpath,$(rt_libdir))
LDFLAGS  += $(foreach library,$(prog_LIBS),-l$(library))

CPPFLAGS += -Wno-misleading-indentation -pthread

all: $(EXECUTABLES)

.PHONY: all clean

.PRECIOUS: $(BUILD_DIR)/%.o $(SHARED_BUILD_DIR)/%.o

$(EXECUTABLE_DIR)/sim-%: $(MAIN_DIR)/%.cc $(SHARED_OBJECTS) $(OBJECTS) $(SOURCE_PARTS)
	$(CC) $< $(SHARED_OBJECTS) $(OBJECTS) $(LDFLAGS) $(CPPFLAGS) -o $@

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cc
	$(CC) $(LDFLAGS) $(CFLAGS) $(CPPFLAGS) $< -o $@

$(SHARED_BUILD_DIR)/%.o: $(SHARED_SOURCE_DIR)/%.cc
	$(CC) $(LDFLAGS) $(CFLAGS) $(CPPFLAGS) $< -o $@

clean:
ifneq ($(OBJECTS) $(EXECUTABLES),)
	rm $(OBJECTS) $(EXECUTABLES)
endif

cleanshared:
ifneq ($(SHARED_OBJECTS),)
	rm $(SHARED_OBJECTS)
endif
