all: kranewl

kranewl: tags
	meson setup build/ --buildtype debugoptimized
	ninja -C build

release:
	meson setup build/ --buildtype release
	ninja -C build

install:
	ninja -C build install

.PHONY: clean tags
clean:
	@rm -rf ./build
	@rm -f ./include/protocols/*
	@rm -f ./tags

tags:
	@git ls-files | ctags -R --exclude=.git --c++-kinds=+p --links=no --fields=+iaS --extras=+q -L-
