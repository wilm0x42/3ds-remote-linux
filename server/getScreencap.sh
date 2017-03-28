#!/bin/bash

eval $(xdotool getmouselocation --shell)

import -strip -depth 8 -window root bmp:framergb.bmp
convert framergb.bmp -draw "image over $X,$Y, 24, 40 'pointer.png'" -rotate 90 -resize 240x320! -separate -swap 0,2 -combine -quality 30 -sampling-factor 4:2:0 -interlace none jpg:frame.jpg
#convert image -separate -swap 0,1 -combine -colorspace sRGB result