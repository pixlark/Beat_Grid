nix:
	g++ -std=c++11 \
		main.cc \
		$(shell sdl2-config --cflags --libs) \
		-lSDL2_ttf -lSDL2_mixer \
		-Wno-write-strings

win:
	g++ -std=c++11 \
		main.cc \
		-I"G:\.minlib\SDL2-2.0.7\x86_64-w64-mingw32\include" \
		-I"G:\.minlib\SDL2_ttf-2.0.14\x86_64-w64-mingw32\include" \
		-I"G:\.minlib\SDL2_mixer-2.0.2\x86_64-w64-mingw32\include" \
		-L"G:\.minlib\SDL2-2.0.7\x86_64-w64-mingw32\lib" \
		-L"G:\.minlib\SDL2_ttf-2.0.14\x86_64-w64-mingw32\lib" \
		-L"G:\.minlib\SDL2_mixer-2.0.2\x86_64-w64-mingw32\lib" \
		-lSDL2 -lSDL2main -lSDL2_ttf -lSDL2_mixer \
		-Wno-write-strings	
