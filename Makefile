BUILDDIR = build
BUILDLIBDIR = lib/build
DEPSDIR = includes
LIBSDIR = lib
SRCDIR = src
SRCLIBDIR = srclib

CONFIG = config
PARSER = parser
SERVER = server

FILES = file
HTTPS = http
ECHOS = echo

TARGET = $(HTTPS)

LIBS = libdaemon libconcurrent libtcp picohttpparser
_LIB = libredes2
LIB = $(patsubst %,$(LIBSDIR)/%.a, $(_LIB))


CC = gcc
AR = ar
CFLAGS = -g -I$(DEPSDIR)
LDFLAGS = -L$(LIBSDIR) -lredes2 -lpthread


.PHONY: all clean clean_libs verbose

all: before lib $(TARGET)

before:
	@[ -d $(BUILDDIR) ] || mkdir $(BUILDDIR)
	@[ -d $(BUILDLIBDIR) ] || mkdir -p $(BUILDLIBDIR)

$(ECHOS): $(BUILDDIR)/$(ECHOS).o $(BUILDDIR)/$(SERVER).o
	@echo "Enlazando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ $^ $(LDFLAGS)

$(FILES): $(BUILDDIR)/$(FILES).o $(BUILDDIR)/$(SERVER).o
	@echo "Enlazando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ $^ $(LDFLAGS)

$(HTTPS): $(BUILDDIR)/$(HTTPS).o $(BUILDDIR)/$(CONFIG).o $(BUILDDIR)/$(PARSER).o
	@echo "Enlazando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compilando $(notdir $@): $(notdir $^)"
	$(CC) -o $@ -c $^ $(CFLAGS)

lib: $(LIB)

$(LIB): $(patsubst %, $(BUILDLIBDIR)/%.o, $(LIBS))
	@echo "Archivando objetos en la librería"
	$(AR) r $@ $^

$(BUILDLIBDIR)/%.o: $(SRCLIBDIR)/%.c
	@echo "Compilando librerías"
	$(CC) -o $@ -c $^ $(CFLAGS)

clean:
	@echo "Limpiando objetos y ejecutables"
	@rm -rf ./$(BUILDDIR)/* ./$(TARGET) ./$(ECHOS) ./$(FILES) ./$(HTTPS)

clean_libs:
	@echo "Limpiando librerías"
	@rm -rf ./$(BUILDLIBDIR)/*
