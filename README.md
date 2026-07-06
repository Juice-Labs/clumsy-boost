# clumsy-boost

Juice Labs' fork of Clumsy, purpose-built for steady, repeatable latency injection during HP Z Boost QA benchmarking. See [Juice Labs Fork Notes](#juice-labs-fork-notes) below for what's different from upstream and why.

__clumsy makes your network condition on Windows significantly worse, but in a managed and interactive manner.__

Leveraging the awesome [WinDivert](http://reqrypt.org/windivert.html), clumsy stops living network packets and capture them, lag/drop/tamper/.. the packets on demand, then send them away. Whether you want to track down weird bugs related to broken network, or evaluate your application on poor connections, clumsy will come in handy:

* No installation.
* No need for proxy setup or code change in your application.
* System wide network capturing means it works on any application.
* Works even if you're offline (ie, connecting from localhost to localhost).
* Your application keeps running, while clumsy can start and stop anytime.
* Interactive control how bad the network can be, with enough visual feedback to tell you what's going on.

See [this page](http://jagt.github.io/clumsy) for more info and build instructions.

## License

MIT

## Third-Party Attribution

This project links against and redistributes the following third-party components (see `external/`):

* [WinDivert](http://reqrypt.org/windivert.html) — dual-licensed under the GNU Lesser General Public License v3 (LGPLv3) or GNU General Public License v2 (GPLv2), at your choice. See `external/WinDivert-2.1.0-A/LICENSE`. WinDivert is used unmodified as a separate, dynamically-linked driver/DLL (`WinDivert.dll` / `WinDivert64.sys`), not statically embedded into `clumsy.exe`.
* [IUP - Portable User Interface](https://www.tecgraf.puc-rio.br/iup/) — MIT licensed, copyright Tecgraf/PUC-Rio. Used as a dynamically-linked GUI library (`iup.dll`).

## Juice Labs Fork Notes

### Why this fork exists:
Clumsy is used to inject fixed latency (5/10/15/30ms) during automated HP Z Boost QA benchmark runs. Upstream Clumsy loses accurate lag timing whenever its window isn't focused or gets minimized — Windows throttles background processes' timer resolution, which corrupts the steady-state latency a benchmark capture depends on. This fork exists to keep that timing steady for the full duration of a run.

### What's different from IntouchHealth/clumsy:
* The window is forced topmost (`SetWindowPos`/`HWND_TOPMOST`) and can't be minimized (title bar button, system menu, Win+M, and Show Desktop are all blocked), so the process stays in the foreground processing tier regardless of focus.
* Rebranded as clumsy-boost, starting at v1.0.0.

### Usage:
* Download the latest clumsy-boost build from the [releases page](https://github.com/Juice-Labs/clumsy-boost/releases).
* Extract the zip file to a folder on your PC.
* Run clumsy.exe as Administrator (required for WinDivert packet interception). If you see any Windows Defender or AV warnings, bypass them to run the app.

## Features

* You can open multiple Clumsy instances.
    * Note that overlapping filters haven't been carefully tested, so use with care.
* Bandwidth cap is available.
    * The cap is shared across up and down traffic, so it's recommended to use a second Clumsy instance for fully independent up vs. down control.
    * This feature is still somewhat experimental, and may not be 100% stable.

## Build Instructions (Windows 10/11, VS 2019)

* Clone the repo.
* Download Premake from here: https://premake.github.io/download/ — use [alpha16](https://github.com/premake/premake-core/releases/tag/v5.0.0-alpha16) (`premake-5.0.0-alpha16-windows.zip`), not the current beta release, since beta removed the legacy `configuration()` API this project's `premake5.lua` relies on. Extract `premake5.exe` into the repo root.
* From a command prompt in the repo folder, execute `premake5 vs2019`. If you see any Windows Defender or AV warnings, bypass them to execute the command.
* Open `build/clumsy.sln` in VS 2019, and accept the default recommendations for updating the Windows SDK Version and Platform Toolset.
* You should now be able to build and debug the various configurations from VS 2019.
* You may delete the premake5.exe file at this point if you'd like.

### Fork History:
* Forked from "official" jagt/clumsy into codyherzog/clumsy.
* Pulled in changes from crunkyball/clumsy to get WinDivert library upgrade which is required for some advanced filters.
* Pulled in changes from rorlork/clumsy to get improvements to bandwidth cap feature.
* Modified the bandwidth cap code, because it had some bugs and wasn't optimal for ITH usage.
* Made a few other tweaks, such as allowing multiple instances.
* Forked from codyherzog/clumsy to IntouchHealth/clumsy, where ongoing ITH development took place.
* Forked from IntouchHealth/clumsy v0.4.2.ITH.Alpha.3 into Juice-Labs/clumsy-boost, to make Clumsy stable enough for unattended benchmark runs.
