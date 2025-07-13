# Deburr
A simple dwl dwm-like status bar made for wayland using wlr

![Deburr](https://user-images.githubusercontent.com/59704655/233614419-b593fb0f-7727-4281-8cf9-a21a5efbfcb8.png)

# Configuration
Like all suckless software all configuration is done by editing the `src/config.h` file

# Installation
Just a simple
```
sudo make install
```
should build it and copy it to `/usr/local/bin`

# Warning
If you get an error like this

```wl_registry#2: error 0: invalid version for global zwlr_layer_shell_v1 (22): have 3, wanted 4```

edit OVERRIDE_ZWLR_VERSION in ``config.h`` to the version provided by dwl.

# Usage
When starting dwl run it as
```
dwl -s deburr
```
This is so that the program can get all the status changes dwl prints out

If you instead wish to use the ipc patch for dwl you can use [dwlmsg](https://codeberg.org/notchoc/dwlmsg)
```
dwlmsg -w | deburr &
```
By adding this to your dwl autostart script.

Setting the status can be done using
```
deburr status <custom status>
```
or by writing to
```
echo "<custom status>" > /tmp/deburrstat
``

# Improvements
As it stands deburr currently lacks being able to click on tags to switch to them, multiple monitor support, and a few font rendering kinks which i will probably fix sometime in the near or far future :3

# Inspirations
This project was inspired by [somebar](https://git.sr.ht/~raphi/somebar), which is a mighty fine project (but it didn't work on my machine and it was written in cpp)
