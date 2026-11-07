# AGENTS.md — GenevensiS contributor and coding-agent guidance

This file applies to the entire repository. It is intended for human maintainers and coding assistants (including Codex). Read it before changing GUI code.

## Project and current direction

- This repository is **MagicSetEditor2-GenevensiS**, a fork of Magic Set Editor 2.
- We are actively polishing **wxWidgets 3.3.3 with the Qt 6 backend (wxQt)** for Linux/KDE Plasma, without sacrificing wxGTK or other supported ports.
- Priorities: **functional correctness first**, then consistent interface styling and KDE integration, then packaging and release checks.
- Make narrow, reviewable changes. Preserve existing architecture and user workflows. Do not introduce broad rewrites or unrelated performance instrumentation.
- The wxQt compatibility work is tracked in PR #8, branch `fix/wxqt-toolbar-layout`. Check the current repository state before continuing work; do not assume this branch or PR remains open forever.

## Critical wxQt 3.3.3 wheel-event bug — DO NOT REGRESS

**Observed:** Under wxQt 3.3.3, wheel scrolling over the content of the Style and Set Info editors and Statistics/gallery sidebar was broken or one-directional, even where scrollbar interaction worked. The native wheel-filter workaround made scrolling over content smooth in user testing.

**Root cause observed in upstream wxWidgets 3.3.3:** In `src/qt/window.cpp`, `wxWindowQt::QtHandleWheelEvent()` determines the wheel axis using `event->angleDelta().y() > 0`. A **negative vertical** delta therefore selects `wxMOUSE_WHEEL_HORIZONTAL` even when the event is vertical; the converted rotation also divides the Qt delta by 8. This makes the wxWidgets wheel event unreliable for affected controls, especially downward scrolling. See upstream `wxWidgets/wxWidgets` tag `v3.3.3`, `src/qt/window.cpp` around `QtHandleWheelEvent`.

**Application-side workaround (do not remove casually):**

- `src/gui/control/qt_wheel_filter.hpp` defines `GenevensisQtWheelFilter` and `installGenevensisQtWheelFilter()`.
- On `__WXQT__`, it installs a Qt event filter on the control's underlying `QWidget` and (where applicable) its scroll-area viewport, intercepting the original `QWheelEvent` **before wxWidgets' faulty conversion**.
- It uses `angleDelta().y()` for native vertical scrolling and consumes handled vertical events; nonvertical events are left for the normal event path.
- `src/gui/control/native_look_editor.cpp` installs this on `NativeLookEditor` (used by Style and Set Info). Its scroll offset is tracked using `cached_scroll`; preserve the bidirectional and fractional-motion behavior.
- `src/gui/control/gallery_list.cpp` installs it for `GalleryList` (including the Statistics sidebar); fractional deltas are accumulated in `wheel_remainder`.
- The workaround is intentionally **wxQt-only**. Do not change wxGTK behavior just to accommodate wxQt.
- Do not revert to generic `EVT_MOUSEWHEEL` handlers, scroll-position tweaks, repeated `Refresh()` calls, or wxWidgets-source edits as a replacement without proving the original defect has been fixed.
- **Revisit when upgrading wxWidgets:** inspect upstream `QtHandleWheelEvent()`; verify both wheel directions on actual wxQt. Remove the workaround only after confirming correct native-to-wx event conversion and functional parity on Qt and GTK. Treat it as a compatibility fix, not a permanent design requirement.

## Other wxQt-specific compatibility notes

- wxQt toolbars may mishandle embedded controls. Cards and Keywords use an auxiliary sizer-managed strip under `__WXQT__`; preserve toolbar/filter alignment and lifecycle.
- **Toolbar panel-switch ordering:** wxQt 3.3.3's `wxToolBar::DoDeleteTool()` deletes the underlying `QToolButton` but doesn't explicitly remove its `QAction`, so native Qt action entries can survive tool deletion and shift the insertion positions of subsequent tools. `SetWindow` records the native toolbar's permanent prefix after its first `Realize()` and removes trailing orphan Qt actions **after** panel `destroyUI()` and **before** the next panel `initUI()`. Keep this wxQt-only synchronization, and verify order by cycling Cards → Style → Set Info → Keywords → Statistics → Cards multiple times. Revisit when upgrading wxWidgets; don't delete the permanent shared toolbar actions.
- wxQt `wxStaticBox`/Qt `QGroupBox` stacking has obscured child controls. Existing fixes lower affected boxes in Preferences and linked-card sections. Check z-order when controls appear missing or unclickable.
- The linked-card picker previously crashed because icons added to `wxImageList(15,15)` were not guaranteed to be 15×15. Keep image dimensions consistent before adding.
- Custom styling dropdowns and their popup/mouse handling remain regression-test targets; don't assume an earlier mouse fix solved every case.
- Existing Qt-only scrollbar repaint workarounds in `item_list.cpp` should be assessed for correctness and responsiveness rather than copied to every control.

## Verification checklist for GUI changes

- Build and test **wxQt** and, where relevant, **wxGTK**. Common local build paths are `build-qt` and `build-ninja`, but they are machine-specific, not required project layout.
- With the pointer over **content** (not just the scrollbar), test wheel **up and down** in Style, Set Info, and Statistics sidebar. Also verify Cards and Keywords remain smooth.
- Test scrollbar thumb dragging, high-resolution touchpad/wheel behavior if available, selection/editing, popup/dropdown mouse interaction, and switching panels.
- Recheck linked-card dialogs/previews, Preferences controls, toolbar filters, and basic set opening/saving.
- Do not claim tests or compilation succeeded unless actually run. If unable to run locally, state that clearly and provide exact reproduction steps.
- The opt-in `--benchmark-gui FILE.mse-set` mode exists for development diagnostics. It intentionally calls `std::_Exit` after first-card paint; it is **not** a normal app shutdown or a measure of sustained editing performance. Do not invoke it during regular user flows.

## Editing guidance

- Favor small cross-platform wxWidgets changes where possible; use `#ifdef __WXQT__` only for demonstrated Qt-backend differences.
- Preserve original line endings when editing existing files, where practical.
- Keep bug-specific rationale near the fix and document non-obvious upstream compatibility issues here.
- Do not silently change resource paths, app packaging, or saved set formats while polishing UI.
