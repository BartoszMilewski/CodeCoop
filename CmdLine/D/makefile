.d.obj:
	dmd -c $*
all: linksto.exe maplinks.exe orphan.exe files.exe

linksto.exe: xml.obj linksto.obj WebAtlas.obj
	dmd -oflinksto.exe -L/NOM linksto.obj WebAtlas.obj xml.obj
files.exe: xml.obj files.obj WebAtlas.obj
	dmd -offiles.exe -L/NOM files.obj WebAtlas.obj xml.obj
orphan.exe: xml.obj orphan.obj WebAtlas.obj
	dmd -oforphan.exe -L/NOM xml.obj orphan.obj WebAtlas.obj
maplinks.exe: maplinks.obj
	dmd -ofmaplinks.exe -L/NOM maplinks.obj
OBJS= xml.obj orphan.obj maplinks.obj WebAtlas.obj linksto.obj files.obj
clean:
	del $(OBJS)
	del orphan.exe
	del maplinks.exe
	del linksto.exe
	del files.exe
