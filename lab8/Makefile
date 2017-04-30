main:
	mpicc Mainprg.c -o prog.out
run:
	mpirun -n ${p}  ./prog.out ${n} ${m} ${sp} "input.txt"
clean:
	rm *.out
