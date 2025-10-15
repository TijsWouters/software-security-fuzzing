# 1) See current setting (save it if you want to restore later)
cat /proc/sys/kernel/core_pattern

# 2) Switch to plain core files (needs root)
echo core | sudo tee /proc/sys/kernel/core_pattern

# 3) Allow core files to be written by the shell you’ll run afl-fuzz from
ulimit -c unlimited