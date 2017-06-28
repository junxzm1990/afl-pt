$(common-objpfx)nptl/pthread_spin_lock.os: \
 ../nptl/sysdeps/x86_64/pthread_spin_lock.S ../include/stdc-predef.h \
 ../include/libc-symbols.h \
 $(common-objpfx)config.h \
 ../sysdeps/generic/symbol-hacks.h \
 ../nptl/sysdeps/unix/sysv/linux/x86_64/lowlevellock.h \
 ../include/stap-probe.h

../include/stdc-predef.h:

../include/libc-symbols.h:

$(common-objpfx)config.h:

../sysdeps/generic/symbol-hacks.h:

../nptl/sysdeps/unix/sysv/linux/x86_64/lowlevellock.h:

../include/stap-probe.h:
