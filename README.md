# Harvest Engine

## Development Setup

### Windows

Install Visual Studio 2019 16.5+, then run `boostrap.sh` using Git Bash from within the cloned folder.

### Linux

Install build dependencies: `apt install build-essential libx11-dev libxi-dev`

Then run `bootstrap.sh` from within the cloned folder.

## Supported Compilers

| Compiler | Minimum Version |
| -------- | --------------- |
| MSVC     | 14.25 (VS2019 v16.5) |
| GCC      | 10.1.0 |
| Clang    | 11.0.0 |

## Supported Platforms

- :x: No support planned
- :new_moon: Planned, but not started
- :waning_crescent_moon: Started, not ready for use
- :last_quarter_moon: Active development, unstable but can be used
- :waning_gibbous_moon: Feature complete, ready for testing
- :heavy_check_mark: Ready for production

|     Platform     |         Status         |  Minimum SDK  | Notes |
| ---------------- | ---------------------- | ------------- | ----- |
| Linux            | :waning_crescent_moon: | glibc 2.30    | |
| Windows          | :last_quarter_moon:    | 10.0.18362.0  | Win 10 1909+ or Win 11 required |
| Xbox Series X\|S | :new_moon:             | ?             | |
| PlayStation 5    | :new_moon:             | ?             | |
| Nintendo Switch  | :new_moon:             | ?             | |
| macOS            | :x:                    | n/a           | |
| Android          | :x:                    | n/a           | |
| iOS              | :x:                    | n/a           | |
