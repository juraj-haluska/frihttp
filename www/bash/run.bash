#!/bin/bash
CMD=''
for i in ${@}
do
	CMD="${CMD} ${i}"
done
${CMD}
