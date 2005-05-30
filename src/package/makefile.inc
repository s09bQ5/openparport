$(O)\custom:
	echo hello,world
	cmd /c bpackage.cmd $(VER_MAJOR) $(VER_MINOR) $(O)
	echo done > $(O)\custom

