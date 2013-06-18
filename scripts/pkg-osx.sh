#!/bin/bash
if [ ! -f ../cg3ide/cg3ide.pro ]; then
	echo "This should be run from the Release build folder!"
	exit
fi
rm -rfv *.dmg *.app *.o *.h moc_* Makefile* *.zip
qmake ../cg3ide/cg3ide.pro
time make -j5

CG3DIST=`~/Desktop/Projects/vislcg3/dist/dist-osx.pl`
time macdeployqt cg3ide.app -dmg
time macdeployqt cg3processor.app -dmg
rsync -av cg3processor.app/Contents/MacOS/cg3processor cg3ide.app/Contents/MacOS/
mkdir -pv cg3ide.app/Contents/Resources/CG-3
rsync -av $CG3DIST/* cg3ide.app/Contents/Resources/CG-3/
mv -v cg3ide.app 'CG-3 IDE.app'
ditto -v -c -k --sequesterRsrc --keepParent --zlibCompressionLevel 9 'CG-3 IDE.app' cg3ide-macosx.zip
rsync -avzP cg3ide-macosx.zip tino@beta.visl.sdu.dk:/home/httpd/html/download/vislcg3/
