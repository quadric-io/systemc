# quadric systemc

### Installing

```bash
# Build and install systemc
> export CC=/quadric/gcc/vcs-latest/bin/gcc (only on linux)
> export CXX=/quadric/gcc/vcs-latest/bin/g++ (only on linux)
> ./config/bootstrap (only if building after clone, or changing any of the config files)
> mkdir build && cd build
> ../configure CXXFLAGS="-std=c++11" --prefix=<installation dir> --with-unix-layout
> make -j8
> make install (or sudo make install, depending on permissions of target location)
```

