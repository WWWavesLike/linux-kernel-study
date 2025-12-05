savedcmd_delaywq.mod := printf '%s\n'   delaywq.o | awk '!x[$$0]++ { print("./"$$0) }' > delaywq.mod
