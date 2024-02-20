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