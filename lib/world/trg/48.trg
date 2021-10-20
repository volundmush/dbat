#4800
Slave Market Guard greet 1~
0 g 100
~
wait 2 s
if %actor.is_PC%
  say You got business in the slave markets? Just say enter and I'll let you in...
%echo% @R(OOC) You could also try to @Cnod @Rat the guard.@n
else
  return 0
end
~
#4801
Greet for Market Guard exit~
0 g 100
~
wait 2 s
if %actor.is_PC%
  say Just say leave if you want to leave the slave markets.
%echo% @R(OOC) You could also try to @Cnod @Rat the guard.@n
else
  return 0
end
~
#4802
Enter slave markets~
0 d 100
enter~
wait 1 s
unlock gate
open gate
say Ok, hurry through.
wait 10 s
close gate
lock gate
~
#4803
Leave slave market trigger~
0 d 100
leave~
wait 1 s
unlock gate
open gate
say Ok, hurry through.
wait 10 s
close gate
lock gate
~
#4804
Slave Shop~
0 c 100
*~
if %cmd% == list
  %send% %actor%   ##  Available   Item                              Cost
  %send% %actor% ----------------------------------------------------------
  %send% %actor%   1)  Unlimited   Human Slave                       1000
  %send% %actor%   2)  Unlimited   Namek Slave                       1200
  %send% %actor%   3)  Unlimited   Mutant Slave                      2000
  %send% %actor%   4)  Unlimited   Kanassan Slave                    1200
  %send% %actor%   5)  Unlimited   Konatsu Slave                     1500
elseif %cmd% == buy
  if %actor.gold% == 0
    tell %actor.name% You have no gold!
    emote puke %actor.name%
    halt
  end
  if human /= %arg% || 1 == %arg%
    set mob_vnum 4801
    set gold_cost 1000
    set mob_name Human Slave
  elseif namek /= %arg% || 2 == %arg% 
    set mob_vnum 4802
    set gold_cost 1200
    set mob_name Namek Slave
  elseif mutant /= %arg% || 3 == %arg%
    set mob_vnum 4805
    set gold_cost 2000
    set mob_name Mutant Slave
  elseif kanassan /= %arg% || 4 == %arg%
    set mob_vnum 4806
    set gold_cost 1200
    set mob_name Kanassan Slave
  elseif konatsu /= %arg% || 5 == %arg%
    set mob_vnum 4809
    set gold_cost 1500
    set mob_name Konatsu Slave
  else
    whisper %actor.name% Which of these A-Grade slaves would you like to purchase?
    halt
  end
  if %actor.gold% < %gold_cost%
    whisper %actor.name% You don't have enough gold to purchase that slave!
  else
    nop %actor.gold(-%gold_cost%)%
    %load% mob %mob_vnum%
    set slave 1
    set target_char %self.room.people%
    while %target_char%
     set tmp_target %target_char.next_in_room%
     if (%target_char.alias% == %mob_name%)
      set slave %target_char%
     end
     set target_char %tmp_target%
    done
    whisper %actor.name% I hope you enjoy your new %mob_name%!
    %force% %slave% follow %actor.name%
  end
elseif %cmd% == sell
  whisper %actor.name% I don't purchase items from players.
else
  return 0
end
~
#4805
Slave Commands~
0 d 100
hello~
wait 2 s
%echo% The master is %self.master%
~
#4806
Dodoria/Ginyu Q4 Drop~
0 f 100
~
if (%actor.varexists(onDodQ4)% || %actor.varexists(onGinyuQ4)%)
  %load% obj 4807
end
~
#4807
Diamond Zenni Trade~
0 j 100
~
if (%object.vnum% == 5602)
%purge% %object%
  wait 2 s
  say Awe, wonderful. You have brought me another precious gem, I will add this to my collection.
  wait 2 s
  say Do not disappoint me, I do expect more. As for your payment.
  wait 2 s
  %send% %actor% %self.name% hands you a pouch of zenni.
  %echoaround% %actor% %self.name% hands %actor.name% a pouch of zenni.
nop %actor.gold(1000)%
elseif (%object.vnum% == 5603)
%purge% %object%
  wait 2 s
  say Awe, wonderful. You have brought me another precious gem, I will add this to my collection.
  wait 2 s
  say Do not disappoint me, I do expect more. As for your payment.
  wait 2 s
  %send% %actor% %self.name% hands you a pouch of zenni.
  %echoaround% %actor% %self.name% hands %actor.name% a pouch of zenni.
nop %actor.gold(600)%
elseif (%object.vnum% == 3201)
%purge% %object%
  wait 2 s
  say Awe, wonderful. You have brought me another precious gem, I will add this to my collection.
  wait 2 s
  say Do not disappoint me, I do expect more. As for your payment.
  wait 2 s
  %send% %actor% %self.name% hands you a pouch of zenni.
  %echoaround% %actor% %self.name% hands %actor.name% a pouch of zenni.
  nop %actor.gold(750)%
else
  return 0
  wait 1 s
  say That has no worth to me, begone, and dont come back unless you have something of real value.
end
~
#4808
Accept Glacier Quest~
0 g 100
~
If (%actor.varexists(GlacierQuest)%)
 Wait 1 s
 %Echo% @nLord turns around from the window with a smile stretched across his face. "@CAwe, so youve returned. I hope I did as I asked and brought me a precious gem or two. If you havent then understandingly you will not be getting paid.@n"
Else
 Wait 2 s
 %Echo% "@COh hello there, I am Lord Glacier, Duke of Slave City.@n" The Icerian Lord offers an elegant bow to show a small hint of courtesy. "@CHave you come to @Raccept@C my trade offer? If not I have no use for you.@n"
 Wait 5 s
 %Echo% @GTry Accept@n
end
~
#4809
Glacier Greet 2~
0 c 100
accept~
If (%cmd% == accept)
  %Echo% "@CVery good, have a seat. Would you like some wine?@n" Lord Glacier pours himself a glass of wine and continues talking. "@CI want your to collect precious gems for me. Diamonds, Emeralds, and Rubies. But! They must be unique and have real worth.@n" The Icer pauses to take a sip of the wine he didnt hand you. "@CI believe the task is pretty simple even for someone like you, and I will promise that on completion of this task I will pay you their intended worth.@n"
  Wait 2 s
  %Echo% "@CWell dont just stand there, get going.@n" Lord Glacier sits down and drinks the rest of his wine.
  Wait 5 s
  %Echo% "@CAre you really this dumb?! Get out of my sight!@n" The Icer screams before turning around in his chair to overlook Slave City.
  Set GlacierQuest
  Remote GlacierQuest %actor.id%
end
~
#4810
Emerald accept~
0 j 100
~
if (%object.vnum% == 5603)
%purge% %object%
  wait 2 s
  say Awe, wonderful. You have brought me another precious gem, I will add this to my collection.
  wait 2 s
  say Do not disappoint me, I do expect more. As for your payment.
  wait 2 s
  %send% %actor% %self.name% hands you a pouch of zenni.
  %echoaround% %actor% %self.name% hands %actor.name% a pouch of zenni.
  nop %actor.gold(1700)%
else
  return 0
  wait 1 s
  say That has no worth to me, begone, and dont come back unless you have something of real value.
end
~
#4811
Ruby accept~
0 j 100
~
if %object.vnum% == 3201
%purge% %object%
  wait 2 s
  say Awe, wonderful. You have brought me another precious gem, I will add this to my collection.
  wait 2 s
  say Do not disappoint me, I do expect more. As for your payment.
  wait 2 s
  %send% %actor% %self.name% hands you a pouch of zenni.
  %echoaround% %actor% %self.name% hands %actor.name% a pouch of zenni.
  nop %actor.gold(750)%
else
  return 0
  wait 1 s
  say That has no worth to me, begone, and dont come back unless you have something of real value.
end
~
#4892
Enter/Exit for Mutes~
0 c 100
nod~
say Quiet bugger, aren't ya?
wait 2 s
%send% %actor% Grumbling, the guard unlocks the gate.
wait 2 s
unlock gate
open gate
wait 1 s
say Alright, go ahead.
wait 10 s
close gate
lock gate
~
$~
