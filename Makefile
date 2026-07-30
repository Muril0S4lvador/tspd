# ==============================================================================
# CONFIGURAÇÕES DO COMPILADOR
# ==============================================================================
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2
LDFLAGS = -lsfml-graphics -lsfml-window -lsfml-system

SRC_DIR   := src
BUILD_DIR := build

ifeq ($(OS),Windows_NT)
    TARGET := main.exe
else
    TARGET := main
endif

# ==============================================================================
# INCLUSÃO DE CABEÇALHOS (-I)
# ==============================================================================
INC_FLAGS := -Isrc -Isrc/instance_reader -Isrc/node -Isrc/tspd

# ==============================================================================
# MAPEAMENTO DOS ARQUIVOS .CPP E .O
# ==============================================================================
# Busca todos os arquivos .cpp nas subpastas conhecidas de src/
SRCS := $(wildcard $(SRC_DIR)/*.cpp) \
        $(wildcard $(SRC_DIR)/instance_reader/*.cpp) \
        $(wildcard $(SRC_DIR)/node/*.cpp) \
        $(wildcard $(SRC_DIR)/tspd/*.cpp) \
        $(wildcard $(SRC_DIR)/graphic/*.cpp)

# Mapeia cada .cpp para o seu respectivo .o dentro de build/
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Mapeia arquivos de dependência de headers (.d)
DEPS := $(OBJS:.o=.d)

# ==============================================================================
# REGRAS DO MAKE
# ==============================================================================
all: $(TARGET)

# Regra para ligar os arquivos objeto e gerar o executável final
$(TARGET): $(OBJS)
	@echo "=========================================="
	@echo "Ligando o executavel final: $@"
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Compilacao concluida com sucesso!"
	@echo "=========================================="

# Regra para compilar cada .cpp em seu respectivo .o dentro de build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compilando: $<"
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -MMD -MP -c $< -o $@

# Regra de limpeza
clean:
	@echo "Limpando arquivos compilados..."
	@rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)

.PHONY: all clean