main:
	mpicc Mainprg.c -o prog.out
run:
	mpirun -n ${p}  ./prog.out ${n} ${m} ${p} "input.txt"
clean:
	rm *.out
