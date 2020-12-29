# Author: Shih-Hao Tseng <st688@cornell.edu>
SOURCE_DIR=sources
BUILD_DIR=build
TEST_SOURCE_DIR=tests
FINISHED_TEST_SOURCE_DIR=finished_tests
EXECUTABLE_DIR=executable

SOURCES=$(shell find $(SOURCE_DIR)/ -name '*.cc')
OBJECTS=$(SOURCES:$(SOURCE_DIR)/%.cc=$(BUILD_DIR)/%.o)
SOURCE_PARTS=$(shell find $(SOURCE_DIR)/ -name '*.cc_part')

TEST_SOURCES=$(shell find $(TEST_SOURCE_DIR)/ -name '*.cc')
FINISHED_TEST_SOURCES=$(TEST_SOURCES:$(TEST_SOURCE_DIR)/%.cc=$(FINISHED_TEST_SOURCE_DIR)/%.cc)
TEST_EXECUTABLES=$(TEST_SOURCES:$(TEST_SOURCE_DIR)/%.cc=$(EXECUTABLE_DIR)/%)

LOCAL_SOCKETS=$(shell find local_socket/ -name 's*')

CC=g++
CFLAGS=-c -g -O3 -pg

prog_INCLUDE_DIRS=sources/
prog_LIBRARY_DIRS=
prog_RUNTIME_LIB_DIRS=
prog_LIBS=

CPPFLAGS += $(foreach includedir,$(prog_INCLUDE_DIRS),-I$(includedir))
LDFLAGS  += $(foreach librarydir,$(prog_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS  += $(foreach rt_libdir,$(prog_RUNTIME_LIB_DIRS),-Wl,-rpath,$(rt_libdir))
LDFLAGS  += $(foreach library,$(prog_LIBS),-l$(library))

CPPFLAGS += -Wno-misleading-indentation -pthread

all: $(TEST_EXECUTABLES)

finish:
ifneq ($(TEST_EXECUTABLES),)
	rm $(TEST_EXECUTABLES)
endif
	make movetest

movetest: $(FINISHED_TEST_SOURCES)
ifneq ($(TEST_SOURCES),)
	rm $(TEST_SOURCES)
endif

.PHONY: all clean

.PRECIOUS: $(BUILD_DIR)/%.o

$(EXECUTABLE_DIR)/%: $(TEST_SOURCE_DIR)/%.cc $(OBJECTS) $(SOURCE_PARTS)
	$(CC) $< $(OBJECTS) $(LDFLAGS) $(CPPFLAGS) -o $@

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.cc
	$(CC) $(LDFLAGS) $(CFLAGS) $(CPPFLAGS) $< -o $@

$(FINISHED_TEST_SOURCE_DIR)/%.cc: $(TEST_SOURCE_DIR)/%.cc
	mv $< $@

clean:
ifneq ($(OBJECTS) $(TEST_EXECUTABLES),)
	rm -f $(OBJECTS) $(TEST_EXECUTABLES)
endif

clearlocalsocket:
	$(foreach socket,$(LOCAL_SOCKETS),unlink $(socket);)

cleanshared:
ifneq ($(SHARED_OBJECTS),)
	rm $(SHARED_OBJECTS)
endif