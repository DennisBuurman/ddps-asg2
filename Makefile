#===Omnipotent Makefile v1.0===
# Author:       Jochem T.S. Ram
# Description:  General-purpose makefile that supports directories and flags.
#               Only the variables in the first sections should have to be
#               touched.
# Functions:    - 'release' for optimized code
#               - 'debug' for debug-friendly code
#               - 'remake' to recompile everything
#               - 'clean' to delete all compiled code
# Copyright (C) Jochem Ram (2019)
#===

# ========== DEFINE VARIABLES HERE ========== #
#Define the compiler
CC = mpicxx

#Path for final binary program
TARGET = raft

#The Source, Includes, Objects and Binary directories
SRC_DIR = src
OBJ_DIR = obj
TARGET_DIR = bin

#File extensions
SRC_EXT = cc
DEP_EXT = d
OBJ_EXT = o

#Flags, Libraries and Includes
CFLAGS = -Wall
LIB = 
INC =

# ========== DO NOT EDIT BEYOND THIS POINT ========== #
#make sure these names are not associated with files
.PHONY: clean release debug all

#Automatically populate list of sources, objects and dependencies
SOURCES = $(shell find $(SRC_DIR) -type f -name *.$(SRC_EXT))
OBJECTS = $(patsubst $(SRC_DIR)/%,$(OBJ_DIR)/%,$(SOURCES:.$(SRC_EXT)=.$(OBJ_EXT)))
DEPS = $(OBJECTS:.$(OBJ_EXT)=.$(DEP_EXT))

#Compile with maximum optimization, disable assertions
release: CFLAGS += -O3 -DNDEBUG
release: all

#more warnings, enable assertions and maximum debug information
debug: CFLAGS += -Wextra -DDEBUG -Og -g3
debug: all

#Clean everything then rebuild
remake: clean all

#Link everything
all: prepare $(TARGET)

#Create directories if needed
prepare:
	mkdir -p $(TARGET_DIR)
	mkdir -p $(OBJ_DIR)

#Remove all compiled output
clean:
	rm -f $(OBJECTS) $(DEPS) $(TARGET)

#Link all objects
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET_DIR)/$(TARGET) $^ $(LIB)

#Compile objects (keeping dependencies in mind)
$(OBJ_DIR)/%.$(OBJ_EXT): $(SRC_DIR)/%.$(SRC_EXT)
	$(CC) $(CFLAGS) $(INC) -MD -o $@ -c $<
-include $(DEPS)
