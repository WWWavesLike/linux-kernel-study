savedcmd_syswq.mod := printf '%s\n'   syswq.o | awk '!x[$$0]++ { print("./"$$0) }' > syswq.mod
