.PHONY : tc ts clean

all: tc ts

tc:
	mkdir bin; cd tc; make; cd ..

ts:
	cd ts; make; cd ..

clean:
	cd tc; make clean; cd ..
	cd ts; make clean; cd ..
