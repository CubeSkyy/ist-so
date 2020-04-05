
# Makefile, versao 03
# Sistemas Operativos, DEI/IST/ULisboa 2017-18
# Miguel Coelho 87687
# Diogo Sousa 87651

CFLAGS= -g -Wall -pedantic
CC= gcc

heatSim: main.o matrix2d.o
	$(CC) $(CFLAGS) -pthread -o heatSim main.o matrix2d.o

main.o: main.c matrix2d.h
	$(CC) $(CFLAGS) -c main.c

matrix2d.o: matrix2d.c matrix2d.h
	$(CC) $(CFLAGS) -c matrix2d.c

clean:
	rm -f *.o heatSim

zip:
	zip heatSim_ex04_solucao.zip *.c *.h Makefile

run:
	./heatSim 8 100.0 50.0 80.0 100.0 1000 4 0.01 save 3
