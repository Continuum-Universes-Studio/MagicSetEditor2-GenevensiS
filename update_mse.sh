#!/bin/bash
# script for keeping mse up to date
# this script is available at:
# https://gist.github.com/ebbit1q/2b72433ef5aea4934235e8188e238346

# This is free and unencumbered software released into the public domain.
#
# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.
#
# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# For more information, please refer to <https://unlicense.org>

# NOTE: you will need these dependencies to compile mse
# packages for arch:
# sudo pacman -Syu hunspell boost git cmake libmspack curl cairo gtk3
# alternatively install wxwidgets-gtk3-unstable from the aur, this replaces the "building wxgtk" section but wxconf and wxcmake will need to be set from /usr, it's notably not faster as it still compiles the exact same sources
# packages for deb:
# sudo apt install libboost-dev libboost-regex-dev libboost-json-dev libhunspell-dev git build-essential cmake libgtk-3-dev libcurl4-openssl-dev
# packages for dnf:
# sudo dnf install boost-devel hunspell-devel git cmake
# sudo dnf install at-spi2-atk-devel at-spi2-core-devel atk-devel brotli brotli-devel bzip2-devel cairo-devel cairo-gobject-devel fontconfig-devel freetype-devel fribidi-devel gdk-pixbuf2-devel gi-docgen-fonts graphite2-devel gtk3-devel harfbuzz-cairo harfbuzz-devel libXcomposite-devel libXcursor-devel libXdamage-devel libXext-devel libXfixes-devel libXft-devel libXi-devel libXinerama-devel libXrandr-devel libXrender-devel libXtst-devel libcloudproviders-devel libdatrie-devel libepoxy-devel libjpeg-turbo-devel liblerc-devel libpng-devel libthai-devel libtiff-devel libwebp-devel mesa-libGLU-devel pango-devel pixman-devel redhat-display-fonts redhat-text-fonts wayland-devel wxGTK-gl wxGTK-i18n wxGTK-media wxGTK-webview libcurl-devel

# NOTE: you can edit these variables
repo="/home/cdjensen94/Projects/MagicSetEditor2-GenevensiS" # location to keep mse's git repo, this location will be symlinked to!
concurrency=7 # amount of cores to use in compilation
pull_templates=1 # set to 0 to not update the templates
recompile=1 # set to 0 to not recompile
prompt_install=1 # set to 0 to not ask about installation

if [[ ! -e "$repo" ]]; then
  echo "$repo does not exist"
  echo "please modify the script to choose a location to place the source files in"
  echo "note that this location should not be moved if you choose the default option of symlinking the files"
  exit 2
fi
set -e

# template data and fonts
msedir="$HOME/.magicseteditor"
if [[ $pull_templates == 1 ]]; then
  cd "$repo"
  repodir="$repo/Full-Magic-Pack"
  if [[ -d $repodir ]]; then
    cd "$repodir"
    git checkout main
    git pull
  else
    git clone "https://github.com/MagicSetEditorPacks/Full-Magic-Pack" "$repodir"
    cd "$repodir"
  fi
  mkdir -p "$msedir"
  relpath="$(realpath --relative-to="$msedir" "./data")"
  ln -s "$relpath" "$msedir/data" -fT
  fontdir="$HOME/.local/share/fonts"
  mkdir -p "$fontdir"
  relpath="$(realpath --relative-to="$fontdir" "./Magic - Fonts")"
  ln -s "$relpath" "$fontdir/magic" -fT
fi

# compilation
if [[ $recompile == 1 ]]; then
  # building wxgtk
  cd "$repo"
  repodir="$repo/wxWidgets"
  wxconf="$repodir/target/bin/wx-config"
  wxcmake="$repodir/target/lib/cmake/wxWidgets"
  if [[ -x $wxconf ]]; then
    # do nothing, we assume wxgtk is properly installed
    echo "found wxwidgets"
  else
    # delete sources and start from scratch
    rm -rf "$repodir"
  fi
  if [[ ! -d $repodir ]]; then
    git clone "https://github.com/wxWidgets/wxWidgets" "$repodir" \
        --depth 1 --branch v3.3.1
    cd "$repodir"
    git submodule update --init src/expat src/stc/scintilla src/stc/lexilla \
        3rdparty/catch 3rdparty/nanosvg 3rdparty/libwebp
    mkdir buildgtk
    mkdir target
    cd buildgtk
    cmake .. -DCMAKE_INSTALL_PREFIX="$repodir/target"
    make -j$concurrency install
  fi

  # mse and resources
  cd "$repo"
  repodir="$repo/MagicSetEditor2"
  if [[ -d $repodir ]]; then
    cd "$repodir"
    git checkout main
    git pull
  else
    git clone "https://github.com/G-e-n-e-v-e-n-s-i-S/MagicSetEditor2" "$repodir"
    cd "$repodir"
  fi
  relpath="$(realpath --relative-to="$msedir" "./resource")"
  ln -s "$relpath" "$msedir/resource" -fT

  # compiling mse
  mkdir -p build
  cd build
  cmake .. -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="$wxcmake" \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=1 # exporting is userful for developers
  make -j$concurrency
fi

if [[ $prompt_install == 1 ]]; then
  # installation is unnecessary, you can just run ./magicseteditor
  echo "would you like to install mse for this user? (Ctrl C exits)"
  read -r
  # it's possible to move the binary to /usr/local/bin for a system
  # installation instead of user only, please do not symlink to /usr though
  # (use eg. sudo cp ./magicseteditor /usr/local/bin)
  # the application file can be placed in /usr/local/share/applications
  installdir="$HOME/.local/bin"
  appdir="$HOME/.local/share/applications"
  mkdir -p "$installdir"
  mkdir -p "$appdir"
  relpath="$(realpath --relative-to="$installdir" "./magicseteditor")"
  ln -s "$relpath" "$installdir/magicseteditor" -fT
  ico="$(realpath ../resource/icon/app.ico)"
  echo "[Desktop Entry]
  Version=1.0
  Name=MagicSetEditor
  Comment=Create custom magic cards
  Exec=env GTK_THEME=Adwaita:light $installdir/magicseteditor %U
  Icon=$ico
  Terminal=false
  Type=Application
  " >"$appdir/mse.desktop"
  echo "installed mse to $installdir and added a desktop entry to applications"
  # NOTE: it's possible to open .mse-set files with magicseteditor by default
  #
  # however, because they are just zip files this will set zip files in
  # general to open with mse
  #
  # to do this you'll need a .mse-set (you can make one with mse) and change
  # the file's properties to open with MagicSetEditor (available on most
  # desktop environments, it's also possible to edit the mime types directly)
fi
