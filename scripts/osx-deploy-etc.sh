rm -rfv *.dmg
CG3DIST=`~/Desktop/Projects/vislcg3/dist/dist-osx.pl`
time macdeployqt cg3ide.app -dmg
time macdeployqt cg3processor.app -dmg
rsync -av cg3processor.app/Contents/MacOS/cg3processor cg3ide.app/Contents/MacOS/
mkdir -pv cg3ide.app/Contents/Resources/CG-3
rsync -av $CG3DIST/* cg3ide.app/Contents/Resources/CG-3/
mv -v cg3ide.app 'CG-3 IDE.app'
ditto -v -c -k --sequesterRsrc --keepParent --zlibCompressionLevel 9 'CG-3 IDE.app' cg3ide.zip
rsync -avzP cg3ide.zip tino@beta.visl.sdu.dk:/home/httpd/html/download/vislcg3/cg3ide-macosx.zip
