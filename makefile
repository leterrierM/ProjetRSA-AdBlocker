Proxy: clean proxy.c
	gcc -W -Wall proxy.c -o Proxy

clean:
	rm -rf *.o
	rm -rf Proxy
	rm -rf TMP*
