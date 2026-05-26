# Build, Test & Target Platform

## Target platform

- **.NET 10** (LTS).
- **Target Framework Moniker:** `net10.0-windows10.0.22621.0`
- **Minimum Windows:** Windows 11 22H2 (build **22621**), declared via
  `SupportedOSPlatformVersion=10.0.22621.0`.
- **Architecture: x64 only.** No AnyCPU, no x86.
- **C++ projects:**
  - Platform Toolset **v143** (VS 2022).
  - `WindowsTargetPlatformVersion=10.0.22621.0`.
  - `_WIN32_WINNT=0x0A00`, `WINVER=0x0A00`, `NTDDI_VERSION=NTDDI_WIN10_NI`.
- **Toolchain:** Visual Studio 2022 (17.12+), Windows 11 SDK 10.0.22621.x,
  .NET 10 SDK.

Common C# settings live in `HCT-LSO/Directory.Build.props`. **Do not
duplicate them in individual `.csproj` files.**

## Build

```powershell
# From a VS 2022 Developer PowerShell at the repo root (huvitz-wlso-oct/):
msbuild HCT-LSO/HCT-LSO.sln /p:Configuration=Debug   /p:Platform=x64 -m
msbuild HCT-LSO/HCT-LSO.sln /p:Configuration=Release /p:Platform=x64 -m

# Or from inside HCT-LSO/:
cd HCT-LSO
msbuild HCT-LSO.sln /p:Configuration=Debug /p:Platform=x64 -m
```

Or just open `HCT-LSO/HCT-LSO.sln` in Visual Studio 2022.

## Output folder roles

All paths below are relative to the repo root.

| Folder                       | Role                                          | Status |
|---                           |---                                            |---|
| `HCT-LSO/build/`             | All MSBuild outputs (centralized).            | Generated, **gitignored**. Never edit by hand. |
| `HCT-LSO/nupkgs/restored/`   | NuGet restore cache.                          | Generated, **gitignored**. |
| `HCT-LSO/nupkgs/local/`      | Vendored / locally produced `.nupkg` files.   | Checked in. Reviewed changes only. |
| `HCT-LSO/externs/`           | Third-party C/C++ libraries.                  | Checked in. Reviewed changes only. |
| `HCT-LSO/deploy/`            | Manually curated installer staging area.      | Hand-curated. See `HCT-LSO/deploy/README.md` for the manifest. |

After a Release build, the team **manually** copies the required files
into `HCT-LSO/deploy/` per the install manifest. This is deliberate — see
`policies.md` for the reasoning.

## Tests

```powershell
# C# unit tests (xUnit)
dotnet test HCT-LSO/program/<...>.Tests

# Native tests (GoogleTest via CTest)
ctest --test-dir HCT-LSO/build/native --output-on-failure
```

- C# tests use **xUnit** with `Microsoft.NET.Test.Sdk`.
- Hardware-in-the-loop tests are tagged `[HIL]` (xUnit trait) or
  `HardwareRequired` (GoogleTest filter) and are **skipped in CI**.
- HIL tests run on bench-side workstations with the instrument connected.

## Local prerequisites

If a build fails out of the box, verify:

1. `dotnet --list-sdks` shows a 10.0.x entry.
2. Visual Studio Installer → Individual components → Windows 11 SDK
   (10.0.22621.x) is installed.
3. The C++ workload ("Desktop development with C++") is installed with
   the v143 toolset.
4. `git lfs` is installed if any prebuilt binaries in `HCT-LSO/externs/`
   use LFS.
