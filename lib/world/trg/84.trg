#8400
Climb up tree~
2 c 100
climb~
if "%arg%" != "tree"
  %send% %actor% Try climbing the tree!
else
   %send% %actor% You begin climbing up the large tree.
   %echoaround% %actor% %actor.name% grasps the branch of a large tree and climbs carefully upwards.
   wait 10 s
   wteleport %actor% 8431
   wait 1 s
   %force% %actor% look
   %send% %actor% After an arduous climb, you have arrived at a large branch.
   %echoaround% %actor% There's a rustle of leaves as %actor.name% emerges from the branches of a tree.
end
~
#8401
Climbing down tree~
2 c 100
climb~
if "%arg%" != "tree"
  %send% %actor% Try climbing the tree!
else
  %send% %actor% You begin climbing down the large tree.
  %echoaround% %actor% %actor.name% grasps the branch of a large tree and climbs carefully downwards.
  wait 10 s
  wteleport %actor% 8422
  wait 1 s
  %force% %actor% look
  %send% %actor% After an arduous climb, you have arrived at the base of the tree.
  %echoaround% %actor% There's a rustle of leaves as %actor.name% emerges from the branches above.
end
~
#8402
give reward~
0 j 100
~
if %object.vnum% == 8400
  mjunk all
  wait 2 s
  say Well done %actor.name%, killing a Karinak is no easy feat. The Hunters guild pays well for their husks.
  wait 1 s
  nop %actor.gold(800)%
  %send% %actor% %self.name% gives you a sack of zenni.
  %echoaround% %actor% %self.name% gives %actor.name% a sack of zenni.
elseif %object.vnum% == 8401
  if !%actor.varexists(huskrewarded)%
    mjunk all
    set huskrewarded
    remote huskrewarded %actor.id%
    wait 1 s
    say What? No way, this husk is massive, could it be? Yes it must be.
    wait 2 s
    say You've killed the Legendary Giant Karinak. It is only fitting I reward you with a rare piece of equipment usually reserved only for those who have obtained the rank of Master Hunter!
    wait 2 s
    eval reward %random.2%
    switch %reward%
      case 1
        %load% obj 8402
        give cloak %actor.name%
        break
      default
        %load% obj 8403
        give spear %actor.name%
        break
      done
    say Enjoy your reward.
  else
    say I already rewarded you for that!
    return 0
  end 
else
  say I don't want that!
  drop %object.vnum%
end
~
#8403
Cuthalion greet~
0 g 100
~
wait 2 s
say You there... bring me husks from the Karinak beasts that inhabit this forest's canopies and I'll reward you accordingly.
~
$~
