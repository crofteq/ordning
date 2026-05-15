set shell := ["bash", "-ec"]

appl := "ordning"

build_version := `git describe --tags --abbrev=7 | sed 's/^v//'`
build_date := `date -u +%Y-%m-%d`
build_hash := `git rev-parse HEAD`
repo_url := `git config --get remote.origin.url | sed 's|^git@github.com:|https://github.com/|' | sed 's|\.git$||'`

container_image := "ghcr.io/crofteq/sdk-qt-6.8.3:1.0.0"

export THIS_FOLDER := justfile_directory()

run_args := \
''' \
  --rm -i \
  --env USER_ID=$(id -u) \
  --env GROUP_ID=$(id -g) \
  --workdir /home/user/work \
  --volume ${THIS_FOLDER}:/home/user/work:rw,z \
  ${EXTRA_RUN_ARGS} \
'''

_default:
  @just --list

[private]
[no-exit-message]
@docker-run +COMMANDS:
  #!/usr/bin/env bash
  if ! $(docker image inspect {{container_image}} >/dev/null 2>&1); then
    docker pull {{container_image}}
  fi
  docker run {{run_args}} {{container_image}} {{COMMANDS}}

# Build application
build: 
  #!/usr/bin/env bash
  set -e # exit on error
  if [[ ! -f /.dockerenv ]]; then
    just docker-run '''bash -c "just build"'''
    exit 0
  fi

  cmake -S . -B build/{{appl}} -DBUILD_VERSION={{build_version}} -DBUILD_DATE={{build_date}} -DBUILD_HASH={{build_hash}} -DREPOSITORY_URL={{repo_url}}
  cmake --build build/{{appl}} --parallel $(nproc)

# Build Linux appimage
build-linux:
  #!/usr/bin/env bash
  set -e # exit on error
  if [[ ! -f /.dockerenv ]]; then
    just docker-run '''bash -c "just build-linux"'''
    exit 0
  fi

  # Configuration
  build_dir="{{justfile_directory()}}/build/{{appl}}_linux"
  # INSTALL_DIR="${build_dir}/install"
  appimage_dir="${build_dir}/{{appl}}.AppDir"
  QT_VERSION="6.8.3"
  QT_DIR="/opt/Qt/${QT_VERSION}/gcc_64"

  # Setup Qt environment
  export PATH="${QT_DIR}/bin:$PATH"
  export LD_LIBRARY_PATH="${QT_DIR}/lib:$LD_LIBRARY_PATH"
  export QT_PLUGIN_PATH="${QT_DIR}/plugins"
  export PKG_CONFIG_PATH="${QT_DIR}/lib/pkgconfig:$PKG_CONFIG_PATH"

  # Build the application
  cmake -S . -B ${build_dir} -DBUILD_VERSION={{build_version}} -DBUILD_DATE={{build_date}} -DBUILD_HASH={{build_hash}} -DREPOSITORY_URL={{repo_url}} -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_PREFIX_PATH="${QT_DIR}"
  cmake --build ${build_dir} --parallel $(nproc)

  # Install the application to a temporary directory
  DESTDIR=${appimage_dir} cmake --install ${build_dir}

  # Prepare AppImage directory
  mkdir -p ${appimage_dir}/usr/share/applications
  mkdir -p ${appimage_dir}/usr/share/icons/hicolor/scalable/apps
  cp {{justfile_directory()}}/resources/{{appl}}.svg ${appimage_dir}/usr/share/icons/hicolor/scalable/apps/{{appl}}.svg
  for size in 16 32 48 64 128 256 512; do
    echo "Generating icon: ${appimage_dir}/usr/share/icons/hicolor/${size}x${size}/apps/{{appl}}.png"
    mkdir -p ${appimage_dir}/usr/share/icons/hicolor/${size}x${size}/apps
    inkscape ${appimage_dir}/usr/share/icons/hicolor/scalable/apps/{{appl}}.svg --export-type=png \
      --export-filename=${appimage_dir}/usr/share/icons/hicolor/${size}x${size}/apps/{{appl}}.png \
      --export-width=$size --export-height=$size 2>/dev/null
  done
  mkdir -p ${appimage_dir}/usr/share/applications
  cp ${appimage_dir}/usr/share/icons/hicolor/256x256/apps/{{appl}}.png ${appimage_dir}/{{appl}}.png
  cp ${appimage_dir}/usr/share/icons/hicolor/256x256/apps/{{appl}}.png ${appimage_dir}/usr/share/applications/{{appl}}.png
  cp {{justfile_directory()}}/resources/{{appl}}.desktop ${appimage_dir}/usr/share/applications/{{appl}}.desktop

  # Removing unwanted SQL drivers before packaging, otherwise it leads to library conflicts
  sudo find ${QT_DIR}/plugins/sqldrivers -type f ! -name "libqsqlite.so" -delete

  # Deploy with linuxdeploy
  export QT_QPA_PLATFORM_PLUGIN_PATH="${QT_DIR}/plugins"
  export APPIMAGE_EXTRACT_AND_RUN=1
  export QMAKE="${QT_DIR}/bin/qmake"
  linuxdeploy --appdir=${appimage_dir} \
      --plugin qt \
      --executable=${appimage_dir}/usr/bin/{{appl}} \
      --desktop-file=${appimage_dir}/usr/share/applications/{{appl}}.desktop \
      --icon-file=${appimage_dir}/usr/share/applications/{{appl}}.png

  # Create AppImage
  ARCH=x86_64 appimagetool ${appimage_dir} ${build_dir}/{{appl}}-{{build_version}}.AppImage
  chmod +x ${build_dir}/{{appl}}-{{build_version}}.AppImage

  # move the AppImage to artifacts folder
  mkdir -p {{justfile_directory()}}/build/artifacts
  chmod +x ${build_dir}/{{appl}}-{{build_version}}.AppImage
  mv ${build_dir}/{{appl}}-{{build_version}}.AppImage {{justfile_directory()}}/build/artifacts/
  echo "AppImage created at: ./build/artifacts/{{appl}}-{{build_version}}.AppImage"

# Build windows version of the application
build-windows:
  #!/usr/bin/env bash
  set -e # exit on error
  if [[ ! -f /.dockerenv ]]; then
    just docker-run '''bash -c "just build-windows"'''
    exit 0
  fi

  export HOME=/home/user
  export WINEPREFIX="$HOME/.wine"

  builddir="{{justfile_directory()}}/build/{{appl}}_windows"
  builddir_win="Z:${builddir}"
  srcdir_win="Z:{{justfile_directory()}}"
  paths="C:\\Qt\\Tools\\CMake_64\\bin;C:\\Qt\\Tools\\mingw1310_64\\bin;C:\\Qt\\6.8.3\\mingw_64\\bin;%PATH%"

  wine cmd /c "set PATH=${paths} && cmake --preset=default -S ${srcdir_win} -B ${builddir_win} -DBUILD_VERSION={{build_version}} -DBUILD_DATE={{build_date}} -DBUILD_HASH={{build_hash}} -DREPOSITORY_URL={{repo_url}}"
  wine cmd /c "set PATH=${paths} && cmake --build ${builddir_win} --parallel %NUMBER_OF_PROCESSORS%"
  mkdir -p ${builddir}/deploy
  cp ${builddir}/src/{{appl}}.exe ${builddir}/deploy/
  wine cmd /c "set PATH=${paths} && windeployqt --compiler-runtime --no-translations ${builddir_win}/deploy/{{appl}}.exe"
  # create a zip package of the deployed application and store it in artifacts folder
  mkdir -p {{justfile_directory()}}/build/artifacts
  cd ${builddir}/deploy
  7z a -r {{justfile_directory()}}/build/artifacts/{{appl}}-windows-{{build_version}}.zip *

# Remove all generated files, excluding artifacts folder
clean:
  #!/usr/bin/env bash
  set -e # exit on error
  if [[ ! -f /.dockerenv ]]; then
    just docker-run '''bash -c "just clean"'''
    exit 0
  fi
  rm -rf build/{{appl}}*
  rm -rf build/migrate*
  rm -rf build/test*

# Remove all generated files, including artifacts folder
clean-all:
  #!/usr/bin/env bash
  set -e # exit on error
  if [[ ! -f /.dockerenv ]]; then
    just docker-run '''bash -c "just clean-all"'''
    exit 0
  fi
  rm -rf build

# Import local development justfile, if it exists.
import? 'develop.just'
