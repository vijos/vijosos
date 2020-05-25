# vijosos
Vijos: Vijos Isn't Just an Operating System

Build musl-libc with:

```bash
ARCH=riscv CROSS_COMPILE="riscv64-unknown-linux-gnu-" CFLAGS="-march=rv64imac" ./configure
make
```

Build libstdc++ with:

```bash
mkdir build
cd build
../configure --host=riscv64-unknown-linux-gnu --enable-clocale=generic
make
```
