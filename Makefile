
all:
	g++ -std=c++11 -fno-exceptions -fno-rtti -O2 -g -c -o libcam.o libcam.cpp
	g++ -std=c++11 -g -c -o test.o test.cpp `pkg-config opencv --cflags --libs`
	g++ -std=c++11 -g -o test test.o libcam.o `pkg-config opencv --cflags --libs`

clean:
	rm -f *.o *.png test
