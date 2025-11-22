CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -Wno-unused-parameter -fPIC
LDFLAGS = -shared

all: my_plugin.so

%.o: %.cpp *.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

my_plugin.so: plugin.o children.o
	$(CXX) $(LDFLAGS) plugin.o children.o -o my_plugin.so

clean:
	rm -f *.so *.o

