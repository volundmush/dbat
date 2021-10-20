#7300
Hop off!~
2 c 100
hop~
wait 1
%send% %actor% You leap from the edge of the cavern opening and fall down through the yellow clouds below!
%teleport% %actor% 7399
wait 1
%force% %actor% look
wait 1 sec
%send% %actor% Yellow clouds and the nearby cliff rush past you quickly as you plummet!
wait 3 sec
%send% %actor% Suddenly the cliffs next to you end in mid air while you rush past!
wait 2 sec
%send% %actor% You see Snake Way rushing up toward you. You barely manage to land on it with a crashing thud!
%teleport% %actor% 6038
~
#7301
Cave Sounds~
2 b 10
~
eval num %random.5%
switch (%num%)
 case 1
  %echo% The dripping sounds of the cavern grow louder for a moment before growing fainter.
 break
 case 2
  %echo% A rock falling can be heard in the distance followed by rapid movement that grows faint.
 break
 case 3
  %echo% A growl can be heard echoing from somewhere else in the cavern.
 break
 case 4
  %echo% It suddenly grows very quiet.
 break
 default
  %echo% The walls of the cavern groan and sense the floating cliffs shifting.
 break 
done
~
#7302
Yossos Blather~
0 b 20
~
eval num %random.4%
switch (%num%)
 case 1
  say Dedras demands to be fed...
 break
 case 2
  say Shall we feed the stinky one to Dedras?
  wait 1
  %echo% %self.name% snickers wickedly.
 break
 case 3
  say Food! Food!
 break
 default
  say Rip off their limbs and throw them in screaming!
 break 
done
~
#7303
Dedras Greet~
0 g 100
~
if (!%actor.varexists(dedras)%)
 wait 1 s
 %echo% %self.name% slowly stirs.
 wait 1 s
 say Ah so they have not failed to deliver my dinner after all. I was afraid I would have to punish them...
 wait 3 s
 say We can do this the easy way or the hard way. Easy way is you accepting your fate in my stomach...
 wait 3 s
 say The hard way ends the same way but wastes more energy than necessary. What do you say?
 wait 3 s
 say Well I'll choose for you then...
 wait 1
 set dedras
 remote dedras %actor.id%
 bite %actor.name%
else
 wait 1 s
 smile
 say My meal returns! Glorious!!!
 wait 1
 bite %actor.name%
end
~
#7304
Dumb Yossos~
0 g 100
~
wait 1 s
%echo% %self.name% notices he is not alone.
wait 2 s
say Uh... Hallos...
wait 2 s
%echo% %self.name% takes a long metal key out of his pocket and scratches his ass with it.
wait 3 s
%echo% %self.name% places the long metal key back into his pocket.
wait 3 s
say Arrrr yoooo here to uh... wait me forget what me goin ta ask...
wait 2 s
%echo% %self.name% scratches his small head with his large hand.
wait 3 s
say Oh well me give up... Hallos new tiny friend.
wait 1
smile
~
$~
