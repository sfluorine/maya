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

To execute the examples programs:

```console
$ ./build/maya -a examples/factorial.masm -o fac.maya -a examples/fibonacci.masm -o fib.maya
$ ./build/maya -e fac.maya
$ ./build/maya -e fib.maya
```

Or u can execute them simultaneously in order:

```console
$ ./build/maya -e fac.maya fib.maya
```

Orrrrr u can do this:

```console
$ ./build/maya -a examples/factorial.masm -o fac.maya -a examples/fibonacci.masm -o fib.maya -e fac.maya fib.maya
```

Pretty cool huh? :^)
