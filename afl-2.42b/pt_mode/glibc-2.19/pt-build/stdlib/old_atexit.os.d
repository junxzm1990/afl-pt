$(common-objpfx)stdlib/old_atexit.os: \
 old_atexit.c ../include/stdc-predef.h ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h ../include/shlib-compat.h \
 $(common-objpfx)abi-versions.h

../include/stdc-predef.h:

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:

../include/shlib-compat.h:

$(common-objpfx)abi-versions.h:
