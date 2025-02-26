COMPILER= clang++
COMMON_FLAGS= -Wall 
D_LINK_FLAGS= -Wl,-Bstatic -lSDL3 -Wl,-Bdynamic
DEV_OPTION_FLAGS= -DIN_DEVELOPMENT=1

# default will make the game code as a shared object for hot reloading 
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
	$(DEV_OPTION_FLAGS) \
	linux_platform.cpp \
	$(COMMON_FLAGS) \
	$(D_LINK_FLAGS) \
	-lm -L./lib/ 

# STATIC_WHOLE_COMPILE will disable hot reloading 
# and include the cpp files in the platform file
# to statically build the whole project 
static: 
	$(COMPILER) \
	-o main \
	-DSTATIC_WHOLE_COMPILE=1 \
	linux_platform.cpp \
	$(COMMON_FLAGS) \
	$(D_LINK_FLAGS)

clean:
	rm -f main lib/libgame.so tmp/*.dat