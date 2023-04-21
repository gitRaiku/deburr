# Deburr
A simple dwl dwm-like status bar made for wayland using wlr

# Configuration
Like all suckless software all configuration is done by editing the `src/config.h` file

# Installation
Just a simple
```
sudo make install
```
should build it and copy it to `/usr/local/bin`

# Usage
When starting dwl run it as
```
dwl -s deburr
```
This is so that the program can get all the status changes dwl prints out

Setting the status can be done using
```
deburr status <custom status>
```

# Improvements
As it stands deburr currently lacks being able to click on tags to switch to them, multiple monitor support, and a few font rendering kinks which i will probably fix sometime in the near or far future :3

# Inspirations
This project was inspired by [somebar](https://git.sr.ht/~raphi/somebar), which is a mighty fine project (but it didn't work on my machine and it was written in cpp)
