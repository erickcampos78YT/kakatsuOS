echo "; Boot loader básico para KakatsOS" > boot/boot.asm
echo "bits 16" >> boot/boot.asm
echo "org 0x7c00" >> boot/boot.asm
echo "start:" >> boot/boot.asm
echo "    cli" >> boot/boot.asm
echo "    hlt" >> boot/boot.asm
echo "times 510-($-$$) db 0" >> boot/boot.asm
echo "dw 0xaa55" >> boot/boot.asm
