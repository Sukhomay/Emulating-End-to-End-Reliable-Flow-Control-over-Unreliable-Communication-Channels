CC = gcc

libmsocket.a: msocket.o
	ar rcs libmsocket.a msocket.o

msocket.o: msocket.c
	$(CC) -c msocket.c -o msocket.o

initmsocket: initmsocket.c libmsocket.a
	$(CC) initmsocket.c -L. -pthread -o initmsocket
	./initmsocket

user1: user1.c libmsocket.a
	$(CC) user1.c -L. -lmsocket -pthread -o user1
	./user1

user2: user2.c libmsocket.a
	$(CC) user2.c -L. -lmsocket -pthread -o user2
	./user2

clean:
	rm -f *.o *.a user1 user2

# one:
# 	gcc initmsocket.c -o initmsocket
# 	./initmsocket
# user1: user1_ex
# 	./user1
# user1_ex:
# 	gcc user1.c msocket.c -o user1
# user2: user2_ex
# 	./user2
# user2_ex:
# 	gcc user2.c msocket.c -o user2
# user3: user3_ex
# 	./user3
# user3_ex:
# 	gcc user3.c msocket.c -o user3
# user4: user4_ex
# 	./user4
# user4_ex:
# 	gcc user4.c msocket.c -o user4


