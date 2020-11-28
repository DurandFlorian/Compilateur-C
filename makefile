# Makefile Compilateur

FLAGS = -Wall -lfl -ly
EXEC = $(BINDIR)/compil
SRCDIR = src
BINDIR = bin
TESTDIR = tests

SRCFLE = $(wildcard $(SRCDIR)/*.lex)
SRCBIS = $(wildcard $(SRCDIR)/*.y)
FILESFLEX = $(SRCFLE:$(SRCDIR)/%.lex=$(BINDIR)/%.yy.c)
FILESBISON = $(SRCBIS:$(SRCDIR)/%.y=$(BINDIR)/%.tab.c)

	

$(EXEC): $(FILESBISON) $(FILESFLEX) $(SRCDIR)/abstract-tree.c $(SRCDIR)/symbol-table.c $(SRCDIR)/main.c
	gcc -o $(EXEC) $^ $(FLAGS)

$(BINDIR)/%.tab.c: $(SRCDIR)/%.y
	bison -v -o $@ -d $<

$(BINDIR)/%.yy.c: $(SRCDIR)/%.lex  
	flex -o $@ $^

clean:
	rm -f $(BINDIR)/*.* $(EXEC) $(TESTDIR)/log.txt
