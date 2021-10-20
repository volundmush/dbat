#13300
Hello~
0 g 100
~
wait 3 sec
say Hello Warrior, How may I help you today?
~
#13301
Grumpy Old Man~
0 g 100
~
%echo% An Old Man looks up from his books and glares at you.
wait 2 sec
say Who are you?
wait 2 sec
say On second thought I don't care! 
wait 1 sec
say Why must all you young idiots barge into people's houses....get out..get out now before I throw my book in your nasty little face!
wait 2 sec
say Begone!
%echo% An Old Man looks back down toward his book.
~
#13302
new trigger~
0 g 100
~
%Echo% The Old Elder turns to face you as you enter his house.
wait 4 sec
say "Ah yes, welcome to our humble village, I hope you enjoy your stay.
wait 4 sec
say Sorry to rush you off visitor, but I am very busy...excuse me.
%echo% An Old Elder motions his hand towards the door as he turns his attention elsewhere.
~
#13303
VR Mode ~
2 g 100
~
%echo% A red light flashes overhead as warning sounds echo through the room.
wait 2 sec
%echo% A computer speaks over the com system.
wait 2 sec
%echo% "Warning, VR mode malfunction, system unstable!"
~
#13304
Dende Trig~
0 g 100
~
wait 4 sec
emote @nlooks up from his old tome, his eyes meetings yours and delving deep into your soul.
wait 4 sec
say Greetings warrior, I trust that you find Namek a welcoming place to be.
wait 4 sec
emote @ncloses his book and coughs slightly.
wait 4 sec
say I assume you have come for some instruction, shall we begin?
~
#13305
Trig for Guard~
0 q 100
~
if (%direction% == east && !%actor.varexists(dendestrialcomplete)%)
  return 0
  wait 1 s
  say Without completing a trial, I cannot let you see Dende.
  wait 2 s
  say Say trial and I will tell you what you need to do.
elseif (%direction% == east && %actor.varexists(dendestrialcomplete)%)
  wait 1 s
  %send% %actor% A Namekian Guard says, 'Enjoy your visit with Dende.'
  return 1
end
~
#13306
Horticulture Elder~
0 g 100
~
if (%actor.race% != Namekian)
 wait 1
 say Hello. I am the Elder in charge of our planet's efforts to save the nearly exinct Ajissa trees.
 wait 3 sec
 say Hmm. If you were a namek I might trust you with helping in this task, but since you aren't...
 wait 3 sec
 say Well it was nice meeting you anyway.
elseif (!%actor.varexists(farmtree)%)
 wait 1
 say Hello. I am the Elder in charge of our planet's efforts to save the nearly exinct Ajissa trees.
 wait 3 sec
 say Say, would you be interested in helping grow some Ajissa trees?
 wait 2 sec
 say It is an honor to help save our planet's trees to begin with, but...
 wait 2 sec
 %echo% %self.name% strokes his chin for a moment.
 wait 3 sec
 say I am willing to pay you since we are having a bit of a shortage on workers for this task.
 wait 3 sec
 say How does seventy five zenni sound per tree planted?
 wait 2 sec
 say If you are interested you need only say Ajissa.
else
 wait 1
 %force% %actor% smote %self.name% smiles at  .
 wait 2 sec
 say Ah hello again.
end
~
#13307
Horticulture Elder Speech~
0 d 100
Ajissa~
if (%actor.race% != Namekian)
 wait 1
 say Sorry but I can not trust a non-namekian for this task.
 wait 2 s
 say Don't take it personal.
elseif (%actor.varexists(farmtree)%)
 wait 1
 say You are already working for me.
 wait 2 s
 say If you want to @Greport@C how many trees you have planted so I may pay you and dispatch caretakers then do so.@n
else
 wait 1
 say Ah, good. I'm glad you have decided to help!
 wait 2 sec
 say Alright here is your bag of seeds. It has twenty seeds in it.
 wait 1
 %load% o 13330
 drop bag
 %force% %actor% get bag
 set farmtree 0
 remote farmtree %actor.id%
 wait 2 sec
 say When you are done planting come back to me and @Greport@C. I will then pay you and give you a new bag of seeds.@n
 wait 3 sec
 say Remember you must plant the seeds in nice ground, and there can't be too many trees clumped together.
 wait 3 sec
 say I think about ten trees close together is the limit. 
 wait 2 sec
 say Well carry on then.
end
~
#13308
Horticulture Elder Report~
0 c 100
report~
return 1
wait 1
if (!%actor.varexists(farmtree)%)
 wait 1
 say You aren't even planting trees for me!
elseif (%actor.farmtree% <= 0)
 wait 1
 say You haven't managed to plant any! If you lost your bag of seeds then you obviously can't be trusted to do this job...
else
 wait 1
 say Alright let's see, you managed to plant %actor.farmtree% Ajissa trees.
 wait 2 s
 if (%actor.farmtree% >= 20)
  say Great work! I'm depositing your pay into your bank holdings. 25 x %actor.farmtree% zenni.
 elseif (%actor.farmtree% > 10)
  say You did alright... I'm depositing your pay into your bank holdings. 25 x %actor.farmtree% zenni.
 else
  say You did pretty badly... Well I'm depositing your pay into your bank holdings. 25 x %actor.farmtree% zenni.
 end
 eval num %actor.farmtree% * 25
 nop %actor.bank(%num%)%
 wait 1
 set farmtree 0
 remote farmtree %actor.id%
 %load% o 13330
 drop bag
 %force% %actor% get bag
end
~
#13309
Ajissa Plant Trigger~
1 c 3
plant~
return 1
if (!%actor.varexists(farmtree)%)
 wait 1
 %send% %actor% You need to speak to the Horticulture Elder before you try planting these.
elseif (%actor.room.vnum% > 11999 || %actor.room.vnum% < 10000)
 wait 1
 %send% %actor% You are not on Namek. Ajissa trees only survive on planet Namek.
elseif (%actor.room.sector% != Plain)
 wait 1
 %send% %actor% This is not where you should plant Ajissa trees. Try a nice flat area exposed to sunlight and obviously hospitable to plants.
elseif (%actor.move% < %actor.maxmove% / 10)
 wait 1
 %send% %actor% You do not have enough stamina..
 wait 2 sec
 %send% %actor% @gOOC@D: @wYes. You do not have enough stamina to plant a single seed. ;)@n
elseif (%actor.room.contents(13331)%)
 wait 1
 set tree %actor.room.contents(13331)%
 if (%tree.weight% >= 10000)
  %send% %actor% There are already ten Ajissa trees planted in the immediate area. Try another area.
 else
  %force% %actor% emote bends over and digs a shallow hole in the ground. Then %actor.heshe% drops a seed in the hole and covers it up. 
  wait 3 sec
  eval num %actor.farmtree% + 1
  set farmtree %num%
  remote farmtree %actor.id%
  if (%self.cost% > 1)
   nop %self.cost(-1)%
   nop %tree.weight(1000)%
   eval drain %actor.maxmove% / 10
   nop %actor.move(-%drain%)%
   wait 1
   %send% %actor% There are %self.cost% seeds left in the bag.
  else
   nop %tree.weight(1000)%
   eval drain %actor.maxmove% / 10
   nop %actor.move(-%drain%)%
   wait 1
   %send% %actor% You throw away the empty bag.
   %purge% self
  end
 end
else
 wait 1
 %force% %actor% emote bends over and digs a shallow hole in the ground. Then %actor.heshe% drops a seed in the hole and covers it up. 
 wait 3 sec
 eval num %actor.farmtree% + 1
 set farmtree %num%
 remote farmtree %actor.id%
  if (%self.cost% > 1)
   nop %self.cost(-1)%
   %load% o 13331
   eval drain %actor.maxmove% / 10
   nop %actor.move(-%drain%)%
   wait 1
   %send% %actor% There are %self.cost% seeds left in the bag.
  else
   %load% o 13331
   eval drain %actor.maxmove% / 10
   nop %actor.move(-%drain%)%
   wait 1
   %send% %actor% You throw away the empty bag.
   %purge% self
  end
end
~
#13310
look soil~
1 c 4
look~
if (%arg% == soil || %arg% == tilled || %arg% == till || %arg% == soi)
return 1
wait 1
eval num %self.weight% / 1000
%send% %actor% The soil appears to have %num% seeds planted in it.
else
return 0
end
~
#13311
Still Planting Trigger~
1 c 3
plant~
return 1
wait 1
%send% %actor% You are still planting a seed from earlier!
~
#13398
Guard gives trial~
0 d 100
*~
if %speech% == trial
  if %actor.varexists(dendestrialincomplete)%
    wait 1 s
    %send% %actor% The Namekian Guard says, 'You are already undertaking the trial. Kill Kiwi and find the puzzle box.'
  elseif %actor.varexists(dendestrialcomplete)%
    wait 1 s
    %send% %actor% The Namekian Guard says, 'You have already completed the trial!'
  else
    wait 1 s
    %send% %actor% The Namekian Guard says, 'Very well, %actor.name%. To see Dende you must do the following..'
    wait 2 s
    %send% %actor% The Namekian Guard says, 'Find a mutant working for Frieza called Kiwi. He has in his possession an ancient puzzle box.'
    wait 2 s
    %send% %actor% The Namekian Guard says, 'Kill him, solve the puzzle box and return to me. I will be able to tell if you have or not.'
    set dendestrialincomplete
    remote dendestrialincomplete %actor.id%
  end
end
~
#13399
Puzzle Box trigger~
1 c 3
press~
if %actor.varexists(dendestrialcomplete)%
  wait 1 s
  %send% %actor% You have already solved the puzzle.
else
   if %arg% == blue  
     if !%actor.varexists(pressedblue)%
       wait 1 s
       %send% %actor% You press the blue side of the puzzle box.
       wait 1 s
       %send% %actor% The blue side lights up indicating you pressed the correct side.
       wait 2 s
       %send% %actor% There are still two sides to activate.
       set pressedblue
       remote pressedblue %actor.id%
     else
       wait 1 s
       %send% %actor% Incorrect. The puzzle box resets itself.
       rdelete pressedblue %actor.id%
       rdelete pressedgreen %actor.id%
     end
   elseif %arg% == green
     if !%actor.varexists(pressedgreen)%
       wait 1 s
       %send% %actor% You press the green side of the puzzle box.
       wait 1 s
       %send% %actor% The green side lights up indicating you pressed the correct side.
       wait 2 s
       %send% %actor% There is still one side to activate.
       set pressedgreen
       remote pressedgreen %actor.id%
     else
       wait 1 s
       %send% %actor% Incorrect. The puzzle box resets itself.
       rdelete pressedblue %actor.id%
       rdelete pressedgreen %actor.id%
     end
   elseif %arg% == red
     if %actor.varexists(pressedgreen)%
       wait 1 s
       %send% %actor% You press the red side of the puzzle box.
       wait 1 s
       %send% %actor% The red side lights up indicating you pressed the correct side.
       wait 2 s
       %send% %actor% Suddenly the entire puzzle box flashes in a bright white light that washes over you.
       wait 2 s
       %send% %actor% The light subsides and you feel rejuvenated. You should return to the guard protecting Dende now.
       set dendestrialcomplete
       remote dendestrialcomplete %actor.id%
       rdelete pressedblue %actor.id%
       rdelete pressedgreen %actor.id%
       rdelete dendestrialincomplete %actor.id%
       nop %actor.exp(200000)%
     else
       wait 1 s
       %send% %actor% Incorrect. The puzzle box resets itself.
       rdelete pressedblue %actor.id%
       rdelete pressedgreen %actor.id%
     end
   else
     wait 1 s
     %send% %actor% Try pressing blue, red or green.
   end
end
~
$~
