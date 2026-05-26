# Architecture & Interop

## High-level shape

- **C# / WPF / .NET 10** — engineer-tool UI, workflow, orchestration.
- **MVVM** via `CommunityToolkit.Mvvm`. No business logic in code-behind.
- **C++17 native DLLs** under `HCT-LSO/module/` — device control, acquisition,
  DSP, calibration algorithms, OCT/OCTA processing.
- **Interop: P/Invoke only.** No C++/CLI, no COM, no IPC.
- All `[DllImport]` declarations live in a dedicated **Interop** project
  under `HCT-LSO/program/`. App and ViewModel projects do not contain
  P/Invoke directly — they consume the managed wrapper classes Interop
  exposes.

Native DLLs are post-build-copied into the C# app's output folder, and
from there manually staged into `HCT-LSO/deploy/` per the install manifest.

## Native ABI rules (`HCT-LSO/module/`)

Every export crossing the managed boundary must follow these rules:

- `extern "C" __declspec(dllexport)` only.
- Calling convention: **`__stdcall`** on every export.
  Match `CallingConvention.StdCall` on the C# side.
- Use **opaque `void*` handles**, exposed as `IntPtr` or (preferred)
  `SafeHandle` in C#. Never expose C++ classes, STL containers, or
  exceptions across the ABI.
- **Never throw across the boundary.** Return `int` error codes.
  Provide `int GetLastErrorMessage(void* handle, char* buf, int len)`
  for human-readable diagnostics.
- Pass image buffers as caller-allocated **`IntPtr + length`**.
  Avoid marshalled `byte[]` for hot paths.
- One source-of-truth header per native lib:
  `HCT-LSO/module/<lib>/include/<lib>_api.h`. The C# Interop mirror must
  be kept in lockstep with this header in the same change-set.

### Structs

- `[StructLayout(LayoutKind.Sequential, Pack = N)]` on the C# side must
  match `#pragma pack(push, N)` on the C++ side.
- Prefer blittable types (primitives, fixed-size primitive arrays,
  pointers). Avoid `bool` across the boundary — use `int` (0/1) and
  marshal explicitly, since Win32 `BOOL` is 4 bytes but C# `bool` defaults
  to 4 bytes only with `[MarshalAs(UnmanagedType.I4)]`.
- Whenever a struct changes, **extend the startup `sizeof` self-check**
  to assert C++ `sizeof` == C# `Marshal.SizeOf`.

### Callbacks (the #1 P/Invoke crash bug)

```cpp
typedef void (__stdcall *FrameReadyCallback)(void* userData,
                                              const FrameInfo* info);

extern "C" __declspec(dllexport)
int OCT_RegisterFrameCallback(void* handle,
                               FrameReadyCallback cb, void* userData);
```

```csharp
public delegate void FrameReadyDelegate(IntPtr userData, ref FrameInfo info);

// CRITICAL: keep the delegate alive in a long-lived field, otherwise
// the GC will collect it while native code still holds the function
// pointer — instant crash, often hours later, hard to reproduce.
private FrameReadyDelegate _frameCallback;
```

Native callbacks arrive on **worker threads**. Always marshal to the WPF
UI thread via `Dispatcher.BeginInvoke` (or `await Dispatcher.InvokeAsync`)
before touching any UI state.

## Live image display — zero-copy via WriteableBitmap

The preferred pattern for OCT B-scans and LSO frames:

1. WPF code creates a `WriteableBitmap` sized for the frame.
2. On each frame: `bitmap.Lock()` → pass `bitmap.BackBuffer` (an `IntPtr`)
   plus stride and dimensions to native code → native fills the buffer →
   `bitmap.AddDirtyRect(...)` → `bitmap.Unlock()`.
3. No marshalled copy. The bitmap flows straight to the GPU.

The Interop layer should expose a method like
`void FillFrame(IntPtr destBuffer, int stride, in FrameRequest request)`
rather than returning byte arrays.

## Threading model

- Acquisition / hardware threads live in native code. They are not
  managed threads and must not call into managed code synchronously
  on hot paths beyond the registered callback.
- Callbacks should be short — copy the frame pointer + metadata,
  signal a `Channel<T>` or `BlockingCollection<T>`, and return.
  Heavy work runs on a managed worker.
- UI updates always via `Dispatcher`. Never block the dispatcher thread
  on a hardware operation.

## Interop project layout (suggested)

```
HCT-LSO/program/Interop/
├── Interop.csproj
├── Native/                  // raw [DllImport] declarations
│   ├── NativeOct.cs
│   ├── NativeLso.cs
│   └── NativeStructs.cs
├── Safe/                    // SafeHandle subclasses
│   ├── OctDeviceHandle.cs
│   └── LsoDeviceHandle.cs
└── Devices/                 // idiomatic managed API (events, IDisposable)
    ├── OctDevice.cs
    └── LsoDevice.cs
```

ViewModels depend only on `Devices/`. They never see `IntPtr` or
`[DllImport]`.
