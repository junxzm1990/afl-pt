# afl-pt

# pt-mode resume fuzzing work:
 1) when fuzz with -P, a rand_map will be written to target_dir, name as ".target.rmap"
 2) when resuming fuzzing with -i-, specify env AFL_PTMODE_RAND_MAP=@rand_map_location
