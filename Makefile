include config.mk
.PHONY: tags bin obj clean install uninstall

CXXFLAGS += ${CXXFLAGS_EXTRA}
LDFLAGS += ${LDFLAGS_EXTRA}

all: kranewl

server: bin obj ${SERVER_LINK_FILES}

kranewl: bin obj ${COMPOSITOR_LINK_FILES}
	${CC} ${CXXFLAGS} ${COMPOSITOR_LINK_FILES} ${LDFLAGS} -o $(BINDIR)/$(PROJECT)

-include $(DEPS)

obj/%.o: src/%.cc
	${CC} ${CXXFLAGS} -MMD -c $< -o $@

obj/server/%.o: src/server/%.cc
	${CC} ${CXXFLAGS} -MMD -c $< -o $@

xdg-shell-protocol.h:
	wayland-scanner server-header ${WAYLAND_PROTOCOLS}/stable/xdg-shell/xdg-shell.xml ${.TARGET}

bin:
	@[ -d bin ] || mkdir -p bin

obj:
	@[ -d obj ] || mkdir -p obj/server

tags:
	@git ls-files | ctags -R --exclude=.git --c++-kinds=+p --links=no --fields=+iaS --extras=+q -L-

clean:
	@rm -rf ./bin ./obj
