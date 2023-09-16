# Maya

A simple stack based virtual machine.

## Quick Start

```console
$ mkdir build
$ cmake -B build -G Ninja
$ cd build
$ ninja
```

## Example

To execute the factorial example program:

```console
$ ./build/maya -a examples/factorial.masm factorial.maya
$ ./build/maya -e factorial.maya
```

Or in one command

```console
$ ./build/maya -a examples/factorial.masm factorial.maya -e factorial.maya
```
