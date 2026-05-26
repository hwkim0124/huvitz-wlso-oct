# Huvitz Widefield Combined OCT and Fundus — Engineering Tool

Internal engineering / service application for the **Huvitz Widefield Combined
OCT and Fundus** imaging system. Used by development and service teams for
calibration, configuration, hardware diagnostics, and system testing. A
separate clinical end-user application will later be built on top of the same
native modules.

- **Repository:** `huvitz-wlso-oct`
- **Visual Studio solution:** `HCT-LSO/HCT-LSO.sln` (nested under `HCT-LSO/`)
- **Platform:** .NET 10 + C++17, Windows 11 22H2+ (x64 only)
- **Interop:** P/Invoke (no C++/CLI, no COM)

## Repository layout

```
huvitz-wlso-oct/                        ← git repo root
├── CLAUDE.md                           ← you are here
├── .gitignore
├── docs/
│   └── claude/                         ← lazy-loaded topic docs (see below)
└── HCT-LSO/                            ← Visual Studio solution directory
    ├── HCT-LSO.sln
    ├── Directory.Build.props           (common MSBuild settings)
    ├── NuGet.config
    ├── program/                        C# / WPF projects
    │   └── CLAUDE.md                   ← auto-loads when working in program/
    ├── module/                         C++ internal library projects
    │   └── CLAUDE.md                   ← auto-loads when working in module/
    ├── externs/                        Third-party C/C++ libraries
    ├── nupkgs/
    │   ├── local/                      Vendored .nupkg files          [CHECKED IN]
    │   └── restored/                   NuGet restore cache            [GITIGNORED]
    ├── build/                          MSBuild outputs                [GITIGNORED]
    └── deploy/                         Manually curated installer staging
```

The repo root holds **project-wide** material (this file, top-level docs, CI
configuration). The `HCT-LSO/` folder holds **the solution and everything it
builds from**. Future sibling folders at the repo root (e.g. `firmware/`,
`hardware/`, `tools/`) are fine; folders inside `HCT-LSO/` are reserved for
the C# / C++ solution.

## Detailed guidance — read when relevant

Load these on demand based on what you're working on:

- @docs/claude/architecture.md — C#/C++ split, P/Invoke ABI, threading,
  zero-copy frame display. **Read before touching Interop or native exports.**
- @docs/claude/build.md — Build / test commands, target platform details,
  output folder roles.
- @docs/claude/conventions.md — Coding standards (C# and C++), naming,
  domain glossary (LSO, OCT, OCTA, FA, FAF, ICGA, …).
- @docs/claude/policies.md — Working agreements (always / never), regulatory
  and reproducibility rules. **Read before adding dependencies or touching
  `HCT-LSO/deploy/`, `HCT-LSO/externs/`, or `HCT-LSO/nupkgs/`.**

Per-area `CLAUDE.md` files auto-load when working in that subtree:

- `HCT-LSO/program/CLAUDE.md` — C# / WPF specifics
- `HCT-LSO/module/CLAUDE.md` — C++ native specifics

## Quick rules (the rest is in policies.md)

- **x64 only.** No AnyCPU, no x86.
- **P/Invoke only.** All `[DllImport]` declarations live in the **Interop**
  project under `HCT-LSO/program/` — never in app or ViewModel code.
- **Never edit** `HCT-LSO/build/`, `HCT-LSO/nupkgs/restored/`, or anything
  under `HCT-LSO/externs/` or `HCT-LSO/deploy/` without explicit approval.
- **Propose diffs first** for any change that touches target framework,
  platform target, package versions, or installer contents.
