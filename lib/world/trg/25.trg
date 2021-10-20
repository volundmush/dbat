#2500
Lost Soldier Greet~
0 g 100
~
if (!%self.varexists(quenched)%)
 wait 1 sec
 say Oooo...
 wait 3 sec
 say Are you lost here too?
 wait 3 sec
 say Water. Tell me you have water...
 wait 4 sec
 say Give me any water you have...
else
 wait 1 sec
 say I need to find my way out of here...
 wait 3 sec
 %echo% %self.name% looks around with a confused expression on %self.hisher% face.
end
~
#2501
Lost Soldier Receive~
0 j 100
~
if (%object.type% == LIQCONTAINER)
 if (%object.val1% > 0)
  if (%object.val2% == 0)
   wait 1
   %echo% %self.name% opens %object.shortdesc% and raises it over %self.hisher% mouth.
   wait 1 sec
   %echo% %self.name% guzzles the water from %object.shortdesc% as it pours out.
   wait 3 sec
   %echo% %self.name% crushes the %object.shortdesc% after it is empty.
   %purge% %object%
   wait 3 sec
   say Thank you.
   wait 2 sec
   say In appreciation I will not kill you, and you can have this.
   wait 1 sec
   %load% obj 2500
   wait 2 sec
   say It has brought me nothing but trouble.
   give flute %actor.name%
   set quenched
   remote quenched %self.id%
   wait 2 sec
   say That flute is said to call the legendary Vaalsh.
   wait 3 sec
   say Go to the center of the wettest area in the desert. @gPlay@C it there.@n
   wait 4 sec
   say Be careful though, I barely managed to escape that nightmare...
   wait 1 sec
   chuckle
wait 1 s
%echo% %self.name% heads towards the east.
%purge% %self%
  else
   wait 1
   %echo% %self.name% opens %object.shortdesc% and raises it to %self.hisher% mouth.
   wait 2 sec
   %echo% %self.name% smells it before taking a drink.
   wait 3 sec
   say I want water... not this.
   wait 1 sec
   drop %object.name%
  end
 else
  wait 1
  %echo% %self.name% opens %object.shortdesc% and raises it over %self.hisher% mouth.
  wait 1 sec
  %echo% %self.name% frowns as nothing comes out.
  wait 3 sec
  %echo% %self.name% throws %object.shortdesc% to the ground in anger.
  drop %object.name%
 end
else
 return 0
 wait 1
 say That doesn't contain water...
end
~
#2502
Lost Soldier Random~
0 b 5
~
if (%self.varexists(quenched)%)
 wait 1
 say That's the way I need to go!
 wait 2 sec
 %echo% %self.name% takes off into the air and starts flying east.
 %purge% %self%
end
~
#2503
Blood Flute Trigger~
1 c 3
play~
if (%actor.room.vnum% != 2609)
 wait 1 sec
 %send% %actor% You blow a few notes on the flute.
 %echoaround% %actor% %actor.name% blows a few notes on the flute.
 wait 4 sec
 %send% %actor% Nothing else happens.
else
 wait 1 sec
 %send% %actor% You blow a few notes on the flute.
 %echoaround% %actor% %actor.name% blows a few notes on the flute.
 set target_char %actor.room.people%
 while %target_char%
  set tmp_tar %target_char.next_in_room%
  if (%target_char.vnum% == 2505)
   eval vaalsh %target_char%
  end
  set target_char %tmp_tar%
 done
 wait 4 sec
 if (!%vaalsh%)
  %echo% Suddenly the waters of the oasis begin to churn with extreme violence!
  wait 3 sec
  %echo% Eight glowing eyes appear beneath the center of the water. Each eye measuring about two feet across.
  wait 5 sec
  %echo% With a roar a large fat lizard esque creature erupts from the oasis floor, spraying hundreds of gallons of water into the air.
  wait 6 sec
  %echo% The large Vaalsh having been awakened from its slumber, uncrosses its large arms as it sits back on its huge hind legs and fat tree like tail.
  wait 6 sec
  %echo% Four sucker tenetacles emerge from pores on its large belly as it prepared to fight!
  wait 2 sec
  %load% mob 2505
  set vaalshtarg
  remote vaalshtarg %actor.id%
  wait 2 sec
  %send% %actor% The %self.shortdesc% crumbles to dust!
  %echoaround% %actor% The %self.shortdesc% crumbles to dust!
  %purge% %self%
 else
  %echo% The Vaalsh shrieks in anger at the sound of the flute!
  wait 2 sec
  %send% %actor% The %self.shortdesc% crumbles to dust!
  %echoaround% %actor% The %self.shortdesc% crumbles to dust!
  %purge% %self%
 end
end
~
#2504
Vaalsh Load Trigger~
0 n 100
~
set target_char %self.room.people%
if (%target_char.varexists(vaalshtarg)%)
 bite %target_char.name%
 %load% obj 2501
 eval num %random.100%
 if (%num% >= 75)
  %load% obj 2502
 end
else
 while %target_char%
  set tmp_tar %target_char.next_in_room%
  if (%target_char.varexists(vaalshtarg)%)
   bite %target_char.name%
  end
  set target_char %tmp_tar%
 done
 %load% obj 2501
 eval num %random.100%
 if (%num% >= 75)
  %load% obj 2502
 end
end
~
#2505
Vaalsh Death Trigger~
0 f 100
~
%echo% The entire area shakes as the Vaalsh's large corpse falls.
~
#2506
Tora Q3 Mob Drop~
0 f 100
~
if (%actor.varexists(onToraQ3)%)
    %load% obj 2506
end
~
#2507
Tora Q4 Mob Drop~
0 f 100
~
if (%actor.varexists(onToraQ4)%)
    %load% obj 2507
end
~
$~
