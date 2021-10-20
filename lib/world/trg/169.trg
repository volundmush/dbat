#16900
Gaxzixite Defense~
2 q 100
~
eval num %random.200%
eval rm 16972 + %random.12%
set target_char %self..people%
while %target_char%
 set tmp_tar %target_char.next_in_room%
 if %target_char.vnum% == 16900
  eval targ %target_char%
 end
 set target_char %tmp_tar%
done
if (%rm% == %self.vnum%)
eval rm %rm% + 1
end
if (%targ%)
 return 0
 wait 1
 %send% %actor% Gaxzixite warriors block the way!
elseif (%num% >= 195)
 %purge%
 return 0
 wait 1
 %teleport% all %rm%
 %at% %actor% %load% m 16900
 %at% %actor% %load% m 16900
 %at% %actor% %load% m 16900
 %at% %actor% %load% m 16900
 %at% %actor% %load% m 16900
 %at% %actor% %echo% Gaxzixite warriors pour out of a hole in the floor and pull you down to an ambush in a lower tunnel!
elseif (%num% >= 180)
 return 0
 wait 1
 %purge%
 %teleport% all %rm%
 %at% %actor% %load% m 16900
 %at% %actor% %load% m 16900
 %at% %actor% %load% m 16900
 %at% %actor% %echo% Gaxzixite warriors pour out of a hole in the floor and pull you down to an ambush in a lower tunnel!
elseif (%num% >= 165)
 return 0
 wait 1
 %purge%
 %teleport% all %rm%
 %at% %actor% %load% m 16900
 %at% %actor% %load% m 16900
 %at% %actor% %echo% Gaxzixite warriors pour out of a hole in the floor and pull you down to an ambush in a lower tunnel!
elseif (%num% >= 150)
 return 1
 wait 1
 %purge%
 %at% %actor% %load% m 16901
 %at% %actor% %load% m 16901
 %at% %actor% %load% m 16901
 %at% %actor% %load% m 16901
 %at% %actor% %echo% A group of Gaxzixite workers carrying silk sacks full of honey scurry into the room.
elseif (%num% >= 140)
 return 1
 wait 1
 %purge%
 %at% %actor% %load% m 16901
 %at% %actor% %load% m 16901
 %at% %actor% %echo% A duo of Gaxzixite workers carrying silk sacks full of honey scurry into the room.
elseif (%num% >= 125)
 return 1
 wait 1
 %purge%
 %at% %actor% %load% m 16902
 %at% %actor% %load% m 16902
 %at% %actor% %load% m 16902
 %at% %actor% %load% m 16902
 %at% %actor% %load% m 16902
 %at% %actor% %load% m 16902
 %at% %actor% %echo% A group of hatchlings are here cleaning up old silk.
elseif (%num% >= 115)
 return 0
 wait 1
 eval go %random.4%
 if (%go% == 4)
  %force% %actor% north
 elseif (%go% == 3)
  %force% %actor% east
 elseif (%go% == 2)
  %force% %actor% west
 elseif (%go% == 1)
  %force% %actor% south
 end
 %send% %actor% You confuse your directions for a moment!
elseif (%num% >= 105)
 return 1
 wait 1
 %purge%
 %send% %actor% You find nothing out of the ordinary here.
elseif (%num% >= 100)
 return 1
 wait 1
 %purge%
 eval chance %random.10%
 if (%chance% == 10)
  %at% %actor% %load% o 16722
 end
 %at% %actor% %load% o 16721
 %at% %actor% %load% o 16721
 %at% %actor% %load% o 16721
 %at% %actor% %load% o 16721
 %force% %actor% look
 %at% %actor% %echo% Several large honeycombs!
elseif (%num% >= 70)
 return 0
 %purge%
 wait 1
 %send% %actor% You lose your footing in the trecherous tunnel and land on your ass!
 %force% %actor% sit
elseif (%num% >= 60)
 return 1
 %purge%
 wait 1
 %at% %actor% %load% o 16901
 %at% %actor% %load% o 16901
 %send% %actor% There are some larger mushrooms growing here!
 %force% %actor% look
elseif (%num% >= 40)
 return 1
 %purge%
 wait 1
 %send% %actor% You hear a faint hissing sound coming from deeper within the hive.
elseif (%num% >= 30 && %num% <= 32)
 return 1
 %purge%
 wait 1
 %send% %actor% You stumble upon a rare abandoned Kilnak weapon below a warriors decaying cacooned corpse!
 eval treasure 16713 + %random.3%
 %at% %actor% %load% o %treasure%
 wait 1
 %force% %actor% look
elseif (%num% <= 20)
 return 1
 %purge%
 wait 1
 %send% %actor% You stumble upon a clutch of Gaxzixite eggs!
 wait 1
 eval reward %random.4%
 while (%reward% > 0)
  %at% %actor% %load% o 16900
  eval reward %reward% - 1
 done
wait 1
%force% %actor% look
end
~
#16901
Gaxzixite Attack!~
0 n 100
~
wait 1 s
set target_char %self.room.people%
while %target_char%
 set tmp_tar %target_char.next_in_room%
 if %target_char.is_pc%
  eval player %target_char%
 end
 set target_char %tmp_tar%
done
if (%player%)
 bite %player.name%
end
~
#16902
Randomly purge hole~
1 ab 15
~
eval rm %self.room.vnum%
eval rdm 0
while (%rdm% == 0)
 eval chance %random.7%
 switch (%chance%)
  case 1
   eval rdm 16941
   break
  case 2
   eval rdm 16947
   break
  case 3
   eval rdm 16955
   break
  case 4
   eval rdm 16961
   break
  case 5
   eval rdm 16965
   break
  case 6
   eval rdm 16971
   break
  case 7
   eval rdm 16972
   break
 done
 if (%rdm% == %rm%)
  eval rdm 0
 end
done
wait 1
%echo% The tunnel hole closes. The sound of another hole opening in the distance is heard.
wait 1
%at% %rdm% %load% o %self.vnum%
%purge% self
~
#16903
Jekkix Greet~
0 g 100
~
wait 1
if (!%actor.varexists(kilnak)%)
 wait 1 s
 growl
 wait 1 s
 say I don't know how you got in here, but you best sneak back out the way you came before the rest of the tribe catches you.
elseif (%actor.kilnak% == Enemy)
 wait 1
 say Fiend!
 wait 1 s
 punch %actor.name%
else
 wait 1 s
 nod %actor.name%
 wait 1 s
 say Be careful below, friend. The Gaxzixite live in the Dark of Arlia and they take to intruders far less hospitably than we Kilnak.
end
~
#16904
Kirala Greet~
0 g 100
~
wait 1 s
say Are you a new worker? No, you can't be. Go bother someone else...
~
$~
