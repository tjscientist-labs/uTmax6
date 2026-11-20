# Building uTmax6

uTmax6 is a Qt 6 program written in C++. It needs Qt 6 (Core, GUI, Widgets,
PrintSupport, SerialPort) and the GNU Scientific Library (GSL). Build it in a
separate `build` folder so the source tree stays clean.

## Linux (Debian, Ubuntu, Raspberry Pi OS)

```sh
sudo apt install qt6-base-dev qt6-base-dev-tools libqt6serialport6-dev libgsl-dev build-essential
git clone https://github.com/tjscientist-labs/uTmax6.git
cd uTmax6
mkdir build && cd build
qmake6 ../src/uTmax.pro CONFIG+=release
make -j4
```

The program is `build/release/uTmax`. On some distributions the SerialPort
package is called `qt6-serialport-dev` instead of `libqt6serialport6-dev`.

### Raspberry Pi

Use the 64-bit Raspberry Pi OS (Bookworm or later) and the same commands. A
Raspberry Pi 5 builds it in a few minutes. Let your user open serial ports:

```sh
sudo usermod -aG dialout $USER     # then log out and back in
```

The uTracer's USB-serial adapter appears as `/dev/ttyUSB0`.

## Windows (MSYS2)

1. Install MSYS2 from <https://www.msys2.org/> and open the **UCRT64** shell.
2. Install the tools and libraries:

   ```sh
   pacman -S mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-qt6-base \
             mingw-w64-ucrt-x86_64-qt6-serialport mingw-w64-ucrt-x86_64-gsl
   ```

3. Build:

   ```sh
   mkdir build && cd build
   qmake6 ../src/uTmax.pro CONFIG+=release
   mingw32-make -j4
   ```

The program is `build/release/uTmax.exe`. To run it outside the MSYS2 shell, copy
the Qt and GSL DLLs next to it with `windeployqt6`.

## Files uTmax needs at run time

uTmax keeps its tube library (`data.csv`), calibration file (`cal.txt`) and
style sheet (`Style.qss`, optional) in a folder called `uTmax_files` in your home
folder. uTmax6 creates the folder the first time it starts and puts a copy of the
tube library there, so nothing needs copying by hand, and it can be started from
any folder. An existing `data.csv` is never replaced.

## Automatic builds

Every push to GitHub builds on Linux x64, Linux arm64 and Windows
(`.github/workflows/build.yml`). The built programs are attached to each run as
downloadable artifacts. A green build means the code compiles; it does not mean
it has been tested on a uTracer.
