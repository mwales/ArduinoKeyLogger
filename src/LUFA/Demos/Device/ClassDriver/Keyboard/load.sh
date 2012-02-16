sudo dfu-programmer at90usb82 erase
sudo dfu-programmer at90usb82 flash Keyboard.hex --suppress-bootloader-mem
sudo dfu-programmer at90usb82 reset

