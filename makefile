## Makefile - build DuckHunt with SFML (uses pkg-config when available)

SRC_DIR := src
BIN_DIR := bin
OBJ_DIR := build

ifeq ($(OS),Windows_NT)
EXE_EXT := .exe
else
EXE_EXT :=
endif

EXE := $(BIN_DIR)/DuckHunt$(EXE_EXT)

CXX := g++
CXXFLAGS := -std=c++17 -O2 -Iinclude

PKG_CONFIG := pkg-config
PKG_SFML_ALL := $(shell $(PKG_CONFIG) --cflags --libs sfml-all 2>/dev/null)
ifeq ($(PKG_SFML_ALL),)
SFML_CFLAGS := $(shell $(PKG_CONFIG) --cflags sfml-graphics 2>/dev/null)
SFML_LIBS := $(shell $(PKG_CONFIG) --libs sfml-graphics 2>/dev/null) $(shell $(PKG_CONFIG) --libs sfml-audio 2>/dev/null)
else
SFML_CFLAGS := $(shell $(PKG_CONFIG) --cflags sfml-all)
SFML_LIBS := $(shell $(PKG_CONFIG) --libs sfml-all)
endif

SRCS := $(SRC_DIR)/main.cpp $(SRC_DIR)/Game.cpp $(SRC_DIR)/Duck.cpp
OBJS := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

all: directories $(EXE)

directories:
	@mkdir -p $(BIN_DIR) $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | directories
	$(CXX) $(CXXFLAGS) $(SFML_CFLAGS) -c $< -o $@

$(EXE): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(SFML_LIBS)

# make run should build first, then run the exe. Works in MSYS/MinGW and Unix shells.
run: all
	@echo "Running $(EXE)"
	@$(EXE)

clean:
	-rm -rf $(OBJ_DIR) $(EXE)

.PHONY: all run clean directories

# Notes:
# - This Makefile prefers pkg-config to locate SFML. If pkg-config is not available,
#   set `SFML_CFLAGS` and `SFML_LIBS` manually near the top of the file, for example:
#     SFML_CFLAGS := -I/mingw64/include
#     SFML_LIBS   := -L/mingw64/lib -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio
# - On MSYS2 install SFML packages: pacman -S mingw-w64-x86_64-SFML
