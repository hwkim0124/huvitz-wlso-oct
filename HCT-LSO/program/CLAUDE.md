# `HCT-LSO/program/` — C# / WPF applications

This subtree contains all managed code: the engineer-tool WPF
application, the shared Interop project, and (eventually) the
clinical end-user application.

## What lives here

- **Engineer tool app** — WPF, MVVM, the main deliverable of this repo.
- **Interop project** — the only place `[DllImport]` is allowed.
- **Tests** — xUnit unit tests for managed code.
- **Future clinical app** — to be added later; will share the Interop
  project and core services with the engineer tool.

## Rules specific to this area

- **No `[DllImport]` outside the Interop project.** ViewModels and
  app code talk to `Interop.Devices.OctDevice` / `LsoDevice` etc.,
  never to raw native APIs.
- **No code-behind business logic.** `.xaml.cs` files contain only
  constructors and trivial plumbing. Logic lives in ViewModels.
- **Dispatcher discipline.** Any code path that originates from a
  native callback must marshal to the UI thread before touching
  observable state bound to the UI.
- **Per-monitor DPI v2** is the assumed mode. Test on at least one
  high-DPI monitor before declaring a layout done.

## Before changing the Interop layer

Read @../../docs/claude/architecture.md — it covers the ABI rules,
SafeHandle patterns, callback lifetime, and the zero-copy
`WriteableBitmap` frame-display pattern in detail. The single most
common bug in this codebase will be a P/Invoke callback delegate
that gets garbage-collected; the architecture doc explains how to
prevent it.

## Related docs

- @../../docs/claude/architecture.md — interop ABI, threading, image display
- @../../docs/claude/conventions.md  — C# coding standards, MVVM, naming
- @../../docs/claude/build.md         — build / test commands
- @../../docs/claude/policies.md      — what requires approval
