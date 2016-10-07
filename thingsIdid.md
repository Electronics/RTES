### bashrc stuff

export PATH=$PATH:/whereyouput_gcc-arm/bin
export CC_DIR=/whereyouput_gcc-arm
export ARMGCC_DIR=/whereyouput_gcc-arm

# Update the board on WINDOWS 7 (don't ask why, it only seems to work)

### KSDK (Kinetis)
- Get it from [Here](community.nxp.com/docs/DOC-328578)
- Build the platform library:
	- lib/ksdk_platform_lib/armgcc/KL46Z4/
	- Build_all (requires cmake and make)


Now you can try the examples etc etc


## ELF -> SREC
- arm-none-eabi-objcopy -O srec release/bubble_level_tpm.elf 
release/bubble_level_tpm.srec

## Copying to the device
- cp --no-preserve=mode,ownership build/$FILENAME 
/run/media/laurie/$DEVICE
