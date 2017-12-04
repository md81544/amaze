FILES=$(shell find . -maxdepth 1 -name \*.cpp -o -name \*.h)

tests: $(FILES)
	echo $(FILES)
	g++ -std=c++14 -Wall -Wextra -Werror -Wpedantic -o tests *cpp
