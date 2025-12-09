savedcmd_proc_sysfs.mod := printf '%s\n'   proc_sysfs.o | awk '!x[$$0]++ { print("./"$$0) }' > proc_sysfs.mod
