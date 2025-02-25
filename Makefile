# default will make the game code as a shared object 
# STATIC_WHOLE_COMPILE off will enable hot reloading 
default: 
	clang++ \
	-o lib/libgame.so \
	-shared -fPIC -rdynamic \
	-Wall \
	lib/game.cpp \
	-Wl,-Bstatic -lSDL3 \
	-Wl,-Bdynamic 

# platform will compile the platform and include 
# the game code as a shared lib for hot reloading
platform: 
	clang++ \
	-o main \
	-pie \
	-Wall \
	linux_platform.cpp \
	-Wl,-Bstatic -lSDL3 \
	-Wl,-Bdynamic -lm \
	-L./lib/ 

# STATIC_WHOLE_COMPILE will disable hot reloading 
# and include the cpp files in the platform file
# to statically build the whole project 
wholestatic: 
	clang++ \
	-o main \
	-DSTATIC_WHOLE_COMPILE=1 \
	-Wall \
	linux_platform.cpp \
	-Wl,-Bstatic -lSDL3 \
	-Wl,-Bdynamic 