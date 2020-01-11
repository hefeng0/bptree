

all:
	cc -g *.c -I ./ -lm -o bptree 

clean:
	rm bptree
