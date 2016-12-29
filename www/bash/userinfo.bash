#!/bin/bash
OUT=$(cat /etc/passwd | grep -e "${1}")
echo '<html><body>Single user info:'
echo '<table border="1">
<tr>
<td><h2>login</h2></td><td><h2>passwd</h2></td><td><h2>uid</h2></td><td><h2>gid</h2></td><td><h2>meno</h2></td><td><h2>home</h2></td><td><h2>shell</h2></td>
</tr></tr>'

IFS=':'
for i in ${OUT}
do
	echo '<td>'${i}'</td>'
done

echo '</tr></table></body></html>'
