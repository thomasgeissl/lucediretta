#!/bin/sh
cd "$(dirname "$0")"
cd ..

curl https://openframeworks.cc/versions/v0.11.0/of_v0.11.0_osx_release.zip --output of_v0.11.0_osx_release.zip
unzip of_v0.11.0_osx_release.zip
bash -c "$(curl -sSL https://raw.githubusercontent.com/thomasgeissl/ofPackageManager/master/scripts/ofPackageManager.sh)" install
make -j4
./scripts/fix_dylib.sh
