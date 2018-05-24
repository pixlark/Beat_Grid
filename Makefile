make:
	g++ -std=c++11 \
		main.cc \
		$(shell sdl2-config --cflags --libs) \
		-lSDL2_ttf -lSDL2_mixer \
		-Wno-write-strings
