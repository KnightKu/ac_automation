CC=gcc 
all: ac_dict.so test_ac perf_ac
.PHONY : all

ac_dict.so : ac.c
	$(CC) $^ -o $@ -shared -fPIC

test_ac: ac.c ac_test.c
	$(CC) -g $^ -o $@
perf_ac: ac.c ac_performance.c
	$(CC) -g $^ -o $@
clean: 
	rm -rf *.so test_ac
