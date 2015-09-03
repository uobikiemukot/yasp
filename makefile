CC ?= gcc

CFLAGS = -Wall -Wextra -std=c99 -pedantic \
	-D_FORTIFY_SOURCE=2 -fstack-protector --param=ssp-buffer-size=4 \
	-O3 -pipe -s

DEBUG_CFLAGS = -Wall -Wextra -std=c99 -pedantic -msse4.2 \
	-Og -g -rdynamic -pg \
	-Wformat=2 -D_FORTIFY_SOURCE=2 --param=ssp-buffer-size=4 \
	-fstack-protector-all -Wstack-protector \
	-fsanitize=address -ftrapv \
	-fstrict-overflow -Wstrict-overflow=5 \
	-fstrict-aliasing -Wstrict-aliasing

LDFLAGS =

NAME = yasp

SRC       = $(NAME).c
DST       = $(NAME)
DST_DEBUG = $(NAME)_debug

HDR = *.h
OBJ =

all: $(DST)

$(DST): $(SRC) $(OBJ) $(HDR)
	$(CC) -o $@ $< $(OBJ) $(CFLAGS) $(LDFLAGS)

debug: $(SRC) $(OBJ) $(HDR)
	$(CC) -o $(DST_DEBUG) $< $(OBJ) $(DEBUG_CFLAGS) $(LDFLAGS)

clean:
	rm -f $(DST) $(DST_DEBUG) $(OBJ)
