default: 
	clang++ -std=c++20 \
	-o main \
	-Wall \
	linux_platform.cpp \
	-Wl,-Bstatic -lSDL3 \
	-Wl,-Bdynamic
	

release: 
	clang++ -std=c++20 \
	-o main_release \
	-O1 \
	linux_platform.cpp \
	-Wl,-Bstatic -lSDL3 \
	-Wl,-Bdynamic