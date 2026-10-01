# TODO — grumnix/alsaplayer flake work

## Current tip

- Branch: `master` (fork of upstream alsaplayer)
- Tip: `cab99b9` flake: add libsysprof-capture and libopus for pkg-config
- Goal: working Nix flake that builds alsaplayer from this tree
- Base of this work line: `05ca204`

## Done

- [x] CMake build system; flake uses cmake instead of autoreconf

- [x] Port GUI from GTK2 to GTK3 (`interface/gtk3/`, configure `--enable-gtk3`, flake uses `gtk3`)

- [x] Add initial `flake.nix` / `flake.lock` (commit bbd6b69)
- [x] Move source to `self` (no external source input)
- [x] Fix version extraction without Nix ERE (`lib.splitString`)
- [x] `makeWrapper`, `intltoolize`, `--disable-esd`
- [x] Document source tree + dead code in **AGENTS.md** (attic ~half the tree, `old_playlist.h`, obsolete m4, niche outputs)
- [x] Harden string handling: ap_strlcpy/cat, path joins, shuffle
- [x] CMake: force HAVE_LINUX_CDROM_H on cdda target
- [x] Fix alsaplayer.pc generation for absolute CMAKE_INSTALL_LIBDIR/INCLUDEDIR (Nix)
- [x] Confirm `nix build -L .` succeeds
- [x] Confirm plugins under `$out/lib/alsaplayer` (input, output, interface, reader, scopes2)
- [x] `alsaplayer --help` reports default interface `gtk3`
- [x] Clear compiler warnings (no silencing):
  - FLAC null-this in meta/err callbacks → alsaplayer_error
  - cdda CDDB snprintf truncation → larger msg buffer + return checks
  - daemon/http write() unused result → check and handle
  - message.c sscanf pattern buffer sizing
  - flake: libsysprof-capture + libopus for pkg-config private deps

## Open

- [ ] Manual smoke test of GTK3 UI (needs display / audio device)
- [ ] Fix any remaining GTK3 deprecation warnings (override_* → CSS, StatusIcon)
- [ ] Port scopes2/* (except opengl_spectrum) from gdk_draw_* to Cairo; re-enable in scopes2/Makefile.am (and CMake if needed)
- [ ] Optional: tighter `cleanSource` / filter to drop `attic/` from the derivation src
- [ ] Optional cleanup (separate decision): remove `alsaplayer/old_playlist.h`; drop unused `m4/gtk.m4`, `m4/qt.m4`
- [ ] Optionally `--enable-systray` / ENABLE_SYSTRAY
- [ ] Optional outputs later: xosd, nas
- [ ] Runtime plugin discovery smoke test beyond path layout
- [ ] Re-run `nix build -L .` after warning fixes (local cmake build is clean)

## Notes

- **`attic/` is not in `SUBDIRS`** — never built. ~3.0 MB / ~51 kLOC vs ~30 kLOC active.
- Version from `AC_INIT` via string splits. `src = lib.cleanSource self`.
- Flake cmakeFlags enable GTK3, ALSA, JACK, OSS, MAD, FLAC, Vorbis, MikMod, sndfile, CDDA, OpenGL; SYSTRAY and NLS off.
- Author: Ingo Ruhnke <grumbel@gmail.com>
- Co-authored-by: Grok <grok@x.ai>

## Handoff

Next optional work: GTK3 deprecation cleanup, Cairo port of remaining scopes, or tighter cleanSource.
Local cmake build is warning-free after the fixes above.
Bundle from base `05ca204` is cumulative through the warning cleanup.
