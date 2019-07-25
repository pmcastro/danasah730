CC = arm-none-linux-gnueabi-gcc
#CFLAGS = -mtune=arm7 -DUNDER_ARM -pipe -ffunction-sections -fomit-frame-pointer -flto -w -s -Ofast -Wall -Wextra -pedantic -Werror -Wfatal-errors
CFLAGS = -mtune=arm7 -DUNDER_ARM -Wall -W -O3 -fno-exceptions -g0 -s -pthread
LIBS = -static -ldl -lgcov -lm -lpthread ./gtbprobe/libgtb.a

.SUFFIXES: .c .o
 
OBJS = aleatorio.o  atacado.o  \
     busqueda.o  danasah.o  egbb.o  entradas.o  \
     evalua.o  fen.o  movimientos.o  hash.o  \
     configura.o  libro.o  salidas.o  see.o  test.o \
     variables.o  xboard.o  uci.o  consola.o  egtb.o
	 
all: danasah

.c.o: 
	$(CC) $(CFLAGS) -c $< -o $@

danasah: $(OBJS) 
	$(CC) -o danasah $(OBJS) $(CFLAGS) $(LIBS)

clean:
	del *.o
	del danasah