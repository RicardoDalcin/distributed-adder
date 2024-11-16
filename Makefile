# Compilador
CXX := g++

# Flags de compilador
CXXFLAGS := -Wall -Wextra -std=c++17

# Arquivos fonte e executável
SRCS := src/main.cpp
TARGET := dist/main

# Target padrão
all: $(TARGET)

# Regras de build
dist/main: $(SRCS)
	mkdir -p dist
	$(CXX) $(CXXFLAGS) -o $@ $^

# Limpa a pasta dist
clean:
	rm -rf dist

.PHONY: all clean
