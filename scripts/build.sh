#!/bin/bash
ninja -C build || exit 1
ninja -C build -t compdb > compile_commands.json
