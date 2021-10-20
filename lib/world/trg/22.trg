#2200
Bardock's Greet Trigger~
0 g 100
~
if (%actor.level% <= 2)
wait 1
say %actor.name% you are one of my newest students. Everyone in my team fights, so you better prepare yourself for battle. Speak with Tora, he will tell you where to go to start training.
 if !%actor.varexists(bardockeq)%
  wait 2 sec
  say Now %actor.name% here is badge of my team, wear it with pride.
wait 1 sec
  mload obj 2200
  give badge %actor.name%
  set bardockeq
  remote bardockeq %actor.id%
 end
elseif (%actor.level% < 5)
wait 1
say %actor.name% you are still pretty new to the team so remember to train hard.
 if %actor.varexists(bardockeq)%
  rdelete bardockeq %actor.id%
end
elseif (%actor.level% < 10)
wait 1
say %actor.name% you haven't disappointed me, keep up your training.
elseif (%actor.level% < 20)
wait 1
say %actor.name% you are doing fine as a member of my team, keep up your training.
elseif (%actor.level% < 40)
wait 1
say %actor.name% you do me proud with your dedication.
elseif (%actor.level% < 75)
wait 1
say %actor.name% you are a well respected member of the team, keep it up.
elseif (%actor.level% < 100)
wait 1
say %actor.name% I would be proud to bleed on the battle field fighting along side you.
elseif (%actor.level% == 100)
wait 1
say %actor.name% shouldn't you be leading your own team by now?
smile
else
wait 1
say %actor.name% I don't know what to say about you.
end
~
#2201
Seripa Newbie Food Greet Trig~
0 g 100
~
if (!%actor.varexists(seripafood)%)
    wait 1
    say Hmm. You must be new to the team. I have some spare @Gfood@C and drink, if you need it. Just ask.@n
else
   wait 1
   say Remember to push yourself to the limit as you train.
end
~
#2202
Tora Guard~
0 q 100
~
if (%direction% == east && %actor.class% != Bardock)
 return 0
 wait 1
 say You're not welcome here. Get lost before I make you.
* elseif (%actor.carry%)
* return 0
* wait 1
* say You may not carry anyone inside.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == east && %actor.class% == Bardock)
wait 1
 say Don't keep him waiting.
%send% %actor% Tora says, '@CDon't keep him waiting.@n'
return 1
end
~
#2203
Tora Greet~
0 g 100
~
if (%actor.class% == Bardock)
    if (%actor.level% <= 2)
        wait 1
emote smirks at %actor.name%, &1Hmpf. Another pathetic warrior that Bardock took in? I don't think you could even take a Saiba.
wait 2 s
say If you think you can prove me wrong, head south to the intersection, go west, and then take the last road north. Fight them, if you can, and return when you're stronger.
    
    elseif (%actor.level% >= 5 && %actor.level% <= 9 && !%actor.varexists(ToraQ1Complete)% && !%actor.varexists(onToraQ1) && !%actor.varexists(ToraQComplete)%)
        wait 1
emote smirks at %actor.name%, &1Well. Seems as though you do have some bark after all. How about you go prove yourself in the arena? Head south to the intersection, then head east, then follow the road and take the first road you see east.
wait 2 s
say Bring back a Gladiatorial Medal to show that you have some guts.
set onToraQ1
        remote onToraQ1 %actor.id%
        
    elseif (%actor.level% >= 10 && %actor.level% <= 14 && !%actor.varexists(ToraQ2Complete)% && !%actor.varexists(onToraQ2)% && !%actor.varexists(ToraQComplete)%)
        wait 1
emote smirks at %actor.name%, &1I'm glad you're getting stronger. You're proving to be useful to the team, unlike some other people we've gotten...&2
wait 2 s
say Anyway, King Vegeta has entrusted Bardock with a task, and Bardock left the choice of who to give this task to to me. And I pick you.
wait 2 s
emote points north, &1You are to head into the palace and find a traitor in the Elite class warriors. Feel free to try and weed out the weaklings as well.
wait 2 s
say You can also head out to the Blood Dunes to the west and train on the Grashl's out there.
        set onToraQ2
        remote onToraQ2 %actor.id%
        
    elseif (%actor.level% >= 15 && %actor.level% <= 19 && !%actor.varexists(ToraQ3Complete)% && !%actor.varexists(onToraQ3)% && !%actor.varexists(ToraQComplete)%)
        wait 1
emote smirks at %actor.name%, &1Well, seems as though you're not dead yet. Good. I have another task for you.&2
wait 2 s
say I have a test of strength for you. In the Blood Dunes to the west there are some Forn's crawling around out there, and they pack quite the punch. I'd like you to bring me back one of their teeth.
wait 2 s
emote lets out a small laugh, &1If you're strong enough for it, anyway.&2
        set onToraQ3
        remote onToraQ3 %actor.id%
        
    elseif (%actor.level% >= 20 && !%actor.varexists(ToraQ4Complete)% && !%actor.varexists(onToraQ4)% && !%actor.varexists(ToraQComplete)%)
        wait 1
emote smirks at %actor.name%, &1Well, well... Look at you now. Trying to become the strongest on the team are ya?&2
wait 2 s
emote laughs and pats you on the back, &1Well good luck with that. I have one final task for you to take on.&2
wait 2 s
say I'd like for you to get us some fresh Thrawln meat from a Thrawln in the Blood Dunes. Be careful though, they're strong. Do this, and I'll consider you to be an official member of Team Bardock!&2
        set onToraQ4
        remote onToraQ4 %actor.id%     
        
    else
        if (!%actor.varexists(onToraQ1)% && !%actor.varexists(onToraQ2)% &&  !%actor.varexists(onToraQ3)%  && !%actor.varexists(onToraQ4)%)
            wait 1
            say Bardock will see you.
        end
    end 
else
    wait 1
    say Bardock doesn't want to see the likes of you.
end
~
#2204
Drunk Saiyan Trigger~
0 ab 50
~
eval num %random.4%
switch %num%
 case 1
  wait 1
  %echo% %self.name% begins to sing a drinking song to himself, but it comes out in slurred mumbles.
  break
 case 2
  wait 1
  %echo% %self.name% looks around the pub as his face starts to look ill.
  wait 3 sec
  %echo% %self.name% stands up shakily.
  wait 2 sec
  %echo% %self.name% seems like he is about to puke!
  wait 3 sec
  %echo% %self.name% looks better after a moment and returns to his seat.
  break
  case 3
   wait 1
   say I think I will have another one...
    wait 2 sec
   %force% barkeep say You haven't finished the drink you are holding you dumbass...
   wait 4 sec
   say Oh....mmmmm
   wait 2 sec
   %echo% %self.name% starts to nod off.
   break
  case 4
   wait 1
   %echo% %self.name% spills some of his drink on the table he is sitting at.
  break
 done
~
#2205
Drunk Sit~
0 n 100
~
sit chair
~
#2206
King Vegeta Load~
0 n 100
~
wait 1
sit throne
~
#2207
King Vegeta Greet~
0 g 100
~
if (%actor.varexists(rebelmission)%)
 if (%actor.rebelmission% == accepted || %actor.rebelmission% == completed)
  wait 1 sec
  say %actor.name% I have heard you sympathise with the rebel scum!
  punch %actor.name%
  set kingmission enemy
  remote kingmission %actor.id%
  wait 4 sec
  say You WILL die!
 else
  if (!%actor.varexists(kingmission)%)
   wait 3 sec
   scowl
   wait 2 sec
   say Those rebels think they can continue to skulk in the shadows...
   wait 4 sec
   say If they would show themselves I would easily eliminate them...
   wait 5 sec
   %send% %actor% %self.name% looks at you.
   %echoaround% %actor% %self.name% looks at %actor.name%.
   wait 3 sec
   say Hey you...
   wait 3 sec
   %send% %actor% %self.name% smiles while looking at you.
   %echoaround% %actor% %self.name% smiles at %actor.name%.
   wait 4 sec
   say Yes. You could be useful.
   wait 4 sec
   say I require your services.
   wait 4 sec
   say Don't worry, you will be compensated.
   wait 4 sec
   say If you are interested then pledge your loyalty to me, I will then explain what I require.
   wait 3 sec
   say As well as what I offer...
   set kingmission proposal
   remote kingmission %actor.id%
  elseif (%actor.kingmission% == proposal)
   wait 2 sec
   %send% %actor% %self.name% looks at you.
   %echoaround% %actor% %self.name% looks at %actor.name%.
   wait 3 sec
   say Well are you interested in my proposal or not? If so then pledge your loyalty!
  elseif (%actor.kingmission% == accepted)
   wait 2 sec
   %send% %actor% %self.name% looks at you.
   %echoaround% %actor% %self.name% looks at %actor.name%.
   wait 3 sec
   say Why haven't you brought me the info I requested yet?
   wait 3 sec
   say Don't take forever. This is important and I am not a patient man.
  elseif (%actor.kingmission% == complete)
   wait 2 sec
   %send% %actor% %self.name% looks at you.
   %echoaround% %actor% %self.name% looks at %actor.name%.
   wait 3 sec
   say %actor.name% I have no further use for you. Leave my sight before you irritate me.
  elseif (%actor.kingmission% == enemy)
   wait 1 sec
   say %actor.name%! You will regret showing your face before me!
   punch %actor.name%
   if (!%self.varexists(loaded)%)
    wait 3 sec
    %echo% Four elite guards rush in to aid the King!
    %load% mob 2237
    %load% mob 2237
    %load% mob 2237
    %load% mob 2237
    set loaded
    remote loaded %self.id%
   end
  end
 end
else
 if (!%actor.varexists(kingmission)%)
  wait 3 sec
  scowl
  wait 2 sec
  say Those rebels think they can continue to skulk in the shadows...
  wait 4 sec
  say If they would show themselves I would easily eliminate them...
  wait 5 sec
  %send% %actor% %self.name% looks at you.
  %echoaround% %actor% %self.name% looks at %actor.name%.
  wait 3 sec
  say Hey you...
  wait 3 sec
  %send% %actor% %self.name% smiles while looking at you.
  %echoaround% %actor% %self.name% smiles at %actor.name%.
  wait 4 sec
  say Yes. You could be useful.
  wait 4 sec
  say I require your services.
  wait 4 sec
  say Don't worry, you will be compensated.
  wait 4 sec
  say If you are interested then pledge your loyalty to me, I will then explain what I require.
  wait 3 sec
  say As well as what I offer...
  set kingmission proposal
  remote kingmission %actor.id%
 elseif (%actor.kingmission% == proposal)
  wait 2 sec
  %send% %actor% %self.name% looks at you.
  %echoaround% %actor% %self.name% looks at %actor.name%.
  wait 3 sec
  say Well are you interested in my proposal or not? If so then pledge your loyalty!
 elseif (%actor.kingmission% == accepted)
  wait 2 sec
  %send% %actor% %self.name% looks at you.
  %echoaround% %actor% %self.name% looks at %actor.name%.
  wait 3 sec
  say Why haven't you brought me the info I requested yet?
  wait 3 sec
  say Don't take forever. This is important and I am not a patient man.
 elseif (%actor.kingmission% == complete)
  wait 2 sec
  %send% %actor% %self.name% looks at you.
  %echoaround% %actor% %self.name% looks at %actor.name%.
  wait 3 sec
  say %actor.name% I have no further use for you. Leave my sight before you irritate me.
 elseif (%actor.kingmission% == enemy)
  wait 1 sec
  say %actor.name%! You will regret showing your face before me!
  punch %actor.name%
  if (!%self.varexists(loaded)%)
   wait 3 sec
   %echo% Four elite guards rush in to aid the King!
   %load% mob 2237
   %load% mob 2237
   %load% mob 2237
   %load% mob 2237
   set loaded
   remote loaded %self.id%
  end
 end
end
~
#2208
King Vegeta Fight~
0 k 100
~
if (%self.pos% == Sitting)
stand
end
eval num %random.100%
if (!%actor.varexists(kingmission)%)
 set kingmission enemy
 remote kingmission %actor.id%
elseif (%actor.varexists(kingmission)%)
 if (%actor.kingmission% != enemy)
  set kingmission enemy
  remote kingmission %actor.id%
 end
elseif (%num% >= 95)
 galik %actor.name%
end
~
#2209
King Vegeta Pledge Trigger~
0 c 100
pledge~
if (%actor.varexists(rebelmission)%)
 if (%actor.rebelmission% == accepted || %actor.rebelmission% == completed)
  wait 1
  say You are working with rebels! You can plead for your life all you want, it will do NO GOOD!
 else
  if (!%actor.varexists(kingmission)%)
   wait 1
   say Let me finish speaking!
  elseif (%actor.kingmission% == enemy)
   wait 1 sec
   say Ha ha! You are going to die! You can be loyal in death though I guess!
  elseif (%actor.kingmission% == accepted)
   wait 1 sec
   say You are already working on a mission for me.
   wait 3 sec
   say Or are you trying to tell me you have failed me?
   wait 4 sec
   say For your sake I hope not...
  elseif (%actor.kingmission% == complete)
   wait 1 sec
   say I have no further need for you.
   wait 3 sec
   say Leave my sight.
  else
   wait 1 sec
   %send% %actor% You kneel before %self.name% and pledge your loyalty.
   %echoaround% %actor% %actor.name% kneels before %self.name% and pledges %actor.hisher% loyalty.
   wait 4 sec
   say Good. You may rise.
   wait 2 sec
   %send% %actor% You return from your kneeling position.
   %echoaround% %actor% %actor.name% stops kneeling.
   wait 3 sec
   say What I need from you is this. Rebels are plotting my kingdom's downfall.
   wait 4 sec
   say They want to return all of Vegeta to the way it used to be, total anarchy.
   wait 4 sec
   say I know this will be the doom of our race, but they don't care.
   wait 3 sec
   say I know of the base they have in this city. However I do not know where their main hideout is.
   wait 5 sec
   say That is where you come in. I need you to infiltrate their base in this city, find out the location of their hideout, and bring that information back to me.
   wait 6 sec
   say I can't use my official troops as they would never manage to get into the base without the rebels destroying the information I seek.
   wait 5 sec
   say If you do this I will pay you 5,000 zenni.
   wait 3 sec
   say Now go.
   set kingmission accepted
   remote kingmission %actor.id%
  end
 end
else
 if (!%actor.varexists(kingmission)%)
  wait 1
  say Let me finish speaking!
 elseif (%actor.kingmission% == enemy)
  wait 1 sec
  say Ha ha! You are going to die! You can be loyal in death though I guess!
 elseif (%actor.kingmission% == accepted)
  wait 1 sec
  say You are already working on a mission for me.
  wait 3 sec
  say Or are you trying to tell me you have failed me?
  wait 4 sec
  say For your sake I hope not...
 elseif (%actor.kingmission% == complete)
  wait 1 sec
  say I have no further need for you.
  wait 3 sec
  say Leave my sight.
 else
  wait 1 sec
  %send% %actor% You kneel before %self.name% and pledge your loyalty.
  %echoaround% %actor% %actor.name% kneels before %self.name% and pledges %actor.hisher% loyalty.
  wait 4 sec
  say Good. You may rise.
  wait 2 sec
  %send% %actor% You return from your kneeling position.
  %echoaround% %actor% %actor.name% stops kneeling.
  wait 3 sec
  say What I need from you is this. Rebels are plotting my kingdom's downfall.
  wait 4 sec
  say They want to return all of Vegeta to the way it used to be, total anarchy.
  wait 4 sec
  say I know this will be the doom of our race, but they don't care.
  wait 3 sec
  say I know of the base they have in this city. However I do not know where their main hideout is.
  wait 5 sec
  say That is where you come in. I need you to infiltrate their base in this city, find out the location of their hideout, and bring that information back to me.
  wait 6 sec
  say I can't use my official troops as they would never manage to get into the base without the rebels destroying the information I seek.
  wait 5 sec
  say If you do this I will pay you 5,000 zenni.
  wait 3 sec
  say Now go.
  set kingmission accepted
  remote kingmission %actor.id%
 end
end
~
#2210
King Vegeta Receive~
0 j 100
~
if (!actor.varexists(kingmission)%)
 return 0
 wait 1 sec
 say I do not want anything from someone of questionable loyalty.
elseif (%actor.kingmission% == enemy)
 return 0
 wait 1
 say I want nothing from YOU!
elseif (%actor.kingmission% == proposal)
 return 0
 wait 1
 say You haven't pledged loyalty to me yet, I want nothing else from you right now.
elseif (%actor.kingmission% == complete)
 return 0
 wait 1
 say I want nothing more from you. Leave my sight...
else
 if (%object.vnum% != 2267)
  return 0
  wait 1
  say Do not hand me garbage.
 else
  wait 1
  %purge% %object%
  say Well done. Now those rebels are as good as crushed.
  wait 3 sec
  say Here is the payment I promised. One of the relics from my treasury and some money.
  %load% o 2272
  drop bracer
  %force% %actor% get bracer
  eval num 5000
  set %actor.gold(%num%)%
  set kingmission complete
  remote kingmission %actor.id%
  wait 1 sec
  %send% %actor% You also receive 15,000 exp!
  eval num2 15000
  set %actor.exp(%num2%)%
  wait 3 sec
  say Now go.
 end
end
~
#2211
Gourd Rebel Greet~
0 g 100
~
if (%actor.varexists(kingmission)%)
 if (%actor.kingmission% == accepted || %actor.kingmission% == complete)
  wait 1
  say Enemy in the base!
  wait 1 sec
  punch %actor.name%
 else
  if (%actor.varexists(rebelmission)%)
   if (%actor.rebelmission% == proposal)
    wait 2 sec
    %send% %actor% %self.name% looks you up and down.
    %echoaround% %actor% %self.name% looks %actor.name% up and down.
    wait 3 sec
    say So are you going to @gaccept@C our offer or not?
   elseif (%actor.rebelmission% == accepted)
    wait 2 sec
    %send% %actor% %self.name% looks you up and down.
    %echoaround% %actor% %self.name% looks %actor.name% up and down.
    wait 3 sec
    say How long do you plan on taking? We need those security layouts.
   elseif (%actor.rebelmission% == completed)
    wait 2 sec
    say Ah %actor.name%, thanks for the job you did for us.
    wait 4 sec
    say We have no further need of you as of right now though.
    wait 3 sec
    say You best not be seen with us, it could attract attention to us as well as yourself.
   end
  else
   wait 2 sec
   %send% %actor% %self.name% looks you up and down.
   %echoaround% %actor% %self.name% looks %actor.name% up and down.
   wait 3 sec
   say Who might you be?
   wait 4 sec
   say I don't have any information on you.
   wait 2 sec
   say You aren't with King Vegeta are you?
   wait 4 sec
   %send% %actor% %self.name% squints at you for a moment.
   %echoaround% %actor% %self.name% squints at %actor.name% for a moment.
   wait 3 sec
   say How would you like to earn some money?
   wait 4 sec
   say I won't bore you with any details. However if you do a job for us we will reward you.
   wait 5 sec
   say You like money don't ya?
   wait 4 sec
   say I have to warn you about one thing though, you won't be popular with the King if you @gaccept@C.@n
   wait 5 sec
   say Though the job will carry a 2500 zenni reward as well as something else nice.
   set rebelmission proposal
   remote rebelmission %actor.id%
  end
 end
else
  if (%actor.varexists(rebelmission)%)
   if (%actor.rebelmission% == proposal)
    wait 2 sec
    %send% %actor% %self.name% looks you up and down.
    %echoaround% %actor% %self.name% looks %actor.name% up and down.
    wait 3 sec
    say So are you going to @gaccept@C our offer or not?
   elseif (%actor.rebelmission% == accepted)
    wait 2 sec
    %send% %actor% %self.name% looks you up and down.
    %echoaround% %actor% %self.name% looks %actor.name% up and down.
    wait 3 sec
    say How long do you plan on taking? We need those security layouts.
   elseif (%actor.rebelmission% == completed)
    wait 2 sec
    say Ah %actor.name%, thanks for the job you did for us.
    wait 4 sec
    say We have no further need of you as of right now though.
    wait 3 sec
    say You best not be seen with us, it could attract attention to us as well as yourself.
   end
  else
   wait 2 sec
   %send% %actor% %self.name% looks you up and down.
   %echoaround% %actor% %self.name% looks %actor.name% up and down.
   wait 3 sec
   say Who might you be?
   wait 4 sec
   say I don't have any information on you.
   wait 2 sec
   say You aren't with King Vegeta are you?
   wait 4 sec
   %send% %actor% %self.name% squints at you for a moment.
   %echoaround% %actor% %self.name% squints at %actor.name% for a moment.
   wait 3 sec
   say How would you like to earn some money?
   wait 4 sec
   say I won't bore you with any details. However if you do a job for us we will reward you.
   wait 5 sec
   say You like money don't ya?
   wait 4 sec
   say I have to warn you about one thing though, you won't be popular with the King if you @gaccept@C.@n
   wait 5 sec
   say Though the job will carry a 2500 zenni reward as well as something else nice.
   set rebelmission proposal
   remote rebelmission %actor.id%
  end
end
~
#2212
Gourd Accept Trigger~
0 c 100
accept~
if (%actor.varexists(kingmission)%)
 if (%actor.kingmission% == accepted || %actor.kingmission% == complete)
  wait 1
  say Ha! I spit on the words of a lapdog of King Vegeta!
 else
  if (%actor.varexists(rebelmission)%)
   if (%actor.rebelmission% == accepted)
    wait 1
    say You are already on a mission for us!
   elseif (%actor.rebelmission% == completed)
    wait 1
    say You already completed your mission. We have no further work for an outsider such as yourself.
   elseif (%actor.rebelmission% == proposal)
    wait 1
    say Ah good, good.
    wait 3 sec
    say The reason we need someone like you to do this job is because anyone else would be recognised trying to enter the palace.
    wait 5 sec
    say Recently our leader was captured and placed in the palace prison.
    wait 4 sec
    say Usually it would be the death penalty immediatly, however they want the knowledge he has.
    wait 5 sec
    say Ryo won't give it up though, we know this...
    wait 6 sec
    say Where you come in though is he managed to steal and hide the security plans of the palace.
    wait 4 sec
    say We need you to enter the palace and try to speak with Ryo.
    wait 4 sec
    say It won't be easy, but there is no possibility of us managing this with our members.
    wait 5 sec
    say It would tip them off if Ryo is freed, so he will have to wait to be rescued.
    wait 4 sec
    say I know he will agree.
    wait 3 sec
    say Go to Ryo, talk to him about @gthe plans@C, and he should tell you where he hid them.@n
    set rebelmission accepted
    remote rebelmission %actor.id%
   end
  else
   wait 1
   say Let me finish speaking...
  end
 end 
else
  if (%actor.varexists(rebelmission)%)
   if (%actor.rebelmission% == accepted)
    wait 1
    say You are already on a mission for us!
   elseif (%actor.rebelmission% == completed)
    wait 1
    say You already completed your mission. We have no further work for an outsider such as yourself.
   elseif (%actor.rebelmission% == proposal)
    wait 1
    say Ah good, good.
    wait 3 sec
    say The reason we need someone like you to do this job is because anyone else would be recognised trying to enter the palace.
    wait 5 sec
    say Recently our leader was captured and placed in the palace prison.
    wait 4 sec
    say Usually it would be the death penalty immediatly, however they want the knowledge he has.
    wait 5 sec
    say Ryo won't give it up though, we know this...
    wait 6 sec
    say Where you come in though is he managed to steal and hide the security plans of the palace.
    wait 4 sec
    say We need you to enter the palace and try to speak with Ryo.
    wait 4 sec
    say It won't be easy, but there is no possibility of us managing this with our members.
    wait 5 sec
    say It would tip them off if Ryo is freed, so he will have to wait to be rescued.
    wait 4 sec
    say I know he will agree.
    wait 3 sec
    say Go to Ryo, talk to him about @gthe plans@C, and he should tell you where he hid them.@n
set kingmission enemy
remote kingmission %actor.id%
    set rebelmission accepted
    remote rebelmission %actor.id%
   end
  else
   wait 1
   say Let me finish speaking...
  end
end
~
#2213
Gourd Receive Trigger~
0 j 100
~
if (%actor.varexists(kingmission)%)
 if (%actor.kingmission% == accepted || %actor.kingmission% == complete)
  return 0
  wait 1
  say We want nothing from you!
 else
  if (%actor.varexists(rebelmission)%)
   if (%actor.rebelmission% == completed)
    return 0
    wait 1
    say We don't need anything further from you.
   elseif (%actor.rebelmission% == proposal)
    return 0
    wait 1
    say Until you accept the deal we don't want anything from you.
   elseif (%actor.rebelmission% == accepted)
    if (%object.vnum% != 2268)
     return 0
     wait 1
     say This isn't what we agreed upon, stop messing around.
    else
     wait 1
     %purge% plans
     wait 1 sec
     say With these plans soon King Vegeta shall fall!
     wait 3 sec
     say Ahem, thanks for your help.
     wait 4 sec
     say As agreed, here is the 2500 zenni and this as well...
     eval num %actor.gold% + 2500
     set %actor.gold(%num%)%
     %load% obj 2269
     give bag %actor.name%
     wait 2 sec
     %send% %actor% You also receive 10000 exp!
     set %actor.exp(10000)%
     set rebelmission completed
     remote rebelmission %actor.id%
    end
   end
  else
   return 0
   wait 1
   say Let me finish speaking.
  end
 end
else
  if (%actor.varexists(rebelmission)%)
   if (%actor.rebelmission% == completed)
    return 0
    wait 1
    say We don't need anything further from you.
   elseif (%actor.rebelmission% == proposal)
    return 0
    wait 1
    say Until you accept the deal we don't want anything from you.
   elseif (%actor.rebelmission% == accepted)
    if (%object.vnum% != 2268)
     return 0
     wait 1
     say This isn't what we agreed upon, stop messing around.
    else
     wait 1
     %purge% plans
     wait 1 sec
     say With these plans soon King Vegeta shall fall!
     wait 3 sec
     say Ahem, thanks for your help.
     wait 4 sec
     say As agreed, here is the 2500 zenni and this as well...
     eval num %actor.gold% + 2500
     set %actor.gold(%num%)%
     %load% obj 2269
     give bag %actor.name%
     wait 2 sec
     %send% %actor% You also receive 10000 exp!
     set %actor.exp(10000)%
     set rebelmission completed
     remote rebelmission %actor.id%
    end
   end
  else
   return 0
   wait 1
   say Let me finish speaking.
  end
end
~
#2214
Gourd Death Trigger~
0 f 100
~
if (%actor.varexists(kingmission)%)
 if (%actor.kingmission% == accepted)
  %load% obj 2267
 end
end
~
#2215
Ryo Speech Trigger~
0 d 100
plans~
if (!%actor.varexists(plansdig)%)
 if (%actor.varexists(kingmission)%)
  if (%actor.kingmission% == accepted || %actor.kingmission% == completed)
   wait 1
   say You wretch! I don't want to speak to you!
  else
   if (%actor.varexists(rebelmission)%)
    if (%actor.rebelmission% == completed)
     wait 1 sec
     say What? I already told you where the plans were...
    elseif (%actor.rebelmission% == proposal)
     wait 1 sec
     say Plans? What plans? I don't trust you. Get lost. 
    elseif (%actor.rebelmission% == accepted)
     wait 2 sec
     say Ah so you were sent by Gourd.
     wait 4 sec
     say That Gourd, he is so reliable.
     wait 3 sec
     say If Gourd trusts you for this then so will I.
     wait 4 sec
     say I hid the plans at the base of the southwest tower.
     wait 3 sec
     say You will have to @gdig@C them up.@n
     set plansdig
     remote plansdig %actor.id%
     wait 2 sec
     say You better hurry before you are spotted by a guard...
    end
   else
    wait 1 sec
    say Who are you? Get lost... 
   end
  end 
 else
   if (%actor.varexists(rebelmission)%)
    if (%actor.rebelmission% == completed)
     wait 1 sec
     say What? I already told you where the plans were...
    elseif (%actor.rebelmission% == proposal)
     wait 1 sec
     say Plans? What plans? I don't trust you. Get lost. 
    elseif (%actor.rebelmission% == accepted)
     wait 2 sec
     say Ah so you were sent by Gourd.
     wait 4 sec
     say That Gourd, he is so reliable.
     wait 3 sec
     say If Gourd trusts you for this then so will I.
     wait 4 sec
     say I hid the plans at the base of the southwest tower.
     wait 3 sec
     say You will have to @gdig@C them up.@n
     set plansdig
     remote plansdig %actor.id%
     wait 2 sec
     say You better hurry before you are spotted by a guard...
    end
   else
    wait 1 sec
    say Who are you? Get lost... 
   end
 end
else
 wait 1 sec
 say I already told you where the plans are burried, hurry up and get them to Gourd!
end
~
#2216
Plans Dig Trigger~
2 c 100
dig~
if (%actor.varexists(plansdig)%)
 if (%actor.plansdig% != dug)
 wait 1
 %send% %actor% You start digging at the base of the tower where some loose dirt can be seen.
 %echoaround% %actor% %actor.name% starts digging at the base of the tower.
 wait 3 sec
 %send% %actor% You continue to dig.
 %echoaround% %actor% %actor.name% continues digging.
 wait 3 sec
 %send% %actor% You dig up a package.
 %echoaround% %actor% %actor.name% digs up a package.
 %load% obj 2268
 %force% %actor% get plans
 set plansdig dug
 remote plansdig %actor.id%
 else
  wait 1
  %send% %actor% You have already dug up the plans.
 end
else
 wait 1
 %send% %actor% You dig and dig but find nothing.
end
~
#2217
Prison Guard Greet~
0 g 100
~
if (%actor.varexists(rebelmission)%)
 if (%actor.rebelmission% == accepted || %actor.rebelmission% == completed)
  wait 1 sec
  say What are you doing in here!?
  wait 1
  %send% %actor% %self.name% rushes at you!
  %echoaround% %actor% %self.name% rushes at %actor.name%!
  wait 1
  punch %actor.name%
 end
end
~
#2218
Nursery Caretaker Trig~
0 ab 10
~
wait 1
%echo% %self.name% checks a chart.
wait 2 sec
say We have some promising little saiyans this time...
~
#2219
Newborn Saiyan Trig~
0 g 50
~
eval num %random.3%
switch (%num%)
 case 1
  wait 1 sec
  %echo% %self.name% starts crying loudly.
  break
 case 2
  wait 1 sec
  %echo% %self.name% coos contentedly.
  break
 case 3
  wait 1 sec
  %echo% %self.name% pumps %self.hisher% fists into the air.
  break
 done
~
#2220
Prince Vegeta Trigger~
0 g 100
~
if (!%actor.varexists(pvegeta)%)
 wait 1 s
 %send% %actor% %self.name% glares at you.
 %echoaround% %actor% %self.name% glares at %actor.name%.
 wait 2 s
 say What are you doing in here?
 wait 4 s
 say Nevermind. It doesn't matter...
 wait 1 s
 %echo% %self.name% kicks a wall smashing a hole into it.
 wait 2 s
 say BAH! Damn that rule.
 wait 1 s
 %send% %actor% Rule huh? Perhaps you should ask about it.
end
~
#2221
Prince Vegeta Speech~
0 d 100
rule~
if (!%actor.varexists(pvegeta)%)
 wait 1 s
 say Huh? You want to know about the rule?
 wait 3 s
 if (%actor.race% == Saiyan)
  say Why should I tell a low classer like you?
 else
  say Why should I tell you?
 end
 wait 3 s
 say Bah fine, I will tell you.
 wait 3 s
 say There exists a tower on this planet that is older than our civilization.
 wait 5 s
 say This tower is known as the Tower of Pride.
 wait 5 s
 say Those who overcome the trials in this tower are known as the greatest warriors on planet Vegeta.
 wait 6 s
 say So of course I should be able to conquer this tower and prove that my pride stands above all!
 wait 7 s
 say But there is magic about the tower and those of royal blood are forbidden from entering.
 wait 7 s
 say Our pride is already assured, but that isn't good enough for me!
 wait 6 s
 say There is a book the Keeper of the tower has.
 wait 4 s
 say I want that book, I would give almost anything to have it...
 set pvegeta
 remote pvegeta %actor.id%
elseif (%actor.pvegeta% == done)
 wait 1 s
 say You already retrieved the book for me.
 wait 2 s
 say Get lost.
else
 wait 1 s
 say I already explained it to you!
 wait 3 s
 say Are you stupid AND useless!?
 wait 3 s
 say Bah fine, I will tell you again.
 wait 4 s
 say There exists a tower on this planet that is older than our civilization.
 wait 5 s
 say This tower is known as the Tower of Pride.
 wait 5 s
 say Those who overcome the trials in this tower are known as the greatest warriors on planet Vegeta.
 wait 6 s
 say So of course I should be able to conquer this tower and prove that my pride stands above all!
 wait 7 s
 say But there is magic about the tower and those of royal blood are forbidden from entering.
 wait 7 s
 say Our pride is already assured, but that isn't good enough for me!
 wait 6 s
 say There is a book the Keeper of the tower has.
 wait 4 s
 say I want that book, I would give almost anything to have it...
end
~
#2222
Prince Vegeta Receive~
0 j 100
~
if (%object.vnum% != 2920)
 return 0
 wait 1
 say What is this trash? I don't want it.
else
 if (%actor.varexists(pvegeta)%)
  if (%actor.pvegeta% == done)
   return 0
   wait 1
   say I don't want a duplicate. I already have gotten the book from you. Remember!?
  else
   wait 1
   %purge% %object%
   say You managed to retrieve it?
   wait 2 s
   say At last...
   wait 4 s
   say Oh I promised you a reward didn't I?
   wait 4 s
   say Normally letting you live would be reward enough however I am feeling generous.
   wait 5 s
   say Take this...
   %load% obj 2921
   give royal %actor.name%
   set pvegeta done
   remote pvegeta %actor.id%
   wait 1 s
   say Now get lost.
  end
 end
end
~
#2223
Vegeta Fight Trigger~
0 k 10
~
eval num %random.100%
if (%num% >= 75)
 galik
end
if (!%self.varexists(tcount)%)
 set tcount 0
 remote tcount %self.id%
elseif (%self.tcount% < 10 && rand_number(1, 5) == 5)
 eval transform %self.tcount% + 1
 set tcount %transform%
 remote tcount %self.id%
elseif (%self.tcount% == 10)
 rdelete tcount %self.id%
 wait 1 sec
 mecho %self.alias% begins to crouch down, hands clenched at his sides. The ground beneath his feet crumbles as his powerlevel begins to skyrocket!
 wait 4 sec
 mecho %self.alias% screams with power and rage as a golden aura bursts up around his body and his hair likewise turns golden!
 %load% blahblah
 %purge% %self%
end
~
#2224
Tora Noob Quest Complete~
0 j 100
~
if (%actor.varexists(onToraQ1)% && %object.vnum% == 2205)
    %echoaround% %actor% %actor.name% gives Tora a Gladiatorial Medal.
    %purge% %object%
    wait 1
    emote takes the medal from %actor.name%, &1You managed to get it. You've surprised me, I didn't think you would!&2
    wait 2 s
    emote lets a loud laugh escape, &1Here, Take this zenni as a reward. And return when you're stronger, I might have something else for you to do.&2
    nop %actor.gold(500)%
    rdelete onToraQ1 %actor.id%
    set ToraQ1Complete
    remote ToraQ1Complete %actor.id%
    
elseif (%actor.varexists(onToraQ2)% && %object.vnum% == 2206)
    %echoaround% %actor% %actor.name% gives Tora some Leaked Information.
    %purge% %object%
    wait 1
    emote takes the documents and promptly destroys them with an energy blast, &1Good. I hope you also weeded out the weaklings.&2
    wait 2 s
    emote pulls out some tickets from his armor, &1Here, take these. They're tickets that you can use at the spaceport, that will let you get on a ship that takes you to a Space Station that has ships that will take you all over the place. Can be quite handy to get around.&2
    wait 1 s
    say And take some money as well.
    nop %actor.gold(500)%
    %load% obj 3999
    %load% obj 3999
    %load% obj 3999
    %load% obj 3999
    give all.ticket %actor.name%
    wait 1
    say The Spaceport is directly south of here. You can't miss it.
    rdelete onToraQ2 %actor.id%
    set ToraQ2Complete
    remote ToraQ2Complete %actor.id%
    
elseif (%actor.varexists(onToraQ3)% && %object.vnum% == 2506)
    %echoaround% %actor% %actor.name% gives Tora a Forn Tooth.
    %purge% %object%
    wait 1
    emote takes the tooth, &1Well, this is quite the fine tooth. I hope you didn't cop out and just take it from an already dead one.&2
    wait 2 s
    emote tosses a bag of zenni at %actor.name%, &1Here is some money.&2
    nop %actor.gold(2000)%
    rdelete onToraQ3 %actor.id%
    set ToraQ3Complete
    remote ToraQ3Complete %actor.id%
    
elseif (%actor.varexists(onToraQ4)% && %object.vnum% == 2507)
    %echoaround% %actor% %actor.name% gives Tora some fresh Thrawln meat.
    %purge% %object%
    wait 1
    emote takes the meat and stares at it, &1This is a good chunk of meat, but it could be bigger. But you did complete this task like I asked, so I can't complain.&2
    wait 3 s
    emote heads instide and returns with some armor with Bardock's symbol on the front, &1This is special battle armor made specifically for our team. Enjoy.&2
    %load% obj 2207
    give armor %actor.name%
    rdelete onToraQ4 %actor.id%
    rdelete ToraQ1Complete %actor.id%
    rdelete ToraQ2Complete %actor.id%
    rdelete ToraQ3Complete %actor.id%
    set ToraQComplete
    remote ToraQComplete %actor.id%
    
else
    return 0
    wait 1
    %echoaround% %actor% %actor.name% attempts to give Tora something.
    say I don't want this.
end
~
#2225
Seripa Food Give~
0 d 100
food~
if (!%actor.varexists(seripafood1)% && !%actor.varexists(seripafood)%)
    wait 1 sec
    say Alright, just give me a second or two.
    wait 3 sec
    emote picks up a sack and a tankard.
    wait 3 sec
    say Here, take this.
    %load% obj 2201
    %load% obj 2201
    %load% obj 2201
    %load% obj 2201
    %load% obj 2204
    give all.meat %actor.name%
    give all.tank %actor.name%
    set seripafood1
    remote seripafood1 %actor.id%
elseif (!%actor.varexists(seripafood2)% && !%actor.varexists(seripafood)%)
    wait 1 sec
    say So, you want more? Good. You need to eat to keep up your strength.
    wait 3 sec
    emote picks up a sack and a tankard.
    wait 3 sec
    say Here, take this.
    %load% obj 2201
    %load% obj 2201
    %load% obj 2201
    %load% obj 2201
    %load% obj 2204
    give all.meat %actor.name%
    give all.tank %actor.name%
    set seripafood2
    remote seripafood2 %actor.id%
elseif (!%actor.varexists(seripafood3)% && !%actor.varexists(seripafood)%)
    wait 1 sec
    say You have a good appetite. It's a good sign of strength. The more you eat, the stronger you get!
    wait 3 sec
    emote laughs as %self.heshe% grabs another sack and tankard.
    wait 3 sec
    say Here, take this and enjoy. It's the last free meal I will give out.
    %load% obj 2201
    %load% obj 2201
    %load% obj 2201
    %load% obj 2201
    %load% obj 2204
    give all.meat %actor.name%
    give all.tank %actor.name%
    rdelete seripafood1 %actor.id%
    rdelete seripafood2 %actor.id%
    set seripafood
    remote seripafood %actor.id%
else
    wait 1 sec
    say I've given you enough freebies. Get lost and find your own now.
end
~
#2226
Tora Quest Checkers~
0 g 100
~
if (%actor.varexists(onToraQ1)%)
    wait 1
    say What are you doing back? Can you not follow simple instructions?
    wait 2 s
    emote shakes his head and sighs, &1Go south to the intersection, east, then south again and take the first road to the east. And remember to bring back a the medal.&2
elseif (%actor.varexists(onToraQ2)%)
    wait 1
    say What are you doing back? Can you not follow simple instructions?
    wait 2 s
    emote points north, &Go into the palace north of here and beat up some of the Elite class there and try to find out who the traitor is. If you're finding them too strong, you can try heading to the Blood Dunes to train on some Grashl's there.&2
elseif (%actor.varexists(onToraQ3)%)
    wait 1
    say What are you doing back? Can you not follow simple instructions?
    wait 2 s
    emote points north, &1Go to the Blood Dunes and get a Forn's tooth to prove your strength. Just go south to the intersection, west, south, then west again.&2
elseif (%actor.varexists(onToraQ4)%)
    wait 1
    say What are you doing back? Can you not follow simple instructions?
    wait 2 s
    say Go to the Blood Dunes again and get some fresh Thrawln meat for us. Got it? Good.
end
~
#2227
Tora Q1 Mob Death~
0 f 100
~
if (%actor.varexists(onToraQ1)%)
    %load% obj 2205
end
~
#2228
Tora Q2 Mob Death~
0 f 100
~
if (%actor.varexists(onToraQ2)%)
    %load% obj 2206
end
~
#2229
Seripa Mute Food~
0 c 3
point~
if %arg% == food
    if (!%actor.varexists(seripafood1)% && !%actor.varexists(seripafood)%)
        wait 1 sec
        say Alright, just give me a second or two.
        wait 3 sec
        emote picks up a sack and a tankard.
        wait 3 sec
        say Here, take this.
        %load% obj 2201
        %load% obj 2201
        %load% obj 2201
        %load% obj 2201
        %load% obj 2204
        give all.meat %actor.name%
        give all.tank %actor.name%
        set seripafood1
        remote seripafood1 %actor.id%
    elseif (!%actor.varexists(seripafood2)% && !%actor.varexists(seripafood)%)
        wait 1 sec
        say So, you want more? Good. You need to eat to keep up your strength.
        wait 3 sec
        emote picks up a sack and a tankard.
        wait 3 sec
        say Here, take this.
        %load% obj 2201
        %load% obj 2201
        %load% obj 2201
        %load% obj 2201
        %load% obj 2204
        give all.meat %actor.name%
        give all.tank %actor.name%
        set seripafood2
        remote seripafood2 %actor.id%
    elseif (!%actor.varexists(seripafood3)% && !%actor.varexists(seripafood)%)
        wait 1 sec
        say You have a good appetite. It's a good sign of strength. The more you eat, the stronger you get!
        wait 3 sec
        emote laughs as %self.heshe% grabs another sack and tankard.
        wait 3 sec
        say Here, take this and enjoy. It's the last free meal I will give out.
        %load% obj 2201
        %load% obj 2201
        %load% obj 2201
        %load% obj 2201
        %load% obj 2204
        give all.meat %actor.name%
        give all.tank %actor.name%
        rdelete seripafood1 %actor.id%
        rdelete seripafood2 %actor.id%
        set seripafood
        remote seripafood %actor.id%
    else
        wait 1 sec
        say I've given you enough freebies. Get lost and find your own now.
    end
else
    %send% %actor% Point at what?
end
~
#2230
Saiyan Baby Tail Rip~
2 c 100
rip~
set target_char %self.people%
while %target_char%
 set tmp_tar %target_char.next_in_room%
 if %target_char.vnum% == 2245
  eval baby %target_char%
 end
 set target_char %tmp_tar%
done
 
if (%arg% == newborn || %arg% == baby || %arg% == saiyan || %arg% == sai)
 if (%baby%)
  if (!%actor.has_item(2402)%)
   %echoaround% %actor% %actor.name% reaches out and rips off the tail of a Saiyan Baby! A nurse quickly takes away the crying baby, shooting %actor.name% a dirty look.
   %send% %actor% You reach out and rip off the tail of a Saiyan Baby! A nurse quickly takes away the crying baby while giving you a dirty look.
   %purge% newborn
   %load% obj 2402
   %force% %actor% get tail
  else
   %send% %actor% You already have one, you monster!
  end
 else
  %send% %actor% You don't see that here.
 end
else
 %send% %actor% Rip what?
end
~
$~
