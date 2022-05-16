all: kranewl

kranewl:
	cmake -S . -B build
	make -C build

.PHONY: clean
clean:
	@rm -rf ./build
