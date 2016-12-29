#!/bin/bash
echo '<html>
<head>
<title>pouzivatelia</title>
</head>
<body>
<table border="1">
<tr>
<td><h2>login</h2></td><td><h2>passwd</h2></td><td><h2>uid</h2></td><td><h2>gid</h2></td><td><h2>meno</h2></td><td><h2>home</h2></td><td><h2>shell</h2></td>
</tr>'

USERS=$(cat /etc/passwd)
IFS=$'\n'
for r in ${USERS}
do
	echo '<tr>'
	IFS=':'
	for i in ${r}
	do
		echo '<td>'${i}'</td>'
	done
	echo '</tr>'
done

echo '</table>
</body>
</html>'
exit
