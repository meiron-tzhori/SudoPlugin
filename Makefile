all: my_plugin.so

plugin.o: plugin.cpp
	gcc -c -fPIC plugin.cpp -o plugin.o

my_plugin.so: plugin.o
	gcc plugin.o -shared -o my_plugin.so

clean:
	rm *.so *.o
