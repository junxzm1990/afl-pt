$(common-objpfx)string/rtld-stpcpy-ssse3.os: \
 ../sysdeps/x86_64/multiarch/stpcpy-ssse3.S ../include/stdc-predef.h \
 ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h \
 ../sysdeps/x86_64/multiarch/strcpy-ssse3.S

../include/stdc-predef.h:

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:

../sysdeps/x86_64/multiarch/strcpy-ssse3.S:
