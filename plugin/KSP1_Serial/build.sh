#!/bin/sh
# Build EelooPadSerial.dll with Mono and install it into KSP.
#   ./build.sh                 build only      -> GameData/EelooPadSerial/EelooPadSerial.dll
#   ./build.sh install         build + copy GameData/EelooPadSerial into the KSP install
# KSPDIR can be overridden: KSPDIR=/path/to/KSP ./build.sh install
set -e
cd "$(dirname "$0")"
KSPDIR="${KSPDIR:-$HOME/.local/share/Steam/steamapps/common/Kerbal Space Program}"
M="$KSPDIR/KSP_Data/Managed"
# compiled against KSP's own .NET libraries (-nostdlib): an API missing from the game's runtime
# is then a compile error instead of a MissingMethodException in flight
mcs -target:library -optimize+ -nowarn:1591 -nostdlib \
    -out:GameData/EelooPadSerial/EelooPadSerial.dll \
    -r:"$M/mscorlib.dll" -r:"$M/System.dll" -r:"$M/System.Xml.dll" -r:"$M/System.Core.dll" \
    -r:"$M/Assembly-CSharp.dll" -r:"$M/UnityEngine.dll" -r:"$M/UnityEngine.CoreModule.dll" \
    src/*.cs
echo "built GameData/EelooPadSerial/EelooPadSerial.dll"
if [ "$1" = "install" ]; then
    mkdir -p "$KSPDIR/GameData/EelooPadSerial"
    cp GameData/EelooPadSerial/EelooPadSerial.dll "$KSPDIR/GameData/EelooPadSerial/"
    [ -f "$KSPDIR/GameData/EelooPadSerial/config.xml" ] || cp GameData/EelooPadSerial/config.xml "$KSPDIR/GameData/EelooPadSerial/"
    echo "installed into $KSPDIR/GameData/EelooPadSerial (existing config.xml kept)"
fi
