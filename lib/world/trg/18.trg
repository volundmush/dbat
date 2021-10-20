#1800
Alarm~
2 g 100
~
wait 1
%echo% Klaxxons begin to blare as the red lights up and down the corridors start flashing.
%asound% Klaxxons begin to blare as the red lights up and down the corridors start flashing.
wait 2 s
%echo% "Intruder Alert...Intruder Alert" a robotic voice can be heard saying over the intercomm.
%asound% "Intruder Alert...Intruder Alert" a robotic voice can be heard saying over the intercomm.
wait 5 s
%echo% The klaxxons subside but the lights continue to flash.
%asound% The klaxxons subside but the lights continue to flash.
~
#1801
Android Nineteen's Greet~
0 g 100
~
wait 1
say You have not been permitted to enter this area.
wait 3 s
say You will die now.
punch %actor.name%
~
#1802
Dr. Gero's Greet~
0 g 100
~
wait 1 s
%echo% %self.name% looks intently at an android blueprint.
wait 3 s
say Yes...yes..this will be perfect. With this I can finally accomplish my goals.
wait 4 s
%echo% %self.name% puts down the blueprint and looks up.
wait 3 s
say Who are you and how did you get in here?
wait 4 s
say Leave now before I have you destroyed.
~
#1803
Dr. Gero Summons Help~
0 k 3
~
wait 1
%echo% A model eleven android rushes into the room, and jumps to Dr. Gero's aid!
wait 1 s
%load% mob 1808
%force% eleven punch %actor.name%
~
#1804
Twenty-one Greet~
0 g 100
~
wait 1 s
%echo% %self.name%'s eyes begin glowing red.
wait 2 s
say Halt intruder. I have been granted the authority to terminate your life.
wait 4 s
say Prepare for termination.
wait 1
knee %actor.name%
~
#1805
Tenty-one's Death~
0 f 100
~
%echo% A metal shield covers the tank containing Cell.
~
#1806
Twenty-one's Fight Trigger~
0 k 5
~
%echo% %self.name%'s eyes glow a brighter red as a green glow forms around his skin.
wait 1 s
slam %actor.name%
~
#1807
Twenty-two's Greet~
0 g 100
~
wait 1 s
%echo% %self.name%'s eyes begin to glow red.
~
#1808
Twenty-two's Fight~
0 k 20
~
dual
~
#1809
Canister Trigger~
1 j 100
~
if (%actor.race% == Android)
wait 1
 %echo% %self.shortdesc% snaps into place and a hissing sound is heard.
end
~
#1810
Canister Trigger 2~
1 l 100
~
if (%actor.race% == Android)
 wait 1
 %echo% %self.shortdesc% hisses as some pressurized fuel leaks out for a moment.
end
~
#1811
Unseal Androids Thirteen, Fourteen, and Fifteen~
2 c 100
access~
if (!%actor.hasitem(blah)%)
 wait 1
 %send% %actor% This computer requires a specific keycard to access.
else
 wait 1
 %send% %actor% The screen displays a prompt asking if you wish to unseal? @GYes@w or @Rno@w?
end
~
#1812
Yes to Unseal~
2 c 100
yes~
set target_char %self.people%
eval fail 0
while %target_char%
 set tmp_tar %target_char.next_in_room%
 if (%target_char.vnum% == 1822 || %target_char.vnum% == 1823)
  wait 1
  %send% %actor% The unsealing process has already been completed!
  eval fail 1
 end
 set target_char %tmp_tar%
done
if (fail == 0)
 wait 1 s
 %send% %actor% You slide the security master card on the access panel.
 wait 2 s
 %send% %actor% You quickly press the confirmation button.
 wait 3 s
 %send% %actor% The android storage capsules built into the wall slowly begin to open.
 %echoaround% %actor% The android storage capsules built into the wall slowly begin to open.
 wait 4 s
 %send% %actor% Three androids step out of the capsules. A short purple skinned android, a gigantic 9 foot pale skinned android, and a silver haired android sporting a truckers hat.
 %load% m 1822
end
~
$~
