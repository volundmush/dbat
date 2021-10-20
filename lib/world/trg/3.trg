#300
Public Transport Trigger One~
0 j 100
~
if (%object.vnum% == 304)
  if (%self.room.vnum% == 408)
   return 0
   wait 1
   say You are already on earth.
  else
   %purge% %object%
   wait 1 sec
   say All aboard!
   wait 1
   set intransport earth
   remote intransport %actor.id%
   mteleport %actor.name% 48
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 2 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 3 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 2 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 408
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   %echoaround% %actor% %actor.name% disembarks from a ship.
  end
elseif (%object.vnum% == 305)
  if (%self.room.vnum% == 4250)
   return 0
   wait 1
   say You are already on Frigid.
  else
   %purge% %object%
   wait 1 sec
   say All aboard!
   wait 1
   set intransport frigid
   remote intransport %actor.id%
   mteleport %actor.name% 48
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 2 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 3 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 2 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 4250
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   %echoaround% %actor% %actor.name% disembarks from a ship.
  end
elseif (%object.vnum% == 308)
  if (%self.room.vnum% == 2323)
   return 0
   wait 1
   say You are already on Vegeta.
  else
   %purge% %object%
   wait 1 sec
   say All aboard!
   wait 1
   set intransport vegeta
   remote intransport %actor.id%
   mteleport %actor.name% 48
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 2 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 3 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 2 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 2323
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   %echoaround% %actor% %actor.name% disembarks from a ship.
  end
elseif (%object.vnum% == 307)
  if (%self.room.vnum% == 11626)
   return 0
   wait 1
   say You are already on Namek.
  else
   %purge% %object%
   wait 1 sec
   say All aboard!
   wait 1
   set intransport namek
   remote intransport %actor.id%
   mteleport %actor.name% 48
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 2 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 3 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 2 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 11626
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   %echoaround% %actor% %actor.name% disembarks from a ship.
  end
elseif (%object.vnum% == 11614)
  if (%self.room.vnum% == 12002)
   return 0
   wait 1
   say You are already on Aether.
  else
   %purge% %object%
   wait 1 sec
   say All aboard!
   wait 1
   set intransport aether
   remote intransport %actor.id%
   mteleport %actor.name% 48
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 2 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 3 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 2 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 12002
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   %echoaround% %actor% %actor.name% disembarks from a ship.
  end
elseif (%object.vnum% == 306)
  if (%self.room.vnum% == 8194)
   return 0
   wait 1
   say You are already on Konack.
  else
   %purge% %object%
   wait 1 sec
   say All aboard!
   wait 1
   set intransport konack
   remote intransport %actor.id%
   mteleport %actor.name% 48
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 2 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 3 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 2 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 8194
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   %echoaround% %actor% %actor.name% disembarks from a ship.
  end
else
 say That isn't something I want...
 give %object.name% %actor.name%
end
~
#301
Hyper Cat~
0 ab 10
~
eval dest %random.20000%
eval rmn %self.room.vnum%
if (%people.dest% == 0)
 %echo% Lighting storms around %self.alias% as her body begins to glow. Suddenly she vanishes leaving behind a loud roar of thunder and a piercing yowl!
 mgoto %dest%
 if (%rmn% != %self.room.vnum%)
  %zoneecho% %self.room.vnum% A sudden shockwave of energy seems to have erupted nearby as if something has just ripped its way through time and space.
 end
elseif (%people.dest% > 0)
 %echo% Lightning storms around %self.alias% as her body begins to glow. Suddenly she vanishes leaving behind a loud roar of thunder and a piercing yowl!
 mgoto %dest%
 if (%rmn% != %self.room.vnum%)
  %zoneecho% %self.room.vnum% A sudden shockwave of energy seems to have erupted nearby as if something has just ripped its way through time and space.
 end
end
~
#302
Hyper Cat Command~
0 c 100
blink~
if (%arg%)
 eval dest %arg%
 eval rmn %self.room.vnum%
 %echo% Lightning storms around %self.alias% as her body begins to glow. Suddenly she vanishes leaving behind a loud roar of thunder and a piercing yowl!
 mgoto %dest%
 if (%rmn% != %self.room.vnum%)
  %zoneecho% %self.room.vnum% A sudden shockwave of energy seems to have erupted nearby as if something has just ripped its way through time and space.
 end
end
~
#303
Mcstuff Greet~
0 g 100
~
wait 2 sec
say Hello, may I take your order?
~
#304
Mcstuff Trigger 2~
0 n 100
~
%load% obj 310
%load% obj 311
%load% obj 4227
put cheeseburger meal
put drink meal
put chips meal
~
#305
Jimmy Jim Jones Greet~
0 g 100
~
wait 1
mecho Jimmy flips through the documents in a folder.
wait 2 sec
say Bah
wait 1 sec
mecho Jimmy slams the folder back into his filing cabinet.
wait 2 sec
mecho Jimmy grabs another folder and starts flipping through it.
~
#306
Public Transport Trigger Two~
0 j 100
~
if (%object.vnum% == 304)
  if (%self.room.vnum% == 408)
   return 0
   wait 1
   say You are already on earth.
  else
   %purge% %object%
   wait 1 sec
   say All aboard!
   wait 1
   set intransport earth
   remote intransport %actor.id%
   mteleport %actor.name% 48
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 2 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 3 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 2 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 408
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   %echoaround% %actor% %actor.name% disembarks from a ship.
  end
elseif (%object.vnum% == 305)
  if (%self.room.vnum% == 4250)
   return 0
   wait 1
   say You are already on Frigid.
  else
   %purge% %object%
   wait 1 sec
   say All aboard!
   wait 1
   set intransport frigid
   remote intransport %actor.id%
   mteleport %actor.name% 48
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 2 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 3 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 2 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 4250
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   %echoaround% %actor% %actor.name% disembarks from a ship.
  end
elseif (%object.vnum% == 308)
  if (%self.room.vnum% == 2323)
   return 0
   wait 1
   say You are already on Vegeta.
  else
   %purge% %object%
   wait 1 sec
   say All aboard!
   wait 1
   set intransport vegeta
   remote intransport %actor.id%
   mteleport %actor.name% 48
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 2 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 3 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 2 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 2323
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   %echoaround% %actor% %actor.name% disembarks from a ship.
  end
elseif (%object.vnum% == 307)
  if (%self.room.vnum% == 11626)
   return 0
   wait 1
   say You are already on Namek.
  else
   %purge% %object%
   wait 1 sec
   say All aboard!
   wait 1
   set intransport namek
   remote intransport %actor.id%
   mteleport %actor.name% 48
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 2 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 3 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 2 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 11626
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   %echoaround% %actor% %actor.name% disembarks from a ship.
  end
elseif (%object.vnum% == 11614)
  if (%self.room.vnum% == 12002)
   return 0
   wait 1
   say You are already on Aether.
  else
   %purge% %object%
   wait 1 sec
   say All aboard!
   wait 1
   set intransport aether
   remote intransport %actor.id%
   mteleport %actor.name% 48
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 2 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 3 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 2 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 12002
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   %echoaround% %actor% %actor.name% disembarks from a ship.
  end
elseif (%object.vnum% == 306)
  if (%self.room.vnum% == 8194)
   return 0
   wait 1
   say You are already on Konack.
  else
   %purge% %object%
   wait 1 sec
   say All aboard!
   wait 1
   set intransport konack
   remote intransport %actor.id%
   mteleport %actor.name% 48
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 2 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 3 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 2 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 8194
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   %echoaround% %actor% %actor.name% disembarks from a ship.
  end
else
 say That isn't something I want...
 give %object.name% %actor.name%
end
~
#307
Reject Ticket~
0 j 100
~
return 0
say Wait your turn!
~
#310
Experience Room Trigger~
2 g 100
~
if %actor.is_pc%
  set room %self.vnum%
  eval number %room.strlen%
  switch %number%
    case 3
      set zone %room.charat(1)%
    break
    case 4
      set 1st %room.charat(1)%
      set 2nd %room.charat(2)%
      set zone %1st%%2nd%
    break
    case 5
      set 1st %room.charat(1)%
      set 2nd %room.charat(2)%
      set 3rd %room.charat(3)%
      set zone %1st%%2nd%%3rd%
    break
  done
  if !%actor.varexists(explored_%zone%)%
    set explored_%zone% 1
    remote explored_%zone% %actor.id%
    if %actor.varexists(zones_explored)%
      eval zones_explored %actor.zones_explored% + 1
      remote zones_explored %actor.id%
      eval recent_exploration_points %zones_explored% * 250
      eval exploration_points %zones_explored% * 250
    else
      set recent_exploration_points 250
      set exploration_points 250
      set zones_explored 1
      remote zones_explored %actor.id%
    end
    remote exploration_points %actor.id%
    wait 1
    %send% %actor% You have discovered a new area in which you have never explored before,
    %send% %actor% gaining %recent_exploration_points% experience for your findings.   
    %send% %actor% You have earned a total of %exploration_points% Exploration Points during your travels.
    set %actor.exp(%recent_exploration_points%)%
  end
end
~
#311
Slap Officer~
0 c 100
slap~
if (%arg% == officer|| %arg% == police|| %arg% == nexus)
return 0
wait 1
say Assualting an officer eh?
punch %actor.name%
end
~
#312
Car 'splodes~
1 b 100
~
if (%self.health% <= 0)
 %echo% %self.name% explodes! Fiery fragments of twisted metal are flung in all directions!
 %purge% %self%
elseif (%self.health% < 80 && %random.4% >= 2)
%echo% %self.name%'s car alarm blares loudly while its lights flash rapidly!
end
~
#313
Gus Hotdog Selling~
0 g 100
~
wait 1 s
say Fresh Hotdogs! Get your fresh hotdogs here! Hotdogs or chili dogs!
~
#314
Street Light Rand~
1 ab 50
~
if (%time.hour% == 18 && %self.cost% == 2)
 wait 1 s
 %echo% The streetlight slowly starts to glow as the sun begins to sink into the horizon.
 set %self.cost(-1)%
elseif (%time.hour% == 5 && %self.cost% == 1)
 wait 1 s
 %echo% The streetlight quickly turns off as the sun glows in the distance.
 set %self.cost(1)%
end
 
if ((%time.hour% >= 18 || %time.hour% <= 5) && %self.cost% == 1)
  eval lnum %random.15%
   switch %lnum%
    case 1
     %echo% The streetlight flickers slightly.
    break
    case 2
     %echo% Bugs fly around the streetlight.
    break
    default
     return 0
    break
   done
end
~
#350
D.Radar Capsule trigger~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  %load% obj 12
  wait 2 s
  %echo% The smoke clears, revealing the capsule's contents.
  %purge% self
else
 return 0
end
~
#351
Hydration Capsule trigger~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  set i 0
  while %i% < 10
    %load% obj 69
    eval i %i% + 1
  done
  wait 2 s
  %echo% The smoke clears, revealing the capsule's contents.
  %purge% self
else
 return 0
end
~
#352
Ration Capsule~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  set i 0
  while %i% < 10
    %load% obj 68
    eval i %i% + 1
  done
  wait 2 s
  %echo% The smoke clears, revealing the capsule's contents.
  %purge% self
else
 return 0
end
~
#353
Survival Capsule Trigger~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  set f 0
  while %f% < 10
    %load% obj 68
    eval f %f% + 1
  done
  set d 0
  while %d% < 10
    %load% obj 69
    eval d %d% + 1
  done
  set l 0
  while %l% < 5
    %load% obj 317
    eval l %l% + 1
  done
  %load% obj 61
  %load% obj 312
  wait 2 s
  %echo% The smoke clears, revealing the capsule's contents.
  %purge% self
else
 return 0
end
~
#354
Lock Capsule Trigger~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  set i 0
  while %i% < 5
    %load% obj 18
    eval i %i% + 1
  done
  wait 2 s
  %echo% The smoke clears, revealing the capsule's contents.
  %purge% self
else
 return 0
end
~
#355
Forgery Capsule trigger~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  set i 0
  while %i% < 5
    %load% obj 19
    eval i %i% + 1
  done
  wait 2 s
  %echo% The smoke clears, revealing the capsule's contents.
  %purge% self
else
 return 0
end
~
#356
Bandages Capsule~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  set i 0
  while %i% < 5
    %load% obj 47
    eval i %i% + 1
  done
  wait 2 s
  %echo% The smoke clears, revealing the capsule's contents.
  %purge% self
else
 return 0
end
~
#357
repair capsule trigger~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  set i 0
  while %i% < 5
    %load% obj 48
    eval i %i% + 1
  done
  wait 2 s
  %echo% The smoke clears, revealing the capsule's contents.
  %purge% self
else
 return 0
end
~
#358
Limb Capsule trigger~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  set i 0
  while %i% < 5
    %load% obj 66
    eval i %i% + 1
  done
  wait 2 s
  %echo% The smoke clears, revealing the capsule's contents.
  %purge% self
else
 return 0
end
~
#359
Kit Capsule Trigger~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  set l 0
  while %l% < 5
    %load% obj 18
    eval l %l% + 1
  done
  set f 0
  while %f% < 5
    %load% obj 19
    eval f %f% + 1
  done
  set b
  while %b% < 5
    %load% obj 47
    eval b %b% + 1
  done
  set r 0
  while %r% < 5
    %load% obj 48
    eval r %r% + 1
  done
  wait 2 s
  %echo% The smoke clears, revealing the capsule's contents.
  %purge% self
else
 return 0
end
~
#360
Training Dummy LVL One~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  wait 1 s
  %echo% The smoke clears, revealing a training dummy!
  %load% m 310
  wait 1
  %send% %actor% The dummy will self destruct after four hours time, so use it quickly.
  %purge% self
else
 return 0
end
~
#361
Training Dummy LVL Two~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  wait 1 s
  %echo% The smoke clears, revealing a training dummy!
  %load% m 311
  wait 1
  %send% %actor% The dummy will self destruct after four hours time, so use it quickly.
  %purge% self
else
 return 0
end
~
#362
Training Dummy LVL Three~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  wait 1 s
  %echo% The smoke clears, revealing a training dummy!
  %load% m 312
  wait 1
  %send% %actor% The dummy will self destruct after four hours time, so use it quickly.
  %purge% self
else
 return 0
end
~
#363
Training Dummy LVL Four~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  wait 1 s
  %echo% The smoke clears, revealing a training dummy!
  %load% m 313
  wait 1
  %send% %actor% The dummy will self destruct after four hours time, so use it quickly.
  %purge% self
else
 return 0
end
~
#364
Training Dummy Timer~
0 ab 100
~
if (%self.varexists(dummytimer)%)
 if (%self.dummytimer% == %time.hour%)
  %echo% %self.name% self destructs in a minor explosion!
  wait 1
  %purge% self
 end
end
~
#365
Training Dummy Load~
0 n 100
~
     if (%time.hour% <= 19)
      eval tme %time.hour% + 4
     elseif (%time.hour% == 20)
      eval tme 1
     elseif (%time.hour% == 21)
      eval tme 2
     elseif (%time.hour% == 22)
      eval tme 3
     elseif (%time.hour% == 23)
      eval tme 4
     end
     set dummytimer %tme%
     remote dummytimer %self.id%
~
#366
Training Dummy LVL Five~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  wait 1 s
  %echo% The smoke clears, revealing a training dummy!
  %load% m 315
  wait 1
  %send% %actor% The dummy will self destruct after four hours time, so use it quickly.
  %purge% self
else
 return 0
end
~
#367
Training Dummy LVL Six~
1 c 3
click~
if (%self.name% /= %arg%)
  %send% %actor% You click %self.name% then throw it on the ground.
  %echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
  wait 2 s
  %echo% A cloud of smoke surrounds the area.
  wait 1 s
  %echo% The smoke clears, revealing a training dummy!
  %load% m 316
  wait 1
  %send% %actor% The dummy will self destruct after four hours time, so use it quickly.
  %purge% self
else
 return 0
end
~
$~
