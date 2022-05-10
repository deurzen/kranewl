PROJECT = kranewl
EXT_DEPS = wlroots pangocairo cairo pixman-1 xkbcommon wayland-server libinput libucl libprocps spdlog

SRCDIR = src
INCDIR = include
OBJDIR = obj

BINDIR = bin
INSTALLDIR = /usr/local/bin

WAYLAND_PROTOCOLS = `pkg-config --variable pkgdatadir wayland-protocols`
PROTOCOL_HEADERS = xdg-shell-protocol.h

COMPOSITOR_SRC_FILES := $(wildcard src/*.cc)
COMPOSITOR_OBJ_FILES := $(patsubst src/%.cc,obj/%.o,${COMPOSITOR_SRC_FILES})

SERVER_SRC_FILES := $(wildcard src/server/*.cc)
SERVER_OBJ_FILES := $(patsubst src/server/%.cc,obj/server/%.o,${SERVER_SRC_FILES})

SERVER_LINK_FILES := ${SERVER_OBJ_FILES}
COMPOSITOR_LINK_FILES := ${SERVER_OBJ_FILES} ${COMPOSITOR_OBJ_FILES}

H_FILES := $(shell find $(INCDIR) -name '*.hh')
SRC_FILES := $(shell find $(SRCDIR) -name '*.cc')
OBJ_FILES := ${SERVER_OBJ_FILES} ${COMPOSITOR_OBJ_FILES}
DEPS = $(OBJ_FILES:%.o=%.d)

SANFLAGS = -fsanitize=undefined -fsanitize=address -fsanitize-address-use-after-scope
CXXFLAGS = -Wall -I. -Iinclude -std=c++20 `pkg-config --cflags ${EXT_DEPS}`
LDFLAGS = `pkg-config --libs ${EXT_DEPS}` -pthread

DEBUG_CXXFLAGS = -Wall -Wpedantic -Wextra -Wold-style-cast -g -DDEBUG ${SANFLAGS}
DEBUG_LDFLAGS = ${SANFLAGS}
RELEASE_CXXFLAGS = -march=native -mtune=native -O3 -flto
RELEASE_LDFLAGS = -flto

CC = g++
