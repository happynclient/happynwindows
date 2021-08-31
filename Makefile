zip: $(shell find .)
	echo $(tagname)
	cp installer/happynet_install.exe happynet-win-x86-x64-all-$(tagname).exe
	zip  happynet-win-x86-x64-all-$(tagname).zip  happynet-win-x86-x64-all-$(tagname).exe
clean:
	rm -rf happynet-win-x86-x64-all-*
