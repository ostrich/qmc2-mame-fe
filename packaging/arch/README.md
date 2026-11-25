# Arch Linux package

Build the standalone qchdman package from this directory:

```sh
makepkg -s
```

The recipe builds the repository-pinned QuickJS-NG QtScript backend in an
isolated staging prefix. It does not install QtScript into the host Qt tree.
The qchdman desktop icon is installed under the freedesktop hicolor hierarchy,
so this package can coexist with `qmc2-qt6-git` without sharing files.
