CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./include
LDFLAGS = -L$(BUILDDIR) -lmulticurvas -lm


SRCDIR = src
BUILDDIR = build
TESTDIR = test
LIBNAME = libmulticurvas.a
LIBPATH = $(BUILDDIR)/$(LIBNAME)


SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SOURCES))
TEST_SOURCES = $(wildcard $(TESTDIR)/*.c)
TEST_BINS = $(patsubst $(TESTDIR)/%.c, $(BUILDDIR)/%.test, $(TEST_SOURCES))


all: library tests run-tests

library: $(LIBPATH)

$(LIBPATH): $(OBJECTS) | $(BUILDDIR)
	ar rcs $@ $^


$(BUILDDIR)/%.test: $(TESTDIR)/%.c $(LIBPATH) | $(BUILDDIR)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

# Target para compilar todos os testes
tests: $(TEST_BINS)

# Target para rodar todos os testes
run-tests: tests
	@for t in $(TEST_BINS); do echo "Executando $$t:"; $$t; done

help:
	@echo "Targets disponíveis:"
	@echo "  all         - Compila a biblioteca, os testes, executa todos os binários de teste e o benchmark."
	@echo "  library     - Compila apenas a biblioteca estática (.a)."
	@echo "  tests       - Compila um executável para cada arquivo .c em test/."
	@echo "  run-tests   - Executa todos os binários de teste."
	@echo "  clean       - Remove arquivos gerados e diretório build."
	@echo "  help        - Mostra esta mensagem de ajuda."

.PHONY: all clean help tests run-tests library
