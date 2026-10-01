{
  description = "AlsaPlayer - PCM audio player for Linux";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      systems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = nixpkgs.lib.genAttrs systems;

      # Shared feature flags for package and develop builds.
      cmakeFeatureFlags = [
        "-DENABLE_GTK3=ON"
        "-DENABLE_ALSA=ON"
        "-DENABLE_JACK=ON"
        "-DENABLE_OSS=ON"
        "-DENABLE_MAD=ON"
        "-DENABLE_FLAC=ON"
        "-DENABLE_VORBIS=ON"
        "-DENABLE_MIKMOD=ON"
        "-DENABLE_SNDFILE=ON"
        "-DENABLE_CDDA=ON"
        "-DENABLE_OPENGL=ON"
        "-DENABLE_SYSTRAY=OFF"
        "-DENABLE_NLS=OFF"
      ];
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          lib = pkgs.lib;

          version =
            let
              configureAc = builtins.readFile (self + "/configure.ac");
              lines = lib.splitString "\n" configureAc;
              initLine = lib.findFirst (l: lib.hasPrefix "AC_INIT" l) "" lines;
              parts = lib.splitString "[" initLine;
              verField =
                if builtins.length parts >= 3 then builtins.elemAt parts 2 else "";
              ver = builtins.head (lib.splitString "]" verField);
            in
            if ver != "" then ver else "0.99.82";

          alsaplayerBuildInputs = with pkgs; [
            alsa-lib
            gtk3
            glib
            # glib's pkg-config Requires.private pulls in sysprof-capture-4
            libsysprof-capture
            libjack2
            libmad
            libid3tag
            flac
            libogg
            libvorbis
            libmikmod
            libsndfile
            # sndfile's pkg-config Requires.private pulls in opus
            libopus
            libGL
            libGLU
            libx11
            zlib
          ];
        in
        {
          default = pkgs.stdenv.mkDerivation {
            pname = "alsaplayer";
            inherit version;

            src = lib.cleanSource self;

            nativeBuildInputs = with pkgs; [
              cmake
              pkg-config
              makeWrapper
            ];

            buildInputs = alsaplayerBuildInputs;

            cmakeFlags = cmakeFeatureFlags ++ [
              "-DCMAKE_BUILD_TYPE=RelWithDebInfo"
            ];

            postInstall = ''
              wrapProgram $out/bin/alsaplayer \
                --prefix LD_LIBRARY_PATH : "$out/lib" \
                --set ALSAPLAYER_PLUGIN_DIR "$out/lib/alsaplayer"
            '';

            meta = with lib; {
              description = "Heavily multi-threaded PCM player that exercises the ALSA library";
              homepage = "https://alsaplayer.sourceforge.net/";
              license = licenses.gpl3Plus;
              platforms = platforms.linux;
              mainProgram = "alsaplayer";
            };
          };
        }
      );

      apps = forAllSystems (system: {
        default = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/alsaplayer";
        };
      });

      devShells = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          lib = pkgs.lib;

          # Same deps as the package so configure/build match nix build.
          alsaplayerBuildInputs = self.packages.${system}.default.buildInputs;

          # Common preamble for all helper scripts.
          # Requires PROJECT_SOURCE (set by shellHook). Clear error otherwise.
          scriptPreamble = ''
            set -euo pipefail
            if [ -z "''${PROJECT_SOURCE:-}" ]; then
              echo "error: PROJECT_SOURCE is unset." >&2
              echo "  Enter the alsaplayer develop shell first:  nix develop" >&2
              echo "  (scripts refuse to run outside that environment)" >&2
              exit 1
            fi
            if [ ! -f "$PROJECT_SOURCE/CMakeLists.txt" ]; then
              echo "error: PROJECT_SOURCE=$PROJECT_SOURCE has no CMakeLists.txt" >&2
              exit 1
            fi
            BUILD_DIR="''${PROJECT_BUILD_DIR:-/tmp/alsaplayer-build}"
            STAGE_DIR="$BUILD_DIR/stage"
            CACHE="$BUILD_DIR/CMakeCache.txt"
            BIN="$STAGE_DIR/bin/alsaplayer"
          '';

          # True when CMake must be re-run (missing cache, different source,
          # different stage prefix, or non-Debug build type).
          needReconfigureFn = ''
            need_reconfigure() {
              [ -f "$CACHE" ] || return 0
              # Source tree moved / different checkout
              local home
              home=$(grep -E '^CMAKE_HOME_DIRECTORY:INTERNAL=' "$CACHE" | cut -d= -f2- || true)
              [ "$home" = "$PROJECT_SOURCE" ] || return 0
              # Stage install prefix must match so ADDON_DIR points at staged plugins
              local prefix
              prefix=$(grep -E '^CMAKE_INSTALL_PREFIX:PATH=' "$CACHE" | cut -d= -f2- || true)
              [ "$prefix" = "$STAGE_DIR" ] || return 0
              # Develop defaults to Debug
              local btype
              btype=$(grep -E '^CMAKE_BUILD_TYPE:STRING=' "$CACHE" | cut -d= -f2- || true)
              [ "$btype" = "Debug" ] || return 0
              return 1
            }
          '';

          # Env for the unwrapped staged binary (mirrors postInstall wrapProgram).
          # Reader/CorePlayer bake ADDON_DIR at compile time → stage install is required.
          # GNUInstallDirs may use lib or lib64 depending on the host; resolve after stage.
          runEnv = ''
            stage_libdir() {
              if [ -e "$STAGE_DIR/lib64/libalsaplayer.so" ] || [ -e "$STAGE_DIR/lib64/libalsaplayer.so.0" ]; then
                echo "$STAGE_DIR/lib64"
              elif [ -e "$STAGE_DIR/lib/libalsaplayer.so" ] || [ -e "$STAGE_DIR/lib/libalsaplayer.so.0" ]; then
                echo "$STAGE_DIR/lib"
              else
                # Prefer cache value if present (relative libdir under prefix)
                local rel
                rel=$(grep -E '^CMAKE_INSTALL_LIBDIR:PATH=' "$CACHE" 2>/dev/null | cut -d= -f2- || true)
                if [ -n "$rel" ] && [ -d "$STAGE_DIR/$rel" ]; then
                  echo "$STAGE_DIR/$rel"
                else
                  echo "$STAGE_DIR/lib"
                fi
              fi
            }
            STAGE_LIBDIR="$(stage_libdir)"
            export LD_LIBRARY_PATH="$STAGE_LIBDIR''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
            export ALSAPLAYER_PLUGIN_DIR="$STAGE_LIBDIR/alsaplayer"
          '';

          alsaplayer-configure = pkgs.writeShellScriptBin "alsaplayer-configure" ''
            ${scriptPreamble}
            ${needReconfigureFn}
            mkdir -p "$BUILD_DIR"
            if need_reconfigure; then
              echo "configuring $PROJECT_SOURCE → $BUILD_DIR (Debug, stage=$STAGE_DIR)"
              cmake -S "$PROJECT_SOURCE" -B "$BUILD_DIR" \
                -DCMAKE_BUILD_TYPE=Debug \
                -DCMAKE_INSTALL_PREFIX="$STAGE_DIR" \
                ${lib.concatStringsSep " " cmakeFeatureFlags}
            else
              echo "cmake cache up to date ($BUILD_DIR)"
            fi
          '';

          alsaplayer-build = pkgs.writeShellScriptBin "alsaplayer-build" ''
            ${scriptPreamble}
            ${needReconfigureFn}
            mkdir -p "$BUILD_DIR"
            if need_reconfigure; then
              echo "configuring $PROJECT_SOURCE → $BUILD_DIR (Debug, stage=$STAGE_DIR)"
              cmake -S "$PROJECT_SOURCE" -B "$BUILD_DIR" \
                -DCMAKE_BUILD_TYPE=Debug \
                -DCMAKE_INSTALL_PREFIX="$STAGE_DIR" \
                ${lib.concatStringsSep " " cmakeFeatureFlags}
            fi
            cmake --build "$BUILD_DIR" -j"''${NIX_BUILD_CORES:-$(nproc)}"
            # Stage so compile-time ADDON_DIR resolves to real plugins
            cmake --install "$BUILD_DIR" --prefix "$STAGE_DIR"
            echo "staged: $BIN"
          '';

          alsaplayer-run = pkgs.writeShellScriptBin "alsaplayer-run" ''
            ${scriptPreamble}
            # Build (and reconfigure if needed) before run
            alsaplayer-build
            ${runEnv}
            echo "running: $BIN $*"
            exec "$BIN" "$@"
          '';

          alsaplayer-run-gdb = pkgs.writeShellScriptBin "alsaplayer-run-gdb" ''
            ${scriptPreamble}
            alsaplayer-build
            ${runEnv}
            echo "gdb: $BIN $*"
            # Auto-run; quit on clean exit; keep session (with backtrace) on failure
            exec gdb -q \
              -ex "set pagination off" \
              -ex "set confirm off" \
              -ex "run" \
              -ex "if \$_exitcode == 0" \
              -ex "  quit 0" \
              -ex "end" \
              -ex "echo \n*** abnormal exit / signal; backtrace:\n" \
              -ex "bt" \
              --args "$BIN" "$@"
          '';
        in
        {
          default = pkgs.mkShell {
            inputsFrom = [ self.packages.${system}.default ];
            packages = [
              pkgs.cmake
              pkgs.ninja
              pkgs.pkg-config
              pkgs.gdb
              pkgs.ccache
              alsaplayer-configure
              alsaplayer-build
              alsaplayer-run
              alsaplayer-run-gdb
            ];
            shellHook = ''
              export PROJECT_SOURCE="''${PROJECT_SOURCE:-$PWD}"
              export PROJECT_BUILD_DIR="''${PROJECT_BUILD_DIR:-/tmp/alsaplayer-build}"
              # Mirror packaged wrapper env once a stage tree exists (lib or lib64).
              _stage="''${PROJECT_BUILD_DIR}/stage"
              if [ -e "$_stage/lib64/libalsaplayer.so" ] || [ -e "$_stage/lib64/libalsaplayer.so.0" ]; then
                _libdir="$_stage/lib64"
              elif [ -e "$_stage/lib/libalsaplayer.so" ] || [ -e "$_stage/lib/libalsaplayer.so.0" ]; then
                _libdir="$_stage/lib"
              else
                _libdir="$_stage/lib"
              fi
              export LD_LIBRARY_PATH="$_libdir''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
              export ALSAPLAYER_PLUGIN_DIR="$_libdir/alsaplayer"
              echo "alsaplayer develop shell"
              echo "  PROJECT_SOURCE=$PROJECT_SOURCE"
              echo "  PROJECT_BUILD_DIR=$PROJECT_BUILD_DIR"
              echo "  alsaplayer-configure | alsaplayer-build | alsaplayer-run | alsaplayer-run-gdb"
            '';
          };
        }
      );
    };
}
