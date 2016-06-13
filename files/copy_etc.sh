#!/bin/bash

mkdir -p /etc/ttydm

if [ ! -f /etc/ttydm/bg.bmp ]; then
    echo "bg not found"
    cp -nr ./etc/bg.bmp /etc/ttydm/
fi

if [ ! -f /etc/ttydm/user.bmp ]; then
    cp -nr ./etc/user* /etc/ttydm/
fi

