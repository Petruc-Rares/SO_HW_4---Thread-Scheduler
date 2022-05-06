build: so_scheduler.o
	gcc -shared so_scheduler.o -o libscheduler.so

so_scheduler.o: so_scheduler.c
	gcc -c -fPIC so_scheduler.c -o so_scheduler.o

clean:
	rm *.o