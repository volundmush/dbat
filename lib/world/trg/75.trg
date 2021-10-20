#7500
Kami Tower Portal IN (1)~
1 c 100
enter~
if %arg% == portal
 if %actor.level% < 40
  %send% %actor% @WYou feel like it would be a bad idea to enter this portal.@n
  wait 1 s
  %send% %actor% @ROOC@n - @WYou need to be a higher level to enter this portal.@n
 else
  if %actor.varexists(deadzonesnare)%
   rdelete deadzonesnare %actor.id%
   wait 1 s
   %send% %actor% You walk toward the portal and get sucked inside it!
   %echoaround% %actor% %actor.name% walks toward the portal and is sucked inside it!
   wait 1 s
   %teleport% %actor% 7500
   %force% %actor% look
  else
   wait 1 s
   %send% %actor% You walk toward the portal and get sucked inside it!
   %echoaround% %actor% %actor.name% walks toward the portal and is sucked inside it!
   wait 1 s
   %teleport% %actor% 7500
   %force% %actor% look
  end
 end
end
~
#7501
HELL PORTAL IN~
1 c 100
enter~
if %arg% == portal
 if %actor.level% < 40
  %send% %actor% @WYou feel like it would be a bad idea to enter this portal.@n
  wait 1 s
  %send% %actor% @ROOC@n - @WYou need to be a higher level to enter this portal.@n
 else
  wait 1 s
  %send% %actor% You walk toward the portal and get sucked inside it!
  %echoaround% %actor% %actor.name% walks toward the portal and is sucked inside it!
  wait 1 s
  %teleport% %actor% 7599
  %force% %actor% look
 end
end
~
#7502
Snared and Can't Move~
1 q 100
~
if %actor.varexists(deadzonesnare)%
 return 0
 %send% %actor% @WYou are currently ensared by the portal to the Dead Zone!@n
 wait 1 s
 %send% %actor% @WTo break free of its snare, you need to @Yresist@n.
end
~
#7503
ENSARE ALL THE PEOPLEZ~
2 g 100
~
if (!%actor.varexists(deadzonesnare)% && %actor.level% > 39)
 set deadzonesnare
 remote deadzonesnare %actor.id%
 wait 1 s
 %send% %actor% You have been ensared by the portal here!
 wait 2 s
 %send% %actor% To break free from its snare, you must @Yresist@n
end
~
#7504
BREAK FREE MY CHILD~
1 c 100
resist~
if (%cmd% == resist && %actor.varexists(deadzonesnare)%)
 if %actor.str% < 40
  wait 1 s
  rdelete deadzonesnare %actor.id%
  %send% %actor% You try to resist the snare of the portal but fail and are sucked inside it!
  %echoaround% %actor% %actor.name% tries to resist the snare of the portal but gets sucked inside it!
  %teleport% %actor% 7500
  wait 1 s
  %force% %actor% look
 else
  eval resistance %random.4%
  switch %resistance%
   case 1
    wait 1 s
rdelete deadzonesnare %actor.id%
    %send% %actor% You try to resist the snare of the portal but fail and are sucked inside it!
    %echoaround% %actor% %actor.name% tries to resist the snare of the portal but gets sucked inside it!
    %teleport% %actor% 7500
    wait 1 s
    %force% %actor% look
   break
   case 2
    wait 1 s
    %send% %actor% You continue to resist the snare of the portal, but are slowly being pulled in.
    %echoaround% %actor% %actor.name% is slowly being pulled into the portal.
wait 10 s
   break
   case 3
    wait 1 s
%send% %actor% You continue to resist the snare of the portal, but are unable to break free!
%echoaround% %actor% %actor.name% resists the snare of the portal, but cannot yet break free!
wait 10 s
   break
   default
    wait 1 s
    rdelete deadzonesnare %actor.id%
    %send% %actor% You manage to break free from the snare of the portal!
    %echoaround% %actor% A look of relief shows on %actor.name%'s face as they break free of the portals snare.
   break
  done
 end
end
~
#7505
KAMI TOWER OUT~
1 c 100
enter~
if %arg% == portal
 if (!%actor.inventory(7504)%)
  wait 1 s
  %send% %actor% The portal doesn't seem to react to your presence.
  wait 2 s
  %send% %actor% @GHint@n: Perhaps you require a special gem to pass through?
 elseif %actor.dead%
  wait 1 s
  %send% %actor% The dead cannot pass through this portal.
 elseif (%actor.inventory(7504)% && !%actor.dead%)
  %purge% %actor.inventory(7504)%
  wait 1 s
  %send% %actor% You jump through the portal and end up at Kami's tower!
  %echoaround% %actor% %actor.name% jumps through the portal!
  wait 1 s
  %teleport% %actor% 1506
  %send% %actor% The gem of arbitration seems to have vanished.
  %force% %actor% look
 end
end
~
#7506
HELL PORTAL OUT~
1 c 100
enter~
if %arg% == portal
 if (!%actor.dead% && %actor.race% != demon)
  wait 1 s
  %send% %actor% The living cannot pass through this portal.
 elseif (%actor.race% == demon)
  wait 1 s
  %send% %actor% You jump through the portal and end up in Hell!
  %echoaround% %actor% %actor.name% jumps through the portal.
  %teleport% %actor% 6312
  wait 1 s
  %force% %actor% look
 else
  wait 1 s
  %send% %actor% You jump through the portal and end up in Hell!
  %echoaround% %actor% %actor.name% jumps through the portal.
  %teleport% %actor% 6312
  wait 1 s
  %force% %actor% look
 end
end
~
$~
