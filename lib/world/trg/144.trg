#14400
Gosho greet~
0 g 100
~
wait 2 s
say Ah! Are you here about the notice?
wait 2 s
say It'd be a great help if you were to find something for me.
wait 2 s
say One of the glowworms from the village have gone missing, putting the colony in a state of unrest.
wait 2 s
say Everyone in the village would appreciate it, and there's even a nice reward.
~
#14401
Gosho quest complete~
0 j 100
~
if %object.vnum% == 14411
wait 2 s
say Thank goodness you found it!
wait 2 s
say Words cannot express my gratitude, so please, take this as a reward.
wait 2 s
%load% obj 14412
give basket %actor.name%
end
~
#14402
Floating Spirit greet~
0 g 100
~
wait 2 s
emote glares at %actor.name%.
wait 2 s
say Why are you here disturbing me?
wait 2 s
say If you would be so kind, say 'leave' and I will remove you from here, so that I may continue my resting in peace.
~
#14403
Floating spirit teleport~
2 d 100
*~
if %speech% == leave
wait 1 s
%force% floating say Thank you.
wait 2 s
%send% %actor% You find yourself elsewhere.
wait 1 s
%teleport% %actor% 14448
%force% %actor% look
end
~
#14404
Peaceful spirit teleport/greet~
0 g 100
~
wait 2 s
say You have done well to make it here, %actor.name%.
wait 2 s
say However, your ordeal is not yet over.
wait 2 s
%echo% %self.name% laughs boisterously.
wait 1 s
%send% %actor% You are enveloped by the mist!
wait 1 s
%send% %actor% The mist disperses and you find yourself elsewhere!
%teleport% %actor% 14468
%force% %actor% look
~
#14405
Jirou greeting~
0 g 100
~
wait 2 s
say Ahh, welcome to my store, %actor.name%.
wait 2 s
say I apologize that my wares are a bit lacking today, but I have yet to receive new materials. 
~
#14406
Kaito shouts~
0 g 100
~
wait 1 s
say Fresh fruits!
~
#14407
No ki~
2 c 100
ch~
if (%cmd% == ch || %cmd% == cha || %cmd% == char || %cmd% == charg || %cmd% == charge)
return 1
%send% %actor% Something about the area disrupts your ability to focus your ki!
end
~
#14408
No punch~
2 c 100
pu~
if (%cmd% == pu || %cmd% == pun || %cmd% == punc || %cmd% == punch)
return 1
%send% %actor% You feel that won't be very effective!
end
~
#14409
no kick~
2 c 100
ki~
if (%cmd% == ki || %cmd% == kic || %cmd% == kick)
return 1
%send% %actor% You feel that won't be very effective!
return 1
end
~
#14410
no throw~
2 c 100
th~
if (%cmd% == th || %cmd% == thr || %cmd% == thro || %cmd% == throw)
return 1
%send% %actor% You feel that won't be very effective!
return 1
end
~
#14411
no elbow~
2 c 100
el~
if (%cmd% == el || %cmd% == elb || %cmd% == elbo || %cmd% == elbow)
return 1
%send% %actor% You feel that won't be very effective!
return 1
end
~
#14412
no roundhouse~
2 c 100
ro~
if (%cmd% == ro || %cmd% == rou || %cmd% == roun || %cmd% == round || %cmd% == roundh || %cmd% == roundho || %cmd% == roundhou || %cmd% == roundhous || %cmd% == roundhouse)
return 1
%send% %actor% You feel that won't be very effective!
return 1
end
~
#14413
no uppercut~
2 c 100
up~
if (%cmd% == up || %cmd% == upp || %cmd% == uppe || %cmd% == upper || %cmd% == upperc || %cmd% == uppercu || %cmd% == uppercut)
return 1
%send% %actor% You feel that won't be very effective!
return 1
end
~
#14414
no heeldrop~
2 c 100
he~
if (%cmd% == he || %cmd% == hee || %cmd% == heel || %cmd% == heeld || %cmd% == heeldr || %cmd% == heeldro || %cmd% == heeldrop)
return 1
%send% %actor% You feel that won't be very effective!
return 1
end
~
#14415
no slam~
2 c 100
sl~
if (%cmd% == sl || %cmd% == sla || %cmd% == slam)
return 1
%send% %actor% You feel that won't be very effective!
return 1
end
~
#14416
no attack~
2 c 100
at~
if (%cmd% == at || %cmd% == att || %cmd% == atta || %cmd% == attac || %cmd% == attack)
return 1
%send% %actor% You feel that won't be very effective!
return 1
end
~
#14417
no focus~
2 c 100
fo~
if (%cmd% == fo || %cmd% == foc || %cmd% == focu || %cmd% == focus)
return 1
%send% %actor% You feel that won't be very effective!
return 1
end
~
#14493
Spirit Teleport Mute~
2 c 100
leave~
wait 1 s
%force% floating say Thank you.
wait 2 s
%send% %actor% You find yourself elsewhere.
wait 1 s
%teleport% %actor% 14448
%force% %actor% look
~
$~
