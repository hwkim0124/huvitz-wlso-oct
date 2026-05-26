# Conventions & Glossary

## C# coding standards

- `Nullable` enabled.
- `TreatWarningsAsErrors=true`.
- `LangVersion=latest`.
- `AnalysisLevel=latest-recommended`. Heed **CA1416** (platform compatibility).
- Async methods suffixed `Async`.
- `.editorconfig` at the repo root governs formatting; do not override it
  per-project without discussion.

### MVVM (CommunityToolkit.Mvvm)

- ViewModels inherit from `ObservableObject` or `ObservableValidator`.
- Properties: `[ObservableProperty] private T _field;` (source-generated).
- Commands: `[RelayCommand]` on methods. Async commands return `Task`.
- Views (`.xaml` / `.xaml.cs`) contain no logic beyond construction and
  trivial event-to-command plumbing.
- DI via `Microsoft.Extensions.DependencyInjection`. ViewModels and
  services registered in an `App.xaml.cs` composition root.

## C++ coding standards

- C++17, `/W4`, **warnings-as-errors**.
- RAII over manual lifetime: no raw `new` / `delete` in new code —
  use `std::unique_ptr`, `std::shared_ptr`, or stack objects.
- No exceptions across the C ABI boundary (see `architecture.md`).
- `clang-format` config at the repo root; format on save expected.
- Headers in `HCT-LSO/module/<lib>/include/`; one ABI header per
  exported lib, named `<lib>_api.h`.
- Include order: own header, then C++ stdlib, then third-party, then Windows.

## Naming

- C# types and members: PascalCase. Private fields: `_camelCase`.
- C# interfaces: `IFoo`.
- C++ types: `PascalCase`. Functions: `lowerCamelCase`. Free functions
  exported from a DLL: `<Module>_<Verb><Noun>` (e.g. `OCT_StartScan`).
- File names match the primary type they declare.

## Logging

- C#: `Microsoft.Extensions.Logging` with structured logging.
  No `Console.WriteLine`, no `Debug.WriteLine` in production code paths.
- C++: a single shared logger interface lives in `HCT-LSO/module/common/`.
  Native modules write through it; messages surface to the C# side via
  a registered log callback.

## Domain glossary

| Term | Meaning |
|---|---|
| **LSO**  | Line Scanning Ophthalmoscope — fundus imaging modality used in this product. |
| **OCT**  | Optical Coherence Tomography — cross-sectional retinal imaging. |
| **OCTA** | OCT Angiography — motion-contrast vascular imaging from repeated B-scans. |
| **FA**   | Fluorescein Angiography. |
| **FAF**  | Fundus Autofluorescence. |
| **ICGA** | Indocyanine Green Angiography. |
| **A-scan** | Axial (depth) profile at a single transverse position. |
| **B-scan** | Cross-section: a series of A-scans along a line. |
| **C-scan** / **en-face** | A plane parallel to the retinal surface, derived from a volume. |
| **Galvo** | Galvanometer scanner — X/Y beam steering. |
| **k-clock** | Linear-in-wavenumber sampling clock for swept-source OCT. |
| **Dispersion compensation** | Numerical correction for mismatched dispersion between OCT reference and sample arms. |
| **Macula / Optic Disc / Anterior** | Standard OCT scan-region patterns supported by this product. |
| **Wide-field** | Imaging field substantially larger than the conventional ~30°–50° fundus view. |
