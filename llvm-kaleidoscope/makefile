CXX = clang++
LLVM_CONFIG = llvm-config # this can be version based, for example: llvm-config-14
CXXFLAGS = -std=c++17 `$(LLVM_CONFIG) --cxxflags`
LDFLAGS = `$(LLVM_CONFIG) --ldflags --system-libs --libs core irreader support`

TARGET = llvm_kaleidoscope
SRC    = $(wildcard *.cpp)
OBJ    = $(SRC:.cpp=.o)

dev: clean all

run: $(TARGET)
	./$(TARGET)

# default 
all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)