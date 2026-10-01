{
  description = "AlsaPlayer - PCM audio player for Linux";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    alsaplayer = {
      url = "github:alsaplayer/alsaplayer";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, alsaplayer }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in
    {
      packages = forAllSystems (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};

          # Extract version from the AC_INIT line in configure.ac
          # AC_INIT([alsaplayer],[0.99.82],...)
          version =
            let
              configureAc = builtins.readFile "${alsaplayer}/configure.ac";
              lines = pkgs.lib.splitString "\n" configureAc;
              initLines = builtins.filter
                (l: builtins.match "AC_INIT.*" l != null)
                lines;
              line = if initLines != [] then builtins.head initLines else "";
              # Character classes for literal [ ] — \[ is invalid in Nix ERE
              match = builtins.match
                ''AC_INIT\([[]alsaplayer[]],[[]([0-9.]+)[]].*''
                line;
            in
            if match != null then builtins.head match else "0.99.82";
        in
        {
          default = pkgs.stdenv.mkDerivation {
            pname = "alsaplayer";
            inherit version;

            src = alsaplayer;

            nativeBuildInputs = with pkgs; [
              autoreconfHook
              pkg-config
              intltool
              gettext
              makeWrapper
            ];

            buildInputs = with pkgs; [
              alsa-lib
              gtk2
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
              xorg.libX11
              zlib
            ];

            configureFlags = [
              "--enable-alsa"
              "--enable-jack"
              "--enable-gtk2"
              "--enable-mad"
              "--enable-flac"
              "--enable-oggvorbis"
              "--enable-mikmod"
              "--enable-sndfile"
              "--enable-opengl"
              "--disable-systray"
              "--disable-esd"
            ];

            preConfigure = ''
              ./autogen.sh
            '';

            postInstall = ''
              wrapProgram $out/bin/alsaplayer \
                --prefix LD_LIBRARY_PATH : "$out/lib" \
                --set ALSAPLAYER_PLUGIN_DIR "$out/lib/alsaplayer"
            '';

            meta = with pkgs.lib; {
              description = "Heavily multi-threaded PCM player that exercises the ALSA library";
              homepage = "https://alsaplayer.sourceforge.net/";
              license = licenses.gpl3Plus;
              platforms = platforms.linux;
              mainProgram = "alsaplayer";
            };
          };
        });

      apps = forAllSystems (system: {
        default = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/alsaplayer";
        };
      });
    };
}
