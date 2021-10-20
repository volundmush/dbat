#9900
Elwin Greet~
0 g 100
~
if (%actor.varexists(ElwinTrain)%)
 if (%actor.ElwinTrain% == done)
  wait 1 s
  say Have you come for more practice honing your inner darkness?
 else
  wait 1 s
  say Well? Have you returned with the amulet?
  wait 2 s
  say I want to see Alloralla dead!
 end
else
 wait 1 s
 emote floats up to %actor.name% slowly, raising his hands in defense. "@CStay your weapons, warrior.  I have no wish to harm you.@n"
wait 10 s
 emote looks the %actor.race% up and down curiously, somewhat surprised that they didn't attack. "@CInteresting.  You seem different than the others.  I have a proposition for you.  A millenia ago, I was killed, in cold blood.@n" He gestures around to the ruined citadel. "@CI was a warlock of house Kashvar, yes, and I was amidst practicing a new spell when all hell broke loose.@n"
 wait 7 s
 emote shakes his head forlornly, "@CThe man that killed me is long dead, having grown old with age.  This does not please me.  I cannot move on from this realm until I have been given my vengeance.@n" The ghost grips his hand tight, frowning heavily. "@CSo I ask you, %actor.name%, to avenge me.  You will be rewarded if you @Raccept@C.@n"
 wait 10 s
 %send% %actor.name% @DTry @Raccept@n
end
~
#9901
Elwin Accept~
0 c 100
accept~
if %actor.varexists(superbuu)%
  wait 1s
  say You have aligned yourself with evil already.  I'll not train you.
elseif %actor.varexists(kktrain)%
  wait 1s
  say You have aligned yourself with good already.  I'll not train you.
elseif %actor.varexists(ElwinTrain)%
  if (%actor.ElwinTrain% == done)
    wait 1 s
    say You've already completed my mission for you.
  else
    say I already told you! Bring back Alloralla's necklace!
  end
elseif %actor.align% > 50
  wait 1s
  say No... No... your inner morality... Why would you help me?  No, I cannot trust you...
else
  set ElwinTrain hunting
  remote ElwinTrain %actor.id%
  wait 1 s
  say Good.  I was hoping you would say that.
  wait 2 s
  emote clasps his hands behind his back, "@CNow, my murderer is long since dead, however.  He had children, and his children had children, and so on.  The last of his line lies in a woman known as Alloralla.  I believe now she lives in the Dres area.@n"
  wait 7 s
  emote gestures towards the exit, "@CBring me her amulet, and I shall teach you a technique that I have spent millenia perfecting.  I am sure you will find that a fair trade, no?@n"
end
~
#9902
Elwin Receive~
0 j 100
~
if (%object.vnum% == 9902)
 if (%actor.varexists(ElwinTrain)%)
  if (%actor.ElwinTrain% == done)
   return 1
   wait 1 s
   say I don't need this anymore.
  else
  set ElwinTrain done
  remote ElwinTrain %actor.id%
  wait 1 s
  say Ahh... this is exactly the proof I need.
  wait 2 s
  say Very well.  I will teach you Dark Metamorphosis.  Be wary, though, for it takes a heavy toll on the user.
  wait 1 s
  grin
  end
 else
 return 1
 say I don't want this.
 end
else
return 1
wait 1 s
say I don't want this.
end
~
#9903
Elwin Prac~
0 c 100
pr~
if (%actor.align% > 50)
 return 1
 emote shakes his head and frowns. "No... Your inner darkness.  I can't have your morality getting in the way of training with me.
else
  if (%actor.varexists(superbuu)%)
   return 1
   say I will not train those who have learned from Buu.
  elseif (%actor.varexists(kktrain)%)
   return 1
   say I won't train someone who have learned from King Kai!
  elseif (!%actor.varexists(ElwinTrain)%)
   return 1
   say I'm not teaching you anything!
  else
   if (%actor.ElwinTrain% == hunting)
    return 1
    say I won't train you until you bring me the amulet!
   elseif (%actor.ElwinTrain% == done)
    return 0
   end
  end
end
~
#9904
Alloralla Death~
0 f 100
~
if (%actor.varexists(ElwinTrain)%)
 if (%actor.ElwinTrain% == hunting)
 %load% o 9902
 else
 end
else
end
~
$~
