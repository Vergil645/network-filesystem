obj-m += networkfs.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(shell pwd) clean

tests: all
	python3 -m tests BasicTestCases -f

bonus-name: all
	python3 -m tests NameTestCases -f

bonus-wr: all
	python3 -m tests WRTestCases -f

bonus-link: all
	python3 -m tests LinkTestCases -f
