CFLAGS = -O2
LDFLAGS = -lraylib -ldl -lpthread -lm

a: blockpuzzle minesweeper

blockpuzzle: blockpuzzle.c
	$(CC) blockpuzzle.c -o blockpuzzle $(LDFLAGS) $(CFLAGS)
minesweeper: minesweeper.c
	$(CC) minesweeper.c -o minesweeper $(LDFLAGS) $(CFLAGS)
