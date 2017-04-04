# 3ds-remote-linux

Tool to control linux computer from a 3ds. Basically like VNC, RDP, etc.
Currently, it can only manage about 2 fps, tops.

It goes without saying, but the the server simply isn't secure,
so don't do anything stupid with it. :wink:

For optimal server performance, you should probably
make the "server/work" folder RAM based. This can be done with:
`sudo mount -t tmpfs -o size=50m tmpfs ./server/work`

## Random info
 * Uses port 55550 UDP
 * NanoJpeg, courtesy of KeyJ. (License in source file)
 * JPEG compression can be edited live by editing the "quality" value in server/getScreencap.sh
 * devkitpro/ctrulib! Thx WinterMute/smea!
 * The mouse pointer graphic is from the GTK theme "greybird"
 * Uses xautomation and xdotool for input events
 * Image encoding is done with image magick
 * The server must be running within the server folder
 * [Git](https://github.com/wilm0x42/3ds-remote-linux)

## Controls
Input | Action
------|-------
A | Left Click
B | Double left click
Y | Right click
X | Speed up mouse movement
Circle pad | Move mouse
D Pad | Arrow keys
Start | Exit
 * Select - Toggle logging