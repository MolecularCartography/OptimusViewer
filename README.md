# Quickmix: An application for visualizing LC-MS features detected with [Optimus workflow](https://github.com/alexandrovteam/Optimus).

## How to build?

1. You will need to install Qt 5.5 on your computer. Perhaps, earlier versions will also work, but it hasn't been tested.
3. Check out this repository with `git clone https://github.com/iprotsyuk/Quickmix`.
After OS-specific steps described below are completed, application binaries can be found in `./Quickmix/_release`.

### Windows

1. Install Microsoft Visual Studio.
2. Add paths to Qt binaries and `msbuild` tool to `Path` environment variable.
Example: `C:\Qt\Qt5.5.0\5.5\msvc2013\bin`, `C:\Program Files (x86)\MSBuild\12.0\Bin`.
3. Execute `.\Quickmix\win_msvc_build_release.cmd`.

### OS X

1. Install XCode.
2. Add paths to Qt binaries to `PATH` environment variable:
  1. Execute `sudo nano /etc/paths/`
  2. Add path to Qt binaries at the end of file.
  Example: `/Users/admin/Qt/5.5/clang_64/bin/`
3. Execute `sh ./Quickmix/osx_clang_build_release.sh`

## License

The content of this project is licensed under the Apache 2.0 licence, see LICENSE.md.
