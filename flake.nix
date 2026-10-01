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
    in
    {
      packages = forAllSystems (
        system:
        let
          pkgs = nixpkgs.legacyPackages.${system};
          lib = pkgs.lib;

          # Parse version from configure.ac without regex.
          # AC_INIT([alsaplayer],[0.99.82],...)
          version =
            let
              configureAc = builtins.readFile (self + "/configure.ac");
              lines = lib.splitString "\n" configureAc;
              initLine =
                lib.findFirst (l: lib.hasPrefix "AC_INIT" l) "" lines;
              # Split on "[" → [ "AC_INIT(", "alsaplayer],", "0.99.82],", ... ]
              parts = lib.splitString "[" initLine;
              verField =
                if builtins.length parts >= 3 then builtins.elemAt parts 2 else "";
              # "0.99.82]," → "0.99.82"
              ver = builtins.head (lib.splitString "]" verField);
            in
            if ver != "" then ver else "0.99.82";
        in
        {
          default = pkgs.stdenv.mkDerivation {
            pname = "alsaplayer";
            inherit version;

            src = lib.cleanSource self;

            nativeBuildInputs = with pkgs; [
              autoreconfHook
              pkg-config
              intltool
              gettext
              makeWrapper
            ];

            buildInputs = with pkgs; [
              alsa-lib
              gtk3
              glib
              libjack2
              libmad
              libid3tag
              flac
              libogg
              libvorbis
              libmikmod
              libsndfile
              libGL
              libGLU
              libx11
              zlib
            ];

            configureFlags = [
              "--enable-alsa"
              "--enable-jack"
              "--enable-gtk3"
              "--enable-mad"
              "--enable-flac"
              "--enable-oggvorbis"
              "--enable-mikmod"
              "--enable-sndfile"
              "--enable-opengl"
              "--disable-systray"
              "--disable-esd"
            ];

            # intltoolize is required; autoreconfHook alone is not enough
            preConfigure = ''
              intltoolize --force --copy --automake
            '';

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
    };
}
