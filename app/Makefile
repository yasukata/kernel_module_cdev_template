PROGS	=	kmod-test
CLEANFILES = $(PROGS) *.o

NO_MAN=
CFLAGS = -O2 -pipe
CFLAGS += -Werror -Wall -Wunused-function
CFLAGS += -Wextra

all: $(PROGS)

clean:
	-@rm -rf $(CLEANFILES)
