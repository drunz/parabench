Parabench
=========

A parallel and programmable benchmark for POSIX and MPI-IO

Dependencies
------------
Required:
```
flex, bison, glib2
```
Optional:
```
mpi
```

Building
--------
```
./waf configure [--with-mpicc=<BIN Name>] [--prefix=<WHERE>]
./waf [build]
./waf install
```
