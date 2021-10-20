#7703
Gero's Lab Passcode Outside~
2 c 100
enter~
if %arg% == 57624
if %self.inside(bits)% != DOOR)
wait 1
%echoaround% %actor% %actor.name% punches in a code on the door panel.
%send% %actor% you punch in the code on the panel.
wait 1 sec
%echo% The door beeps loudly for a second before sliding open. A counter above the panel starts ticking off seconds starting with ten.
%load% mob 1702
%teleport% govmir 1716
%echoaround% govmir The door beeps loudly for a second before sliding open. A counter above the panel starts ticking off seconds starting with ten.
wdoor 1715 10 flags a
wdoor 7701 11 flags a
wait 10 sec
%echo% The door beeps loudly and then closes with a whoosh. Locking with a loud grinding sound.
%echoaround% govmir The door beeps loudly and then closes with a whoosh. Locking with a loud grinding sound.
%teleport% govmir %self.vnum%
%purge% govmir
wdoor 1715 10 flags abc
wdoor 7701 11 flags abc
else
wait 1 sec
%send% %actor% The door is already open, you had best hurry through it.
end
else
wait 1 sec
%echoaround% %actor% %actor.name% punches in a code on the door panel.
%send% %actor% you punch in the code on the panel.
wait 1 sec
%echo% The door doesn't respond.
end
~
#7704
Gero's Lab Passcode Inside~
2 c 100
enter~
if %arg% == 57624
if %self.inside(bits)% != DOOR)
wait 1
%echoaround% %actor% %actor.name% punches in a code on the door panel.
%send% %actor% you punch in the code on the panel.
wait 1 sec
%echo% The door beeps loudly for a second before sliding open. A counter above the panel starts ticking off seconds starting with ten.
%load% mob 1702
%teleport% govmir 1715
%echoaround% govmir The door beeps loudly for a second before sliding open. A counter above the panel starts ticking off seconds starting with ten.
wdoor 1715 10 flags a
wdoor 7701 11 flags a
wait 10 sec
%echo% The door beeps loudly and then closes with a whoosh. Locking with a loud grinding sound.
%echoaround% govmir The door beeps loudly and then closes with a whoosh. Locking with a loud grinding sound.
%teleport% govmir %self.vnum%
%purge% govmir
wdoor 1715 10 flags abc
wdoor 7701 11 flags abc
else
wait 1 sec
%send% %actor% The door is already open, you had best hurry through it.
end
else
wait 1 sec
%echoaround% %actor% %actor.name% punches in a code on the door panel.
%send% %actor% you punch in the code on the panel.
wait 1 sec
%echo% The door doesn't respond.
end
~
$~
