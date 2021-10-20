#15902
Outlaw Jim~
0 h 100
~
wait 1 s
say What do you want, chump? Scram before I dust ya.
~
#15903
Outlaw Jim Block~
0 q 100
~
if (%direction% == north)
  %send% %actor% %self.name% prevents you from moving to the south.
  %echoaround% %actor% As %actor.name% tries to move to the south but %self.name% stops %actor.himher%
  say Only members allowed in here!
  return 0
else
  return 1
end
~
#15936
Videl Portal~
2 c 100
enter~
if (%cmd% == enter && %arg% == portal)
  wait 1 s
  %send% %actor% You step through the hazy portal and find yourself on the other side!
  wait 1 s
  %echoaround% %actor% %actor.name% enters the portal and vanishes!
  %teleport% %actor% 15938
  %at% %actor% %force% %actor% look
else
  return 0
  %send% %actor% Try entering the portal.
end
~
#15938
Videl Portal 2~
2 c 100
enter~
If (%cmd% == enter && %arg% == portal)
    wait 1 s
    %send% %actor% You step through the hazy portal and find yourself on the other side!
    wait 1 s
    %echoaround% %actor% %actor.name% enters the portal and vanishes!
    %teleport% %actor% 15936
    %at% %actor% %force% %actor% look
Else
    return 0
    %send% %actor% Try entering the portal.
End
~
#15939
Videl Greet~
0 g 100
~
if (%actor.is_pc%)
if (!%actor.varexists(vidqcomplete)% && !%actor.varexists(onvidq)%)
wait 1 s
say @COh thank goodness! You there! You seem like you're strong enough! Please you've got to help me. This time rip brought me here but I don't know what to do! A strange maniac is casting dark magics on my friend and forcing him to destroy the whole city! Please, @Gaccept@C my plea!@n
elseif (%actor.varexists(onvidq)%)
wait 1 s
say @CPlease hurry! That maniac will only wreak more havoc!
else
wait 1 s
say @COh, it's you! Thank you again so much for your help! Now we can begin rebuilding.@n
end
~
#15940
Videl Accept~
0 cd 3
Accept~
if (%actor.varexists(vidqcomplete)%)
wait 1 s
say You've already helped so much, don't worry yourself.
elseif (%actor.varexists(onvidq)%)
wait 1 s
say Haven't you already agreed to help me?! Please hurry!
else
wait 1 s
say Oh, thank you! In the very ruins of the School is where this maniac is, defeat him and bring me proof. I can't go home until I know he's dead!
set onvidq
remote onvidq %actor.id%
end
~
#15941
Videl Receive~
0 j 100
~
If (!%actor.varexists(onvidq)%)
    return 0
    wait 1 s
    say I don't want this.
Else
    if (%object.vnum% != 15998)
    return 0
    wait 1 s
    say This isn't what I asked for.
Elseif (%actor.varexists(vidqcomplete)%)
    return 0
    wait 1 s
    say I don't know how you got this again, but keep it.
Else
    %purge% %object.name%
    wait 1 s
    say You've ... really done it. Thank you! Thank you so much! Here, take this ... maybe I won't need them anymore.
    %load% obj 15999
    give pads %actor.name%
    rdelete onvidq %actor.id%
rdelete talms %actor.id%
    set vidqcomplete
    remote vidqcomplete %actor.id%
  End
End
~
#15998
Follower Death II~
0 f 100
~
If (%actor.varexists(talms)%)
    Mload o 15967
Elseif (%actor.varexists(vidqcomplete)%)
    Mload o 15967
Else
    Set talms
    Remote talms %actor.id%
    Mload o 15998
end
~
#15999
undefined~
0 f 100
~
* No Script
~
$~
