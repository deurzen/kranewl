all: kranewl

kranewl:
	cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build
	make -C build

release:
	cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
	make -C build

install:
	make -C build install

test: kranewl
	ctest --verbose --test-dir build

.PHONY: clean
clean:
	@rm -rf ./tags
	@rm -rf ./build
	@rm -f ./include/protocols/*
