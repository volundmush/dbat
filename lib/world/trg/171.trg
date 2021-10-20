#17100
Security Machine~
1 c 4
push~
if (%actor.gold% < 100000)
 wait 1
 %send% %actor% You don't have enough money to make it work.
elseif (!%actor.inventory(17100)%)
 wait 1
 %send% %actor% You don't have the required security keycard to upgrade.
else
 nop %actor.gold(-100000)%
 %purge% %actor.inventory(17100)%
 wait 1
 %send% %actor% You insert the required money and your old keycard. You then push the button.
 %echoaround% %actor% %actor.name% inserts the required money and keycard. %actor.heshe% then pushes the button.
 wait 1 s
 %echo% The machine rumbles for a few seconds as it manufactures a new keycard.
 wait 2 s
 %echo% A new keycard pops out of a slot on the machine.
 %load% obj 17104
 %force% %actor% get keycard
end
~
#17101
Swipe Teleport~
2 c 100
swipe~
if (!%actor.inventory(17104)%)
 wait 1
 %send% %actor% You do not have the required keycard to use the teleporter.
elseif (%arg% == exit)
 wait 1
 %send% %actor% You swipe the keycard and are suddenly teleported!
 %echoaround% %actor% %actor.name% swipes their keycard and suddenly disappears in a flash of light!
 %teleport% %actor% 17100
 wait 1
 %force% %actor% look
 %echoaround% %actor% %actor.name% suddenly appears in a flash of light!
elseif (%arg% == stairwell)
 wait 1
 %send% %actor% You swipe the keycard and are suddenly teleported!
 %echoaround% %actor% %actor.name% swipes their keycard and suddenly disappears in a flash of light!
 %teleport% %actor% 17164
 wait 1
 %force% %actor% look
 %echoaround% %actor% %actor.name% suddenly appears in a flash of light!
else
 wait 1
 %send% %actor% Syntax: swipe (exit - stairwell)
end
~
#17102
Cyber Attack~
0 k 20
~
honoo
~
#17103
Archvile Attack!~
0 k 75
~
eval num %random.2%
switch (%num%)
 case 1
  honoo
  break
 case 2
  %echo% %self.name% Raises his hands and raises a Lost Soul back to life!
  %load% m 17111
  break
done
~
$~
