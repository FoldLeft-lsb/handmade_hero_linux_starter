default: 
	clang++ \
	-o lib/libgame.so \
	-fsanitize=address \
	-shared -fPIC -rdynamic \
	-Wall \
	lib/game.cpp \
	-Wl,-Bstatic -lSDL3 \
	-Wl,-Bdynamic 

platform: 
	clang++ \
	-o main \
	-fsanitize=address -pie \
	-Wall \
	linux_platform.cpp \
	-Wl,-Bstatic -lSDL3 \
	-Wl,-Bdynamic -lm \
	-L./lib/ 
