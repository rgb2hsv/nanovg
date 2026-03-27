---
name: no-third-party-header-edits
description: Prevents edits to vendored/third-party headers (especially stb_* headers and fontstash.h). Use whenever modifying NanoVG, integrating third-party code, fixing build errors, or making changes that might touch vendored headers; prefer wrappers, local shims, compile definitions, or upstream patching instead.
---

# No third-party header edits

## Rule

Do **not** modify vendored/third-party headers in this repository, including:

- `stb_*` headers (any `stb_*.h`)
- `fontstash.h`

If a requested change would normally be made by editing one of these headers, choose an alternative.

## Preferred alternatives (in order)

1. **Change your code, not the header**:
   - Add a wrapper layer (e.g., `compat_*.h/.cpp`) that adapts API/behavior.
   - Isolate header-specific behavior behind your own functions/types.

2. **Use build-system switches**:
   - Prefer compile definitions (`target_compile_definitions`) or include order changes.
   - Prefer per-target defines rather than global ones.

3. **Patch as an explicit upstream delta**:
   - Add a patch file (e.g., under `patches/`) and document how/when it applies.
   - Keep patches minimal and easy to rebase when vendor code updates.

4. **Update the vendored dependency**:
   - If the fix exists upstream, prefer upgrading the vendored copy.

## When exceptions are unavoidable

Avoid exceptions. If absolutely necessary, isolate the change so it is:

- minimal and mechanical
- easy to detect/revert
- clearly attributable to an upstream change request

