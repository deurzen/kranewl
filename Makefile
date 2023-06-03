all: kranewl

kranewl: tags
	cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build
	make -C build -j$(nproc)

release:
	cmake -DCMAKE_BUILD_TYPE=Release -S . -B build
	make -C build -j$(nproc)

install:
	make -C build install

test: kranewl
	ctest --verbose --test-dir build

.PHONY: clean tags
clean:
	@rm -rf ./tags
	@rm -rf ./build
	@rm -f ./include/protocols/*

tags:
	@git ls-files | ctags -R --exclude=.git --c++-kinds=+p --links=no --fields=+iaS --extras=+q -L-
