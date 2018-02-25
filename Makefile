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
SERVER = server
DUPER = remap-pipe-fds

# test main programs

TESTCGI = testcgi

# main programs

FILES = file
HTTPS = http
ECHOS = echo

# core target of the makefile

TARGET = $(TESTCGI)

# library list of modules
LIBS = libdaemon libconcurrent libtcp picohttpparser
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
$(ECHOS): $(BUILDDIR)/$(ECHOS).o $(BUILDDIR)/$(SERVER).o
	@echo "Enlazando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ $^ $(LDFLAGS)

# file: file(main) server
$(FILES): $(BUILDDIR)/$(FILES).o $(BUILDDIR)/$(SERVER).o
	@echo "Enlazando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ $^ $(LDFLAGS)

# http: http(main) config server
$(HTTPS): $(BUILDDIR)/$(HTTPS).o $(BUILDDIR)/$(CONFIG).o $(BUILDDIR)/$(SERVER).o
	@echo "Enlazando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ $^ $(LDFLAGS)

#
## test programs
#

$(TESTCGI): $(BUILDDIR)/$(TESTCGI).o $(BUILDDIR)/$(DUPER).o $(BUILDDIR)/$(SCRIPT).o
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
	@rm -rf ./$(BUILDDIR)/* ./$(TARGET) ./$(ECHOS) ./$(FILES) ./$(HTTPS) ./$(TESTCGI)

clean_libs:
	@echo "Limpiando librerías"
	@rm -rf ./$(BUILDLIBDIR)/*
