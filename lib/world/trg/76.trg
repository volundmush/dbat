#7600
Hydra Head #2 Spawn~
0 f 100
~
%load% mob 7601
~
#7601
Hydra Head #2 Appearance~
0 n 100
~
wait 1 s
%echo% The head that is defeated quickly retreats into the hole- only to be replaced by another!
~
#7602
Hydra Head #3 Spawn~
0 f 100
~
%load% mob 7602
~
#7603
Hydra Head #3 Appearance~
0 n 100
~
wait 1 s
%echo% The head that is defeated quickly retreats into the hole- only to be replaced by another!
~
#7604
Hydra Head #4 Spawn~
0 f 100
~
%load% mob 7603
~
#7605
Hydra Head #4 Appearance~
0 n 100
~
wait 1 s
%echo% The head that is defeated quickly retreats into the hole- only to be replaced by another!
~
#7606
Hydra Head #5 Spawn~
0 f 100
~
%load% mob 7604
~
#7607
Hydra Head #5 Appearance~
0 n 100
~
wait 1 s
%echo% The head that is defeated quickly retreats into the hole- only to be replaced by another!
~
#7608
Hydra Head #6 Spawn~
0 f 100
~
%load% mob 7605
~
#7609
Hydra Head #6 Appearance~
0 n 100
~
wait 1 s
%echo% The head that is defeated quickly retreats into the hole- only to be replaced by another!
~
#7610
Hydra Head #7 Spawn~
0 f 100
~
%load% mob 7606
~
#7611
Hydra Head #7 Appearance~
0 n 100
~
wait 1 s
%echo% The head that is defeated quickly retreats into the hole- only to be replaced by another!
~
#7612
8 Headed Hydra Spawn~
0 f 100
~
%load% mob 7607
~
#7613
8 Headed Hydra Angry!~
0 n 100
~
wait 1 s
%echo% Every head but one has been defeated.  Enraged, the Eight Headed Hydra bursts out of the small crevice, rocks skattering everywhere! It bellows, coming straight after you!
~
#7614
The Fall of Hydra~
0 f 100
~
%echo% The Eight Headed Hydra flails about, roaring angrily.  The roar shakes the entire mountain, eventually leveling off to nothing as the beast finally dies, drifting through the water, and hitting the bottom, with a thud.
%load% obj 7605
set HydraQuest dead
remote HydraQuest %actor.id%
~
#7615
Martuku Greet~
0 g 100
~
if (%actor.varexists(HydraQuest)%)
 if (%actor.HydraQuest% == done)
wait 1 s
  emote looks up to you, his face brightening, "@CAh! I'm glad to see you, do you need any more training?@n"
elseif (%actor.HydraQuest% == dead)
wait 2 s
  emote strides up to %actor.name% upon their entrance, looking them up and down in shock. "@CWhen I saw you run off, I couldn't believe you actually would do it! I figured my story about the Hydra would send you off runnin'!@n"  Despite his joke, he seemed honestly impressed about the feat.
  wait 5 s
  emote scratches under his fishy chin.  "@CI heard you fighting, and the Hydras bellows were certainly loud enough... and I could feel the beasts death cry.  So that leaves me with one thing.  I am a Kanassan of my word.@n"  He stretched, glancing around, "@CI will now teach you how to properly utilize your inner darkness.  How to go under a chance that will really bring it to light, so to speak.  But what am I saying, better to show you, than to explain.@n"  The Kanassan grins.
  set HydraQuest done
  remote HydraQuest %actor.id%
elseif (%actor.HydraQuest% == searching)
wait 1 s
 emote offers an apologetic smile. "@CLook, if we knew where it lurked, wed tell ya. Its out there somewhere.@n"
 wait 1 s
 emote narrows his eyes somewhat, "@CUnless.... youre actually scared and dont want to go fight it. I dont blame you, it is one of the most deadly beasts out there, save the Dreaded Griknaug.@n"
  end
else
 wait 1 s
 emote glances up from his position on the ground, noticing you.  He rises to his feet, dusting off his clothes as he steps forward to get a good look at you.  The old Kanassan narrows his eyes somewhat, then nods.  "@CYou have some darkness about you, traveller.  What brings you here?@n"
wait 6 s
 emote shakes his head, waving it off. "@CNo matter.  I'm surprised you made it here without being chased off by the beast that dwells here.@n"  After a moment or two, he raises his brow in surprise. "@CYou have not heard of the Eight Headed Hydra that calls this place home?  Why do you think no one stays here?@n"
wait 7 s
 emote nods his head to where his two companions are. "@COur crew has been staying here for a while now, trying to plan on how were going to take down this beast.  Do you have any idea how hard it is to kill something with eight heads snapping at you, blowing you up all at once?@n" After a mild pause, he shook his head. "@CI'm getting off track.  If you help us out, I can teach you how to truly control the darkness within you.@n" The Kanassan extended his hand, looking you in the eyes. "@CDo you accept?@n"
end
~
#7616
Martuku Accept~
0 c 100
accept~
if (%actor.varexists(HydraQuest)%)
 return 1
else
 set HydraQuest searching
 remote HydraQuest %actor.id%
 emote chuckles and says, "@CIts your funeral, kid.@n"
 wait 2 s
 emote gives a soft sigh, gesturing out widely, at the whole area, "@CSomewhere around here exists a big nasty creature called the Eight Headed Hydra.  My group and I have been tracking it for some time, and we cornered it to somewhere around here.@n"
 wait 4 s
 emote crosses his arms over his chest, "@CLike I told you before, if you can kill it, Ill teach you a technique that has helped me defeat beasts like that one for several years... and if you do fell this beast, youll be a true beast hunter.@n"
 wait 7 s
 emote frowns, "@CWell? What are you waiting for, go get it.  That is, unless you're scared?@n"
end
~
#7617
Martuku Practice~
0 c 100
pr~
if (%actor.align% > -50)
 return 1
 emote shakes his head and frowns. "@CNo... Your inner darkness... It is very weak.  This skill takes a toll on your morality side- and I cannot have your goody-two shoes side get in the way.@n"
elseif (!%actor.varexists(HydraQuest)%)
 return 1
 emote frowns and says, "Im not teaching you anything!"
else
 if (%actor.HydraQuest% == done)
 return 0
elseif (%actor.HydraQuest% == searching)
 return 1
 say I told you you have to go kill the hydra for me to teach you!
elseif (%actor.HydraQuest% == dead)
 return 1
 say Let me finish speaking!
end
end
~
#7618
Hydra's bubbles!~
2 g 100
~
if (%actor.varexists(HydraQuest)%)
 if (%actor.HydraQuest% == searching)
 wait 1 s
 %send% %actor% You notice some bubbles rising up from beneath you. Strange.
 end
elseif (!%actor.varexists(HydraQuest)%)
end
~
#7619
Rocky Crevice ~
2 c 100
inspect~
if (%arg% == crevice) 
 if (%actor.varexists(HydraQuest)%)
  if (%actor.HydraQuest% == done)
  wait 1 s
  %echoaround% %actor% %actor.name% investigates the rocky crevice, but finds nothing.
  %send% %actor% You slide into the rocky crevice, glancing around. This used to be where the hydra lived.  Thankfully, there's nothing that's taken over the empty space.
  elseif (%actor.HydraQuest% == searching)
  wait 1 s
  %echoaround% %actor% %actor.name% swims into the rocky crevice, but immediately backpedals as a serpentine head lunges out at them!
  %send% %actor% You swim into the rocky crevice, but suddenly sixteen different red eyes glow angrily at you.  You here a faint rumbling, and a hiss as a serpentine head snaps its teeth at you while you backpedal quickly!
  wait 2 s
  %load% mob 7600
  %force% %actor% look 
 set HydraQuest killing
  remote HydraQuest %actor.id%
  end
 elseif (!%actor.varexists(HydraQuest)%)
 wait 1 s
 %echo% %actor.name% swims up to the crevice, finds sixteen different sets of glowing eyes. There is a faint rumbling as %actor.name% swims away swiftly.
 end
else
wait 1 s
%send% %actor% Try inspecting the crevice!
end
~
#7620
Flee in Terror~
0 q 100
~
if (%direction% == up)
set HydraQuest searching
remote HydraQuest %actor.id%
%purge% %self%
end
~
#7621
PS item~
1 c 3
invoke~
if (%arg% == white || %arg% == lotus)
wait 1 s
%echo% %actor.name% grabs the stem of the lotus with both hands, closing their eyes and focusing their ki into the bud of the flower.
wait 2 s
%echo% %actor.name% opens their eyes and lets go of the lotus, watching as it rises up and and vanishes in a burst of light.
wait 2 s
%send% %actor.name% You feel... peaceful.
wait 2 s
eval num %random.1500%
eval %actor.prac(%num%)%
%send% %actor% @W<@GYou've received @Y%num%@G practice sessions!@W>@n
%purge% %self%
else
%send% %actor% Try to @Rinvoke@n the @Rlotus@n!
end
~
#7622
get lotus~
1 g 100
~
wait 1 s
%send% %actor% The @WL@no@Yt@nu@Ws@n seems to glow with a mysterious energy. You should try to invoke it with your energy. 
~
$~
