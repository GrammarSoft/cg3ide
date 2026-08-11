#!/bin/bash
# export $PATH="/path/to/qt/bin:$PATH"
set -e
DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
ARCH=$(uname -m)

rm -rfv /tmp/cg3ide
mkdir -pv /tmp/cg3ide
cd /tmp/cg3ide

qt-cmake "$DIR/.."
make -j VERBOSE=1 2>&1 | tee build.log

pushd src
macdeployqt cg3ide.app -dmg
macdeployqt cg3processor.app -dmg
rsync -av cg3processor.app/Contents/MacOS/cg3processor cg3ide.app/Contents/MacOS/
mkdir -pv cg3ide.app/Contents/Resources/CG-3
mv -v cg3ide.app 'CG-3 IDE.app'
ditto -v -c -k --sequesterRsrc --keepParent --zlibCompressionLevel 9 'CG-3 IDE.app' cg3ide-macos-$ARCH.zip
rsync -avzP cg3ide-macos-$ARCH.zip apertium@apertium.projectjj.com:/home/apertium/public_html/osx/nightly/$ARCH/
popd
