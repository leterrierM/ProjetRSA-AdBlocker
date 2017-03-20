Proxy: clean main.c
	gcc -W -Wall main.c -o Proxy

clean:
	rm -rf *.o
	rm -rf Proxy
