# TODO — grumnix/alsaplayer flake work

## Current tip

- Branch: `master` (fork of upstream alsaplayer)
- Goal: working Nix flake that builds alsaplayer from this tree

## Done

- [x] Add initial `flake.nix` / `flake.lock` (commit bbd6b69)
- [x] Move source to `self` (flake lives in the project; no external source input)
- [x] Fix version extraction: Nix ERE rejects `\[`; use `lib.splitString` instead
- [x] Add `makeWrapper` for `wrapProgram`
- [x] Disable obsolete ESD plugin
- [x] Keep `intltoolize` in `preConfigure` (required beyond plain `autoreconfHook`)

## Open

- [ ] Confirm `nix build` succeeds on a machine with Nix
- [ ] Confirm plugins land under `$out/lib/alsaplayer` and runtime finds them
- [ ] Optionally enable `--enable-systray` if desired
- [ ] Consider packaging optional deps (xosd, nas) as separate outputs later
- [ ] Upstream or document any patches needed for modern GCC / gtk2

## Notes

- Version is read from `AC_INIT` in `configure.ac` via string splits (no regex).
- `src = lib.cleanSource self` so the flake builds the local tree.
- After changing inputs, run `nix flake lock` if the lockfile needs refresh.
- Author for commits: Ingo Ruhnke <grumbel@gmail.com>
- Co-authored-by: Grok <grok@x.ai>

## Handoff

Next agent: run `nix build -L .` from the repo root, fix any remaining configure/build errors, update this file, then produce a cumulative git bundle per AGENTS rules.
