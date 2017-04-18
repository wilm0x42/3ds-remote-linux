# 3ds-remote-linux

Tool to control linux computer from a 3ds. Basically like VNC, RDP, etc.
Currently, it can only manage about 4 fps, tops.

It goes without saying, but the the server simply isn't secure,
so don't do anything stupid with it. :wink:

For optimal server performance, you should probably
make the "server/work" folder RAM based. This can be done with:
`sudo mount -t tmpfs -o size=50m tmpfs ./server/work`

## Random info
 * Runs on port 55550 UDP
 * Uses NanoJpeg, by KeyJ. (License in source file)
 * Compression: dynamic PNG/JPEG, or static JPEG. (Toggleable at runtime)
 * devkitpro/ctrulib! Thx WinterMute/smea!
 * The mouse pointer graphic is from the GTK theme "greybird"
 * Uses xautomation and xdotool for input events
 * Image encoding is done with image magick
 * The server must be running within the server folder
 * [Git](https://github.com/wilm0x42/3ds-remote-linux)
 
## Requirements
Packages required:
 * imagemagick
 * xdotool
 * xautomation
`sudo apt-get install imagemagick xdotool xautomation`

## Building
The client requires devkitARM and ctrulib. To compile, just
enter `make` in the root of the repository.

To compile the server, just enter `make` in the "server" folder.
The server only needs the cstdlib, and BSD sockets for compilation.
However, the packages listed above are required at runtime.

## Usage
### Startup
 1. Make sure port 55550 UDP isn't blocked by a firewall. (`sudo ufw allow 55550/udp`)
 2. Start the server, from within the server folder. (`cd server; ./server`)
 3. Enter your IP address into the "serveraddr" field of 3ds-remote-linux.ini
 4. Place 3ds-remote-linux.ini on your SD card in: "3ds/3ds-remote-linux/3ds-remote-linux.ini"
 5. Start 3ds-remote-linux on your 3ds.
 
### GUI
 Tap the buttons labeled under "Video mode," to switch between dynamic, and static compression.
 
 Dynamic compression is ideal for tasks like web browsing, or anything where
 the screen isn't changing to quickly.
 Static compresssion is best suited for watching videos, or perhaps gaming.
 
 When using static compression, the slider labeled "Video quality" can
 be used to set the JPEG quality.

### Controls
Input        | Action
-------------|-------------
A            | Left Click
B            | Double left click
Y            | Right click
X            | Speed up mouse movement
Circle pad   | Move mouse
D Pad        | Arrow keys
Start        | Exit
R + Dpad L/R | Change logging verbosity