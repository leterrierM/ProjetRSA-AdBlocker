Proxy: clean proxy.c
	gcc -W -Wall proxy.c -o Proxy

clean:
	@echo removing *.adlog
	@rm -rf *.adlog
	@echo removing *.o
	@rm -rf *.o
	@echo old executable
	@rm -rf Proxy
	@echo removing tmpFILEs
	@rm -rf TMP*
	@echo reset logs
	@rm -rf logs
	@touch logs
