CC := cc
CFLAGS := -march=native -Wall -Wextra -Wshadow -Wpedantic -Werror -O2
CFILE := ocsh.c
OFILE := ocsh 

all:
	$(CC) $(CFLAGS) $(CFILE) -o $(OFILE)
clean:
	rm -rf $(OFILE)
run:
	DEBUG=1 ./$(OFILE)
