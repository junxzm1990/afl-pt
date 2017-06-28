$(common-objpfx)csu/abi-note.o: \
 abi-note.S ../include/stdc-predef.h ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h \
 $(common-objpfx)csu/abi-tag.h

../include/stdc-predef.h:

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:

$(common-objpfx)csu/abi-tag.h:
