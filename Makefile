default: 
	clang++ \
	-o main \
	-Wall \
	linux_platform.cpp lib/*.cpp \
	-Wl,-Bstatic -lSDL3 \
	-Wl,-Bdynamic
	

release: 
	clang++ \
	-o main_release \
	-O1 \
	linux_platform.cpp lib/*.cpp \
	-Wl,-Bstatic -lSDL3 \
	-Wl,-Bdynamic