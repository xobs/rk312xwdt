all:
	$(CC) rk312x-wdt-userspace.c syscalls.S -o rk312x-wdt-userspace -marm -nostdlib -Os
