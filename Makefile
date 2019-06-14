
.PHONY: default
default:
	@echo 'Run "make good" to see the v1.14.105 kernel'
	@echo 'Run "make bad" to see the v1.14.106 kernel'

.PHONY: good
good: good-efi.iso
	linuxkit run -iso -uefi good-efi.iso

.PHONY: bad
bad: bad-efi.iso
	linuxkit run -iso -uefi bad-efi.iso

good-efi.iso: good.yml .detect-image
	linuxkit build -format iso-efi good.yml

bad-efi.iso: bad.yml .detect-image
	linuxkit build -format iso-efi bad.yml 

.detect-image:
	(cd detect && docker build -t detect .) && touch .detect-image

.PHONY: clean
clean:
	rm -f .detect-image *.iso
	rm -rf *-state
