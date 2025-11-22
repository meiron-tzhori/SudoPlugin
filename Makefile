all: my_plugin.so

%.o: %.cpp
	g++ -c -fPIC $< -o $@

my_plugin.so: plugin.o children.o
	g++ plugin.o children.o -shared -o my_plugin.so

clean:
	rm -f *.so *.o

