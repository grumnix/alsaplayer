# AGENTS.md — alsaplayer (grumnix fork)

## Project

Nix packaging / flake for [AlsaPlayer](https://github.com/alsaplayer/alsaplayer), a plugin-based PCM audio player (ALSA, JACK, GTK2 UI, MAD/FLAC/Vorbis/etc.).

Upstream: `https://github.com/alsaplayer/alsaplayer`. This fork adds a Nix flake and packaging notes.

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

Flake uses `self` as source. Version is parsed from `configure.ac` without regex (Nix ERE rejects `\[`).

---

## Source tree overview

Top-level layout (checked-in tree ≈ **5.8 MB**; **`attic/` alone ≈ 3.0 MB**):

| Path | Role | Built? |
|------|------|--------|
| `app/` | Core player (Main, CorePlayer, Playlist, AlsaNode, …) | yes |
| `alsaplayer/` | Public headers / plugin API | installed headers |
| `libalsaplayer/` | Control-socket client library | yes |
| `interface/` | UI plugins: **gtk3** (ported from gtk2), text, daemon, xosd | conditional |
| `interface/gtk2/` | Original GTK2 UI sources (not built) | **no** |
| `input/` | Decoders: mad, flac, vorbis, mikmod, sndfile, cdda | conditional |
| `output/` | Backends: alsa, jack, oss, null, **esound**, nas, sgi, sparc | conditional |
| `reader/` | file + http readers | yes |
| `scopes2/` | Visualization plugins (monoscope, blurscope, …) | yes (OpenGL ones need GL) |
| `examples/` | libalsaplayer sample clients | yes |
| `extra/` | desktop file, icon, theme | yes |
| `po/`, `m4/`, `docs/` | i18n, autoconf macros, doxygen | partial |
| **`attic/`** | Historical / unfinished plugins and UIs | **no** (not in `SUBDIRS`) |

Root `Makefile.am` `SUBDIRS`:

```text
po libalsaplayer interface output input app alsaplayer extra docs
examples m4 reader scopes2
```

`attic` is never built or installed by the normal autotools flow.

Approximate C/C++ size:

- **Active tree** (excluding `attic/`): ~30 kLOC
- **`attic/` only**: ~51 kLOC  
  So the majority of checked-in C/C++ is inactive.

---

## Dead / inactive code

### 1. Entire `attic/` (not built)

Intentionally parked code. Safe to ignore for packaging; do not delete without an explicit upstream/product decision.

| Subtree | What it is | Notes |
|---------|------------|--------|
| `attic/midi/` (~1 MB) | TiMidity-derived MIDI **input** plugin | Separate `configure`/`bootstrap`; needs soundfonts |
| `attic/python/` | Old Python bindings to the control API | Standalone `setup.py` |
| `attic/fftscope/` | FFT scope as a **separate** mini-project | Duplicates ideas later in `scopes2/` |
| `attic/experimental/ape/` | Monkey’s Audio input | Experimental Makefile only |
| `attic/experimental/ffmpeg/` | FFmpeg input | Experimental |
| `attic/experimental/tta/` | TTA lossless input | Experimental |
| `attic/experimental/wv/` | WavPack input | Experimental |
| `attic/experimental/obsolete/interface/gtk/` | **GTK1** UI (Glade-era) | Replaced by `interface/gtk2/` |
| `attic/experimental/obsolete/interface/qt/` | **Qt3-era** UI (Rik Hemsley, 2001) | Never productized in current tree |
| `attic/experimental/obsolete/input/mad/` | MAD engine **plus vendored libmad sources** | Current tree uses system `libmad` (`input/mad/`) |
| `attic/experimental/obsolete/scopes/` | Older scope plugins | Superseded by `scopes2/` |
| `attic/experimental/obsolete/fftscope/` | Another fftscope copy | Redundant |

### 2. Dead file still in the *active* tree

- **`alsaplayer/old_playlist.h`** (~279 lines)  
  - Not listed in `alsaplayerinclude_HEADERS` or `EXTRA_DIST` in a way that matters for install of this name.  
  - **No `#include` references** anywhere in the tree.  
  - Appears to be an abandoned earlier playlist API; live code uses `Playlist.h` / `Playlist.cpp`.  
  - Candidate for removal in a cleanup commit (coordinate with upstream if aiming to push back).

### 3. Leftover autoconf macros (`m4/`)

Still shipped; several are unused by current `configure.ac`:

- **`m4/gtk.m4`** — classic GTK+ **1.x** `AM_PATH_GTK` (1997). UI detection is **pkg-config gtk+-2.0**.
- **`m4/qt.m4`** — Qt detection for the dead Qt interface.
- **`m4/esd.m4`** — EsounD; still referenced if ESD is enabled, but ESD itself is obsolete on modern desktops.

Other gettext/iconv macros are the usual gettextize set; noisy but expected.

### 4. Output plugins that are effectively dead on modern Linux

Still in `output/` and `configure.ac`, built only when headers/libs exist:

| Plugin | Status on typical Linux |
|--------|-------------------------|
| `output/esound/` | EsounD abandoned (GNOME → Pulse → PipeWire) |
| `output/nas/` | Network Audio System; rare |
| `output/sgi/` | IRIX-only |
| `output/sparc/` | Sun/`sys/audioio.h` |

Kept for portability; the flake disables ESD. ALSA + JACK (+ optional OSS/null) are the practical set.

### 5. Historical bulk (not code, but tree weight)

| File | ~Size | Note |
|------|-------|------|
| `ChangeLog` | 316 KB | Long project history |
| `ChangeLog.old` | 62 KB | Older log |
| `ABOUT-NLS` | 94 KB | Standard gettext boilerplate |

Harmless for builds; dominate non-`attic` text weight together with `po/`.

### 6. “Soft” dead / cleanup-marked code (still linked)

- **`app/Effects.cpp`** — header comment: *“Various effects implementations, needs a cleanup”*. Still called (`init_effects`, `volume_effect32`, etc.).
- **`app/reverbst.cpp`**, **`app/convolve.c`**, **`app/fft.c`** — DSP helpers; used by effects/scopes path; not removed.
- **Systray** (`StatusIcon`) — optional, off by default (`--enable-systray`); needs gtk ≥ 2.10.

These are not safe to delete without behavior analysis.

---

## Active plugin map (what actually matters)

**Interfaces:** **gtk3** (primary; ported from gtk2), text, daemon, xosd (if libxosd). GTK2 sources remain under `interface/gtk2/` but are not in `SUBDIRS`.  
**Inputs:** mad (libmad), flac, vorbis, mikmod, sndfile, cdda (`linux/cdrom.h`).  
**Outputs:** alsa, jack, oss, null; others niche.  
**Readers:** file, http.  
**Scopes:** under `scopes2/` (OpenGL spectrum needs GL).

README still mentions an older mpg123-based MPEG plugin as being phased out in favor of MAD; **no mpg123 sources remain** in the active tree (only MAD).

---

## Packaging implications

- Do **not** treat `attic/` as build inputs; excluding it from `src` (e.g. tighter `cleanSource` filter) would shrink the Nix store footprint but is optional.
- Optional configure flags can stay minimal: ALSA, JACK, GTK2, MAD, FLAC, Vorbis, MikMod, sndfile, OpenGL scopes; disable ESD/systray unless requested.
- Removing `old_playlist.h` or pruning `m4/gtk.m4` / `m4/qt.m4` is a separate cleanup, not required for a working flake.

## Handoff

See **TODO.md** for flake status and next build/verify steps.


## GTK3 UI port

The active GUI is **`interface/gtk3/`**, derived from `interface/gtk2/`.

Mechanical / API changes applied:

- `gtk_box_new` / `gtk_scale_new` with orientation (replacing hbox/vbox/hscale)
- Tooltips via `gtk_widget_set_tooltip_text` (no `GtkTooltips`)
- Stock items → icon names / mnemonics
- `gtk_menu_shell_append`, `gtk_dialog_get_content_area`
- `GDK_KEY_*` keysyms; removed `GDK_THREADS_*`, `gtk_exit`, `gtk_set_locale`, `gdk_rgb_init`, `gtk_rc_parse`
- Style: `gtk_widget_override_{color,background_color,font}` (deprecated but functional on GTK3)
- Systray still uses `GtkStatusIcon` (deprecated in 3.14, absent in GTK4)

`configure.ac` checks `gtk+-3.0`; flag `--enable-gtk3`. Flake depends on `gtk3`.


### Scopes and GTK3

Most visualization plugins under `scopes2/` (monoscope, blurscope, levelmeter, …) still call **removed GDK drawing APIs** (`gdk_draw_indexed_image`, `GdkRgbCmap`, `widget->style->white_gc`, `GDK_THREADS_*`). They are **not built** with the GTK3 port until rewritten with Cairo/`draw`.

`scopes2/Makefile.am` currently only builds **`opengl_spectrum`**.

Known follow-ups: replace override_* with CSS providers; migrate off StatusIcon; verify DnD and file chooser on GTK 3.24+.


## CMake

Primary build path for the flake is **CMake** (autotools remain in-tree).

```bash
cmake -B build -DENABLE_GTK3=ON -DENABLE_ALSA=ON
cmake --build build
cmake --install build
```

Plugins install under `$prefix/lib/alsaplayer/{interface,input,output,reader,scopes2}/`.
Default UI plugin name: `gtk3` (`libgtk3_interface.so`).
