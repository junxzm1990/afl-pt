Command line used to find this crash:

./afl-fuzz -Q -d -i test_progs/exif_dir/exif-0.6.21/exif-samples/jpg/exif-org/ -o /tmp/ramdisk/exif-qemu-10-25 test_progs/exif_dir/exif-0.6.21/obj-qemu/exif/exif @@

If you can't reproduce a bug outside of afl-fuzz, be sure to set the same
memory limit. The limit used for this fuzzing session was 200 MB.

Need a tool to minimize test cases before investigating the crashes or sending
them to a vendor? Check out the afl-tmin that comes with the fuzzer!

Found any cool bugs in open-source tools using afl-fuzz? If yes, please drop
me a mail at <lcamtuf@coredump.cx> once the issues are fixed - I'd love to
add your finds to the gallery at:

  http://lcamtuf.coredump.cx/afl/

Thanks :-)
