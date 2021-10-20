#17200
Clown greet~
0 g 100
~
wait 1
emote chuckles, &1Hey there! How about a gag gift?!&2 The clown chuckles again and honks his nose.
~
#17207
Faye Plushy~
1 j 100
~
wait 2 sec
%send% %actor% The Faye "Missing Requiem" Plushy comes to life as you grasp it.
%echoaround% %actor% The Faye "Missing Requiem" Plushy comes to life as %actor.name% grasps it.
wait 2 sec
%send% %actor% The Faye "Missing Requiem" Plushy clasps her arms in a prayer and starts to sing a sweet melodious song.
%echoaround% %actor% The Faye "Missing Requiem" Plushy clasps her arms in a prayer and starts to sing a sweet melodious song.
wait 2 sec
%send% %actor% The Faye "Missing Requiem" Plushy dances and spins majestically as she continues to sing.
%echoaround% %actor% The Faye "Missing Requiem" Plushy dances and spins majestically as she continues to sing.
wait 2 sec
%send% %actor% The Faye "Missing Requiem" Plushy bows to you as she stops dancing and singing, beaming at you.
%echoaround% %actor% The Faye "Missing Requiem" Plushy bows to %actor.name% as she stops dancing and singing, beaming at %actor.himher%.
wait 2 sec
%send% %actor% The Faye "Missing Requiem" Plushy says, "Thank you for listening. Press my hand again to replay my song."
%echoaround% %actor% The Faye "Missing Requiem" Plushy says, "Thank you for listening. Press my hand again to replay my song."
~
#17290
Serra Mallins, Greet~
0 g 100
~
wait 2 s
%echo% %self.name% rustles some papers as she looks through them.
wait 2 s
%echo% %self.name% looks up.
wait 3 s
say Greetings. May I interest you in one of our economy model ships? Freshly refurbished and with battle scratches buffed out of the surface?
wait 5 s
say The prices are listed on the sign. Take your time...
~
#17291
Serra Mallins~
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
$~
