#6800
Otherworld Pilot~
0 g 100
~
wait 1 s
say Pay me 20 zenni and I will fly you back to King Yemma's
~
#6801
Pilot Fly~
0 m 0
~
wait 1
if (%amount% > 20)
  say Thanks for the tip. Hop aboard.
  %teleport% %actor% 6030
  mgoto 6030
  wait 1
  %force% %actor% look
  wait 2 s
  %send% %actor% %self.name% gets behind the flight-stick of the small plane and starts the engine.
  wait 4 s
  %send% %actor% The plane takes off the incredibly short runway flying through the yellow skies of the afterlife.
  wait 5 s
  %send% %actor% The plane sets down at the Higher Plane Docks gently and in an odd way.
  wait 3 s
  say I hope you enjoyed the flight.
  smile
  %teleport% %actor% 6029
  mgoto 6800
elseif (%amount% < 20)
  return 0
  say That wasn't enough, keep the money...
else
  say Hop aboard.
  %teleport% %actor% 6030
  mgoto 6030
  wait 1
  %force% %actor% look
  wait 2 s
  %send% %actor% %self.name% gets behind the flight-stick of the small plane and starts the engine.
  wait 4 s
  %send% %actor% The plane takes off the incredibly short runway flying through the yellow skies of the afterlife.
  wait 5 s
  %send% %actor% The plane sets down at Higher Plane Docks gently and in an odd way.
  wait 3 s
  say I hope you enjoyed the flight.
  smile
  %teleport% %actor% 6029
  mgoto 6800
end
~
#6802
Iovan Sage Greet~
0 g 100
~
if (!%actor.varexists(good_dedras)%)
 wait 1 s
  %echo% %self.name% looks around.
 wait 1 s
  say The Great Iovan has proclaimed that the first one to silence the evil and dangerous Dedras shall receive a mighty reward!
 wait 4 s
  say Bring me the heart of this foul beast and the reward is yours! Do not delay. Other forces contrary to Iovan's will seek the heart as well!
end
~
#6803
Iovan Sage Reward~
0 j 100
~
if !%actor.varexists(good_dedras)%
 if (%object.vnum% == 7305)
  %purge% %object%
  wait 1
   say GLORIOUS DAY! That foul beast no longer breathes and the heart is safely delivered!
  wait 2 s
   say I did promise you a reward from the Great Iovan, did I not? Here you go!
  wait 1
  %load% obj 75
  drop symbol
  %force% %actor% get symbol
  set good_dedras
  remote good_dedras %actor.id%
 else
  return 0
  wait 1
   say I do not want this!
 end
else
 wait 1
 say You've already been rewarded. You have my blessing though.
 drop all
end
~
$~
