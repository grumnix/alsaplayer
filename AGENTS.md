# AGENTS.md — alsaplayer (grumnix fork)

## Project

Nix packaging / flake for [AlsaPlayer](https://github.com/alsaplayer/alsaplayer), a plugin-based PCM audio player (ALSA, JACK, GTK2 UI, MAD/FLAC/Vorbis/etc.).

Upstream lives at `https://github.com/alsaplayer/alsaplayer`. This fork adds a Nix flake.

## Rules

- Read **TODO.md** for current tip and open work.
- Deliverables are **git bundles** only (no patch files). See session instructions for naming and cumulative range.
- Author: `Ingo Ruhnke <grumbel@gmail.com>`
- Trailer on every commit: `Co-authored-by: Grok <grok@x.ai>`
- Prefer correct design over quick hacks; document outcomes in TODO.md.

## Build

```bash
nix build -L .
nix run
```

Flake uses `self` as source. Version is parsed from `configure.ac` without regex.
