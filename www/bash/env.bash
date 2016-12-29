#!/bin/bash
echo "<html><head><title>env</title></head><body><ul>"
for OUT in $(env)
do
	echo "<li>"
	echo $OUT
	echo "</li>"
done
echo "</ul></body></head>"
