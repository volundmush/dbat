#7100
Mean Rooms~
2 b 50
~
 eval num %actor.maxhitp% / 200
if (%actor.hitp% > %num%)
 nop %actor.hitp(-%num%)%
 %send% %actor% @rYou feel some energy drained from your very soul!@n
end
~
#7101
Purge Object~
1 ab 10
~
eval num %4.random%
if (%num% == 4)
 %purge% %self%
end
~
#7102
Vitash Spawn... Vitash Spawns.~
0 k 100
~
eval num %4.random%
if (!%self.varexists(spawncount)%)
 if (%num% >= 2)
  eval num 0
  set target_char %self.room.people%
  while (%target_char%)
   set tmp_target %target_char.next_in_room%
   if (%target_char.vnum(7101)%)
    eval num %num% + 1
   end
   set target_char %tmp_target%
  done
  if (%num% < 5)
   %echo% Several of Vitash's scales fall off and transform into Vitash Spawn!
   while (%num% > 0)
    eval num %num% - 1
    %load% m 7101
   done
  eval spawncount 0
  remote spawncount %self.id%  
  end
 end
elseif (%self.spawncount% < 10)
 if (%num% >= 2)
  eval num 0
  set target_char %self.room.people%
  while (%target_char%)
   set tmp_target %target_char.next_in_room%
   if (%target_char.vnum(7101)%)
    eval num %num% + 1
   end
   set target_char %tmp_target%
  done
  if (%num% < 5)
   %echo% Several of Vitash's scales fall off and transform into Vitash Spawn!
   while (%num% > 0)
    eval num %num% - 1
    %load% m 7101
   done
  eval spawncount %self.spawncount% + 1
  remote spawncount %self.id%
  end
 end
else
 eval num2 %4.random%
 switch %num2%
  case 1
   bigbang
   break
  case 2
   kousengan
break
  default
   beam
   break
 done
end
~
#7103
They win!?~
0 f 100
~
 set target_char %self.room.people%
 while (%target_char%)
  set tmp_target %target_char.next_in_room%
  if (%target_char.is_pc%)
   %teleport% %target_char% 7100
  end
  set target_char %tmp_target%
 done
%teleport% %self% 7100
~
#7104
Magic Words!~
2 d 100
yooserif vaktos~
if (%speech% == Yooserif Vaktos)
 wait 1 s
 if (%findmob.7199(7102)%)
  %echo% As the words are spoken a green flash is seen.
  wait 1
  %echo% Moments later all are transported to another location!
  %teleport% all 7198
 else
  %echo% It appears the magic of this room is currently not active. Perhaps try later?
 end
end
~
#7105
Glyph trigger~
2 c 100
trace~
wait 1
%send% %actor% You trace the glyph in the same pattern as the light that flows through it.
%echoaround% %actor% %actor.name% traces the glyph in the same pattern as the light that flows through it.
wait 1 s
%echo% Suddenly a bright flash of green light fills the room and everyone in it is teleported some where else!
%teleport% all 7199
~
#7106
No one below level 90~
1 c 4
enter~
if (vortex /= %arg% || swirling /= %arg%)
 if (%actor.level% < 90)
  %send% %actor% You feel you are not strong enough to survive this area. (OOC: Min Level 90)
  return 1
 end
return 0
end
~
#7107
Creepy echoes~
0 b 10
~
eval num %4.random%
switch %num%
 case 1
  %echo% A voice from the distance can be heard whispering "Come this way..."
  break
 case 2
  %echo% Faint laughter can be heard from the other side of a nearby wall.
  break
 case 3
  %echo% A pair of red eyes flash in the distance accompanied by the scraping of scales on stone.
  break
 default
  %echo% Shrill piercing screams can be heard from somewhere else deep in the maze.
  break
done
~
$~
