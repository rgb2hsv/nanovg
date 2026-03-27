---
name: verify
description: Build the NanoVG project with CMake using the `build/` directory, then run the example_gl3 test (`example_gl3 --test 4`) and verify the process exit code is 0. Use when the user asks to build NanoVG, run example_gl3 tests, or verify example_gl3 test exit codes.
---

# Verify: build + run `example_gl3 --test 4`

## Preconditions

- Run from the NanoVG repository root (the directory containing `CMakeLists.txt`).
- Windows + PowerShell is OK.

## Workflow

1. Prefer building an existing CMake build directory if present:
   - Default build directory: `build/`
2. Build `example_gl3`:
   - If using Visual Studio generator: build with a configuration (prefer `Debug`, else `Release`).
3. Run the test and verify exit code:
   - Command: `example_gl3 --test 4`
   - **Pass condition**: process exit code is **0**

## Commands (PowerShell)

### Configure (only if `build/` does not exist)

```powershell
cmake -S . -B build
```

### Build

Change working directory to `build/` first:

```powershell
Push-Location build
```

Try Debug first (Visual Studio multi-config):

```powershell
cmake --build . --config Debug --target example_gl3
```

If Debug doesn’t exist for this build, try Release:

```powershell
cmake --build . --config Release --target example_gl3
```

### Run and verify exit code == 0

Run Debug binary first:

```powershell
./Debug/example_gl3.exe --test 4
if ($LASTEXITCODE -ne 0) { throw "example_gl3 --test 4 failed with exit code $LASTEXITCODE" }
```

If the Debug binary path doesn’t exist, run Release:

```powershell
./Release/example_gl3.exe --test 4
if ($LASTEXITCODE -ne 0) { throw "example_gl3 --test 4 failed with exit code $LASTEXITCODE" }
```

### Fallback: locate the binary and run it

If you’re unsure where CMake placed the executable, locate it under the current directory and run it:

```powershell
$exe = Get-ChildItem -Path . -Recurse -Filter example_gl3.exe -File -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $exe) { throw "Could not find example_gl3.exe under build/ (build may have failed or output path differs)" }
& $exe.FullName --test 4
if ($LASTEXITCODE -ne 0) { throw "example_gl3 --test 4 failed with exit code $LASTEXITCODE" }
```

Always restore the previous working directory when done:

```powershell
Pop-Location
```
