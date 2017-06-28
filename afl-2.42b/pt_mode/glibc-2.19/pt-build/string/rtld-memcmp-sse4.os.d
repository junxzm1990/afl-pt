$(common-objpfx)string/rtld-memcmp-sse4.os: \
 ../sysdeps/x86_64/multiarch/memcmp-sse4.S ../include/stdc-predef.h \
 ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/stdc-predef.h:

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:
