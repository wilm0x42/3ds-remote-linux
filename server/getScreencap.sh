#!/bin/bash

eval $(xdotool getmouselocation --shell)
quality=80
if [[ -n $2 ]]
then
    quality=$2
fi

import -strip -depth 8 -window root bmp:./work/framergb.bmp

if [ $1 == 0 ]; then
    convert ./work/framergb.bmp -draw "image over $X,$Y, 24, 40 'pointer.png'" -rotate 90 -resize 240x400! -separate -swap 0,2 -combine -quality $quality -sampling-factor 4:2:0 -interlace none jpg:work/frame.jpg
    convert ./work/framergb.bmp -draw "image over $X,$Y, 24, 40 'pointer.png'" -rotate 90 -resize 240x400! -separate -swap 0,2 -combine -interlace none png:work/prevframe.png
    
elif [ $1 == 1 ]; then
    if [ -e ./work/prevframe.png ]; then
        convert ./work/framergb.bmp -draw "image over $X,$Y, 24, 40 'pointer.png'" -rotate 90 -resize 240x400! -separate -swap 0,2 -combine -interlace none png:work/newframe.png
        convert ./work/newframe.png ./work/prevframe.png \( -clone 0 -clone 1 -compose difference -composite -threshold 0 \) -delete 1 -alpha off -compose copy_opacity -composite \( +clone -alpha off \) -compose SrcIn -composite -interlace none ./work/frame.png
        mv ./work/newframe.png ./work/prevframe.png
    else
        convert ./work/framergb.bmp -draw "image over $X,$Y, 24, 40 'pointer.png'" -rotate 90 -resize 240x400! -separate -swap 0,2 -combine -interlace none png:work/prevframe.png
        cp ./work/prevframe.png ./work/frame.png
    fi
    #cp ./work/frame.png ./work/prevframe.png
fi

rm ./work/framergb.bmp
#convert image -separate -swap 0,1 -combine -colorspace sRGB result

#convert cyclops_question.png cyclops.png \
#\( -clone 0 -clone 1 -compose difference -composite -threshold 0 \) \
#-delete 1 -alpha off -compose copy_opacity -composite -trim \
#cyclops_sep.png