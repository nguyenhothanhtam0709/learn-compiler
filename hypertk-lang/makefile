CXX = clang++
LLVM_CONFIG = llvm-config
CXXFLAGS = -Wall -std=c++20 `$(LLVM_CONFIG) --cxxflags`
LDFLAGS = `$(LLVM_CONFIG) --ldflags --system-libs --libs core irreader support`

TARGET = hypertk
SRC    = $(wildcard src/*.cpp)
OBJ    = $(SRC:src/%.cpp=build/%.o)

dev: clean run

run: $(TARGET)
	./$(TARGET)

# default 
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

# compile .cpp in src/ to .o in build/
build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)