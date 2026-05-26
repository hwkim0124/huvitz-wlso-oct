# Policies & Working Agreements

This is a medical-imaging product. Decisions about dependencies,
versions, and shipped artifacts are not just engineering choices —
they have regulatory consequences. The rules below exist to keep the
repo audit-friendly without slowing day-to-day development.

## Reproducibility & medical-device hygiene

- **NuGet lock files are enabled** on every C# project
  (`RestorePackagesWithLockFile=true`). The generated `packages.lock.json`
  files **are checked in**. CI uses `RestoreLockedMode=true` to fail on
  drift.
- **`HCT-LSO/externs/MANIFEST.md`** lists every third-party C/C++ library:
  name, version, source URL, license, and the SHA-256 of the imported
  archive or commit. Update it whenever `HCT-LSO/externs/` changes.
- **`HCT-LSO/deploy/README.md`** is the install-package manifest: every
  file that ships, where it comes from, and its SHA-256. Update it
  whenever `HCT-LSO/deploy/` changes.
- Adding any new NuGet package, any new library in `HCT-LSO/externs/`,
  or any file to `HCT-LSO/deploy/` is a **reviewed change** — never
  automated, never bundled into an unrelated PR.

## Working agreements for Claude

### Always

- Read the root `CLAUDE.md` at the start of every session, plus any
  auto-loaded per-area `CLAUDE.md` for the subtree you are working in.
- When adding a new native export: update **both** the C++ header in
  `HCT-LSO/module/<lib>/include/<lib>_api.h` AND the C# Interop mirror
  in the same change, and run the build.
- When changing a native struct: update both sides AND extend the
  startup `sizeof` self-check that asserts C++ `sizeof` == C#
  `Marshal.SizeOf`.
- Prefer **adding** to the Interop wrapper layer over sprinkling new
  `[DllImport]` declarations through the codebase.
- Propose diffs and wait for approval before touching anything under
  `HCT-LSO/deploy/`, `HCT-LSO/externs/`, `HCT-LSO/build/`, or
  `HCT-LSO/nupkgs/`.

### Never (without explicit approval)

- Add or upgrade NuGet packages — licensing + lock-file impact.
- Add or modify third-party libraries under `HCT-LSO/externs/` —
  licensing + audit-trail impact.
- Change `TargetFramework`, `SupportedOSPlatformVersion`,
  `PlatformToolset`, `Platforms`, or `PlatformTarget`.
- Edit anything under `HCT-LSO/build/` or `HCT-LSO/nupkgs/restored/` —
  they are regenerated outputs.
- Edit files in `HCT-LSO/deploy/` — that folder is hand-curated to
  match the install-package manifest. Propose changes; the engineer
  applies.
- Introduce C++/CLI, COM, or IPC for interop. **P/Invoke only.**
- Commit binaries, generated artifacts, secrets, signing keys, or
  developer-specific files.

## Branch & commit hygiene

- Work on a topic branch off `main` (or whatever the integration
  branch is named). Never commit directly to the integration branch.
- Commit messages: imperative mood, one logical change per commit.
- A change that crosses the C++/C# boundary is still **one** commit
  (header + Interop mirror + tests together), not three.
- Before letting any automated edit touch the working tree, ensure
  the tree is clean. Easy rollback is a feature of small commits.

## Asking before acting

When unsure whether a change falls under "Always" or "Never":
**propose the diff and ask**. The cost of a clarifying turn is far
lower than the cost of a regressed installer or a surprise dependency.
