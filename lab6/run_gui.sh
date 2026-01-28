#!/bin/bash
# Wrapper script to run the temperature GUI application

cd "$(dirname "$0")"

if [ -f "temperature_gui.app/Contents/MacOS/temperature_gui" ]; then
    open temperature_gui.app
elif [ -f "build/temperature_gui" ]; then
    ./build/temperature_gui
elif [ -f "./temperature_gui" ]; then
    ./temperature_gui
else
    echo "Error: temperature_gui application not found"
    echo "Please run './build.sh' to build the application first"
    exit 1
fi
