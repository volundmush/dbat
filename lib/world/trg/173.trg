#17300
Energy Shield Timer~
2 c 100
swipe~
if (!%actor.has_item(17300)%)
 wait 1
 %send% %actor% You don't seem to have the right keycard to swipe.
elseif (%self.south(bits)% == DOOR)
 wait 1
 %send% %actor% The energy shield is already down.
else
 wait 1
 %echoaround% %actor% %actor.name% swipes a keycard at the security panel. A moment later the energy shield blocking access to the elevator powers down.
 %send% %actor% You swipe a keycard at the security panel. A moment later the energy shield blocking access to the elevator powers down.
 %door% 17300 2 flags a
 wait 5 sec
 %echo% The energy shield powers back on.
 %door% 17300 2 flags abc
end
~
#17301
Elevator Trigger~
2 c 100
push~
if (%arg% == one || %arg% == 1)
 if (%self.vnum% == 17301)
  %send% %actor% You are already on the first floor.
 else
  %echoaround% %actor% %actor.name% Pushes the floor one button.
  %send% %actor% You push the floor one button.
  wait 1 sec
  %echo% The elevator moves down to the first floor.
  %teleport% all 17301
 end
elseif (%arg% == two || %arg% == 2)
 if (%self.vnum% == 17302)
  %send% %actor% You are already on the second floor.
 elseif (%self.vnum% == 17301)
  %echoaround% %actor% %actor.name% Pushes the floor two button.
  %send% %actor% You push the floor two button.
  wait 1 sec
  %echo% The elevator moves up to the second floor.
  %teleport% all 17302
 elseif (%self.vnum% == 17366)
  %echoaround% %actor% %actor.name% Pushes the floor two button.
  %send% %actor% You push the floor two button.
  wait 1 sec
  %echo% The elevator moves down to the second floor.
  %teleport% all 17302
 elseif (%self.vnum% == 17391)
  %echoaround% %actor% %actor.name% Pushes the floor two button.
  %send% %actor% You push the floor two button.
  wait 1 sec
  %echo% The elevator moves down to the second floor.
  %teleport% all 17302
 end
elseif (%arg% == three || %arg% == 3)
 if (%self.vnum% == 17366)
  %send% %actor% You are already on the third floor.
 elseif (%self.vnum% == 17301)
  %echoaround% %actor% %actor.name% Pushes the floor three button.
  %send% %actor% You push the floor three button.
  wait 1 sec
  %echo% The elevator moves up to the third floor.
  %teleport% all 17366
 elseif (%self.vnum% == 17302)
  %echoaround% %actor% %actor.name% Pushes the floor three button.
  %send% %actor% You push the floor three button.
  wait 1 sec
  %echo% The elevator moves up to the third floor.
  %teleport% all 17366
 elseif (%self.vnum% == 17391)
  %echoaround% %actor% %actor.name% Pushes the floor three button.
  %send% %actor% You push the floor three button.
  wait 1 sec
  %echo% The elevator moves down to the third floor.
  %teleport% all 17366
 end
elseif (%arg% == four || %arg% == 4)
 if (%self.vnum% == 17391)
  %send% %actor% You are already on the fourth floor.
 elseif (%self.vnum% == 17301)
  %echoaround% %actor% %actor.name% Pushes the floor four button.
  %send% %actor% You push the floor four button.
  wait 1 sec
  %echo% The elevator moves up to the fourth floor.
  %teleport% all 17391
 elseif (%self.vnum% == 17366)
  %echoaround% %actor% %actor.name% Pushes the floor four button.
  %send% %actor% You push the floor four button.
  wait 1 sec
  %echo% The elevator moves up to the fourth floor.
  %teleport% all 17391
 elseif (%self.vnum% == 17302)
  %echoaround% %actor% %actor.name% Pushes the floor four button.
  %send% %actor% You push the floor four button.
  wait 1 sec
  %echo% The elevator moves up to the fourth floor.
  %teleport% all 17391
 end
else
 %send% %actor% You do not see that button on the panel.
end
~
#17302
Mechanic Trigger~
0 b 15
~
eval line %random.4%
switch %line%
 case 1
  %echo% The mechanic inspects several parts of a ship carefully.
  break
 case 2
  %echo% The mechanic starts tinkering with a ship's engine.
  break
 case 3
  say Do they stop to think before breaking shit I work on? No.....
  wait 1 s
  %echo% The mechanic grumbles on about his problems under his breath.
  break
 default
  say Hey, I'm busy working. If you don't mind getting the hell out of my way.
  break
done
~
#17303
Energy Shield Timer~
2 c 100
swipe~
if (!%actor.has_item(17320)%)
 wait 1
 %send% %actor% You don't seem to have the right keycard to swipe.
elseif (%self.east(bits)% == DOOR)
 wait 1
 %send% %actor% The energy shield is already down.
else
 wait 1
 %echoaround% %actor% %actor.name% swipes a keycard at the security panel. A moment later the energy shield blocking access to the warehouse powers down.
 %send% %actor% You swipe a keycard at the security panel. A moment later the energy shield blocking access to the warehouse powers down.
 %door% 17355 1 flags a
 wait 5 sec
 %echo% The energy shield powers back on.
 %door% 17355 1 flags abc
end
~
#17304
Third Floor West Shield~
2 c 100
swipe~
if (!%actor.has_item(17321)%)
 wait 1
 %send% %actor% You don't seem to have the right keycard to swipe.
elseif (%self.west(bits)% == DOOR)
 wait 1
 %send% %actor% The energy shield is already down.
else
 wait 1
 %echoaround% %actor% %actor.name% swipes a keycard at the security panel. A moment later the energy shield blocking access to the lab powers down.
 %send% %actor% You swipe a keycard at the security panel. A moment later the energy shield blocking access to the lab powers down.
 %door% 17371 3 flags a
 wait 5 sec
 %echo% The energy shield powers back on.
 %door% 17371 3 flags abc
end
~
#17305
Third Floor East Shield~
2 c 100
swipe~
if (!%actor.has_item(17321)%)
 wait 1
 %send% %actor% You don't seem to have the right keycard to swipe.
elseif (%self.east(bits)% == DOOR)
 wait 1
 %send% %actor% The energy shield is already down.
else
 wait 1
 %echoaround% %actor% %actor.name% swipes a keycard at the security panel. A moment later the energy shield blocking access to the holding pen powers down.
 %send% %actor% You swipe a keycard at the security panel. A moment later the energy shield blocking access to the holding pen powers down.
 %door% 17381 1 flags a
 wait 5 sec
 %echo% The energy shield powers back on.
 %door% 17381 1 flags abc
end
~
#17306
Captain Greet~
0 g 100
~
wait 1
say What are you doing here?
wait 2 s
say Outsiders aren't welcome on my space-station.
wait 3 s
say I'm responsible for the security of this space-station. You don't want to mess with me...
~
#17307
Yazto's Greet~
0 g 100
~
wait 2 sec
%echo% %self.name% looks at an important document.
wait 3 sec
%echoaround% %actor% %self.name% looks up at %actor.name% and frowns.
%send% %actor% %self.name% looks up at you and frowns.
wait 2 sec
say I'm far too busy to deal with you. Leave now before I call security.
~
#17308
Gein's Greet~
0 g 100
~
wait 2 s
%send% %actor% %self.name% turns towards you.
%echoaround% %actor% %self.name% turns towards %actor.name%.
wait 3 s
say Is it right for a crew member of this station to be out of uniform?
wait 4 s
say Things better shape up now that I'm here.
~
#17309
Fourth Floor Energy Shield~
2 c 100
swipe~
if (!%actor.has_item(17323)%)
 wait 1
 %send% %actor% You don't seem to have the right keycard to swipe.
elseif (%self.north(bits)% == DOOR)
 wait 1
 %send% %actor% The energy shield is already down.
else
 wait 1
 %echoaround% %actor% %actor.name% swipes a keycard at the security panel. A moment later the energy shield blocking access to the lab powers down.
 %send% %actor% You swipe a keycard at the security panel. A moment later the energy shield blocking access to the lab powers down.
 %door% 17391 0 flags a
 wait 5 sec
 %echo% The energy shield powers back on.
 %door% 17391 0 flags abc
end
~
#17310
Washio's Greet~
0 g 100
~
if (!%actor.varexists(washio)%)
  wait 1
  say Hmm. I haven't seen you before.
  wait 2 s
  say Well I leave worrying about security matters to my underlings so I'm not going to delve too far into your purpose here.
  wait 5 s
  say Though you look to be quite strong...
  wait 2 s
  say Would you like to earn a little bit of cash?
  wait 3 s
  say I need a certain item obtained for me. If you are willing you can earn some zenni.
  wait 3 s
  say Well not just zenni I guess. This is a research station afterall. My company, Chimera Tek, is in the business of manufacturing various weapons and fighting techniques.
  wait 6 s
  say If you obtain this object I seek then I will provide you with a secret technique as well.
  wait 4 s
  say What I want is a DNA sample. I've heard talk about these things called "bio-androids" and I hear that the best sample of one can be found on a very particular spaceship with an advanced laboratory and some of the best minds in science..
  wait 6 s
  say Bring me it and you shall receive the payment. Bwahaha
else
  wait 1
  say Ah. I don't have any more jobs you can do. Feel free to work on that technique though...
end
~
#17311
Washio Refuse~
0 c 100
pr~
if (!%actor.varexists(washio)%)
 return 1
 say You can't learn the skill I offer without giving me that sample!
else
return 0
end
~
#17312
Washio Receive~
0 j 100
~
if (%object.vnum% != 1805)
return 0
wait 1
say That's not what I want!
elseif (!%actor.varexists(washio)%)
 %purge% %object%
 wait 1
 say Aha! So you brought me a sample.
 wait 2 s
 %echo% %self.name% inspects the DNA sample for a few moments.
 wait 3 s
 say This is fantastic! Alright here you go. 10,000 zenni for you, and also I will now teach you the technique paralyze whenever you want.
 wait 1
 nop %actor.gold(10000)%
 set washio
 remote washio %actor.id%
else
 return 0
 wait 1
 say I already got a sample from you!
end
~
#17313
Bought Prism~
0 c 100
buy~
return 0
wait 1
if (%actor.has_item(1902)%)
 set BoughtPrism 0
 remote BoughtPrism %actor.id%
end
~
$~
