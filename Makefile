.PHONY: debug, clean

CPP_FILES=$(shell find . -maxdepth 1 -name \*.cpp)
H_FILES=$(shell find . -maxdepth 1 -name \*.h)

amaze: $(CPP_FILES) $(H_FILES)
	g++ -c -O3 -std=c++14 -Wall -Wextra -Werror -Wpedantic $(CPP_FILES)
	g++ *.o -o amaze \
		-lboost_system -lboost_filesystem \
		-lsfml-graphics -lsfml-window -lsfml-system

debug: $(CPP_FILES) $(H_FILES)
	g++ -g -O0 -std=c++14 -Wall -Wextra -Werror -Wpedantic -o amaze \
		$(CPP_FILES) \
		-lboost_system -lboost_filesystem

clean: $(CPP_FILES) $(H_FILES)
	rm -f amaze core *.o

