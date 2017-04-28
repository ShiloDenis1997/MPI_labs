main:
	mpicc Mainprg.c -o prog.out
run:
	mpirun -n ${p}  ./prog.out ${n} ${p} "input.txt"
clean:
	rm *.out
