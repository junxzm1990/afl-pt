# Frequently Asked Questions

1. The patched binary won’t run?

A. Because we modified the dynamic loader(ld) from libc, the patched binary will not run normally unless for fuzzing with PTrix.  

2. How to fuzz a shared library?

A. Since we configure iPT to only trace the .text section of the fuzzed binary, if you need to fuzz a shared library please statically link to library into your harness binary. 

3. Can I run PTrix in a VM or Docker container?

A. Short answer is no. 
The current virtualization tools don't support virtualizing iPT yet, lets wait for the updates from Xen and Hyper-V, they should be out soon. As for Docker, we didn't test, but if your container host is Ubuntu 14.04 and the docker image is 14.04 too it might work.

4. How to check whether Intel PT is support in your envrionment?

A. `grep intel_pt /proc/cpuinfo` or run `reinstall_ptmod.sh`
