# Structurally sane Makefile for a C project.
# Author: Diego Sáinz de Medrano

# directories

BUILDDIR = build
BUILDLIBDIR = lib/build
DEPSDIR = includes
LIBSDIR = lib
SRCDIR = src
SRCLIBDIR = srclib

# auxiliary modules

CONFIG = config
SCRIPT = cgi
DUPER = remap-pipe-fds
FINDER = finder
HEADERS = headers
HTTP = http

# test main programs

TESTCGI = testcgi
TESTCONF = testconfig
TESTSCRIPTS = testscripts

# main programs

ECHOS = echo
FILES = file
SERVER = server

# core target of the makefile

TARGET = $(SERVER)

# cleaning targets

CLEAN = ./$(TESTCGI) ./$(TESTCONF) ./$(TESTSCRIPTS)
CLEAN += ./$(ECHOS) ./$(FILES) ./$(SERVER)

# library list of modules
LIBS = libdaemon libconcurrent libtcp libserver picohttpparser
# library name
_LIB = redes2
LIB = $(patsubst %,$(LIBSDIR)/lib%.a, $(_LIB))

# tooling

CC = gcc
AR = ar

# flags

#        v debug v includes
CFLAGS = -g      -I$(DEPSDIR)
LDFLAGS = -L$(LIBSDIR) -l$(_LIB) -lpthread
#         ^ libs       ^ our lib ^ threads


.PHONY: all clean clean_libs verbose

all: before lib $(TARGET)

# phony target to prepare the directories

before:
	@[ -d $(BUILDDIR) ] || mkdir $(BUILDDIR)
	@[ -d $(BUILDLIBDIR) ] || mkdir -p $(BUILDLIBDIR)

#
## linking targets
#

# echo: echo(main) server
$(ECHOS): $(BUILDDIR)/$(ECHOS).o
	@echo "Enlazando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ $^ $(LDFLAGS)

# file: file(main) server
$(FILES): $(BUILDDIR)/$(FILES).o
	@echo "Enlazando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ $^ $(LDFLAGS)

# http: http(main) parser
$(SERVER): $(BUILDDIR)/$(SERVER).o $(BUILDDIR)/$(HTTP).o $(BUILDDIR)/$(CONFIG).o $(BUILDDIR)/$(FINDER).o $(BUILDDIR)/$(HEADERS).o $(BUILDDIR)/$(DUPER).o $(BUILDDIR)/$(SCRIPT).o
	@echo "Enlazando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ $^ $(LDFLAGS)

#
## test programs
#

$(TESTCGI): $(BUILDDIR)/$(TESTCGI).o $(BUILDDIR)/$(DUPER).o $(BUILDDIR)/$(SCRIPT).o
	@echo "Enlazando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ $^ $(LDFLAGS)

$(TESTCONF): $(BUILDDIR)/$(TESTCONF).o $(BUILDDIR)/$(CONFIG).o
	@echo "Enlazando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ $^ $(LDFLAGS)


$(TESTSCRIPTS): $(BUILDDIR)/$(TESTSCRIPTS).o $(BUILDDIR)/$(DUPER).o $(BUILDDIR)/$(SCRIPT).o $(BUILDDIR)/$(FINDER).o
	@echo "Enlazando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ $^ $(LDFLAGS)

#
## compile targets
#

# every requested src/%.c file gets compiled into build/%.o
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compilando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ -c $^ $(CFLAGS)

#
## library targets
#

# lib: name for `make lib`
lib: $(LIB)

# compile all targets in $(LIBS) and archive into $(LIB)
$(LIB): $(patsubst %, $(BUILDLIBDIR)/%.o, $(LIBS))
	@echo "Archivando objetos en la librería"
	$(AR) r $@ $^

# every requested srclib/%.c file gets compiled into lib/build/%.o
$(BUILDLIBDIR)/%.o: $(SRCLIBDIR)/%.c
	@echo "Compilando librerías"
	$(CC) -o $@ -c $^ $(CFLAGS)

#
## clean targets
#

clean:
	@echo "Limpiando objetos y ejecutables"
	@rm -f ./$(BUILDDIR)/* ./$(CLEAN)

clean_libs:
	@echo "Limpiando librerías"
	@rm -f ./$(BUILDLIBDIR)/*
