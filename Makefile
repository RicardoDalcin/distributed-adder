# Compilador
CXX := g++

# Flags de compilador
CXXFLAGS := -Wall -Wextra -std=c++17

DEBUG ?= 0
ifeq ($(DEBUG), 1)
    CXXFLAGS += -DDEBUG
endif

# Diretórios e arquivos fonte
SRV_SRC := src/server/main.cpp
CLI_SRC := src/client/main.cpp
DIST_DIR := dist

# Executáveis
SRV_TARGET := $(DIST_DIR)/server
CLI_TARGET := $(DIST_DIR)/client

# Target padrão
all: $(SRV_TARGET) $(CLI_TARGET)

# Regras de build
$(SRV_TARGET): $(SRV_SRC)
	mkdir -p $(DIST_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(CLI_TARGET): $(CLI_SRC)
	mkdir -p $(DIST_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Limpa a pasta dist
clean:
	rm -rf $(DIST_DIR)

.PHONY: all clean
