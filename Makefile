all: kranewl

kranewl:
	cmake -S . -B build
	make -C build

test: kranewl
	ctest --verbose --test-dir build

.PHONY: clean
clean:
	@rm -rf ./tags
	@rm -rf ./build
	@rm -f ./include/protocols/*
