all: send recv

link_emulator/lib.o:
	$(MAKE) -C link_emulator

send: send.o link_emulator/lib.o
	g++ -Werror -g send.o link_emulator/lib.o -o send

recv: recv.o link_emulator/lib.o
	g++ -Werror -g recv.o link_emulator/lib.o -o recv

.cpp.o:
	g++ -Werror -Wall -g -c $?

clean:
	$(MAKE) -C link_emulator clean
	-rm -f *.o send recv
