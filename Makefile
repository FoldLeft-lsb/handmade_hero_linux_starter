default: 
	clang++ \
	-o main \
	-lSDL3 \
	-Wall \
	linux_platform.cpp lib/*.cpp

release: 
	clang++ \
	-o main_release \
	-lSDL3 \
	-O1 \
	linux_platform.cpp lib/*.cpp