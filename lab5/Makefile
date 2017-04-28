main:
	mpicc Mainprg.c -o prog.out
run:
	mpirun -n ${np} ./prog.out ${n1} ${n2} ${r}
clean:
	rm *.out
