savedcmd_wtk.mod := printf '%s\n'   wtk.o | awk '!x[$$0]++ { print("./"$$0) }' > wtk.mod
