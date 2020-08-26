# quadric systemc

### Installing
Installation dir:
OSX: `/usr/local/systemc`
linux: `/quadric/usr/local`

```bash
# Build and install systemc
> mkdir build && cd build
> export CC=/quadric/gcc/vcs-latest/bin/gcc (only on linux)
> export CXX=/quadric/gcc/vcs-latest/bin/g++ (only on linux)
> cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DCMAKE_CXX_STANDARD=11 -DCMAKE_INSTALL_PREFIX=<installation dir> ..
> make -j8
> make install (or sudo make install, depending on permissions of target location)
```

