#9600
Namekian Respawn~
0 f 100
~
%load% mob 9605
%echo% @CAs %self.name%@C dies, he thrusts his head forward, spitting out an @YE@Wg@Yg@C!@n
~
#9601
Egg Gestation~
0 n 100
~
if %self.room.vnum% == 9624
  wait 3s
  emote pulsates as something grows inside it.
  wait 7s
  emote pulsates as something grows inside it.
  wait 10s
  emote pulsates and begins to crack!
  wait 10s
  emote cracks open completely, and something emerges!
  if %findmob.9624(9602)% < 1
    %load% mob 9602
  end
  %purge% %self%
elseif %self.room.vnum% == 9623
  wait 3s
  emote pulsates as something grows inside it.
  wait 7s
  emote pulsates as something grows inside it.
  wait 10s
  emote pulsates and begins to crack!
  wait 10s
  emote cracks open completely, and something emerges!
  if %findmob.9623(9600)% < 1
    %load% mob 9600
  end
  %purge% %self%
elseif %self.room.vnum% == 9622
  wait 3s
  emote pulsates as something grows inside it.
  wait 7s
  emote pulsates as something grows inside it.
  wait 10s
  emote pulsates and begins to crack!
  wait 10s
  emote cracks open completely, and something emerges!
  if %findmob.9622(9601) < 1
    %load% mob 9601
  end
  %purge% %self%
end
~
#9602
Namekian Prepares for Death~
0 g 100
~
wait 1s
emote looks up at *%actor.name%.  %self.name% says nothing, but stands upright with a posture and look that says they have accepted their fate and made peace with the end of their life.
~
#9603
Namekian Reborn~
0 n 100
~
wait 2s
emote breathes deeply, pondering the memories that have been passed to it.  @Y'@GI understand...@Y'@n
~
#9604
Morteus - Yes, I will die~
0 d 100
yes~
* @G********************************************************
* @G*@W  This script will damage a player until they have a 
* @G*@W  certain amount of HP left!
* @G********************************************************
wait 1s
if %actor.hitp% < 10000
  emote shakes his head.  @Y'@GI'm sorry.  You would never survive the next floor at that power...@Y'@n
else
  emote nods his head.  @Y'@GVery well.  Be mindful that if you are not strong enough, the room will @Rkill@G you.  So be prepared...@Y'@n
  wait 2s
  while %actor.hitp% > 10000
    set mydamage 100000
    set currenthp %actor.hitp%
    eval mydamage %currenthp% - 759
    nop %actor.hitp(-%mydamage%)%
  done
  %send% %actor% @RYou feel your life force drained by the room!@n
  wait 2s
  emote observes *%actor.name% sadly, being careful to keep from touching.  @Y'@GIf you are ready for the final test, you can attempt to @Rascend@G with your mind now...@Y'@n
end
~
#9605
First Ascend Test~
2 c 100
ascend~
* @C************************************************************@n
* @C*@W  ...and now, the player tries to ascend.  Will we  
* @C*@W  let them Ascend to the next level?
* @C************************************************************@n
wait 1s
if %actor.hitp% > 760
  wait 1s
  %send% %actor% @RYou may only attempt to ascend directly after Morteus has prepared your life force... Heed his words.@n
else
  if %actor.varexists(wisdomascend)%
    %send% %actor% @YYou have already ascended beyond fear of death!@n
  else
    wait 1s
    %send% %actor% @RThe room darkens, and you feel it prepare to drain you of life.@n
    wait 3s
    %send% %actor% @RYou will not survive this.  You can feel it deep in your bones.@n
    wait 3s
    %send% %actor% @RDeath comes for you, but you do not fear it...@n
    wait 5s
    %send% %actor% @RYou have @YDIED@R!@n
    wait 3s
    %send% %actor% @R...and have been reborn, your mind @Yascended@R.@n
    wait 2s
    %send% %actor% @RYou may now @Yascend@R to the 2nd floor in the ascending room.@n
    set wisdomascend 1
    remote wisdomascend %actor.id%
  end
end
~
#9690
Progression Trigger~
0 g 100
~
if ( %actor.varexists(kiwidead)% )
  set kiwidead %actor.kiwidead%
else
  set kiwidead 0
end
wait 2s
emote opens his eyes, sensing a presence before him.  Or perhaps a shadow as it crosses over his form.  He takes in a breath, then slowly lets it out.
wait 3s
if %kiwidead% > 0
  emote nods his head.  @Y'@GYou have done Namek a favor by removing Kiwi from it.  The Tower accepts you...@Y'@n
  wait 1s
%teleport% %actor% 9604
  %force% %actor% l
else
  emote looks out over the ocean.  @Y'@GThe tower cannot accept you.  It does not know where your heart lies.  Perhaps if you prove yourself...@Y'@n
  wait 2s
  emote turns back to *%actor.name%.  @Y'@GFreiza has a ship here.  On it is a thing named Kiwi.  Remove him from our planet, and the tower will accept you...@Y'@n
end
~
#9691
Morteus Greet~
0 g 100
~
wait 2s
emote breathes deeply as another being enters the room.  He examines *%actor.name% closely in silence for several seconds.
wait 6s
emote speaks with its soft voice.  @Y'@GDeath is only another birth.  It is a matter of perspective.  It is natural to cling to life, but death should not be feared.  Particularly not so much that it prevents one from acting true to one's @Wself@G.@Y'@n
wait 5s
emote ponders for a moment before speaking again.  @Y'@GOne cannot @Rascend@G by killing others.  Only through the death of our previous mentality can we @Rascend@G to the next plane of the mind.@Y'@n
wait 4s
emote lowers his face, raising his eyes to meet you in a critical gaze.  @Y'@GIf I told you you had to die to reach the next floor, perhaps only if you can be resummoned to life, would you still wish to @Rascend@G?@Y'@n
~
#9692
Leave the tower~
2 c 100
leave~
%teleport% %actor% 9601
%force% %actor% l
~
#9693
Ascend Command~
2 c 100
ascend~
if %actor.varexists(wisdomascend)%
  wait 1
  %send% %actor% @RYou concentrate, ascending your consciousness to the next level.  As you do so, the tower ascends your body to the next level as well...@n
  %echoaround% %actor% @R%actor.name% concentrates for a minute, then suddenly disappears.@n
  wait 1
  %teleport% %actor% 9620
else
  %send% %actor% @RYou attempt to ascend, but you have not yet passed the test...@n
end
~
$~
