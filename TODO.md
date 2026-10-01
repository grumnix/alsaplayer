# TODO — grumnix/alsaplayer flake work

## Current tip

- Branch: `master` (fork of upstream alsaplayer)
- Goal: working Nix flake that builds alsaplayer from this tree

## Done

- [x] Port GUI from GTK2 to GTK3 (`interface/gtk3/`, configure `--enable-gtk3`, flake uses `gtk3`)

- [x] Add initial `flake.nix` / `flake.lock` (commit bbd6b69)
- [x] Move source to `self` (no external source input)
- [x] Fix version extraction without Nix ERE (`lib.splitString`)
- [x] `makeWrapper`, `intltoolize`, `--disable-esd`
- [x] Document source tree + dead code in **AGENTS.md** (attic ~half the tree, `old_playlist.h`, obsolete m4, niche outputs)

## Open

- [ ] Build/run GTK3 UI: `nix build -L .` and manual smoke test
- [ ] Fix any remaining GTK3 deprecation warnings (override_* → CSS, StatusIcon)

- [ ] Confirm `nix build -L .` succeeds
- [ ] Confirm plugins under `$out/lib/alsaplayer` and runtime discovery
- [ ] Optional: tighter `cleanSource` / filter to drop `attic/` from the derivation src
- [ ] Optional cleanup (separate decision): remove `alsaplayer/old_playlist.h`; drop unused `m4/gtk.m4`, `m4/qt.m4`
- [ ] Optionally `--enable-systray`
- [ ] Optional outputs later: xosd, nas
- [ ] Modern GCC / gtk2 fixes if the build surfaces any

## Notes

- **`attic/` is not in `SUBDIRS`** — never built. ~3.0 MB / ~51 kLOC vs ~30 kLOC active.
- Version from `AC_INIT` via string splits. `src = lib.cleanSource self`.
- Author: Ingo Ruhnke <grumbel@gmail.com>
- Co-authored-by: Grok <grok@x.ai>

## Handoff

Next: run `nix build -L .`, fix configure/build errors, update this file, emit cumulative git bundle from base `05ca204`.
