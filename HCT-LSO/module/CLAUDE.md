# `HCT-LSO/module/` — C++ native libraries

This subtree contains the native DLLs that the engineer tool (and
later the clinical app) consume via P/Invoke: OCT acquisition and
processing, LSO acquisition and processing, calibration algorithms,
hardware I/O, and shared utilities.

## What lives here

Each native lib is its own `.vcxproj`, exporting a thin C ABI defined
in `include/<lib>_api.h`. Internal implementation may use modern C++17
freely; the **only constraint is at the export boundary**.

## Rules specific to this area

- **Exports follow the C ABI.** `extern "C" __declspec(dllexport)`,
  `__stdcall`, opaque `void*` handles, integer return codes, no
  exceptions across the boundary. Full rules in
  @../../docs/claude/architecture.md.
- **Build settings:** Platform Toolset `v143`,
  `WindowsTargetPlatformVersion=10.0.22621.0`,
  `_WIN32_WINNT=0x0A00`. x64 only.
- **One source-of-truth ABI header per lib**, in
  `HCT-LSO/module/<lib>/include/<lib>_api.h`. Every change to this
  header requires a matching change in the C# Interop project, **in
  the same commit**.
- **Struct changes** require updating the startup `sizeof` self-check
  that asserts C++ `sizeof` matches C# `Marshal.SizeOf`. Without it,
  packing/alignment drift becomes silent corruption.
- **No raw `new` / `delete`** in new code. Use `std::unique_ptr` and
  RAII. Smart pointers must not cross the C ABI — convert to raw
  `void*` at the boundary.
- **Logging** goes through the shared logger in
  `HCT-LSO/module/common/`, which surfaces to C# via a registered
  callback. No `printf` or `std::cout` in production code paths.

## Threading

- Acquisition and hardware threads are native. They are not managed
  threads and have no `Dispatcher`. Native code must not assume any
  particular thread context when invoking the registered callbacks.
- Callbacks should be cheap — copy pointer + metadata, signal the
  managed side, return promptly. Long-running work must not run on
  the acquisition thread.

## Related docs

- @../../docs/claude/architecture.md — ABI rules, callback patterns, threading
- @../../docs/claude/conventions.md  — C++ coding standards, naming, includes
- @../../docs/claude/build.md         — build / test, toolset versions
- @../../docs/claude/policies.md      — what requires approval (esp. `externs/`)
