#19400
Door Testing~
2 d 100
Boop~
if (%speech% == Boop)
 wait 1
 %echo% The hatch security panel flashes as the password is analyzed.
 wait 1 s
 %echo% The hatch opens with a hiss.
 if (%self.vnum% == 19352)
  %at% 19366 %echo% The hatch opens with a hiss.
 else
  %at% 19352 %echo% The hatch opens with a hiss.
 end
 %door% 19366 5 flags a
 %door% 19352 4 flags a
 wait 3 s
 %echo% The hatch closes with a hiss.
 if (%self.vnum% == 19365)
  %at% 19366 %echo% The hatch closes with a hiss.
 else
  %at% 19352 %echo% The hatch closes with a hiss.
 end
 %door% 19366 5 flags abc
 %door% 19352 4 flags abc
else
 wait 1
 %echo% The hatch security panel flashes as the password is analyzed.
 wait 1 s
 %echo% The security panel flashes a failure message and the hatch remains closed.
end
~
#19401
No Charge~
2 c 100
ch~
if (%cmd% == ch || %cmd% == cha || %cmd% == char || %cmd% == charg || %cmd% == charge)
  %send% %actor% You feel something stopping you.
  return 1
end
~
#19402
No Focus~
2 c 100
fo~
if  (%cmd% == fo || %cmd% == fo || %cmd% == foc || %cmd% == focu || %cmd% == focus)
  %send% %actor% You feel something stopping you.
  return 1
end
~
#19403
No Paralyze~
2 c 100
par~
if  (%cmd% ==  par || %cmd% == para || %cmd% == paral || %cmd% == paraly || %cmd% == paralyz || %cmd% == paralyze)
  %send% %actor% You feel something stopping you.
  return 1
end
~
#19404
No Trip~
2 c 100
tri~
if (%cmd% == tri || %cmd% == trip)
  %send% %actor% You feel something stopping you.
  return 1
end
~
#19405
No Inst~
2 c 100
inst~
if (%cmd% == inst || %cmd% == insta || %cmd% instan || %cmd% == instant)
  %send% %actor% You feel something stopping you.
  return 1
end
~
#19406
No Stone~
2 c 100
sto~
if (%cmd% == sto || %cmd == ston || %cmd% stone)
  %send% %actor% You feel something stopping you.
  return 1
end
~
#19407
No Songs~
2 c 100
son~
if (%cmd% == son || song )
%send% %actor% You feel something stopping you.
return 1
else
end
~
#19489
Fighttrig~
0 k 13
~
eval atk %random.8%
wait 1
switch %atk%
  case 1
    %echo% The shadows swird around %self.name%, and a Shadow of Piccolo appears and launches a Special Beam Cannon!
    sbc
  break
  case 2
    %echo% The shadows swirl around %self.name%, and a Shadow of Goku appears and launches a Kamehameha!
    kamehameha
  break
  case 3
    %echo% The shadows swirl around %self.name%, and a Shadow of Vegeta appears and launches a Galik Gun!
    galik
  break
  case 4
    %echo% The shadows swirl around %self.name%, and a Shadow of Tien appears and launches a Tribeam!
    tri
  break
  default
    %echo% The shadows swirl around %self.name%, but nothing happens.
  break
done
~
#19490
Overlord Spawn~
2 g 100
~
* No Script
~
#19491
Spawntest~
2 ag 100
~
if (%Spawned% == 0)
  wait 2 s
  %echo% The shadows shift and the fog swirls around as you enter the large chamber.
  wait 2 s
  %echo% A laugh echos throughout the chamber, '@CSo, you think you're strong enough to take me on? Very well.@n'
  wait 2 s
  %echo% The shadows continue to shift and the fog clears out and a large dark shape emerges from the darkness, eyes glowing red. '@CLet's see what you can do!@n'
  %load% mob 19499
  set Spawned 1
  global Spawned
  %force% shadow punch %actor.name%
else
  wait 2 s
  %echo% The Shadows swirl around.
end
~
#19492
Death~
0 af 100
~
%force% %actor% xzwdeathxzw
~
#19493
Overlord Dead~
2 ac 100
xzwdeathxzw~
wait 2 s
%echo% The shadows disapate at the death of the Overlord before reforming into a smaller shadow.
wait 2 s
%echo% Features start to become more defined on the darkness, at it seems to form into a shadow version of %actor.name%!
wait 2 s
%echo% Shadow %actor.name% grins, '@CGood job beating him. And now, for your reward.@n'
wait 2 s
%send% %actor% Shadow %actor.name% holds out %actor.hisher% hand and the shadows from the room start to converge into a wispy orb. Shadow %actor.name% hands the orb over to you and fades away.
%echoaround% %actor% Shadow %actor.name% holds out %actor.hisher% hand and the shadows from the room start to converge into a wispy orb. Shadow %actor.name% hands the orb over to %actor.name% and fades away.
%load% obj 19405
%force% %actor% get orb
set Spawned 0
global Spawned
rdelete smazeallowed %actor.id%
set smazecomplete
remote smazecomplete %actor.id%
wait 2 s
%echoaround% %actor% %actor.name% is swallowed up by the Shadows! When they disapate, %actor.heshe% is no longer there.
%send% %actor% The shadows swirl around you, and when they vanish you're outside the Maze!
%teleport% %actor% 19365
~
#19494
new trigger~
0 d 100
Shla~
set %self.hitp(-70)%
if (%self.hitp% < %self.maxhitp%)
  eval num1 %random.10%
  eval hst %self.hitp% + (%self.maxhitp%) / 50)
  if (%hst% > %self.maxhitp%)
    eval hst2 %hst% / %num1%
    %echo% Overkill: Current Hitp: %self.hitp%/%self.maxhitp% Healing for: %hst2%
    set %self.hitp(%hst2%)
  else
    %echo% Not overkill: Current Hitp: %self.hitp%/%self.maxhitp% Healing for: %hst%
    set %self.hitp(%hst%)
  else
    %echo% Not damaged enough.
  end
end   
~
#19499
Spawncheck Reset~
2 af 100
~
set target_char %self.people%
while %target_char%
  set tmp_tar %target_char.next_in_room%
  if %target_char.vnum% == 19499
    eval spn %target_char%
  end
  set target_char %tmp_tar%
done
if (!%spn%)
  set Spawned 0
  global Spawned
end
~
$~
