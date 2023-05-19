#!/bin/bash


inotifywait -r -e close_write -m --exclude '__targets' . | while read -r directory events filename; do
	make && printf '\e[3J' && echo "Compilation completed"
done