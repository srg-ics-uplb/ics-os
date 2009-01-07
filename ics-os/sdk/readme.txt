To build C applications within ICS-OS environment, tcc should be invoked in
the manner below (hello.c is the source file).

%/icsos/apps/tcc.exe -ohello,exe hello.c -B/icsos/tcc /icsos/tcc/tccsdk.c /icsos/tcc/crt1.c

