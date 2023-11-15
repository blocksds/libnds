# FatFS + BlocksDS patches

This is a modified version of ChaN's [FatFS](http://elm-chan.org/fsw/ff/) filesystem library.

## Updating to newer FatFS versions

Updating to newer versions of FatFS can be done as follows:

* `diskio.c`, `ffconf.h` and `ffsystem.c` are heavily modified for BlocksDS; they should not be overwritten, but edited with the necessary changes, if any.
* `ff.c` and `ff.h` are modified for BlocksDS, but every modification area is marked with a "/* BlocksDS: ... */" comment. One can, as such, overwrite them and re-apply the BlocksDS patches by hand.
  * Patches marked as "(Feature)" are required for correct execution of BlocksDS user code.
  * Patches marked as "(Optimization)" only provide a speed benefit, and are not necessary for correct execution of BlocksDS code.
* `diskio.h` and `ffunicode.c` are unmodified relative to FatFS.
* `cache.c` and `cache.h` are custom files provided by BlocksDS entirely and do not belong to the official FatFS tree.
* Any files other than those should be overwritten directly.
