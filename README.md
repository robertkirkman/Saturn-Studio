# Saturn Studio Android
#### Experimental Saturn Studio for Android (in X11 mode) for [Dominicentek](https://github.com/Dominicentek). Many things are very broken and not working!
> Bring your own graphics driver! This example is for **Adreno 6XX+ GPUs only**. the [virglrenderer-android](https://github.com/robertkirkman/sm64ex-coop?tab=readme-ov-file#how-to-get-this-mode) OpenGL ES graphics pipe has broader hardware compatibility than the Turnip Vulkan driver and Zink OpenGL 4.0+ driver, but I think it could be slightly harder to run this app on it. The llvmpipe software OpenGL implementation runs only on the CPU (software renderer), but it's the most broadly compatible. To use it, launch _without_ setting the `MESA_LOADER_DRIVER_OVERRIDE=zink` environment variable.
- Install this app https://f-droid.org/repo/com.termux_118.apk
- Install this app https://github.com/termux/termux-x11/releases/download/nightly/app-universal-debug.apk
- Open both apps
- (Optional) Strongly recommend this keyboard https://f-droid.org/repo/org.pocketworkstation.pckeyboard_1041001.apk
- (Optional) Drag down notifications, touch Termux:X11 settings, set Termux:X11 into "direct touch" mode
- Run these commands
```
termux-change-repo
yes | pkg upgrade -y
termux-setup-storage # if you will want to select baserom.us.z64 from anywhere at runtime
pkg install -y x11-repo
pkg install -y git wget make python getconf clang binutils sdl2 \
               libglvnd-dev glu termux-x11-nightly xfce mesa-vulkan-icd-freedreno-dri3 ffmpeg xorgproto
git clone --branch android https://github.com/robertkirkman/Saturn-Studio.git
cd Saturn-Studio
FILE_PICKER=1 DISCORDGAMESDK=0 DISCORDRPC=0 DISCORD_SDK=0 NO_PIE=0 TOUCH_CONTROLS=1 make -j6
termux-x11 :0 -xstartup "xfce4-session" & sleep 10
DISPLAY=:0 MESA_LOADER_DRIVER_OVERRIDE=zink build/us_pc/sm64.us.f3dex2e
```
[saturnstudioandroid.webm](https://github.com/robertkirkman/Saturn-Studio/assets/31490854/53f083c8-f9a2-429f-94d7-865fd69cf4bb)

# Saturn Studio

An experimental fork of [Saturn](https://github.com/Llennpie/Saturn) that has features originally planned for the original version,
but would make too much of a difference to implement, so it made more sense to make it a separate version.

**I still encourage using the original version.** This version is intended for users that are more experienced with the program.

Donate to sm64rise, the creator of Saturn, [here](https://ko-fi.com/J3J05B5WR).

### Features of the Studio version

- [x] A different UI, closer to Moon edition
- [x] Multiple Mario support
  - [x] Models
  - [x] Colorcodes
  - [x] Animations
  - [x] Keyframing
  - [x] Misc settings
  - [x] In-game bone editor
- [X] Input recording/playback
- [x] Recording transparent .webm videos (or .png sequences) directly in the editor
- [x] Capturing transparent screenshots
- [x] Mouse-based camera controls

### Controls

* Left click to spawn a new Mario
* Right click him to modify his properties (or use the Marios menu incase it doesn't work)
* Mouse based camera controls
  * Hold left click to pan the camera
  * Hold right click to rotate it
  * Use the scrollwheel to zoom in and out
  * Hold left shift to zoom slower
  * Hold left ctrl to zoom faster
* Keyboard based camera controls
  * WSAD to move, W and S is forward and backward respectively
  * Holding P makes the W and S keys up and down respectively
  * Holding O makes the camera rotate instead
  * Left shift to move slower
  * Left ctrl to move faster

### Installing FFmpeg

#### Windows

1. You can get ffmpeg from [here](https://www.gyan.dev/ffmpeg/builds/)
2. Once you download it, extract the archive to C:/ffmpeg
3. Make sure that there's a directory C:/ffmpeg/bin and that it contains "ffmpeg.exe"
4. In the start menu, search for "environment variables"
5. On the dialog, click "environment variables"
6. In the "system" section, find the "Path" entry
7. Select it and click edit
8. Add a new entry and type in "C:\ffmpeg\bin"
9. Apply everything
10. Restart Saturn Studio if it's running

#### Linux

You can use your distribution's package manager

* Debian: `sudo apt install ffmpeg`
* Arch: `sudo pacman -S ffmpeg`
* Fedora: `sudo dnf install ffmpeg`
* SUSE: `sudo zypper install ffmpeg`

After it's installed, restart Saturn Studio if it's running
