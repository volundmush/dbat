#1400
Fireplace trigger~
1 ab 25
~
if (%self.timer% <= 0)
 if (%self.room.vnum% != 1440 && %self.room.vnum% != 1447)
  wait 10 s
  %echo% The campfire dies out leaving behind nothing but a pile of ash.
  if (!%self.room.contents(1613)%)
   %load% o 1613
  end
  %purge% self
 end
else
 eval num %random.10%
 switch %num%
  case 1
   wait 10 s
   %echo% A stick in the fire pops.
   break
  case 2
   wait 10 s
   %echo% The fire flashes brightly before growing dimmer again.
   break
  case 3
  case 4
  case 5
   wait 10 s
   %echo% Sparks fly up from the fire into the air.
   break
  default
  wait 10 s
  %echo% The fire gives off thick smoke for a moment.
  break
 done
end
~
#1401
Chief Bora Greet~
0 g 100
~
if (%actor.align% > -51)
wait 3 sec
%echo% Chief Bora raises his hand in greeting.
wait 3 sec
say Greetings stranger.
wait 2 sec
say If you are going to be here make sure you respect the tower of Korin.
wait 4 sec
say I am charged with protecting it from those who do not deserve to touch it.
wait 5 sec
say I hope you are someone deserving.
else
wait 1 s
say You had better leave now, you are not welcome. I sense evil about you.
end
~
#1402
Upa Greet~
0 g 100
~
if (%actor.align% > -51)
wait 3 sec
greet %actor.name%
wait 2 sec
say If you are going to be in this area, you had better speak with my father first.
wait 4 sec
say If you already have and were welcomed, then I too welcome you.
wait 5 sec
say By the way, if you happen to have a deer pelt, I will trade a rare rainbow trout for it.
wait 4 sec
say That is all I have to trade at the moment.
else
wait 1 s
say You don't look like a nice person, you had better leave.
end
~
#1403
Korin's Greet~
0 g 100
~
if (%actor.align% > -51)
wait 2 sec
say Hello there...
wait 3 sec
say I can tell you have traveled far warrior.
wait 5 sec
say If you come in search of senzu beans, I believe I can help you.
wait 4 sec
say First you need to do something for me... I am craving the taste of a rare fish.
wait 5 sec
say It is called a rainbow trout. If you can bring me a fresh one, I will give you a senzu.
else
wait 1 sec
say I can sense the evil coming from you. You will get nothing from me.
end
~
#1404
Korin Receive~
0 j 100
~
if (%actor.align% > -51 && %object.vnum(1409)%)
 wait 1 sec
 %purge% %object%
 say AH you got it. Thanks I really couldn't wait to get this.
 wait 2 sec
 say Here you go then, one senzu. Don't waste it, they are not easy to come by.
 %load% obj 1
 wait 1
 drop bean
 %force% %actor% get senzu
 wait 3 sec
 say Now then...
 wait 1 sec
elseif (%actor.align% > -51 && !%object.vnum(1409)%)
 wait 2 sec
 say What am I going to do with this?
 wait 1 sec
 drop %object.name%
else
 wait 1 s
 say I don't want anything from the likes of you!
 wait 1 s
 drop %object.name%
end
~
#1405
Kami Greet~
0 g 100
~
if (%actor.align% > -51)
wait 2 sec
say Greetings warrior. Welcome to my lookout.
wait 4 sec
say You are welcome to train in the hyperbolic time chamber, or to follow Mr. Popo here to the Pendulum room.
wait 5 sec
say If you go to the Hyperbolic Time Chamber you need to remember to be very careful while inside.  One could get lost quite easily. Make your visits inside count, warrior.
else
wait 2 sec
say You are an evil person, warrior, and I do not like you being here. Leave.
end
~
#1406
Mr Popo Greet~
0 g 100
~
if (%actor.align% >= -51)
wait 3 sec
wait 1
say Hello warrior. Have you spoken with Master Kami?
wait 3 sec
say I can let you into the pendulum room and allow you to battle enemies from the past within your mind.
wait 4 sec
say You will not be in any danger if you die there, but will return to the Pendulum room.
wait 4 sec
say If you would like to do this then tell me 'Let's Go'
else
wait 1 sec
say You are an evil person, please leave.
end
~
#1407
Mr Popo Speech~
0 d 100
Let's~
if (%speech% == Let's Go || %speech% == Let's Go. || %speech% == let's go)
 if (%actor.align% >= -51)
  wait 1 sec
  say Alright, we shall go to the Pendulum room.
  wait 2 s
  mteleport %actor% 1561
  mgoto 1561
  %send% %actor% Mr. Popo leads you through the west side of the Lookout until you arrive at the door of the Pendulum room.
  %echoaround% %actor% Mr. Popo arrives with %actor.name% following.
  wait 2 s
  %force% %actor% look
  wait 1 s
  unlock door
  wait 1
  open door
  wait 1
  w
  wait 1
  %force% %actor% w
  close door
  lock door
  wait 2 sec
  say Alright now. Close your eyes slowly and clear your mind.
  wait 4 sec
  %send% %actor% You slowly close your eyes and find yourself transported across time and space.
  wait 2 sec
  mteleport %actor% 1580
  %force% %actor% look
  mgoto 1589
  %load% mob 1409
  mgoto 1556
  %purge% pendulum
  %purge% self
  else
  wait 1 s
  say You are not welcome to use the Pendulum room...
  end
else
 wait 1
 say Whatever that means...
end
~
#1408
Pumpkin Death~
0 f 100
~
mload mob 1410
~
#1409
Artichoke Load~
0 n 100
~
wait 1
%echo% Suddenly a tall lean saiyan busts up through the floor.
wait 2 sec
cackle
wait 1 sec
say You are doomed weakling!
~
#1410
Artichoke death~
0 f 100
~
set pendcomp
remote pendcomp %actor.id%
mteleport %actor% 1562
%at% 1562 mload mob 1411
~
#1411
Pendulum Comp~
0 n 100
~
wait 1
set warrior %self.next_in_room%
set target_char %self.room.people%
 while %target_char%
  set tmp_war %target_char.next_in_room%
if (%tmp_war% != %self% && %tmp_war%)
set warrior %tmp_war%
  end
  set target_char %tmp_war%
 done
mload obj 1450
wait 1 s
mteleport %warrior% 1562
mgoto 1562
wait 1 s
%send% %warrior% You open your eyes slowly and find yourself in the pendulum room.
%force% %warrior% look
wait 1 s
if %warrior.varexists(pendcomp)%
 eval rand %random.3%
 wait 1
 if (%rand% >= 3)
  say Well done warrior. You have proven your worth.
  wait 1 sec
  say Here, take this as proof of your deed.
  mload obj 1452
  give pearl %warrior.name%
  rdelete pendcomp %self.next_in_room.id%
  unlock door
  open door
  wait 1
  %force% %warrior% e
  close door
  lock door
  %purge% key
  %purge% self
 else
  say Well done warrior. You have proven your worth.
  wait 1 sec
  say I usually have a pearl shard, but I'm out right now.
  wait 3 sec
  say Here, take this as proof of your deed.
  mload obj 14015
  mload obj 14015
  mload obj 14015
  mload obj 14015
  mload obj 14015
  give all.orb %warrior.name%
  rdelete pendcomp %warrior.id%
  unlock door
  open door
  wait 1
  %force% %warrior% e
  close door
  lock door
  %purge% key
  %purge% self
 end
else
 wait 1 s
 say It seems you did not defeat both saiyans somehow. You will have to try again later.
 wait 2 s
 unlock door
 open door
 wait 1
 %force% %warrior% e
 close door
 lock door
 %purge% key
 %purge% self
end
~
#1412
Pendulum Comp~
0 g 100
~
if %actor.varexists(pendcomp)%
wait 1
say Well done warrior. You have proven your worth.
wait 1 sec
say Here, take this as proof of your deed.
mload obj 1452
give pearl %actor.name%
rdelete pendcomp %actor.id%
%purge% key
%purge% self
~
#1413
Upa Receive~
0 j 100
~
if (%object.vnum% == 1604)
%purge% pelt
wait 1
say This is exactly what I was looking for, this is great.
wait 4 s
say This is a very high quality pelt as well.
wait 4 s
say Here you go, for this pelt you may have this rare rainbow trout. I caught it just recently.
wait 1 s
mload obj 1409
give rainbow %actor.name%
wait 4 s
%echo% Upa takes the pelt and stuffs it in a sack strapped over his shoulder.
else
wait 1
say What is this? Oh well, thanks anyway...
end
~
#1414
Krillin Quest Buffalo MEat~
0 f 100
~
if (%actor.varexists(onkq2)% || %actor.varexists(onGohanQ2)% || %actor.varexists(onTienQ2)%)
    %load% obj 1410
end
~
#1415
TenQ Bobcat Pelt Drop~
0 f 100
~
if (%actor.varexists(onTenQ2)%)
   %load% obj 1411
end
~
#1416
Campfire Timer~
1 n 100
~
set %self.timer(8)%
~
$~
