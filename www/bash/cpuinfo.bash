#!/bin/bash
echo "<html><head><title>env</title></head><body><table>"
IFS=$'\n'
for OUT in $(cat /proc/cpuinfo)
do
	echo "<tr><td>"${OUT%:*}:"<td><td>"${OUT#*:}"<td></tr>" 
done
echo "</table></body></head>"
