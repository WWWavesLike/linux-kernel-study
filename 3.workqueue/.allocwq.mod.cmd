savedcmd_allocwq.mod := printf '%s\n'   allocwq.o | awk '!x[$$0]++ { print("./"$$0) }' > allocwq.mod
