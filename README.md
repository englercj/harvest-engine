# Harvest Engine

## Development Setup

### Windows

1. Install Visual Studio 2022.
2. Open PowerShell as Administrator and run:
```powershell
Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy RemoteSigned
```
3. Install Git Bash from [git-scm.com](https://git-scm.com/download/win)
4. Run `hemake.sh` using Git Bash from within the cloned folder.

#### WASM Targets

For WASM support you'll also need to install [LLVM 20](https://github.com/llvm/llvm-project/releases/download/llvmorg-20.1.8/LLVM-20.1.8-win64.exe) and re-run `bootstrap.sh`.

### Linux

1. Install system dependencies:
```sh
apt install build-essential binutils-dev libdw-dev libx11-dev libxi-dev liburing-dev
```
2. Run `hemake.sh` from within the cloned folder.

#### WASM Targets

For WASM support you'll also need to install LLVM 20:

```sh
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 20
```

## Compiler Support

The recommended version is the version that is actively used for development and testing. Compatibility with the minimum version is maintained, but may not actively be tested for new changes.

| Compiler |    Minimum Version    |  Recommended Version  |
| -------- | --------------------- | --------------------- |
| MSVC     | 14.29 (VS2019 v16.10) | 14.44 (VS2022 v17.14) |
| GCC      | 12.4.0                | 14.3.0                |
| Clang    | 14.0.0                | 18.0.0                |

## Platform Support

- :x: No support planned
- :new_moon: Planned, but not started
- :waning_crescent_moon: Started, not ready for use
- :last_quarter_moon: Active development, unstable but can be used
- :waning_gibbous_moon: Feature complete, ready for testing
- :heavy_check_mark: Ready for production

|     Platform     |         Status         |  Minimum SDK  | Notes |
| ---------------- | ---------------------- | ------------- | ----- |
| Windows          | :last_quarter_moon:    | 10.0.18362.0  | Win 10 1909+ or Win 11 required |
| Linux            | :waning_crescent_moon: | kernel 5.18   | glibc 2.30+, liburing v2.2+ required |
| WebAssembly      | :waning_crescent_moon: | LLVM 17       | WebGPU and WASM Threads support required |
| macOS            | :new_moon:             | ?             | |
| Android          | :new_moon:             | ?             | |
| iOS              | :new_moon:             | ?             | |
| Xbox Series X\|S | :new_moon:             | ?             | |
| PlayStation 5    | :new_moon:             | ?             | |
| Nintendo Switch  | :new_moon:             | ?             | |
