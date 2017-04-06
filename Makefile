run: compile
	redis-server /etc/redis.conf --loadmodule ./libredisringbuffer.so

compile: clean
	g++ -I. -Wall -std=c++11 -O3 -c ring_buffer_test.cc -o ring_buffer_test.o
	g++ ring_buffer_test.o -o ring_buffer_test
	g++ -I. -W -Wall -g -O3 -fPIC -fno-common -c redisringbuffer.cc -o redisringbuffer.o
	g++ -o libredisringbuffer.so redisringbuffer.o -shared -fPIC
 
clean:
	rm -f *.o *.so ring_buffer_test

test-ring-buffer:
	./ring_buffer_test

test-redis-ring-buffer: compile
	rm -f appendonly.aof dump.rdb
	redis-server ./redis.conf --loadmodule ./libredisringbuffer.so &
	sleep 1
	redis-cli FLUSHDB
	redis-cli RingBufferCreate AAA 8
	[ `redis-cli RingBufferIsEmpty AAA` == '1' ] || exit 1
	[ `redis-cli RingBufferIsFull AAA` == '0' ] || exit 1
	[ `redis-cli RingBufferSize AAA` == '8' ] || exit 1
	[ `redis-cli RingBufferLength AAA` == '0' ] || exit 1
	[ !`redis-cli RingBufferFront AAA` ] || exit 1
	[ !`redis-cli RingBufferBack AAA` ] || exit 1
	redis-cli RingBufferWrite AAA 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24
	[ `redis-cli RingBufferIsEmpty AAA` == '0' ] || exit 1
	[ `redis-cli RingBufferIsFull AAA` == '1' ] || exit 1
	[ `redis-cli RingBufferSize AAA` == '8' ] || exit 1
	[ `redis-cli RingBufferLength AAA` == '8' ] || exit 1
	[ `redis-cli RingBufferFront AAA` == '17' ] || exit 1
	[ `redis-cli RingBufferBack AAA` == '24' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '17' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '18' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '19' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '20' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '21' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '22' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '23' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '24' ] || exit 1
	[ !`redis-cli RingBufferRead AAA` ] || exit 1
	redis-cli RingBufferWrite AAA 1 2 3 4
	[ `redis-cli RingBufferIsEmpty AAA` == '0' ] || exit 1
	[ `redis-cli RingBufferIsFull AAA` == '0' ] || exit 1
	[ `redis-cli RingBufferSize AAA` == '8' ] || exit 1
	[ `redis-cli RingBufferLength AAA` == '4' ] || exit 1
	[ `redis-cli RingBufferFront AAA` == '1' ] || exit 1
	[ `redis-cli RingBufferBack AAA` == '4' ] || exit 1
	[ `redis-cli RingBufferReadAll AAA | head -n1` == '1' ] || exit 1
	[ `redis-cli RingBufferReadAll AAA | tail -n1` == '4' ] || exit 1
	redis-cli RingBufferClear AAA
	[ `redis-cli RingBufferIsEmpty AAA` == '1' ] || exit 1
	[ `redis-cli RingBufferIsFull AAA` == '0' ] || exit 1
	[ `redis-cli RingBufferSize AAA` == '8' ] || exit 1
	[ `redis-cli RingBufferLength AAA` == '0' ] || exit 1
	[ !`redis-cli RingBufferFront AAA` ] || exit 1
	[ !`redis-cli RingBufferBack AAA` ] || exit 1
	redis-cli RingBufferWrite AAA 1
	redis-cli RingBufferWrite AAA 2
	redis-cli RingBufferWrite AAA 3
	redis-cli RingBufferWrite AAA 4
	redis-cli RingBufferWrite AAA 5
	redis-cli RingBufferWrite AAA 6
	redis-cli RingBufferWrite AAA 7
	redis-cli RingBufferWrite AAA 8
	redis-cli RingBufferWrite AAA 9
	redis-cli RingBufferWrite AAA 10
	redis-cli RingBufferWrite AAA 11
	redis-cli RingBufferWrite AAA 12
	[ `redis-cli RingBufferIsEmpty AAA` == '0' ] || exit 1
	[ `redis-cli RingBufferIsFull AAA` == '1' ] || exit 1
	[ `redis-cli RingBufferSize AAA` == '8' ] || exit 1
	[ `redis-cli RingBufferLength AAA` == '8' ] || exit 1
	[ `redis-cli RingBufferFront AAA` == '5' ] || exit 1
	[ `redis-cli RingBufferBack AAA` == '12' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '5' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '6' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '7' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '8' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '9' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '10' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '11' ] || exit 1
	[ `redis-cli RingBufferRead AAA` == '12' ] || exit 1
	[ `redis-cli RingBufferIsEmpty AAA` == '1' ] || exit 1
	[ `redis-cli RingBufferIsFull AAA` == '0' ] || exit 1
	[ `redis-cli RingBufferSize AAA` == '8' ] || exit 1
	[ `redis-cli RingBufferLength AAA` == '0' ] || exit 1
	[ !`redis-cli RingBufferFront AAA` ] || exit 1
	[ !`redis-cli RingBufferBack AAA` ] || exit 1
	redis-cli SAVE
	kill -9 `pidof redis-server`