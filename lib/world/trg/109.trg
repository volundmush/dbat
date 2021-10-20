#10900
Majin Hypergate~
1 c 100
candybeam~
If %arg% == hypergate
 If (%actor.race% != majin)
  Wait 1s
  %send% %actor%  @MYou can't do that, meat sack!@n
 Else
  Set candygate
  Remote candygate %actor.id%
  Wait 1s
  %send% %actor% With a flick of your forelock, you fire a beam of energy into the empty event horizon of the Hyper Gate!
  %echoaround% %actor% %actor.name% flicks %actor.hisher% forelock and fires a beam of energy at the empty event horizon of the Hyper Gate!
  Wait 3s
  %send% %actor% As the candy-colored beam of energy fills the Gate's horizon, your body begins to pull and stretch out like taffy as it starts to get pulled toward the now-swirling Hyper Gate!
  %echoaround% %actor% As %actor.name%'s candy-colored beam of energy fills the Gate's horizon, %actor.hisher% body starts to stretch out like taffy as it begins to be pulled toward the now-swirling Hyper Gate!
  Wait 3s
  %force% %actor% drag
  %teleport% %actor% 3796
  Rdelete candygate %actor.id%
  %send% %actor% Sucked through time and space, your body slowly coils itself into a puddle and reforms!
  %echoaround% %actor% Drawn through the swirl of colors above like a string of pulled taffy, %actor.name%'s body slowly coils onto the ground and reforms!
  %force% %actor% look
 Else
  Return 0
End
End
~
#10901
Majinton Portal~
2 c 100
portal~
If (%actor.race% != Majin)
  %send% %actor% @RThis place is not for you.@n
Else
  Set candygate
  Remote candygate %actor.id%
  If (%arg% == earth || %arg% == Earth)
   Wait 1s
   %send% %actor% The swirls of color seem to shift, and you can see Earth on the other side! Your body begins to get sucked up into the portal, elongating like a stretchy piece of rubber!
   %echoaround% %actor% %actor.name%'s body begins to stretch out like a soft piece of rubber, before being sucked up into the giant swirl of color!
   wait 3s
   %teleport% %actor% 1420
   %send% %actor% Slithering out of the Hyper Gate, your body coils into a puddle on the ground and reforms!
   %echoaround% %actor% Suddenly %actor.name%'s body is slithering out through the horizon of the Hyper Gate, and %actor.hisher% body coils onto the ground in a puddle and reforms!
   Rdelete candygate %actor.id%
   %force% %actor% look
  Elseif (%arg% == aether || %arg% == Aether)
   wait 1s
   %send% %actor% The swirls of color seem to shift, and you can see Earth on the other side! Your body begins to get sucked up into the portal, elongating like a stretchy piece of rubber!
   %echoaround% %actor% %actor.name%'s body begins to stretch out like a soft piece of rubber, before being sucked up into the giant swirl of color!
   wait 3s
   %teleport% %actor% 12340
   %send% %actor% Slithering out of the Hyper Gate, your body coils into a puddle on the ground and reforms!
   %echoaround% %actor% Suddenly %actor.name%'s body is slithering out through the horizon of the Hyper Gate, and %actor.hisher% body coils onto the ground in a puddle and reforms!
   Rdelete candygate %actor.id%
   %force% %actor% look
  Elseif (%arg% == frigid || %arg% == Frigid)
   wait 1s
   %send% %actor% The swirls of color seem to shift, and you can see Earth on the other side! Your body begins to get sucked up into the portal, elongating like a stretchy piece of rubber!
   %echoaround% %actor% %actor.name%'s body begins to stretch out like a soft piece of rubber, before being sucked up into the giant swirl of color!
   wait 3s
   %teleport% %actor% 4924
   %send% %actor% Slithering out of the Hyper Gate, your body coils into a puddle on the ground and reforms!
   %echoaround% %actor% Suddenly %actor.name%'s body is slithering out through the horizon of the Hyper Gate, and %actor.hisher% body coils onto the ground in a puddle and reforms!
   Rdelete candygate %actor.id%
   %force% %actor% look
  Elseif (%arg% == konack || %arg% == Konack)
   wait 1s
   %send% %actor% The swirls of color seem to shift, and you can see Earth on the other side! Your body begins to get sucked up into the portal, elongating like a stretchy piece of rubber!
   %echoaround% %actor% %actor.name%'s body begins to stretch out like a soft piece of rubber, before being sucked up into the giant swirl of color!
   wait 3s
   %teleport% %actor% 8968
   %send% %actor% Slithering out of the Hyper Gate, your body coils into a puddle on the ground and reforms!
   %echoaround% %actor% Suddenly %actor.name%'s body is slithering out through the horizon of the Hyper Gate, and %actor.hisher% body coils onto the ground in a puddle and reforms!
   Rdelete candygate %actor.id%
   %force% %actor% look
  Elseif (%arg% == vegeta || %arg% == Vegeta)
   wait 1s
   %send% %actor% The swirls of color seem to shift, and you can see Earth on the other side! Your body begins to get sucked up into the portal, elongating like a stretchy piece of rubber!
   %echoaround% %actor% %actor.name%'s body begins to stretch out like a soft piece of rubber, before being sucked up into the giant swirl of color!
   wait 3s
   %teleport% %actor% 2879
   %send% %actor% Slithering out of the Hyper Gate, your body coils into a puddle on the ground and reforms!
   %echoaround% %actor% Suddenly %actor.name%'s body is slithering out through the horizon of the Hyper Gate, and %actor.hisher% body coils onto the ground in a puddle and reforms!
   Rdelete candygate %actor.id%
   %force% %actor% look
  Elseif (%arg% == namek || %arg% == Namek)
   wait 1s
   %send% %actor% The swirls of color seem to shift, and you can see Earth on the other side! Your body begins to get sucked up into the portal, elongating like a stretchy piece of rubber!
   %echoaround% %actor% %actor.name%'s body begins to stretch out like a soft piece of rubber, before being sucked up into the giant swirl of color!
   wait 3s
   %teleport% %actor% 10983
   %send% %actor% Slithering out of the Hyper Gate, your body coils into a puddle on the ground and reforms!
   %echoaround% %actor% Suddenly %actor.name%'s body is slithering out through the horizon of the Hyper Gate, and %actor.hisher% body coils onto the ground in a puddle and reforms!
   Rdelete candygate %actor.id%
   %force% %actor% look
  Else
   return 1
   %send% %actor% Try using @mportal @Y<@Wdestination@Y>@n. Valid destinations are @GEarth@n, @cFrigid@n, @gNamek@n, and @CAether@n.
End
End
~
$~
