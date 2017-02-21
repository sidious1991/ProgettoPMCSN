# Makefile

OBJDIR = obj
INCDIR = include
SRCDIR = src

PROGNAME = simulation

CC = gcc
FLAGS = -Wall -Wextra -O2 -I $(INCDIR)

_SRC = $(filter-out $(PROGNAME).c, $(shell ls -a $(SRCDIR) | grep .c))
SRC = $(patsubst %,$(SRCDIR)/%,$(_SRC))
_DEPS = $(shell ls -a $(INCDIR) | grep .h)
DEPS = $(patsubst %,$(INCDIR)/%,$(_DEPS))
_OBJ =  $(_SRC:%.c=%.o)
OBJ = $(patsubst %,$(OBJDIR)/%,$(_OBJ))

$(info $$SRC is ${SRC})
$(info $$DEPS is ${DEPS})
$(info $$OBJ is ${OBJ})

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(FLAGS)

all: clean simulation run

simulation: $(OBJ)
	$(CC) $(FLAGS) -o $@ $^ $(SRCDIR)/$@.c -lm

run:
	./$(PROGNAME)

.PHONY: clean

clean:
	rm -f $(OBJDIR)/*.o $(PROGNAME)
