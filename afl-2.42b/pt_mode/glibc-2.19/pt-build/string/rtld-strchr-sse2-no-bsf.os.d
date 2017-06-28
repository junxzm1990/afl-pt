$(common-objpfx)string/rtld-strchr-sse2-no-bsf.os: \
 ../sysdeps/x86_64/multiarch/strchr-sse2-no-bsf.S \
 ../include/stdc-predef.h ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h

../include/stdc-predef.h:

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:
