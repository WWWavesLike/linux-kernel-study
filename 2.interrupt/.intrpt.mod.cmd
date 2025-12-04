savedcmd_intrpt.mod := printf '%s\n'   intrpt.o | awk '!x[$$0]++ { print("./"$$0) }' > intrpt.mod
