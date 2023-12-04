#ENTRY
HALT
MOV R0, 50 * 16

$a = 5
$b = 20

SECONDS_IN_WEEK:
	-((60 * 60) * 24 * -7)
	$a + $b    $a * $b    $b / $a