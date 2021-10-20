#4200
Bounty Hunter Quest giver~
0 d 100
level one, level two, level three, level four, level five~
if %speech% == level one
  if !%actor.varexists(bounty_completed_1)%
    say Ahh, the level one bounty. That one's a mutant named Johto, been harrassing a noble's daughter. Too bad for him that when you've got money in this town, bounties come before alerting the guards. He should still be around the rich district. If you want the bounty, bring me back his head and you'll get paid for it.
    %at% 4007 %load% mob 4214
    wait 2 s
    say You'd better hurry on along, you're not the only one that'll be going for it.
  else
    say You've already done in Johto.
  end
end
if %speech% == level two
  if !%actor.varexists(bounty_completed_2)%
    say Mmm, a girl named Aleria tried to steal something from Amnu-nation. I'd say she's lucky she got the bounty instead of the owner coming after her.
    wait 2 s
    say Anyway, if you're quick you might find her somewhere in the commercial district still.
    %at% 4043 %load% mob 4215
  else
    say You already did in that Aleria.
  end
end
if %speech% == level three
  if !%actor.varexists(bounty_completed_3)%
    say That would be an icer with a gambling problem. He owed some money to the types who expect you to pay up or else.
    wait 2 s
    say And guess what? It's come to the else. You'll find Azora somewhere in the Security Headquarters.
    wait 1 s
    say But I'm sure that you've got the skill to kill someone under the noses of the Security forces.
    %at% 4192 %load% mob 4216
  else
    say What are you on about? You run Azora through already.
  end
end
if %speech% == level four
  if !%actor.varexists(bounty_completed_4)%
    say There's this human who killed one of the guards... his brother, also a guard wants revenge.. only problem is this human and his gang are hiding out in the abandonned level.
    wait 1 s
    say And we all know the guards are too scared to go any further than a the checkpoint. You're gonna need to get a pass from the Chief of Security before you can get in too.
    wait 2 s
    say You're still a wet ear so be careful, this Damian Bloodhound is a ruthless character.
    %at% 4113 %load% mob 4217
    %at% 4113 %load% mob 4218
    %at% 4113 %load% mob 4218
  else
    say Look kid, you did well to kill that Bloodhound guy, but maybe you should get your head checked out. After all, he is DEAD. Of course I aint gonna have another bounty on him.
  end
end
if %speech% == level five
  if !%actor.varexists(bounty_completed_5)%
    say You're either crazy or real good. Either way this bounty is as tough as they will come. Some konatsu female thought she'd try and kill our fair king... and she almost did it.
    wait 2 s
    say There's not much I can tell you other than she was last sighted in the palace gardens and that she can go invisible. So unless you wanna get sent home in a body bag, you'd best bring your a game.
    %at% 4198 %load% mob 4219
  else
    say Man that assassin must of done a number on your head, she's dead remember? By your hands no less.
  end
end
~
#4201
Bounty hunter master greet~
0 g 100
~
wait 1 s
say Are you interested in collecting bounties? Of course you are, why else would you be here.
wait 2 s
say Just ask me what available bounties there are and I'll let you know.
~
#4202
Bounty hunter master list bounties~
0 d 100
available bounties~
if %speech% == available bounties
  wait 1 s
  say Good. We have 5 bounties going at the moment. They are ranked level one through to level 5, just ask about an individual one and I'll give you more information on it.
end
~
#4203
Bounty hunter master receive lvl 1~
0 j 100
~
if !%actor.varexists(bounty_completed_1)% && %object.vnum% == 4244
  set bounty_completed_1
  remote bounty_completed_1 %actor.id%
  %purge% %object.name%
  say Not bad %actor.name%. Here's you're reward of 1500 gold.
  nop %actor.gold(1500)%
elseif !%actor.varexists(bounty_completed_2)% && %object.vnum% == 4245
  set bounty_completed_2
  remote bounty_completed_2 %actor.id%
  %purge% %object%
  say So you killed a thief %actor.name% Don't let it get to your head, 3000 gold zenni is the reward.
  nop %actor.gold(3000)%
elseif !%actor.varexists(bounty_completed_3)% && %object.vnum% == 4246
  set bounty_completed_3
  remote bounty_completed_3 %actor.id%
  purge% %object%
  say So Azora is dead? Here's your reward of 7500 zenni.
  nop %actor.gold(7500)%
elseif !%actor.varexists(bounty_completed_4)% && %object.vnum% == 4247
  set bounty_completed_4
  remote bounty_completed_4 %actor.id%
  %purge% %object%
  say Well done %actor.name%. You did well bringing down that thug Damian and his crew. Your reward is 15000 zenni.
  nop %actor.gold(15000)%
elseif !%actor.varexists(bounty_completed_5)% && %object.vnum% == 4248
  set bounty_completed_5
  remote bounty_completed_5 %actor.id%
  %purge% %object%
  say I can't believe you took her down, %actor.name%. You've earned every bit of this 30000 zenni reward.
  nop %actor.gold(30000)%
else
  say I don't want that!
  drop %object.name%
end
~
#4204
undefined~
0 j 100
~
%echo% This trigger is not finished!
~
#4205
undefined~
0 j 100
~
%echo% This trigger is not finished!
~
#4206
undefined~
0 j 100
~
%echo% This trigger is not finished!
~
#4207
undefined~
0 j 100
~
* No Script
~
#4208
Johto head load~
0 f 100
~
%load% obj 4244
~
#4209
Aleria head load~
0 f 100
~
%load% obj 4245
~
#4210
Azora head lead~
0 f 100
~
%load% obj 4246
~
#4211
Damian head load~
0 f 100
~
%load% obj 4247
~
#4212
Assassin head load~
0 f 100
~
%load% obj 4248
~
#4213
Watch trigger~
1 c 100
watch~
if %arg% == time
  if %time.hour% == 12
    %send% %actor% @D _____@n
    %send% %actor% @D/            \
    %send% %actor% @D|  @R1.00@D          |@n
    %send% %actor% @D\_____/@n 
  end
end  
~
#4224
Dodoria guard~
0 q 100
~
if (%direction% == south && %actor.class% != Frieza)
 return 0
 wait 1
 say Get lost, you have no business with Lord Frieza.
* elseif (%actor.carry%)
* return 0
* wait 1
* say You may not carry anyone inside.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == south && %actor.class% == Frieza)
wait 1
 say Well hurry on in then, don't test Lord Frieza's patience.
%send% %actor% Dodoria says, 'Well hurry on in then, don't test Lord Frieza's patience.'
return 1
end
~
#4225
Dodoria greet~
0 g 100
~
if (%actor.class% == Frieza)
    if (%actor.level% <= 2)
        wait 1      
        emote sneers at %actor.name%, &1Well, look at what we have here. A new grunt who thinks that they're worth something?&2
        wait 2 s
        say I bet you can't even handle the Frozen Fist worms. If you manage to survive and get stronger, well then I just might have a task for you.
    
    elseif (%actor.level% >= 5 && %actor.level% <= 9 && !%actor.varexists(DodQ1Complete)% && !%actor.varexists(onDodQ1) && !%actor.varexists(DodQcomplete)%)
        wait 1
        emote chuckles briefly then sneers at %actor.name%, &1Well well. Seems like you're not as pethetic as I origionally thought.&2
        wait 2 s
        say Since you're still alive, and atleast a tiny bit stronger than you were before, you should go find a Frozen Fist Elite and get their Badge and bring it to me.
        set onDodQ1
        remote onDodQ1 %actor.id%
        
    elseif (%actor.level% >= 10 && %actor.level% <= 14 && !%actor.varexists(DodQ2Complete)% && !%actor.varexists(onDodQ2)% && !%actor.varexists(DodQcomplete)%)
        wait 1
        emote sneers at %actor.name%, &1Still living, are we? Well good, I have a new task for you.&2
        wait 2 s
        say Leave the Dojo, and go east and get into the Elevator. You are then to go to the second level, then head north to the intersection and then head east from there. Then head north into the caverns and follow it until you find some Bison, and bring back its pelt.
        set onDodQ2
        remote onDodQ2 %actor.id%
        
    elseif (%actor.level% >= 15 && %actor.level% <= 19 && !%actor.varexists(DodQ3Complete)% && !%actor.varexists(onDodQ3)% && !%actor.varexists(DodQcomplete)%)
        wait 1
        emote yells at %actor.name% as they arrive, &1You, newbie, I have a mission for you.&2
        wait 2 s
        say Lord Frieza wants to send out some troops to scout out some near by planets. You are to head to Earth and climb a mountain that I've heard is called Cherry Blossom Mountain. On this mountain you should be able to get a good overview of Earth's main city.
       wait 2 s
        say Bring back the skull of an animal up there to proove you've been there.
        set onDodQ3
        remote onDodQ3 %actor.id%
        
    elseif (%actor.level% >= 20 && !%actor.varexists(DodQ4Complete)% && !%actor.varexists(onDodQ4)% && !%actor.varexists(DodQcomplete)%)
        wait 1
        emote sneers at %actor.name%, &1Well, you live still. I have one more task for you.&2
        wait 2 s
        say Lord Frieza wants you to head to the Slave Market and rough them up a bit. They haven't been paying Lord Frieza what they should. Bring back the Payment and you will be recognised as a true student of Lord Frieza.
        wait 2 s
        say To get to the Slave Market, just head to the Ice Highway and follow it. You'll eventually come across it.
        set onDodQ4
        remote onDodQ4 %actor.id%     
        
    else
        if (!%actor.varexists(onDodQ1)% && !%actor.varexists(onDodQ2)% && !%actor.varexists(onDodQ3)% && !%actor.varexists(onDodQ4)%)
            wait 1
            say Lord Frieza is expecting you.
        end
    end 
else
    wait 1
    say Get lost before I make you.
end
~
#4226
Zarbon food~
0 g 100
~
if (!%actor.varexists(zarbonfood)%)
    wait 1
    say Hello there, one of Lord Frieza's new recruits? Well, we can't have you falling over on the job. Just ask me for some @Gfood@C.@n
else
   wait 1
   say Make Lord Frieza proud.
end
~
#4227
Frieza Greet~
0 g 100
~
if (%actor.level% <= 2)
wait 1
say Hello %actor.name%, you little worm. As you leave, Dodoria will give you some advice..
 if !%actor.varexists(friezaeq)%
  wait 2 sec
  say Now %actor.name% here is the official symbol of my servants.
wait 1 sec
  mload obj 4264
  give brand %actor.name%
  set friezaeq
  remote friezaeq %actor.id%
 end
elseif (%actor.level% < 5)
wait 1
say %actor.name% you are still a little maggot. Get stronger fast, if you know what is good for you.
 if %actor.varexists(friezaeq)%
  rdelete friezaeq %actor.id%
end
elseif (%actor.level% < 10)
wait 1
say %actor.name% you're still a weakling. If you don't keep up your training, I won't hestitate to kill you myself.
elseif (%actor.level% < 20)
wait 1
say Hmmph, %actor.name%, better. Though a worm like you won't get much stronger..
elseif (%actor.level% < 40)
wait 1
say You're still alive, %actor.name%? That's quite a surprise.
elseif (%actor.level% < 75)
wait 1
say %actor.name% you are impressive. I might have to send you on a mission you might not come back from..
elseif (%actor.level% < 100)
wait 1
say Congratulations, you are now amongst my elites! But %actor.name%, don't get any ideas about rising above your station.
elseif (%actor.level% == 100)
wait 1
say You have done well and are one of the best that has served me.
grin
else
wait 1
say %actor.name% I don't know what to say about you.
end
~
#4228
Ginyu Greet~
0 g 100
~
if (%actor.level% <= 2)
wait 1
say Hello %actor.name%, so you want to become a member of the Ginyu Force? Only the best initiates make it.
 if !%actor.varexists(ginyueq)%
  wait 2 sec
  say Now %actor.name% here is the official badge of the Ginyu Corps.
wait 1 sec
  mload obj 4265
  give badge %actor.name%
  set ginyueq
  remote ginyueq %actor.id%
 end
elseif (%actor.level% < 5)
wait 1
say No, no, no! %actor.name%, your technique is all wrong! You're going to have to work harder.
 if %actor.varexists(ginyueq)%
  rdelete ginyueq %actor.id%
end
elseif (%actor.level% < 10)
wait 1
say %actor.name%, only the best posers get into the Ginyu Force and you've got a long way to go.
elseif (%actor.level% < 20)
wait 1
say Hmmph, %actor.name%, better. But you must improve your technique.
elseif (%actor.level% < 40)
wait 1
say I'm surprised that you are still here, %actor.name%. Develop your poses!
elseif (%actor.level% < 75)
wait 1
say %actor.name%, you are almost good enough to join the Ginyu Force.
elseif (%actor.level% < 100)
wait 1
say %actor.name%, you are amazing! Show me your poses and I might let you into the Ginyu Force now!
elseif (%actor.level% == 100)
wait 1
say You have done well.
grin
wait 1
emote mutters something about swapping bodies.
else
wait 1
say %actor.name% I don't know what to say about you.
end
~
#4229
Ginyu Guard block~
0 q 100
~
if (%direction% == north && %actor.class% == Frieza)
  return 0
  wait 1
  say Hah, we don't want any of Frieza's weak students here!
elseif (%direction% == north && %actor.class% != Ginyu && %actor.class% != Frieza)
 return 0
 wait 1
 say Get lost, you have no business with the Ginyu Corps.
* elseif (%actor.carry%)
* return 0
* wait 1
* say You may not carry anyone inside.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == north && %actor.class% == Ginyu)
wait 1
 say Well hurry on in then, don't keep the Capt'n waiting.
%send% %actor% A Ginyu Guard says, 'Well hurry on in then, don't keep the Capt'n waiting.'
return 1
end
~
#4230
Ginyu Guard great~
0 g 100
~
if (%actor.class% == Ginyu)
    if (%actor.level% <= 2)
        wait 1
        say Hey newbie.
        wait 2 s
        say Unless you need something from the city, Captain Ginyu suggests all new recruits stay in the dojo for training.
        wait 2 s
        say When you get stronger, come to me and I'll point you in the right direction for further training.
    
    elseif (%actor.level% >= 5 && %actor.level% <= 9 && !%actor.varexists(GinyuQ1Complete)% && !%actor.varexists(onGinyuQ1) && !%actor.varexists(GinyuQcomplete)%)
        wait 1
        say Newbie, I see you've grown a bit stronger. I have a mission for you.
        wait 2 s
        say You are to head up to the Fourth level and head to Lord Frieza's dojo and find a @GFrozen Fist Elite@C, fight him, and return with his badge.@n
        wait 2 s
        say To get there, head west to the Elevator, then head up to the Fourth level, then head west again and you'll see Lord Frieza's dojo. You can't miss it.       
        set onGinyuQ1
        remote onGinyuQ1 %actor.id%
        
    elseif (%actor.level% >= 10 && %actor.level% <= 14 && !%actor.varexists(GinyuQ2Complete)% && !%actor.varexists(onGinyuQ2)% && !%actor.varexists(GinyuQcomplete)%)
        wait 1
        say Newbie, I see you've grown a bit stronger. Good, I have another mission for you.
        wait 2 s
        say You are to go west, north, east, then north again into the Caverns and then hunt some Bison in the fields past there. You are to return with some Bison Meat for Captain Ginyu's next meal.
        set onGinyuQ2
        remote onGinyuQ2 %actor.id%
        
    elseif (%actor.level% >= 15 && %actor.level% <= 19 && !%actor.varexists(GinyuQ3Complete)% && !%actor.varexists(onGinyuQ3)% && !%actor.varexists(GinyuQcomplete)%)
        wait 1
        say Newbie, I see you've grown even stronger. Good, I have yet another mission for you.
        wait 2 s
        say You are to head to a planet called Earth and hunt a Tyrannosaurus Rex. Captain Ginyu has expressed a desire for their scales. And no, I don't know why.
        wait 2 s
        say Just head up to the Fourth level and head to the spaceport on the southeast corner of the city, take it to the Spacestation and hop on a ship heading for Earth. From Earth's spaceport, head north of the city and climb a mountian called Cherry Blossom Mountain.
        set onGinyuQ3
        remote onGinyuQ3 %actor.id%
        
    elseif (%actor.level% >= 20 && !%actor.varexists(GinyuQ4Complete)% && !%actor.varexists(onGinyuQ4)% && !%actor.varexists(GinyuQcomplete)%)
        wait 1
        say Ah, Newbie. I see that you are yet again even stronger than before. I have one final mission for you.
        wait 2 s
        say You are to head to the Slave Market. They have not paid the Captain the protection money they owe. You are to ruff them up and bring back the money from one of the guards.
        wait 2 s
        say I cannot tell you directly how to get there, as you've reached a point where you need to figure this stuff out for yourself, but I can tell you it's on the Ice Highway. And becareful, those guards can put up a fight of their own.
        set onGinyuQ4
        remote onGinyuQ4 %actor.id%     
        
    else
        if (!%actor.varexists(onGinyuQ1)% && !%actor.varexists(onGinyuQ2)% && !%actor.varexists(onGinyuQ3)% && !%actor.varexists(onGinyuQ4)%)
            wait 1
            emote nods his head at %actor.name%
        end
    end 
else
    wait 1
    say Halt! You don't have permission to be inside.
end
~
#4231
Ginyu Supply Clerk Food Greet~
0 g 100
~
if (!%actor.varexists(ginyufood)%)
    wait 1
    say I'm under orders to supply new recruits with rations. Just ask for the @Gfood@C and I will get it ready.@n
else
   wait 1
   say Hope you enjoyed the food.
end
~
#4232
Jerodel Receptionist part 1~
0 q 100
~
if (!%actor.varexists(has_appointment)% && %direction% == southwest)
  return 0 
  say Sorry, you can't enter the clinic without an appointment.
  wait 2 s
  say If you want an appointment, say appointment.
elseif (%direction% == north || %direction% == southwest)
  rdelete has_appointment %actor.id%
end
~
#4233
Jerodel receptionist part 2~
0 d 100
appointment~
if !%actor.varexists(has_appointment)%
  wait 1 s
  say Ok, one moment %actor.name%.
  wait 2 s
  say It appears as if Dr Jerodel is free at the moment. You can go through.
  set has_appointment
  remote has_appointment %actor.id%
end
~
#4234
Dodoria Noob complete~
0 j 100
~
if (%actor.varexists(onDodQ1)% && %object.vnum% == 4270)
    %echoaround% %actor% %actor.name% gives Dodoria a Frozen Fist Badge.
    %purge% %object%
    wait 1
emote takes the badge and looks it over, &1Well, you're not totally incompetent after all. Good to know.&2
    wait 2 s
    emote throws a small bag at %actor.name%
    nop %actor.gold(500)%
    rdelete onDodQ1 %actor.id%
    set DodQ1Complete
    remote DodQ1Complete %actor.id%
    
elseif (%actor.varexists(onDodQ2)% && %object.vnum% == 4903)
    %echoaround% %actor% %actor.name% gives Dodoria a Bison Pelt.
    %purge% %object%
    wait 1
    emote takes the belt and looks it over, &1Good. Now that I got this maybe Zarbon will shut up about wanting a new fur coat...&2
    wait 2 s
    emote peers at %actor.name%, &1Still here are you? Well what do you want, a reward or something? Fine. Take these tickets. They can be used at the spaceport to take you to a space station that can get you to other planets.&2
    nop %actor.gold(500)%
    %load% obj 3999
    %load% obj 3999
    %load% obj 3999
    %load% obj 3999
    give all.ticket %actor.name%
    wait 1
    say To get to the spaceport, just take the elevator to the top level, and head to the south east corner. You can't miss it.
    rdelete onDodQ2 %actor.id%
    set DodQ2Complete
    remote DodQ2Complete %actor.id%
    
elseif (%actor.varexists(onDodQ3)% && %object.vnum% == 1203)
    %echoaround% %actor% %actor.name% gives Dodoria a T-Rex Skull.
    %purge% %object%
    wait 1
    emote takes the skull and sets it off to the side, &1You went? Good. Tell me all that you've learned.&2
    wait 2 s
    emote listens to what %actor.name% has to say, &1Good. This information should be useful. And take this as a reward.&2
    wait 1 s
    emote tosses a bag of zenni at %actor.name%.
    nop %actor.gold(2000)%
    rdelete onDodQ3 %actor.id%
    set DodQ3Complete
    remote DodQ3Complete %actor.id%
    
elseif (%actor.varexists(onDodQ4)% && %object.vnum% == 4807)
    %echoaround% %actor% %actor.name% gives Dodoria a large sack.
    %purge% %object%
    wait 1
    say Good, you brought the payment back.
    wait 2 s
    emote hands %actor.name% some Icerian armor with Frieza's symbol on the front, &1Take this, it's a sign that you are a true student of Lord Frieza.&2
    %load% obj 4271
    give armor %actor.name%
    rdelete onDodQ4 %actor.id%
    rdelete DodQ1Complete %actor.id%
    rdelete DodQ2Complete %actor.id%
    rdelete DodQ3Complete %actor.id%
    set DodQcomplete
    remote DodQcomplete %actor.id%
    
else
    return 0
    wait 1
    %echoaround% %actor% %actor.name% attempts to give Dodoria something.
    say I don't want this.
end
~
#4235
Ginyu Mutant Quest Receive~
0 j 100
~
if (%actor.varexists(onGinyuQ1)% && %object.vnum% == 4270)
    %echoaround% %actor% %actor.name% gives the Guard an Elite's Badge.
    %purge% %object%
    wait 1
    say Good. Take this as a reward. And return later for something else.
    nop %actor.gold(500)%
    rdelete onGinyuQ1 %actor.id%
    set GinyuQ1Complete
    remote GinyuQ1Complete %actor.id%
    
elseif (%actor.varexists(onGinyuQ2)% && %object.vnum% == 4904)
    %echoaround% %actor% %actor.name% gives the Guard some Bison Meat.
    %purge% %object%
    wait 1
    say Good. Captain Ginyu will enjoy this.
    wait 2 s
    say Take this zenni, and these tickets as well. They'll let you use the shuttles to take you to a Spacestation that has shuttles that take you all over the galaxy.
    nop %actor.gold(500)%
    %load% obj 3999
    %load% obj 3999
    %load% obj 3999
    %load% obj 3999
    give all.ticket %actor.name%
    wait 1
    say Oh and don't forget, the Spaceport is on the Fith level in the Southeast corner.
    rdelete onGinyuQ2 %actor.id%
    set GinyuQ2Complete
    remote GinyuQ2Complete %actor.id%
    
elseif (%actor.varexists(onGinyuQ3)% && %object.vnum% == 1205)
    %echoaround% %actor% %actor.name% gives the Guard some T-Rex scales.
    %purge% %object%
    wait 1
    say Ah, good job. Captain Ginyu will want these.
    wait 2 s
    emote tosses a bag of Zenni at %actor.name%
    nop %actor.gold(2000)%
    rdelete onGinyuQ3 %actor.id%
    set GinyuQ3Complete
    remote GinyuQ3Complete %actor.id%
    
elseif (%actor.varexists(onGinyuQ4)% && %object.vnum% == 4807)
    %echoaround% %actor% %actor.name% gives the Guard a large sack.
    %purge% %object%
    wait 1
    say Good job getting the protection money from the Slave Market. I hope it wasn't too much trouble.
    wait 2 s
    emote hands over some Icerian armor with Ginyu's symbol on the front to %actor.name%, &1Here, take this. It is a sign you are a true student of the Captain.&2
    %load% obj 4272
    give armor %actor.name%
    rdelete onGinyuQ4 %actor.id%
    rdelete GinyuQ1Complete %actor.id%
    rdelete GinyuQ2Complete %actor.id%
    rdelete GinyuQ3Complete %actor.id%
    set GinyuQcomplete
    remote GinyuQcomplete %actor.id%
    
else
    return 0
    wait 1
    %echoaround% %actor% %actor.name% attempts to give the Guard something.
    say I don't want this.
end
~
#4236
Zarbon Food Give~
0 d 100
food~
if (!%actor.varexists(zarbonfood1)% && !%actor.varexists(zarbonfood)%)
    wait 1 sec
    say I'll gather up some of my favorite things to eat, then.
    wait 3 sec
    emote runs around gathering up some stuff.
    wait 3 sec
    say Here, this should give you the energy you need to make Lord Frieza proud.
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 4269
    give all.cake %actor.name%
    give all.smoothie %actor.name%
    set zarbonfood1
    remote zarbonfood1 %actor.id%
elseif (!%actor.varexists(zarbonfood2)% && !%actor.varexists(zarbonfood)%)
    wait 1 sec
    say You like this as well? Not many people enjoy this.
    wait 3 sec
    emote gathers up some food and drinks.
    wait 3 sec
    emote smiles as he says, &1I'm so glad you enjoy it.&2
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 4269
    give all.cake %actor.name%
    give all.smoothie %actor.name%
    set zarbonfood2
    remote zarbonfood2 %actor.id%
elseif (!%actor.varexists(zarbonfood3)% && !%actor.varexists(zarbonfood)%)
    wait 1 sec
    say More it is! You know, I make these myself. It's my secret to keeping myself fabulous!
    wait 3 sec
    emote winks as he hands it over, &1Just don't tell anyone.&2
    wait 1 sec
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 4269
    give all.cake %actor.name%
    give all.smoothie %actor.name%
    rdelete zarbonfood1 %actor.id%
    rdelete zarbonfood2 %actor.id%
    set zarbonfood
    remote zarbonfood %actor.id%
else
    wait 1 sec
    say I can't give you anymore. Giving you everything for free would just make you weak. And we can't have any weaklings in Lord Frieza's army.
end
~
#4237
Gnyu Supply Clerk Food Give~
0 d 100
food~
if (!%actor.varexists(ginyufood1)% && !%actor.varexists(ginyufood)%)
    wait 1 sec
    say Yeah sure, right away.
    wait 2 sec
    emote digs around in a box and pulls out some food.
    wait 2 sec
    say Here, take it.
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 4232
    give all.cake %actor.name%
    give all.cup %actor.name%
    set ginyufood1
    remote ginyufood1 %actor.id%
elseif (!%actor.varexists(ginyufood2)% && !%actor.varexists(ginyufood)%)
    wait 1 sec
    say Some more, eh? Just don't eat too much.
    wait 2 sec
    emote digs around in a box and pulls out some food.
wait 2 sec
say Here, take it.
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 4232
    give all.cake %actor.name%
    give all.cup %actor.name%
    set ginyufood2
    remote ginyufood2 %actor.id%
elseif (!%actor.varexists(ginyufood3)% && !%actor.varexists(ginyufood)%)
    wait 1 sec
    say This is the last batch I can give out for free.
    wait 2 sec
emote digs around in a box and pulls out some food.
    wait 1 sec
say Here, take it. And remember, I can't give more.
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 4232
    give all.cake %actor.name%
    give all.cup %actor.name%
    rdelete ginyufood1 %actor.id%
    rdelete ginyufood2 %actor.id%
    set ginyufood
    remote ginyufood %actor.id%
else
    wait 1 sec
    say Orders are I can't give out anymore.
end
~
#4238
Dodoria Quest Checkers~
0 g 100
~
if (%actor.varexists(onDodQ1)%)
    wait 1
    say What, did you get lost, worm?
    wait 2 s
    say Just go get me a badge from a @GFrozen Fist Elite@C, who is in this building.@n
   
elseif (%actor.varexists(onDodQ2)%)
    wait 1
    say What, did you get lost, worm?
    wait 2 s
    say Just head out of here, head east and go into the elevator. Then go to the second floor, head north, then east again at the intersection. Then head north into the canverns and look around for some Bison and collect a pelt.
   
elseif (%actor.varexists(onDodQ3)%)
    wait 1
    say What, did you get lost, worm?
    wait 2 s
    say Head to the spaceport and travel to the Space station, then hitch a ride on a shuttle there to Earth. Head north of the city to Cherry Blossom Mountain and climb it, scout out the main city and the surrounding area from there and bring back a skull from an animal up there as proof you actually did your job.
   
elseif (%actor.varexists(onDodQ4)%)
    wait 1
    say What, did you get lost, worm?
    wait 2 s
    say Just head down to the Slave Market by following Ice Highway, and bring back the Payment they owe us.
   
end
~
#4239
Dodoria/Ginyu Q1 drop~
0 f 100
~
if (%actor.varexists(onDodQ1)% || %actor.varexists(onGinyuQ1)%)
    %load% obj 4270
end
~
#4240
Ginyu Quest checkers~
0 g 100
~
if (%actor.varexists(onGinyuQ1)%)
    wait 1
    say What are you doing back, newbie? Did you forget where to go?
    wait 2 s
    say Head to the Elevator to the west and head up to the Fourth Floor and then head West again and head into Lord Frieza's Dojo. In there you will find some Frozen Fist Elite's. Fight them and bring back their badge.
elseif (%actor.varexists(onGinyuQ2)%)
    wait 1
    say What are you doing back, newbie? Did you forget where to go?
    wait 2 s
    say Head west, north, east, and then north again and head into the Caverns. You should find some Bison either in there or in the field's outside. And don't forget the Bison Meat.
elseif (%actor.varexists(onGinyuQ3)%)
    wait 1
    say What are you doing back, newbie? Did you forget where to go?
    wait 2 s
    say Head to the spaceport on the Fith level, in the southeast corner of the city, and take a shuttle to the spaceport. From there head to Earth, and then go to Cherry Blossom Mountain. And don't forget the T-Rex Scales.
elseif (%actor.varexists(onGinyuQ4)%)
    wait 1
    say What are you doing back, newbie? Did you forget where to go?
    wait 2 s
    say Head to the Slave Market and bring back the protection money from a Guard there. The Slave Market is on the Ice Highway.
    
end
~
#4241
Zarbon Mute Food~
0 c 3
point~
if %arg% == food
    if (!%actor.varexists(zarbonfood1)% && !%actor.varexists(zarbonfood)%)
        wait 1 sec
        say I'll gather up some of my favorite things to eat, then.
        wait 3 sec
        emote runs around gathering up some stuff.
        wait 3 sec
        say Here, this should give you the energy you need to make Lord Frieza proud.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 4269
        give all.cake %actor.name%
        give all.smoothie %actor.name%
        set zarbonfood1
        remote zarbonfood1 %actor.id%
    elseif (!%actor.varexists(zarbonfood2)% && !%actor.varexists(zarbonfood)%)
        wait 1 sec
        say You like this as well? Not many people enjoy this.
        wait 3 sec
        emote gathers up some food and drinks.
        wait 3 sec
        emote smiles as he says, &1I'm so glad you enjoy it.&2
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 4269
        give all.cake %actor.name%
        give all.smoothie %actor.name%
        set zarbonfood2
        remote zarbonfood2 %actor.id%
    elseif (!%actor.varexists(zarbonfood3)% && !%actor.varexists(zarbonfood)%)
        wait 1 sec
        say More it is! You know, I make these myself. It's my secret to keeping myself fabulous!
        wait 3 sec
        emote winks as he hands it over, &1Just don't tell anyone.&2
        wait 1 sec
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 4269
        give all.cake %actor.name%
        give all.smoothie %actor.name%
        rdelete zarbonfood1 %actor.id%
        rdelete zarbonfood2 %actor.id%
        set zarbonfood
        remote zarbonfood %actor.id%
    else
        wait 1 sec
        say I can't give you anymore. Giving you everything for free would just make you weak. And we can't have any weaklings in Lord Frieza's army.
    end
else
    %send% %actor% Point at what?
end
~
#4242
Ginyu Supply Mute Food~
0 c 3
point~
if %arg% == food
    if (!%actor.varexists(ginyufood1)% && !%actor.varexists(ginyufood)%)
        wait 1 sec
        say Yeah sure, right away.
        wait 2 sec
        emote digs around in a box and pulls out some food.
        wait 2 sec
        say Here, take it.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 4232
        give all.cake %actor.name%
        give all.cup %actor.name%
        set ginyufood1
        remote ginyufood1 %actor.id%
    elseif (!%actor.varexists(ginyufood2)% && !%actor.varexists(ginyufood)%)
        wait 1 sec
        say Some more, eh? Just don't eat too much.
        wait 2 sec
        emote digs around in a box and pulls out some food.
        wait 2 sec
        say Here, take it.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 4232
        give all.cake %actor.name%
        give all.cup %actor.name%
        set ginyufood2
        remote ginyufood2 %actor.id%
    elseif (!%actor.varexists(ginyufood3)% && !%actor.varexists(ginyufood)%)
        wait 1 sec
        say This is the last batch I can give out for free.
        wait 2 sec
        emote digs around in a box and pulls out some food.
        wait 1 sec
        say Here, take it. And remember, I can't give more.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 4232
        give all.cake %actor.name%
        give all.cup %actor.name%
        rdelete ginyufood1 %actor.id%
        rdelete ginyufood2 %actor.id%
        set ginyufood
        remote ginyufood %actor.id%
    else
        wait 1 sec
        say Orders are I can't give out anymore.
    end
else
    %send% %actor% Point at what?
end
~
#4243
Friza Action Figure~
1 c 10
press~
if ("%arg%" != "button")
  wait 1s
  %send% %actor% @RTry pressing the button scum!@n
else
  %send% %actor% You press the button.
  %echoaround% %actor% %actor.name% presses the button on the @CF@Wriez@Ca @RAction@W Figure@n.
  wait 1s
  eval nuz %random.1000%
  if (%nuz% >= 900)
    wait 1s
    %send% %actor% The action figure shouts, '@CIm the strongest in the universe!@n'
    %echoaround% %actor% The action figure shouts, '@CIm the strongest in the universe!@n'
  elseif (%nuz% >= 750)
wait 2s
    %send% %actor% The action figure shouts '@CI'm so much smarter than Cooler!@n'
    %echoaround% %actor% The action figure shouts '@CI'm so much smarter than Cooler!@n'
  elseif (%nuz% >= 500)
    wait 2s
    %send% %actor% The action figure shouts, '@CDie, scum!@n'
    %echoaround% %actor% The action figure shouts, @CDie, scum!@n'
  elseif (%nuz% >= 250)
    wait 2s
    %send% %actor% The action figure shouts, '@CDie, scum!@n'
    %echoaround% %actor% The action figure shouts, @CDie, scum!@n'
  else
    wait 2s
    %send% %actor% The action figure shouts, '@CI will have my Immortality!@n'
    %echoaround% %actor% The action figure shouts, '@CI will have my Immortality!@n'
  end
end
~
$~
