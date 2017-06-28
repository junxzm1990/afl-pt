$(common-objpfx)string/rtld-stpcpy-sse2-unaligned.os: \
 ../sysdeps/x86_64/multiarch/stpcpy-sse2-unaligned.S \
 ../include/stdc-predef.h ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h \
 ../sysdeps/x86_64/multiarch/strcpy-sse2-unaligned.S

../include/stdc-predef.h:

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:

../sysdeps/x86_64/multiarch/strcpy-sse2-unaligned.S:
