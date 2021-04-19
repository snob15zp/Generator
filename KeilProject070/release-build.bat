del /q Objects\*

c:\Keil_v5\UV4\uv4 -b Generator070_release.uvprojx -t "Target 1" -o "release-build.log"

bootloader-tools\hex2bin.exe Objects\gen-g070-release.hex

bootloader-tools\ba-fw-builder.exe -v 0.2.12 -ib Objects\gen-g070-release.bin -t dev_gen_g070 -o dev_gen_g070

bootloader-tools\srec_cat.exe ba.hex -Intel romba.hex -Intel Objects\gen-g070-release.bin -Binary -offset 0x08003800 -o dev_gen_g070_release.hex -Intel

pause