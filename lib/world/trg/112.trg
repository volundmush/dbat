#11200
Oboe's Help Crying~
0 g 100
~
wait 2 s
say You!  Have you come to help fight against the those Icer invaders?
wait 2 s
emote gestures towards the encampment to the south, "@CI would go in myself, but they've taken our eggs as hostage, so we can't fight back.  They'll kill them!@n"
wait 3 s
say Please, you've got to help. I don't have much but I'll pay you for every egg you return to us safely.
~
#11201
Oboe's Egg Reward~
0 j 100
~
if (%object.vnum% == 11401)
  wait 2 s
  say One of our eggs! Thank you so much.  Here, take this.  It's the least I can do.
  wait 2 s
  emote hands you a bag of zenni.
  nop %actor.gold(100)%
  purge egg
else
  say This... I don't want- or even need this.
  drop %object.name%
end
~
$~
