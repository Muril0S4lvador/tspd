# ==============================================================================
# CONFIGURAÇÕES DO COMPILADOR
# ==============================================================================
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -O2

SRC_DIR   := src
BUILD_DIR := build

ifeq ($(OS),Windows_NT)
    TARGET := main.exe
    GRAPHICS_FLAGS := -DTSPD_ENABLE_GRAPHICS
    GRAPHICS_LIBS := -lsfml-graphics -lsfml-window -lsfml-system
    GRAPHICS_SRCS := $(wildcard $(SRC_DIR)/graphic/*.cpp)
else
    TARGET := main
endif

# ==============================================================================
# INCLUSÃO DE CABEÇALHOS (-I)
# ==============================================================================
INC_FLAGS := -Isrc -Isrc/edge -Isrc/instance_reader -Isrc/node -Isrc/tspd -Isrc/utils

# ==============================================================================
# MAPEAMENTO DOS ARQUIVOS .CPP E .O
# ==============================================================================
# Busca todos os arquivos .cpp nas subpastas conhecidas de src/
SRCS := $(wildcard $(SRC_DIR)/*.cpp) \
        $(wildcard $(SRC_DIR)/TSP/*.cpp) \
        $(wildcard $(SRC_DIR)/hgvns/*.cpp) \
        $(wildcard $(SRC_DIR)/hga_tac/*.cpp) \
        $(wildcard $(SRC_DIR)/tspd/*.cpp) \
        $(wildcard $(SRC_DIR)/node/*.cpp) \
        $(wildcard $(SRC_DIR)/instance_reader/*.cpp) \
        $(wildcard $(SRC_DIR)/solution_reader/*.cpp) \
        $(wildcard $(SRC_DIR)/edge/*.cpp) \
        $(wildcard $(SRC_DIR)/solution/*.cpp) \
        $(wildcard $(SRC_DIR)/kruskal/*.cpp) \
        $(wildcard $(SRC_DIR)/agatz_solution/**/*.cpp) \
        $(GRAPHICS_SRCS)

# Mapeia cada .cpp para o seu respectivo .o dentro de build/
OBJS := $(SRCS:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)

# Mapeia arquivos de dependência de headers (.d)
DEPS := $(OBJS:.o=.d)

# ==============================================================================
# REGRAS DO MAKE
# ==============================================================================
all: $(TARGET)

background: export dir := $(dir)
background:
ifeq ($(OS),Windows_NT)
	@echo "Use make background no Linux, macOS ou WSL." >&2
	@exit 1
else
	@if [ -z "$$dir" ] || [ ! -d "$$dir" ]; then \
		echo 'Informe uma pasta de instancias existente em dir.' >&2; \
		echo 'Exemplo: make background dir=data/singlecenter' >&2; \
		exit 1; \
	fi
	@$(MAKE) all
	@mkdir -p results
	@log=$$(mktemp results/segundo-plano.XXXXXX) || exit 1; \
		nohup "./$(TARGET)" "$$dir" </dev/null >"$$log" 2>&1 & \
		printf 'Rodada iniciada em segundo plano (PID %s). Log: %s\n' "$$!" "$$log"
endif

# Regra para ligar os arquivos objeto e gerar o executável final
$(TARGET): $(OBJS)
	@echo "=========================================="
	@echo "Ligando o executavel final: $@"
	$(CXX) $(OBJS) -o $@ $(LDFLAGS) $(GRAPHICS_LIBS)
	@echo "Compilacao concluida com sucesso!"
	@echo "=========================================="

# Regra para compilar cada .cpp em seu respectivo .o dentro de build/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@echo "Compilando: $<"
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) $(GRAPHICS_FLAGS) -MMD -MP -c $< -o $@

# Regra de limpeza
clean:
	@echo "Limpando arquivos compilados..."
	@rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)

.PHONY: all clean background
