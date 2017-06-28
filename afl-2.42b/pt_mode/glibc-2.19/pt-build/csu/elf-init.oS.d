$(common-objpfx)csu/elf-init.oS: \
 elf-init.c ../include/stdc-predef.h ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h \
 /usr/lib/gcc/x86_64-linux-gnu/4.8/include/stddef.h

../include/stdc-predef.h:

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:

/usr/lib/gcc/x86_64-linux-gnu/4.8/include/stddef.h:
