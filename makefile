# Directorios de origen y destino
SRC_DIR := src
BIN_DIR := bin

SFML := -lsfml-graphics -lsfml-window -lsfml-system -lsfml-audio -lbox2d

# Obtener todos los archivos .cpp en el directorio de origen
CPP_FILES := $(wildcard $(SRC_DIR)/*.cpp)

# Generar los nombres de los archivos .exe en el directorio de destino
EXE_FILES := $(patsubst $(SRC_DIR)/%.cpp,$(BIN_DIR)/%.exe,$(CPP_FILES))

# Regla para compilar cada archivo .cpp y generar el archivo .exe correspondiente
$(BIN_DIR)/%.exe: $(SRC_DIR)/%.cpp
	g++ $< -o $@ $(SFML) -Iinclude

# Regla por defecto para compilar todos los archivos .cpp
all: $(EXE_FILES)

# Explicit DuckHunt target (build only our new game sources)
$(BIN_DIR)/DuckHunt.exe: $(SRC_DIR)/main.cpp $(SRC_DIR)/Game.cpp $(SRC_DIR)/Duck.cpp
	g++ $^ -o $@ $(SFML) -Iinclude

.PHONY: duckhunt
duckhunt: $(BIN_DIR)/DuckHunt.exe
	@echo "Built DuckHunt.exe -> $(BIN_DIR)/DuckHunt.exe"

# Regla para ejecutar cada archivo .exe
run%: $(BIN_DIR)/%.exe
	./$<

# Regla para limpiar los archivos generados
clean:
	rm -f $(EXE_FILES)

.PHONY: all clean
.PHONY: run-%

# Shortcut to run DuckHunt
.PHONY: runduckhunt
runduckhunt: $(BIN_DIR)/DuckHunt.exe
	./$(BIN_DIR)/DuckHunt.exe
