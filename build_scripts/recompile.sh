#!/bin/bash

inotifywait -r -e close_write -m --exclude '__targets' . | while read -r directory events filename; do
	clear && printf '\e[3J'
	make
done