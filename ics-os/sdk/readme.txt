To build C applications within ICS-OS environment, tcc should be invoked in
the manner below (hello.c is the source file).

%/icsos/apps/tcc.exe -ohello.exe hello.c -B/icsos/tcc1 /icsos/tcc1/tccsdk.c /icsos/tcc1/crt1.c

