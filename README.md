Parabench
=========

A parallel and programmable benchmark for POSIX and MPI-IO

Dependencies
------------
```
flex, bison, glib2, mpi
```
MPICH2 is recommended. Not tested against Open MPI.


Building
--------
```
./waf configure [--with-mpicc=<BIN Name>] [--prefix=<WHERE>]
./waf [build]
./waf install
```
The project also compiles without MPI installed. However, some example programs require a proper MPI build to run.
