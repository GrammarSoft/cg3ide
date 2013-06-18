#!/bin/bash
if [ ! -f cg3ide.pro ]; then
	echo "This should be run from the project root folder!"
	exit
fi

ROOT=`realpath "$0"`
ROOT=`dirname "$ROOT"`
ROOT="$ROOT/.."

rm -rfv cg3ide-windows.* CG3-IDE.wix*

echo 'Making Windows Installer (1 of 3)'
candle scripts/CG3-IDE.wxs
echo 'Making Windows Installer (2 of 3)'
light CG3-IDE.wixobj
echo 'Making Windows Installer (3 of 3)'
mv -fv 'CG3-IDE.msi' 'cg3ide-windows.msi'

echo 'Making Windows ZIP (1 of 3)'
rm -rfv '/tmp/CG-3 IDE'
mkdir -pv '/tmp/CG-3 IDE/platforms'
cd '/tmp/CG-3 IDE'
echo 'Making Windows ZIP (2 of 3)'
rsync -av "$ROOT/../cg3ide-build-Desktop/release/cg3ide.exe" './CG-3 IDE.exe'
rsync -av "$ROOT/../cg3ide-build-Desktop/release/cg3processor.exe" /cygdrive/c/Applications/ICU/49.1.2/bin/icuio49.dll /cygdrive/c/Applications/Qt5/5.0.2/msvc2010/bin/Qt5Core.dll /cygdrive/c/Applications/Qt5/5.0.2/msvc2010/bin/Qt5Gui.dll /cygdrive/c/Applications/Qt5/5.0.2/msvc2010/bin/Qt5Widgets.dll /cygdrive/c/Applications/Qt5/5.0.2/msvc2010/bin/icudt49.dll /cygdrive/c/Applications/Qt5/5.0.2/msvc2010/bin/icuuc49.dll /cygdrive/c/Applications/Qt5/5.0.2/msvc2010/bin/icuin49.dll /cygdrive/c/Applications/Qt5/5.0.2/msvc2010/bin/D3DCompiler_43.dll /cygdrive/c/Applications/Qt5/5.0.2/msvc2010/bin/libEGL.dll /cygdrive/c/Applications/Qt5/5.0.2/msvc2010/bin/libGLESv2.dll /cygdrive/c/Applications/ICU/49.1.2/bin/icuio49.dll ~/vislcg3/build/VS10/src/Release/*.exe '--exclude=test*' ./
rsync -av /cygdrive/c/Applications/Qt5/5.0.2/msvc2010/plugins/platforms/qminimal.dll /cygdrive/c/Applications/Qt5/5.0.2/msvc2010/plugins/platforms/qwindows.dll ./platforms/
echo 'Making Windows ZIP (2 of 3)'
cd ..
zip -9r cg3ide-windows.zip 'CG-3 IDE'
mv -fv cg3ide-windows.zip "$ROOT/"

cd "$ROOT/"

rsync -avzP cg3ide-windows.zip cg3ide-windows.msi tino@beta.visl.sdu.dk:/home/httpd/html/download/vislcg3/
