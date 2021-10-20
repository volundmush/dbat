#15400
Do NOT REMOVE THIS HAT BLARGH!~
1 c 3
rem~
If (remove /= %cmd% || remove == %cmd%)
return 1
%send% %actor% NO! IT'S YOUR BIRTHDAY AND YOU WILL PARTY!
%echoaround% %actor% %actor.name% parties because it's %actor.hisher% birthday!
end
~
#15490
Bracer Influence TP trigger test~
1 b 50
~
if (%self.worn_by%)
  eval ch %self.worn_by%
  %send% %ch% You feel the sudden and savage urge to give in to the rage that inexpicably builds up within your soul!
  wait 2s
else
end
~
#15498
Trigger for xeno fighters~
0 m 1
~
if (%amount% >= 2500000)
 return 0
 wait 1
 say Sorry, but that model is not available at this time.
 give %amount% zenni %actor.name% 
elseif (%amount% >= 400000)
 return 0
 wait 1
 say Sorry, but that model is not available at this time.
  give %amount% zenni %actor.name%
elseif (%amount% >= 150000)
 return 0
 wait 1
 say Sorry, but that model is not available at this time.
  give %amount% zenni %actor.name%
elseif (%amount% >= 50000)
 eval numret %amount% - 50000
 if (%numret% > 0)
  set %actor.gold(%numret%)%
  wait 1
  %send% %actor% You are given your change back.
  wait 2 s
 end
  wait 1
  %echo% %self.name% has some documents signed and then begins to look through a drawer full of ship keys.
  if (%amount% - %numret% <= 50000)
   set finded 0
   remote finded %actor.id%
   eval gowhere 17277
   while (%dne% != done)
    set fail nofail
    eval gowhere %gowhere% + 1
    mgoto %gowhere%
    eval count 46099
    while (%dne% != done && %fail% != failed)
     eval count %count% + 1
     if (%self.room.contents(%count%)%)
      set finded %count%
      remote finded %actor.id%
      set dne done
     elseif (%count% >= 46199)
      set fail failed
     end
    done
    if (%gowhere% >= 17295)
     set dne done
    end
   done
   mgoto 17277
   if (%actor.finded% == 0)
    wait 1
    %echo% %self.name% Stops rummaging through the keys and looks up.
    wait 2 s
    say It appears that we are out of that model currently. Here's a refund.
    eval refund %amount% - %numret%
    give %refund% zenni %actor.name%
   else
    wait 1
    %echo% %self.name% holds up a pair of keys.
    wait 1 s
    eval keyo %actor.finded% + 1000
    %load% o %keyo%
    rdelete finded %actor.id%
    give key %actor.name%
    drop key
    wait 1
    say I recommend you get your ship out of here. If it's left here it will be reclaimed as property of Celestial Corp.
   end
 end
elseif (%amount% >= 15000)
 eval numret %amount% - 15000
 if (%numret% > 0)
  set %actor.gold(%numret%)%
  wait 1
  %send% %actor% You are given your change back.
  wait 2 s
 end
  wait 1
  %echo% %self.name% has some documents signed and then begins to look through a drawer full of ship keys.
  if (%amount% - %numret% <= 15000)
   set finded 0
   remote finded %actor.id%
   eval gowhere 17277
   while (%dne% != done)
    set fail nofail
    eval gowhere %gowhere% + 1
    mgoto %gowhere%
    eval count 45999
    while (%dne% != done && %fail% != failed)
     eval count %count% + 1
     if (%self.room.contents(%count%)%)
      set finded %count%
      remote finded %actor.id%
      set dne done
     elseif (%count% >= 46099)
      set fail failed
     end
    done
    if (%gowhere% >= 17295)
     set dne done
    end
   done
   mgoto 17277
   if (%actor.finded% == 0)
    wait 1
    %echo% %self.name% Stops rummaging through the keys and looks up.
    wait 2 s
    say It appears that we are out of that model currently. Here's a refund.
    eval refund %amount% - %numret%
    give %refund% zenni %actor.name%
   else
    wait 1
    %echo% %self.name% holds up a pair of keys.
    wait 1 s
    eval keyo %actor.finded% + 1000
    %load% o %keyo%
    rdelete finded %actor.id%
    give key %actor.name%
    drop key
    wait 1
    say I recommend you get your ship out of here. If it's left here it will be reclaimed as property of Celestial Corp.
   end
 end
else
 return 0
 wait 1
 say The cheapest model we offer is 15,000 zenni. You can't explore the depths of space while being a bum!
 give %amount% zenni %actor.name%
end
~
#15499
Kanji Pulse~
1 c 3
pulse~
%echoaround% %actor% @wThe %self.shortdesc%@w on %actor.name%'s back @rpulses@w and seems to briefly radiate a @mghostly purple aura@w which quickly fades away.@n
%send% %actor% @wThe %self.shortdesc%@w on your back @rpulses@w and seems to briefly radiate a @mghostly purple aura@w which quickly fades away.@n
~
$~
