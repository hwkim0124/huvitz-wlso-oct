# Third-party libraries and binary assets — `HCT-LSO/externs/`

Every third-party C/C++ library and every binary asset the native modules depend on is listed
here with its version, origin, licence and the SHA-256 of what was imported. Update this file
whenever `externs/` changes (see `docs/claude/policies.md`). The folder itself is gitignored; this
manifest is the tracked record of what belongs in it.

Hashes: `certutil -hashfile <file> SHA256` or `sha256sum <file>`.

## Inference runtime

| Item | Version | Source | Licence | Location | SHA-256 |
|---|---|---|---|---|---|
| ONNX Runtime (CPU) | 1.22.1 | NuGet `Microsoft.ML.OnnxRuntime` 1.22.1, https://www.nuget.org/packages/Microsoft.ML.OnnxRuntime/1.22.1 | MIT | Restored by NuGet into `HCT-LSO/packages/Microsoft.ML.OnnxRuntime.1.22.1/` (referenced by `module/EyeAlign/packages.config`), not vendored here | `.nupkg`: `7b091e012dcfef2f9cc965f2252041101b1840c748ffbc1c18ba047bd6508b1c`; `runtimes/win-x64/native/onnxruntime.dll`: `7788f3f38e9a339003f7d7e1bf47f928287cf409bb5273273149e1282fbf503f` |

Used by `module/EyeAlign` (pupil-centre detection). CPU execution provider only; no CUDA, TensorRT or
DirectML provider is shipped. `onnxruntime.dll` is copied next to `eyealign.dll` into `system_libs\`.

## Models — `externs/models/`

| File | Description | Source | Licence | SHA-256 |
|---|---|---|---|---|
| `pupil_track_engine.onnx` | `UNextPupil` pupil-centre network (ONNX, IR 8, opset 17, ~1.5 M parameters). Input `input` float32 `[1,1,192,256]`; outputs `center` `[1,2]` (normalised x, y), `presence` `[1]`, `heatmap` `[1,1,48,64]` (unused). | `huvitz-wlso-image` repository, export of 2026-09-23 (`root/engines/pupil-track-unext/260923/`, identical to `root/deploy/csharp/PupilTracker/engine/`) | Huvitz internal / proprietary | `9f9544c9299cbf366efd6c56f29e644c26b5d9e2ae982b6aea08b46304f038d5` |
| `pupil_track_engine.json` | Metadata sidecar of the model (preprocessing, the exporter's presence threshold 0.5, source frame size). Informational; the native detector hard-codes the same preprocessing but defaults to a presence threshold of 0.9 (`PupilModelConfig`). | same as above | Huvitz internal / proprietary | `6319b82fd95b7ab6c675200a12d762a519bedd0b1dce7d67cf1dda401f326567` |

The build copies `externs/models/*` to `<output>\models\` (`program/*/prebuild.bat`) and on to the
deploy folder (`program/*/postbuild.bat`). The runtime path is configurable in `WsoConfig.xml`
(`AlignmentSetting/Detector@modelPath`, default `models/pupil_track_engine.onnx`).

## Pre-existing libraries (to be completed)

The following folders predate this manifest. Their entries (version, source URL, licence, SHA-256)
still have to be filled in by the owners of the respective modules:

| Folder | Used by | Notes |
|---|---|---|
| `CameraCmosOctUsb3/` | OctGrab | USB3 CMOS camera SDK |
| `FTDI/` | WsoDevice | FTDI D3XX USB bridge |
| `Spinnaker/` | WsoDevice | FLIR Spinnaker camera SDK |
| `dlib-19.24/` | CppUtil | dlib 19.24 |
| `dlls/` | pre-build copy | Redistributable runtime DLLs listed in `program/*/prebuild.bat` |
| `include/cereal`, `include/json` | various | header-only (cereal, nlohmann/json) |
| `openvino/` | SemtSegm | OpenVINO Inference Engine (2021-era API) |
| `sqlite3/` | OctResult | SQLite 3 |
