CXX = g++
CXXFLAGS = -std=c++17
TARGET = l1_cache
SRC = l1_cache.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)