build: libscheduler.so

libscheduler.so: so_scheduler.o priqueue.o
	gcc -shared so_scheduler.o priqueue.o -o ../libscheduler.so

so_scheduler.o: so_scheduler.c
	gcc -c -fPIC so_scheduler.c -o so_scheduler.o

priqueue.o: priqueue.c
	gcc -c -fPIC priqueue.c -o priqueue.o

clean:
	rm *.o libscheduler.so