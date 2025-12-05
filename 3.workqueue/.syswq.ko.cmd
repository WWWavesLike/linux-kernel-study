savedcmd_syswq.ko := ld -r -m elf_x86_64 -z noexecstack --no-warn-rwx-segments --build-id=sha1  -T /home/waveslike/vm/linux-6.17.9/scripts/module.lds -o syswq.ko syswq.o syswq.mod.o .module-common.o
