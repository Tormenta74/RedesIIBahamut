BUILDDIR = build
BUILDLIBDIR = lib/build
DEPSDIR = includes
LIBSDIR = lib
SRCDIR = src
SRCLIBDIR = srclib

SERVER = server

HTTP = http
ECHO = echo

TARGET = $(ECHO)

LIBS = libdaemon libconcurrent libtcp
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
	@[ -d $(BUILDLIBDIR) ] || mkdir $(BUILDLIBDIR)

$(ECHO): $(BUILDDIR)/$(ECHO).o $(BUILDDIR)/$(SERVER).o
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
	@echo "Limpiando build/ y $(TARGET)"
	@rm -rf ./$(BUILDDIR)/* ./$(TARGET)

clean_libs:
	@echo "Limpiando librerías"
	@rm -rf ./$(BUILDLIBDIR)/*
