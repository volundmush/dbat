#3700
Majinton Portal~
2 c 100
enter~
if %arg% == portal
 if %actor.level% < 100
  %send% %actor% You feel like it would be a bad idea to enter this portal.
  wait 1 s
  %send% %actor% OOC - You need to be a higher level to enter this portal.
 else
  wait 1 s
  %send% %actor% You walk toward the portal and get sucked inside it!
  %echoaround% %actor% %actor.name% walks toward the portal and is sucked inside it!
  wait 1 s
  %teleport% %actor% 3667
  %force% %actor% look
 end
end
~
#3701
Kilos Greet~
0 g 50
~
if %actor.race% != Majin
 wait 2 s
 emote ruffles through some paperwork and grumbles to himself.
 wait 2 s
 say You aren't Majinkind, so I got nothing for ya. Scram.
else
 wait 2 s
 emote ruffles through some paperwork and grumbles to himself.
 wait 2 s
 say You don't look like one of my factory workers..
 wait 2 s
 say Tell you what, go see Easy Pete on the first floor.
 wait 2 s
 say Buy a candy requisition order form from him and then give it to me, I'll hook you up with some product.
 wait 2 s
 %send% %actor.name% @ROOC@n - @WGive the candy order forms ONE AT A TIME@n
end
~
#3702
Kilos Candy Script~
0 j 100
~
if %object.vnum% == 3704
 if %actor.race% != Majin
  return 0
  wait 2 s
  say You're not a majin. This is no good to you.
  wait 1 s
 else
  %purge% %object%
  wait 2 s
  say Well, the order form is good. Here's your candy.
  eval candy %random.3%
  switch %candy%
   case 1
    %load% obj 93
    give jawbreaker %actor.name%
    break
   case 2
    %load% obj 94
    give chocolate %actor.name%
    break
   default
    %load% obj 95
    give chocolate %actor.name%
   done
 end
else
 return 0
 say I don't want this!
end
~
#3703
Easy Pete Greet~
0 g 100
~
if %actor.race% != Majin
  wait 1 s
  say I don't mind you browsing my wares, but they're only good for Majin-kind.
  wait 2 s
  say Just a polite warning.
end
~
$~
