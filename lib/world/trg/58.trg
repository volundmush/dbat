#5800
Earth-Spacemall~
0 m 100
~
if (%amount% == 100)
   wait 1 sec
   say All aboard!
   wait 1 sec
   set intransport spacemall
   remote intransport %actor.id%
   mteleport %actor.name% 5824
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 10 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 10 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 10 sec
   %send% %actor% The ship begins to slow down as it approaches the destination.
   wait 10 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 17200
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   if (%actor.varexists(selfalias)%)
     %echoaround% %actor% %actor.selfalias% disembarks from a ship.
    else
     %echoaround% %actor% %actor.race% disembarks from a ship.
   end 
else
  say Sorry, I can't do anything with this much.
  give %amount% zenni %actor.name%
end
~
#5801
Spacemall-Earth~
0 m 100
~
if (%amount% == 100)
   wait 1 sec
   say All aboard!
   wait 1 sec
   set intransport earth
   remote intransport %actor.id%
   mteleport %actor.name% 5825
   set target_char %self.room.people%
   wait 1 sec
   %send% %actor% The ship roars as you take your seat and begins its journey.
   wait 10 sec
   %send% %actor% Stars begin to fly by as the ship reaches full speed.
   wait 10 sec
   %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
   wait 10 sec
   %send% %actor% The ship begins to slow down as it approaches the destination.
   wait 10 sec
   %send% %actor% The ship begins to shake as it settles down.
   wait 1 sec
   mteleport %actor% 408
   %force% %actor% look
   %send% %actor% You have arrived, and disembark.
   if (%actor.varexists(selfalias)%)
     %echoaround% %actor% %actor.selfalias% disembarks from a ship.
    else
     %echoaround% %actor% %actor.race% disembarks from a ship.
   end 
else
  say Sorry, I can't do anything with this much.
  give %amount% zenni %actor.name%
end
~
#5802
Zel cleanup~
2 b 100
~
set target_char %self.people%
while %target_char%
 set tmp_target %target_char.next_in_room%
 if (%target_char.varexists(intransport)%)
  if (%target_char.intransport% == earth)
  wait 40 sec
   if (%target_char.room% == 5825)
     %teleport% %target_char% 408
   else
    return 0
   end
  elseif (%target_char.intransport% == spacemall)
   wait 40 sec
   if (%target_char.room% == 5824)
    %teleport% %target_char% 17200
   else
    return 0
   end
  end
 end
 set target_char %tmp_target%
done
~
$~
