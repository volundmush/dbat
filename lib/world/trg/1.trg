#100
noob guard~
0 d 100
begin~
mteleport %actor% 101
wait 1 s
%at% 101 mforce %actor.% look
wait 1 s
%at% 101 %send% %actor% You have now entered the MUD school.
~
#101
Skip Newbie Zone~
0 c 100
skip~
if (%actor.rpp% >= 2)
 %load% obj 108
 %load% obj 110
 %load% obj 111
 %load% obj 112
 %load% obj 113
 %load% obj 114
 %load% obj 4901
 %load% obj 4901
 %load% obj 317
 %load% obj 61
 drop all
 %force% %actor% get all
 %send% %actor% Away you go!
 %actor.exp(1500)%
 %actor.gold(250)%
 if (%actor.class% == Roshi)
  %teleport% %actor% 1130
  %force% %actor% west
 elseif (%actor.class% == Kurzak)
  %teleport% %actor% 16101
  %force% %actor% east
 elseif (%actor.class% == Kibito)
  %teleport% %actor% 12098
  %force% %actor% west
 elseif (%actor.class% == Nail)
  %teleport% %actor% 11683
  %force% %actor% north
 elseif (%actor.class% == Bardock)
  %teleport% %actor% 2268
  %force% %actor% north
 elseif (%actor.class% == Krane)
  %teleport% %actor% 13009
  %force% %actor% east
 elseif (%actor.class% == Tapion)
  %teleport% %actor% 8231
  %force% %actor% north
 elseif (%actor.class% == Piccolo)
  %teleport% %actor% 1659
  %force% %actor% west
 elseif (%actor.class% == Android 16)
  %teleport% %actor% 1713
  %force% %actor% north
 elseif (%actor.class% == Dabura)
  %teleport% %actor% 6486
  %force% %actor% northeast
 elseif (%actor.class% == Frieza)
  %teleport% %actor% 4282
  %force% %actor% east
 elseif (%actor.class% == Ginyu)
  %teleport% %actor% 4289
  %force% %actor% north
 elseif (%actor.class% == Jinto)
  %teleport% %actor% 3498
  %force% %actor% north
 elseif (%actor.class% == Tsuna)
  %teleport% %actor% 15006
  %force% %actor% north
 end
else
 %send% %actor% You can't do that unless you've earned at least 2 RPP. It doesn't cost RPP, but it requires them as proof of already completing the school on another character.
end
~
#102
End of movement zone trigger~
2 d 100
continue~
wteleport %actor% 114
wait 2 s
%at% %actor% wforce %actor% look
~
#103
End of basics trigger~
2 d 100
continue~
wteleport %actor% 123
wait 2 s
%at% %actor% wforce %actor% look
~
#104
End of combat trigger~
2 d 100
continue~
wteleport %actor% 129
wait 2 s
%at% %actor% wforce %actor% look
~
#105
Practice mob~
2 d 100
practice~
if !%actor.varexists(MUDschoolpracticemob)%
  if %findmob.126(101)% >= 1
    %send% %actor% Sorry. But there is already a rookie in the room.
  else 
    wload mob 101
    %echo% Type punch rookie to begin combat!
  end
else
  %echo% %actor.name%, you have already killed a practice mob.
end
~
#106
Give equipment trigger~
0 d 100
equipment~
if !%actor.varexists(gotequipment)%
  if (%actor.race% != Truffle)
    wait 1 s
    mload obj 110
    mload obj 111
    mload obj 112
    mload obj 113
    mload obj 114
    give all.simple %actor.name%
    set gotequipment
    remote gotequipment %actor.id%
  else
    wait 1 s
    mload obj 115
    mload obj 116
    mload obj 117
    mload obj 118
    mload obj 119
    give all.simple %actor.name%
    set gotequipment
    remote gotequipment %actor.id%
  end
else
wait 1
say You already have received your free equipment.
  return 0
end
~
#107
Waiting Room Trigger~
2 g 100
~
eval count 0
%teleport% %actor% 133
set target_char %actor.room.people%
 while %target_char%
  set tmp_tar %target_char.next_in_room%
  if (%target_char.is_pc% && %target_char% != %actor%)
   eval count %count% + 1
  end
  set target_char %tmp_tar%
 done
%teleport% %actor% 134
set target_char %actor.room.people%
 while %target_char%
  set tmp_tar %target_char.next_in_room%
  if (%target_char.is_pc% && %target_char% != %actor%)
   eval count %count% + 1
  end
  set target_char %tmp_tar%
 done
%teleport% %actor% 139
if (%count% > 0)
 eval count 0
 %teleport% %actor% 135
 set target_char %actor.room.people%
  while %target_char%
   set tmp_tar %target_char.next_in_room%
   if (%target_char.is_pc% && %target_char% != %actor%)
    eval count %count% + 1
   end
   set target_char %tmp_tar%
  done
 %teleport% %actor% 136
 set target_char %actor.room.people%
  while %target_char%
   set tmp_tar %target_char.next_in_room%
   if (%target_char.is_pc% && %target_char% != %actor%)
    eval count %count% + 1
   end
   set target_char %tmp_tar%
  done
 %teleport% %actor% 139
 if (%count% > 0)
  eval count 0
  %teleport% %actor% 137
  set target_char %actor.room.people%
   while %target_char%
    set tmp_tar %target_char.next_in_room%
    if (%target_char.is_pc% && %target_char% != %actor%)
     eval count %count% + 1
    end
    set target_char %tmp_tar%
   done
  %teleport% %actor% 138
  set target_char %actor.room.people%
   while %target_char%
    set tmp_tar %target_char.next_in_room%
    if (%target_char.is_pc% && %target_char% != %actor%)
     eval count %count% + 1
    end
    set target_char %tmp_tar%
   done
   %teleport% %actor% 139  
  if (%count% > 0)
   return 0
   wait 1
   %send% %actor% The tutor rooms are currently full, wait a moment and try again.
  else
   %force% %actor% east
  end
 else
  %force% %actor% north
 end
else
 %force% %actor% west
end
~
#108
North Room Clean~
2 ab 75
~
eval target %self.people%
 while %target%
  eval tmp_target %target.next_in_room%
  if (%target.is_pc%)
   eval play %target%
  end
  eval target %tmp_target%
 done
if (!%play%)
 eval target %self.people%
 while %target%
  eval tmp_target %target.next_in_room%
  if (!%target.is_pc% && %self.vnum% == 134)
   %teleport% %target% 133
  elseif (!%target.is_pc% && %self.vnum% == 136)
   %teleport% %target% 135
  elseif (!%target.is_pc% && %self.vnum% == 138)
   %teleport% %target% 137
  end
  eval target %tmp_target%
 done
end
~
#109
Mud School Get Trigger~
1 g 100
~
if (%actor.room.vnum% > 100 && %actor.room.vnum% <= 153)
 if (%actor.has_item(%self.vnum%)%)
  return 0
  wait 1
  %send% %actor% You already have taken your share of (%self.shortdesc%), don't be greedy.
 else
  %load% obj %self.vnum%
 end
end
~
#110
Mud School Drop Trigger~
1 h 100
~
if (%actor.room.vnum% > 100 && %actor.room.vnum% <= 153)
 return 0
 wait 1
 %send% %actor% You can't drop that in the mudschool.
end
~
#116
tutor enter~
0 g 100
~
if %actor.is_pc%
  if !%actor.varexists(tutor1yes)%
    say Welcome %actor.name%.
    wait 2 s
    say You must be a new recruit for Dragonball Advent Truth.. 
    wait 4 s
    say and by the empty expression on your face I take it are here to train in the mystic arts of Advent Truth.
    wait 4 s
    say If you wish to learn please say "@YYes.@n"
  end
end
~
#117
tutor 1 yes~
0 d 100
yes~
set tutor1yes
remote tutor1yes %actor.id%
say @WExcellent... excellent@n
wait 2 s
say @WOk.. what I plan to do is randomly spawn five level one monsters around the newbie training dimension@n
wait 4 s
say @WThis is what I want you to do, all 5 monsters will drop a jewel..@n
wait 2 s
say @WGo defeat these five monsters and give me the jewels which I will trade for 1 of the 2 keys to exit this dimension@n
wait 3 s
say @WNow brace yourself..@n
%echo% @YThe first tutor raises his hand and begins to summon 5 monsters@n
wait 2 s
Say @WI must remind you that you can only use @n@RPUNCH@n@W on them, now go and get the jewels@n
set i 0
while %i% < 5
set bottom 101
set range 95
  set room %%random.%range%%%
  eval room %room% + %bottom%
  %at% %room% %load% mob 101
  eval i %i% + 1
done
who
~
#118
jewels trigger~
0 j 100
~
eval jewel %actor.jewel% + 1
remote jewel %actor.id%
if %actor.jewel%
  if %actor.jewel% >= 5
    say Excellent.. you got a lot further then i thought you would..
    wait 2 s
    say Here you earned it..
    emote Hands you the first of 2 keys..
    %load% obj 103 %actor%
    wait 3 s
    say Now i suggest you look for the second tutor.
    wait 2 s
    Say I believe he is on the floor below..
    mjunk all
  end
end
~
#119
Set practicemob var~
0 f 100
~
set MUDschoolpracticemob
remote MUDschoolpracticemob %actor.id%
~
#129
Begin Knowledge Gauntlet~
2 d 100
*~
if %speech% == begin
  wait 1 s
  %send% %actor% Transferring you to the Knowledge Gauntlet.
  wait 1 s
  %teleport% %actor% 141
  %at% 141 %force% %actor% look
end
~
#130
Questions for Knowledge Gauntlet~
2 c 100
question~
if %self.vnum% == 141
  wait 1 s
  %send% %actor% If I have a weapon equipped, what command do I use to attack?
  wait 2 s
  %send% %actor% To answer, please type answer followed by your answer.
elseif %self.vnum% == 142
  wait 1 s
  %send% %actor% Name one of the three options for skill perfection.
  wait 2 s
  %send% %actor% To answer, please type answer followed by your answer.
elseif %self.vnum% == 143
  wait 1 s
  %send% %actor% What does the acronym OOC stand for?
  wait 2 s
  %send% %actor% To answer, please type answer followed by your answer.
elseif %self.vnum% == 144
  wait 1 s
  %send% %actor% What is the command to attempt to stop someone fleeing from you?
  wait 2 s
  %send% %actor% To answer, please type answer followed by your answer.
elseif %self.vnum% == 145
  wait 1 s
  %send% %actor% What is the max powerlevel a basic scouter can read up to?
  wait 2 s
  %send% %actor% To answer, please type answer followed by your answer.
elseif %self.vnum% == 146
  wait 1 s
  %send% %actor% I want do gain powerlevel by gravity training. Which command do I use?
  wait 2 s
  %send% %actor% To answer, please type answer followed by your answer.
elseif %self.vnum% == 147
  wait 1 s
  %send% %actor% This is an RP Enforced MUD. Will I need a bio at some point? (help biography)
  wait 2 s
  %send% %actor% To answer, please type answer followed by your answer.
elseif %self.vnum% == 148
  wait 1 s
  %send% %actor% What command do you use to see a list of skills you have learned?
  wait 2 s
  %send% %actor% To answer, please type answer followed by your answer.
elseif %self.vnum% == 149
  wait 1 s
  %send% %actor% What command do you use to change the voice people hear over snet? (help voice)
  wait 2 s
  %send% %actor% To answer, please type answer followed by your answer.
elseif %self.vnum% == 150
  wait 1 s
  %send% %actor% I've just met someone. I want to give him (or her) a name. What command do I use?
  wait 2 s
  %send% %actor% To answer, please type answer followed by your answer.
else
  wait 1 s
  %send% %actor% The sky is falling. Please report this issue to an immortal.
end
~
#131
Answer 1~
2 c 100
answer~
if %arg% == attack
  wait 1 s
  %send% %actor% Congratulations, you have got the question correct. Moving you to the next room.
  %teleport% %actor% 142
  %at% %actor% %force% %actor% look
else
  wait 1 s
  %send% %actor% Sorry, but that answer is not correct.
end
~
#132
Answer 2~
2 c 100
answer~
if (%arg% == over charge || %arg% == efficiency || %arg% == accuracy)
  wait 1 s
  %send% %actor% Congratulations, you have got the question correct. Moving you to the next room.
  %teleport% %actor% 143
  %at% %actor% %force% %actor% look
else
  wait 1 s
  %send% %actor% Sorry, but that answer is not correct.
end
~
#133
Answer 3~
2 c 100
answer~
if (%arg% == out of character)
  wait 1 s
  %send% %actor% Congratulations, you have got the question correct. Moving you to the next room.
  %teleport% %actor% 144
  %at% %actor% %force% %actor% look
else
  wait 1 s
  %send% %actor% Sorry, but that answer is not correct.
end
~
#134
Answer 4~
2 c 100
answer~
if (%arg% == block)
  wait 1 s
  %send% %actor% Congratulations, you have got the question correct. Moving you to the next room.
  %teleport% %actor% 145
  %at% %actor% %force% %actor% look
else
  wait 1 s
  %send% %actor% Sorry, but that answer is not correct.
end
~
#135
Answer 5~
2 c 100
answer~
if (%arg% == 149,999 || %arg% == 149999)
  wait 1 s
  %send% %actor% Congratulations, you have got the question correct. Moving you to the next room.
  %teleport% %actor% 146
  %at% %actor% %force% %actor% look
else
  wait 1 s
  %send% %actor% Sorry, but that answer is not correct.
end
~
#136
Answer 6~
2 c 100
answer~
if (%arg% == pushup)
  wait 1 s
  %send% %actor% Congratulations, you have got the question correct. Moving you to the next room.
  %teleport% %actor% 147
  %at% %actor% %force% %actor% look
else
  wait 1 s
  %send% %actor% Sorry, but that answer is not correct.
end
~
#137
Answer 7~
2 c 100
answer~
if (%arg% == yes)
  wait 1 s
  %send% %actor% Congratulations, you have got the question correct. Moving you to the next room.
  %teleport% %actor% 148
  %at% %actor% %force% %actor% look
else
  wait 1 s
  %send% %actor% Sorry, but that answer is not correct.
end
~
#138
Answer 8~
2 c 100
answer~
if (%arg% == skill || %arg% == skills)
  wait 1 s
  %send% %actor% Congratulations, you have got the question correct. Moving you to the next room.
  %teleport% %actor% 149
  %at% %actor% %force% %actor% look
else
  wait 1 s
  %send% %actor% Sorry, but that answer is not correct.
end
~
#139
Answer 9~
2 c 100
answer~
if (%arg% == voice)
  wait 1 s
  %send% %actor% Congratulations, you have got the question correct. Moving you to the next room.
  %teleport% %actor% 150
  %at% %actor% %force% %actor% look
else
  wait 1 s
  %send% %actor% Sorry, but that answer is not correct.
end
~
#140
Answer 10~
2 c 100
answer~
if (%arg% == dub)
  wait 1 s
  %send% %actor% Congratulations, you have got the question correct. Moving you on to the Final Trainer now.
  %teleport% %actor% 139
  %at% %actor% %force% %actor% look
else
  wait 1 s
  %send% %actor% Sorry, but that answer is not correct.
end
~
#141
Dur I need to go back check~
2 q 100
~
eval num %self.vnum%
set gauntletreturn %num%
remote gauntletreturn %actor.id%
~
#142
Return to last gauntlet room we left.~
2 g 100
~
if (%actor.varexists(gauntletreturn)%)
 %teleport% %actor% %actor.gauntletreturn%
 %force% %actor% look
 wait 1
 %send% %actor% You have returned to the last question you were on.
 rdelete gauntletreturn %actor.id%
end
~
#145
greet second tutor~
0 g 100
~
if !%actor.varexists(tutor2greets)%
  if %actor.inventory(103)%
    say @YCongratulations %actor.name%.@n
    wait 2 s
    say @YYou must be a pretty good fighter to get past the first tutor's challanges.@n
    wait 3 s
    say @YBecause I notice you have his key to the gate.@n
    wait 3 s
    say @YAnyways if you are ready for the next challange please say "@RYes@Y."@n
    set tutor2greets
    remote tutor2greets %actor.id%
  end
end
~
#146
2nd tutor yes~
0 d 100
yes~
if %actor.inventory(103)%
say @CExcellent... excellent@n
wait 2 s
say @COk.. as you know last time you had to go kill 5 monsters for a key@n
wait 4 s
say @CWell this time you have to go kill only one..@n
wait 2 s
say @CNow stand back while I summon it..@n
wait 1 s
echo @WYou take a few steps backwards@n
wait 4 s
%echo% @CThe second tutor raises his hand and begins to summon the boss.@n
wait 2 s
say @CIt is done now go and bring me the object which it drops@n
set i 0
while %i% < 1
  set bottom 100
  set range 96
  set room %%random.%range%%%
  eval room %room% + %bottom%
  %at% %room% %load% mob 102
  %at% %room% %load% obj 104 %room.people%
  eval i %i% + 1
done
end
~
#147
2nd tutor recieve~
0 j 100
~
if %object.vnum% == 104
  say Well done %actor.name%.
  wait 2 s
  say You are indeed a greater warrior
  wait 3 s
  say Thank you and here is your second key..
  %load% obj 105 %actor%
  wait 4 s
  say And now your third tutor awaits downstairs.
  mjunk all
else
  say That is not what I require, bring me the bosses head.
  drop %object.name%
end
~
#150
Power is Maximum~
1 j 100
~
wait 1
%echo% An explosion of power explodes forth in an explodey fashion from the cape as it is worn!@n
~
#155
Construction Worker Working~
0 b 15
~
wait 3
emote eyes you with a raised brow for a brief moment before shrugging.
wait 3
emote wipes the sweat from his eyes before he hefts his mallet once more.
~
#156
Gine's Receive~
0 j 100
~
set stcount 0
set stpay 0
%purge% prime
%load% obj 2399
wait 1
set myobj %self.inventory.vnum%
set myname %self.inventory.name%
while %myobj% != 2399
  if %myobj% == 1221
    eval stpay %stpay% + 100
    %purge% %myname%
    set myobj %self.inventory.vnum%
    set myname %self.inventory.name%
  elseif %myobj% == 1612
    eval stpay %stpay% + 50
    %purge% %myname%
    set myobj %self.inventory.vnum%
    set myname %self.inventory.name%
  else
    wait 1s
    say This... this isn't anything I want to buy!
    drop %myname%
    set myobj %self.inventory.vnum%
    set myname %self.inventory.name%
  end
done
if %stpay% > 0
  wait 2s
  say Thanks for bringing us more steak!
  wait 2s
  emote hands *%actor.name% a bag of zenni.
  nop %actor.gold(%stpay%)%
end
~
#157
Gine's Greet~
0 g 100
~
wait 3
emote looks up at you and stops slicing a cut of meat, "@CAre you here to donate food to the center? We'll pay for any @wCooked @rSteaks@C or @RRaw Steaks@C you bring in!@n" She says cheerily.
~
#183
1~
0 g 100
~
if %actor.inventory(103)% && %actor.inventory(105)%
  say @YWell, I can see that you have the two keys I require.@n
  wait 3 s
  say @YBeyond this door, is the exit of this dimension.@n
  wait 3 s
  say @YSo that you may progress, I will now take these keys from you.@n
  wait 4 s
  emote sits down and begins to meditate.
  wait 3 s
  %send% %actor% A soft glow appears around the keys as they begin to float toward the tutor.
%purge% %actor.inventory(103)%
%purge% %actor.inventory(105)%
%purge% %actor.inventory(102)%
%purge% %actor.inventory(102)%
%purge% %actor.inventory(102)%
%purge% %actor.inventory(102)%
%purge% %actor.inventory(102)%
  wait 3 s
  emote stands up and takes the keys with his right hand, before turning to face the trans-dimensional door.
  wait 2 s
  say @YI'll just unlock this now....@n
  emote inserts the keys and turns...
  wait 2 s
  say @YGood luck in your journeys.@n
  wait 1 s
  %send% %actor% You feel compelled to walk through the door.
  %teleport% %actor% 197
end
~
#188
Tutor 3 Greet~
0 g 100
~
 get key
 wait 2 sec
 say Well hello there new warrior, I see you have made it this far with no problem. I'm a little impressed.
 wait 5 sec
 say Ok now I am going to teach you a few things about trainers before you enter the universe.
 wait 5 sec
 say Trainers are people who teach you what to do with the experience you have aquired, train you in various skills and feats, and can train your bodily stats higher in some cases.
 wait 6 sec
 say Your first lesson is in how to aquire a higher level. You continue to gain experience without leveling up when you fight.
 wait 5 sec
 say You must be present at a trainer to spend your experience on leveling up. There are two main reasons for this.
 wait 6 sec
 say One is that you require the teaching of your trainer to advance to the next level to begin with, and the other is that you can gather multiple levels worth of experience before setting out to your trainer and advancing your level.
 wait 8 sec
 say Now let's try to advance your level, you see I am a registered trainer, so I can help you. First off, let's make sure you can level up to begin with.
 wait 5 sec
if (%actor.exp% < 1500)
%actor.exp(1500)%
end
 say There now you should have enough to advance a level.
 wait 1 sec
 say Now enter the command @Ggain@C.
~
#189
Tutor 3 Follow~
0 q 100
~
if (%direction% == north)
eval num %self.room.vnum%
eval rm %self.room%
wait 1
north
close door
lock door
wait 1 s
say Alright, let us continue.
wait 3 sec
say Let us cover something else trainers do. Teaching skills. Now I see that you have the skill punch, and that you know the basics of it already. So let's try something else...
wait 8 sec
say Type skills to see the skills you currently have. When you are at a trainer, type practice to see what skills the trainer teaches, and your current level in any of them.
wait 8 sec
say Type practice and a skillname to train the skill, as long as you have enough practice sessions(PS) available. Let's try practice kick. Once you have practiced kick to your liking, then say "finished" and we will move on.
else
return 0
wait 1 sec
say Advance on life, don't retreat. You need to head north.
unlock door
open door
end
~
#190
Tutor 3 Speech~
0 d 100
finished~
if (%actor.level% <= 1)
wait 1 sec
say You haven't gained a level yet...
else
%self.gold(500)%
wait 2 sec
say I am now going to tell you a little about the play in Dragonball Advent Truth, so pay attention. This is important.
wait 4 sec
say First thing you need to know is that gameplay in Dragonball AT revolves around roleplay. You roleplay out your character while you play. 
wait 7 sec
say So while you are in the game, in any IN CHARACTER location or channel you either need to not say anything, or behave as your character would. The two areas that are not IC(In Character) is the ooc channel, and the congrats channel.
wait 8 sec
say Your character only knows what they have seen or experienced IC. Nothing OOC is to be shared IC, and nothing IC is to be shared in ooc or grats.
wait 7 sec
say See help rules for more about this. If you just remember that your character is to be played like they really exist in this world, then you should do fine and have lots of fun.
wait 4 sec
say When you are ready, this tutorial is finished. Say "done" and you will be able to enter the game.
end
~
#191
Tutor 3 Done~
0 d 100
done~
if (%self.gold% < 500)
wait 1 sec
say You haven't told me you were were finished yet...
else
 wait 1 sec
 say Alright, you are ready to go. I will give you some Practice Sessions, and a little bit of zenni to start you out.
 eval amt 40 + %random.20%
 %actor.prac(10)%
 %actor.gold(%amt%)%
 rdelete MUDschoolpractice %actor.id%
 rdelete gotequipment %actor.id%
 wait 3 sec
 if (%actor.class% == Roshi)
  %teleport% %actor% 1130
  %force% %actor% west
 elseif (%actor.class% == Kurzak)
  %teleport% %actor% 16101
  %force% %actor% east
 elseif (%actor.class% == Kibito)
  %teleport% %actor% 12098
  %force% %actor% west
 elseif (%actor.class% == Nail)
  %teleport% %actor% 11683
  %force% %actor% north
 elseif (%actor.class% == Bardock)
  %teleport% %actor% 2268
  %force% %actor% north
 elseif (%actor.class% == Krane)
  %teleport% %actor% 13009
  %force% %actor% east
 elseif (%actor.class% == Tapion)
  %teleport% %actor% 8231
  %force% %actor% north
 elseif (%actor.class% == Piccolo)
  %teleport% %actor% 1659
  %force% %actor% west
 elseif (%actor.class% == Android 16)
  %teleport% %actor% 1713
  %force% %actor% north
 elseif (%actor.class% == Dabura)
  %teleport% %actor% 6486
  %force% %actor% northeast
 elseif (%actor.class% == Frieza)
  %teleport% %actor% 4282
  %force% %actor% east
 elseif (%actor.class% == Ginyu)
  %teleport% %actor% 4289
  %force% %actor% north
 elseif (%actor.class% == Jinto)
  %teleport% %actor% 3498
  %force% %actor% north
 elseif (%actor.class% == Tsuna)
  %teleport% %actor% 15006
  %force% %actor% north
 end
 if (%self.room.vnum% == 134)
  mgoto 133
  set %self.gold(0)%
 elseif (%self.room.vnum% == 136)
  mgoto 135
  set %self.gold(0)%
elseif (%self.room.vnum% == 138)
  mgoto 137
  set %self.gold(0)%
 end
close door
lock door
end
~
#192
Final Tutor Command~
0 c 100
gain~
if (%actor.exp% > 800)
return 0
detach 188 %self.id%
wait 1 sec
unlock door
open door
mforce %actor.name% north
attach 188 %self.id%
else
return 0
end
~
#193
Final Tutor Load~
0 ab 100
~
if (%self.room.contents(184)%)
get key
end
~
$~
