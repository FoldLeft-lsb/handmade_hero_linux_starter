COMPILER= clang++
COMMON_FLAGS= -Wall -Wl,-Bstatic -lSDL3 -Wl,-Bdynamic
DEV_MEM_OPTIONS= -DIN_DEVELOPMENT=1

# default will make the game code as a shared object 
# STATIC_WHOLE_COMPILE off will enable hot reloading 
default: 
	$(COMPILER) \
	-o lib/libgame.so \
	-shared -fPIC -rdynamic \
	lib/game.cpp \
	$(COMMON_FLAGS)

# platform will compile the platform and include 
# the game code as a shared lib for hot reloading
platform: 
	$(COMPILER) \
	-o main \
	-pie \
	$(DEV_MEM_OPTIONS) \
	linux_platform.cpp \
	$(COMMON_FLAGS) \
	-lm -L./lib/ 

# STATIC_WHOLE_COMPILE will disable hot reloading 
# and include the cpp files in the platform file
# to statically build the whole project 
wholestatic: 
	$(COMPILER) \
	-o main \
	-DSTATIC_WHOLE_COMPILE=1 \
	linux_platform.cpp \
	$(COMMON_FLAGS)

clean:
	rm main lib/libgame.so