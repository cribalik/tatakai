LFLAGS = -lGLEW -lSDL2 -lGL -lSOIL -lfreetype
CFLAGS = -std=c++11 -Wall -g -I/usr/include/freetype2 -Iinclude

all: src/*
	@g++ $(CFLAGS) $(IFLAGS) src/flatland.cpp $(LFLAGS)

tags: src/*
	@rm tags
	@g++ -H $(CFLAGS) $(IFLAGS) src/flatland.cpp $(LFLAGS) 2>&1 | grep '^\.' | awk '{print $$2}' | xargs ctags -a -R
	@ctags -a -R src/*.cpp
.PHONY: tags
