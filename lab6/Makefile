main:
	mpicc Mainprg.c -o prog.out
run:
	mpirun -n ${np}  ./prog.out 10 ${r}
clean:
	rm *.out
