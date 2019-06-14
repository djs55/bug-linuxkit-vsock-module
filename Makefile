
good-efi.iso: good.yml detect-image
	linuxkit build -format iso-efi  good.yml 

.PHONY: detect-image
detect-image:
	(cd detect && docker build -t detect .)

