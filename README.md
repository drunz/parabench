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
To build the release target, run:
```
./waf configure [--with-mpicc=<BIN Name>] [--prefix=<WHERE>] [--target=<LIST>]
./waf [build]
./waf install
```
The binaries are placed in the `build` directory.


Alternative targets
-------------------
There are more targets available next to `release`, which are `debug` and `gen`. To additionally build these optional targets, specify them when running configure, e.g.:
```
./waf configure --target=debug,gen
./waf
```
For each target, there will be one directory in the `build` folder. When the `gen` target is specified, each folder will have a sub-folder `gen` containing the binaries for the generated version of the respective target, e.g. `build/debug/gen` for the generated verion of the debug target.

**Note:** When using the `gen` target, then `./waf build` needs to be run twice in order to make waf recognize the generated files.


Parabench Programming Language (PBL)
------------------------------------
Parabench is designed to execute specific application I/O behaviour using I/O kernels defined in a custom scripting language. Example kernels can be found in the `examples` folder. A more detailed documentation for the language features will follow in the future.


Preprocessor
------------
The preprocessor is responsible for integrating the I/O modules defined in the `modules` folder. To run the preprocessor, specify the `gen` target when running configure. For each module, an I/O command is made available in the PBL language. For instance, with the `dread.c` module, a `dread` command is available and can be used when writing I/O kernels. The parameters for those calls directly correspond to the C-function signature. In case of `dread`, a string is expected as single parameter. Example usage of the provided modules `dread` and `dwrite` is given in the `examples/directio.pbl` kernels.

To clean the generated code use `./waf ppc_clean`.


License
-------
Parabench is licensed under GPLv3. Please find a copy in the `LICENSE` file.


Acknowledgments
---------------
Thanks to the following persons who participated improvements and fixes to the codebase:

* Julian Kunkel
* Michael Kuhn

