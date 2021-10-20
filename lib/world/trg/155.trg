#15501
Heal script~
1 c 3
inject~
eval num1 5000
eval num2 %actor.level% 
eval numroll %random.10%
eval heal (%num1% * %num2%) * %numroll%
if (%actor.hitp% != %actor.maxhitp%)
  if (%actor.level% < 80)
   if (%heal% + %actor.hitp% > %actor.maxhitp%)
    if (%actor.hitp% > %actor.maxhitp%)
     eval diff %actor.hitp% - %actor.maxhitp%
     set %actor.hitp(-%diff%)%
    else
      eval diff %actor.maxhitp% - %actor.hitp%
      set %actor.hitp(%diff%)%
   end
   else
     set %actor.hitp(%heal%)%
   end
   else
     eval heal %heal% * 10
     set %actor.hitp(%heal%)%
   end
%send% %actor% You inject your self with the stim pack, and feel slightly better.
%echoaround% %actor% %actor.name% injects %actor.himher%self with a stim pack, and they look slightly better.
%send% %actor% [@m%heal%@n]
%purge% self
else
   %send% %actor% You are at full health!
end
~
#15502
undefined~
1 c 0
~
* No Script
~
#15503
Doctor1~
0 g 100
~
if (%actor.is_pc%)
   wait 1 s
   %echo% The doctor sighs, '@CWhat am I going to do with all these extra medical supplies...@n'
end
~
#15504
Doctor3~
0 m 1
~
if (!%actor.varexists(medhas)%)
      eval cost 35000
  if (%amount% == %cost%)
       wait 1 s
       say Alright, we had a deal. Wait here while I go get some supplies.
       wait 1 s
       %echo% The doctor runs out of the room for a few minutes, then returns pushing a large crate infront of him
       wait 1 s
       say Here we go, the supplies you needed. I'm sure I'll have more in the future to get rid of.
       %load% obj 15501
       give supplies %actor.name%
       wait 1 s
       %echo% The doctor heads off deeper into the hospital, mumbling something about shipments.
       set medhas
       remote medhas %actor.id%
       %purge% self
  elseif (%amount% > %cost%)
       return 0
       wait 1
       say I'd like to take to take this much, but I just couldn't....
  elseif (%amount% < %cost%)
       return 0
       wait 1 s
       say There isnt enough here.
  end
else
      wait 1 s
      say I don't have anymore supplies to spare, keep your money.
      give %amount% zenni %actor.name%
end
~
#15505
Doctor2~
0 d 100
Medical Supplies~
if (%actor.varexists(medhas)%)
   wait 1 s
   say I don't have any more supplies to give to you.
else
   wait 1 s
   %echo% The doctor looks over at %actor.name%, '@CWhat? You need some medical supplies? I do have some to spare, but I can't exactly give them away.@n'
  wait 1 s
  sigh
  wait 1 s
  say I suppose, if you gave me 35,000 zenni, I can spare some supplies.
end
~
#15506
Bottle Spin~
1 ac 4
spin~
set targ %random.char%
while %targ%
     eval sel %random.3%
     switch (%sel%)
     
    case 1
      %send% %actor% You spin the bottle.
      %echoaround% %actor% %actor.name% reaches out and spins the bottle. Where it stops, no one knows!
      wait 3 s
      %echo% The bottle lands on %targ.name%!
      unset targ
      break
    case 2
        %send% %actor% You spin the bottle.
        %echoaround% %actor% %actor.name% reaches out and spins the bottle. Where it stops, no one knows!
        wait 3 s
        %echo% The bottle lands on %targ.name%!
        unset targ
        break
        
    default
        %send% %actor% You spin the bottle.
        %echoaround% %actor% %actor.name% reaches out and spins the bottle. Where it stops, no one knows!
        wait 3 s
        %echo% The bottle fails to land on anyone.
        unset targ
     done
done
~
#15507
Emblem Give~
0 j 100
~
if (!%actor.varexists(kerinhelp)%)
  return 0
  wait 1
  say I don't need this.
 
else
  if (%object.vnum% != 15599)
  return 0
  wait 1 s
  say What do I want with this? Take it back.
 
 elseif (%actor.varexists(kerincomplete)%)
  return 1
  wait 1
  say I don't know where you got this, but I thank you for giving this to me. It will help to boost moral.
  rdelete kerinhelp %actor.id%
else
  wait 1
  %purge% %object.name%
 emote takes the emblem from %actor.name%, &1Y-you actually beat him?!&2 Kerin slowly turns the emblem over in his hands, then tosses it to a guard, &1Show the men! I'm sure this will be a much needed moral boost!&2
  wait 2 s
  %echo% The guard catches the emblem and then runs outside, seconds later cheering erupts around the camp.
wait 2 s
  emote stares after the guard, listening to the cheering with a huge smile on his face. Kerin looks back at %actor.name%, &1With this act, you have earned the trust of this camp. Go speak to the Armory Keeper. I think he would like a word with you.&2
  set kerincomplete
  remote kerincomplete %actor.id%
 end
end
~
#15508
Wish to help~
0 d 100
Help~
if (%actor.varexists(kerincomplete)%
   wait 1 s
   say I thank you for the offer, but there is nothing more you can help with right now.
elseif (%actor.varexists(kerinhelp)%)
   wait 1 s
   say Well what are you waiting for?! We won't last much longer!
else
   wait 1 s
   emote says, &1Really? You do?&2 He gestures %actor.name% over to the table &1The mayor is being held captive by a warlord in his office, here&2 he points to a location on the map, &1We don't know if he is still alive or not, but he is not your target. We need you to take out the Warlord thats leading this attack on our city. Return to me when your job is complete, and I will reward you.&2 
   wait 2 s
   emote nods to himself, then looks back at the map and starts issuing orders out.
   set kerinhelp
   remote kerinhelp %actor.id%
end
~
#15509
Kerin~
0 g 100
~
if (%actor.varexists(KerinHelp)%)
  if %actor.is_pc%
     wait 1 s
     emote looks up from a map of the city as %actor.name% enters his tent, &1Thanks to your help, we have hope for taking back this city!&2
  end
elseif (%actor.varexists(KerinComplete)%)
  wait 1 s
  emote looks up from a map of the city, &1Ah, %actor.name%, thanks again for your help. The effort to retake the city is doing better now.&2
else
  wait 1 s
  emote looks up from a map of the city as %actor.name% enters his tent, &1Unless you want to help, get out. I'm very busy right now.&2
end
~
#15510
Refugee ambient~
2 b 5
~
%echo% A fight breaks out somewhere in the tent, and some guards make their way to the location and break up the fight.
~
#15511
Supply run~
0 g 100
~
wait 1 s
emote looks over at %actor.name% as they enter the tent, &1Now is not a good time.&2
wait 3 s
say You're still here, eh? Well maybe you can help us out.
wait 2 s
emote takes a look at the tent and sighs, &1We're running out of supplies.&2 The medic looks back over at %actor.name%, &1If you want to be of some help, we could use some more supplies. But the only place to get them is from a hospital on Earth, and we can't spare anyone to go get them...&2
wait 2 s
emote sighs again, shaking his head, &1If you can go get us some supplies, I'll make it worth your time. Now please, I have work to do here.&2
~
#15512
Armory Greet~
0 g 100
~
if (%actor.varexists(kerincomplete)%)
   wait 1 s
   %echo% The armory keeper says, '@CAh, %actor.name%, Kerin told me you might be stopping by. I heard what you did for us. I'd like to offer you a little reward of my own, one of our swords. Now, even though you helped us out, I just can't go giving this away for free. We gotta fund this army some how, or else we would all starve!@n' The keeper laughs, '@CTake a look at the list for the pricing.@n'
else
  wait 1 s
  say What do you want? If it's nothing, get out of here! You're in the way.
end
~
#15513
Medic Supplies~
0 j 100
~
if (%object.vnum% != 15501)
     return 0
     say I don't want this.
 else
    %purge% medical
     wait 1 s
     say Ah! You got some medical supplies! These will be greatly needed.
     wait 2 s
     say Hrm, you can't go away empty handed...
     wait 2 s
     %echo% The medic opens the crate and rummages around in it.
     wait 1 s
     say Ah, here we go. Take this.
     %load% obj 15502
      give stim %actor.name%
      wait 2 s
      say If you ever run into more supplies, I'm sure we could use them.
      wait 1 s
      %echo% The medic starts rushing around the tent, giving out the supplies to those in need.
      rdelete medhas %actor.id%
end
~
#15514
Draknos sword buy~
0 c 100
bu~
if (%cmd% == bu || %cmd% == buy)
 if (%actor.varexists(Kerincomplete)% && !%actor.varexists(draknosswd)%)
  return 0
  wait 1
  if (%actor.has_item(15514)%)
   set draknosSwd
   remote draknosSwd %actor.id%
  end
  elseif (!%actor.varexists(kerincomplete)%)
   return 1
   wait 1
   say You have to earn my trust. Go check with the commander to see if you can help out in the fight in anyway.
 else
  return 1
  wait 1
  %send% %actor% The shopkeeper informs you that you are only allowed to purchase one. 
 end
end
~
#15515
Event Test~
0 m 100
~
eval eventobj 15510
eval saveroom 19195
eval eventpay 5000
eval perchar 3
eval eventcoutner 15508
if (%actor.varexists(HouseRaffle)% && %actor.HouseRaffle% < %perchar% && %amount% == %eventpay%)
   say Thank you for your entry! (OOC: Type "@Gticketnumber@C" to set a number to it, if you bought more than one you will have to put one you already set a number to in a bag)@n
   %at% 19195 %load% obj 15508
   %at% 19195 drop counter
   %load% obj %eventobj%
   give event %actor.name%
   eval HouseRaffle %actor.HouseRaffle% + 1
   remote HouseRaffle %actor.id%
elseif (!%actor.varexists(HouseRaffle)% && %amount% == %eventpay%)
   say Thank you for your entry! (OOC: Type "@Gticketnumber@C" to set a number to it, if you bought more than one you will have to put one you already set a number to in a bag)@n
   %at% 19195 %load% obj 15508
   %at% 19195 drop counter
   %load% obj %eventobj%
   give event %actor.name%
   eval HouseRaffle 1
   remote HouseRaffle %actor.id%
elseif (%actor.HouseRaffle% == %perchar%)
   say You have bought all you could! Sorry!
   return
elseif (%amount% < %eventpay%)
   say You didn't give me enough money!
   return
end
~
#15516
Child Found~
0 g 100
~
if (%actor.is_pc%)
   if (%actor.follower.vnum(15593)%)
    wait 2 s
    emote looks up as the pair enter.
    say L...Liara?! Liara is that you?!
    wait 2 s
    emote runs over to her daughter, embracing her in a big hug. They both start letting out loud sobs as they hold each other. After a few minutes pass, the mother looks back up at %actor.name%, &1Thank you! This means more then you know to me!&2 She returns to the cot to pick up some things and pulls a small bag out of a backpack, &1I know this isn't much, but please, take it.&2 she tosses the bag to %actor.name% and leads her daughter out of the tent.
    %send% %actor% You look inside the bag and see 1000 zenni.
    nop %actor.gold(1000)%
    set Childcomplete Complete
    remote Childcomplete %actor.id%
    %purge% Lost Child
    %purge% Woman
 else
   if (%actor.varexists(Childcomplete)%)
     wait 1 s
     say Thank you for finding my daughter! I'll never forget you, %actor.name%!
   else
     wait 1 s
     emote sobs into her hands.
     wait 2 s
     emote looks up at %actor.name% as %actor.heshe% enters the tent.
     wait 3 s
     say Please... Please... My little girl is missing. We got seperated while we were fleeing the city.
     wait 3 s
     emote puts her head back in her hands, as a new wave of tears rolls down her face, &1Please, find my little girl! She is all I have left... If anything happens to her, I'd...&2
     wait 3 s
     emote starts sobbing even harder.
     set sobstory
     remote sobstory %actor.id%
   end
 end
end
~
#15517
Hystaric Woman1~
0 g 100
~
if (!%actor.varexists(sobstory)%)
 emote sobs into her hands.
 wait 2 s
 emote looks up at %actor.name% as %actor.heshe% enters the tent.
 wait 1 s
 say Please... Please... My little girl is missing. We got seperated while we were fleeing the city.
 wait 2 s
 emote puts her head back in her hands, as a new wave of tears rolls down her face, &1 Please, find my little girl! She is  all I have left... If anything happens to her, I'd...&2
 wait 2 s
 emote starts sobbing even harder.
 set sobstory
 remote sobstory %actor.id%
else
detach 15517 %self.id%
end
~
#15518
Tablet Test~
1 c 3
itruth~
if %arg% == Earth
  %send% %actor% Please select an option:
  %send% %actor% Option 1) Popular Food and Beverages
  %send% %actor% Option 2) Health Services
  %send% %actor% Option 3) Tourist Hotspots
  %send% %actor% Option 4) Danger Zones
  %send% %actor% Option 5) Notable Martial Artists
  %send% %actor% Syntax: iMuffin <planet name> <option number>
  if (%arg% == 1)
    %send% %actor%  Earth - Popular Food and Beverage services
    %send% %actor% 1) McStuff Restaurant - Evergreen Street - Nexus City
    %send% %actor% 2) Taco Hell - Hercule Street - Satan City
  elseif (%arg% /= Earth && %arg% /= 2)
    %send% %actor% Earth - Health Services
    %send% %actor% 1) Satan City Hospital - Hospital Street - Satan City
    %send% %actor% 2) Ahh Spa - Akibura District - Nexus City
  elseif (%arg% == 3)
    %send% %actor% Earth - Tourist Hotspots
    %send% %actor% 1) Nexus Starport - Shale Boulevard - Nexus City
    %send% %actor% 2) Nexus City Theater - Shale Boulevard - Nexus City
    %send% %actor% 3) World Martial Arts Tournament - Currently being remodeled
    %send% %actor% 4) Satan City Zoo - Fauna Street - Satan City
  elseif (%arg% == 4)
    %send% %actor% Earth - Danger Zones
    %send% %actor% 1) Cherry Blossom Mountain. OOC - Levels 10-20
    %send% %actor% 2) Northern Plains. OOC - Level 20+
    %send% %actor% 3) Silver Mines, Lower Levels. OOC - Level 30+
  elseif (%arg% /= Earth && %arg% /= 5)
    %send% %actor% Earth - Notable Martial Artists - Rough Locations
    %send% %actor% 1) Master Roshi - South Sea
    %send% %actor% 2) Android Sixteen - Nexus Field
    %send% %actor% 3) Master Krane - Satan City
    %send% %actor% 4) Piccolo - Shadow Forest
  end
else
  %echoaround% %actor% %actor.name% taps their iMuffin screen a few times.
  %send% %actor% Please select an option:
  %send% %actor% Option 1) Earth
  %send% %actor% Option 2) Vegeta
  %send% %actor% Option 3) Namek
  %send% %actor% Option 4) Aether
  %send% %actor% Option 5) Konack
  %send% %actor% Option 6) Frigid
  %send% %actor% Option 7) Arlia
  %send% %actor% Option 8) Zenith
  %send% %actor% Option 9) Kanassa
  %send% %actor% Syntax: iMuffin <planet name>
end
~
#15519
Ticket Number Setter~
1 c 3
ticketnumber~
eval randnumber %random.100%
if (%self.cost% == 1)
   eval %self.shortdesc(%randnumber%)%
   eval %self.cost(2)%
   %send% %actor% Ticket number: %randnumber%
end
~
#15520
Makai Guard~
0 g 100
~
wait 1 s
grin
wait 1 s
say Another pathetic rebel attempting to get in?
wait 1 s
laugh
wait 1 s
say I'd like to see you try!
wait 1 s
punch %actor.name%
~
#15521
Ticket Seller Greeter~
0 g 100
~
wait 1 s
say Hey there! I'm selling tickets for a raffle, but only three per person! If you would like one, just give me 5000 zenni.
~
#15523
Empty~
1 c 100
~
* No Script
%echo% Script is unfinished.
~
#15530
Scared Makai death~
0 b 10
~
emote screams in fear as a ki ball comes flying through the gaps in a near by building, and hits him right in the chest.
%damage% %self% 50000000
~
#15531
Scared Makai1~
0 g 45
~
say Please! Don't hurt me!
~
#15532
Scared Makai2~
0 b 25
~
emote quickly darts out from behind some cover, before running behind something else.
~
#15544
Insane Makai Laugh~
0 b 25
~
emote looks around the area, letting off a slight laugh before firing an energy blast in a random direction.
~
#15550
Skill Restrict~
2 c 100
trip~
%send% %actor% You feel something restricting you from doing that...
~
#15551
Magic Eightball~
1 c 3
shake~
if (%cmd% == shake && %arg% /= eig)
  %send% %actor% You shake the Magic Eight-ball
  %echoaround% %actor% %actor.name% starts vigerously shaking a black ball.
  wait 2 s
  %send% %actor% You stop shaking the Eight-ball and stare at it for the answer.
  %echoaround% %actor% %actor.name% stops shaking the black ball and stares at it.
  eval eightball %random.20%
  switch (%eightball%)
    
    case 1
      %send% %actor% Letters start to appear, '@CI don't like you.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CI don't like you.@n'
    break
    
    case 2
      %send% %actor% Letters start to appear, '@CProbably yes.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CProbably yes.@n'
    break
    
    case 3
      %send% %actor% Letters start to appear, '@CNo way.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CNo way.@n'
    break
    
    case 4
      %send% %actor% Letters start to appear, '@CDefinite yes.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CDefinate yes.@n'
    break
    
    case 5
      %send% %actor% Letters start to appear, '@CMaybe tomorrow.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CMaybe tomorrow.@n'
    break
    
    case 6
      %send% %actor% Letters start to appear, '@CShut up will ya.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CShut up will ya.@n'
    break
    
    case 7
      %send% %actor% Letters start to appear, '@CIf you let me out of here, perhaps I'll tell you.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CIf you let me out of here, perhaps I'll tell you.@n'
    break
    
    case 8
      %send% %actor% Letters start to appear, '@CKill them all, they're plotting against you!@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CKill them all, they're plotting against you!@n'
    break
    
    case 9
      %send% %actor% Letters start to appear, '@CMy sources say, you should get a life.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CMy sources say, you should get a life.@n'
    break
    
    case 10
      eval ranrace %random.16%
      if (%ranrace% == 1)
        set racesay a Namek
      elseif (%ranrace% == 2)
        set racesay a Saiyan
      elseif (%ranrace% == 3)   
        set racesay a Half-breed
      elseif (%ranrace% == 4)
        set racesay an Android
      elseif (%ranrace% == 5)
        set racesay a Bioandroid
      elseif (%ranrace% == 6)
        set racesay a Majin
      elseif (%ranrace% == 7)
        set racesay a Hoshijin
      elseif (%ranrace% == 8)
        set racesay a Konatsu
      elseif (%ranrace% == 9)
        set racesay an Arlian
      elseif (%ranrace% == 10)
        set racesay a Tuffle
      elseif (%ranrace% == 11)
        set racesay a Kanassan
      elseif (%ranrace% == 12)
        set racesay an Icer
      elseif (%ranrace% == 13)
        set racesay a Kai
      elseif (%ranrace% == 14)
        set racesay a Mutant
      elseif (%ranrace% == 15)
        set racesay a Demon
      else
        set racesay Human
      end
      
      %send% %actor% Letters start to appear, 'Be warned, your demise will be brought on by... %racesay%!' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared,'Be warned, your demise will be brought on by... %racesay%!'
    break
    case 11
      %send% %actor% Letters start to appear, '@CYou don't want to know.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CYou don't want to know.@n'
    break
    
    case 12
      %send% %actor% Letters start to appear, '@CThe answer is written in the stars.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CThe answer is written in the stars.@n'
    break
    
    case 13
      %send% %actor% Letters start to appear, '@CYes, you do look good in that dress.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CYes, you do look good in that dress.@n'
    break
    
    case 14
      %send% %actor% Letters start to appear, '@CI'm getting dizzy...@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CI'm getting dizzy...@n'
    break
    
    case 15
      eval ranchar %random.char%
      if (%ranchar% == %actor%)
       %send% %actor% Letters start to appear, '@CThe voices are coming! Run!@n' You hide the answer before anyone can see.
       %echoaround% %actor% %actor.name% hides what the eight ball says.
      else
       %send% %actor% Letters start to appear, '@CShh, you didn't hear this from me, but...%ranchar.name%@C is out to get you!@n' You hide the answer before anyone can see.
       %echoaround% %actor% %actor.name% hides what the eight ball says.
      end
    break
    
    case 16
      %send% %actor% Letters start to appear, '@CThe person sitting across from you has the answer.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CThe person sitting across from you has the answer.@n'
    break
    
    case 17
      %send% %actor% Letters start to appear, '@CCould you speak up? I'm not wearing any pants.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CCould you speak up? I'm not wearing any pants.@n'
    break
    
    case 18
      %send% %actor% Letters start to appear, '@CThe answer you seek is: Pie. Pie solves everything.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CThe answer you seek is: Pie. Pie solves everything.@n'
    break
    
    case 19
      %send% %actor% Letters start to appear, '@CShhh, just be quiet. It will all be over soon.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CShhh, just be quiet. It will all be over soon.@n'
    break
    
    default
      %send% %actor% Letters start to appear, '@CTry again later.@n' You turn the eightball around so others can see.
      %echoaround% %actor% %actor.name% shows what appeared, '@CTry again later.@n'
    break
  done
else
  return 0
end
~
#15552
Bat Summon~
0 ab 0
~
eval num %random.4%
  switch (%num%)
   case 1
    %echo% A Black Bat lets out a small screach and swoops down at somebody.
   break
   case 2
    %echo% A Black Bat chases an insect around.
    break
   case 3
     %echo% A Black Bat lands on a perch and hangs there, staring at anything that moves. After a few seconds, it flies back up into the air.
   default
     %echo% The black bat lets out a really loud screach and another bat joins the first one.
     %load% mob 75
   break
  done
~
#15553
Zombie Summon~
0 ab 0
~
eval num %random.4%
  switch (%num%)
   case 1
    %echo% The zombie shuffles around, while mumbling "Brains."
   break
   case 2
     %echo% The zombie trips over its own feat and falls flat on its face. After a minute of struggling it gets back up.
   break
   case 3
     %echo% The zombie picks up an arm that was on the grown and takes a bite out of it, before throwing it back down.
   break
   default
     %echo% The zombie all of a sudden runs into a near by secluded place. A few seconds later screams are heard and two zombies emerge from the location.
     %load% mob 76
   break
  done
~
#15554
Warewolf Summon~
0 ab 0
~
eval num %random.4%
  switch (%num%)
   case 1
    %echo% A werewolf lets out a low and deep growl.
   break
   case 2
     %echo% The werewolf knocks over some stuff.
    break
   case 3
     %echo% The Werewolf lets off a tiny howl.
    break
   default
     %echo% The werewolf puts its hands down on the ground and arches its back as it lets out a long howl. A few seconds later, another werewolf runs into the area.
     %load% mob 77
   break
  done
~
#15555
Vampire Summon~
0 ab 20
~
eval num %random.4%
  switch (%num%)
   case 1
    %echo% A vampire licks their lips and peers around.
   break
   case 2
     %echo% A vampire starts coughing.
    break
   case 3
      %echo% A Vampire parts their lips slightly and lets out a small hiss.
     break
   default
     %echo% The vampire sniffs the air then runs off quickly. A few seconds later a scream is heard, and then two vampires emerge from the direction the first ran off in.
     %load% mob 78
   break
  done
~
#15556
Mummy Summon~
0 f 100
~
eval num %random.2%
  switch (%num%)
   case 1
    %echo% The corpse twitches for a few seconds before stopping.
   break
   default
     %echo% The corpse starts twitching as an eerie mist rolls into the area. The mist completly covers the corpse, and a few seconds later a 'Splat' is heard and a shape appears in the mist. The mist clears quickly.
     %load% mob 79
   break
  done
~
#15557
Zombie Revive~
0 f 100
~
eval num %random.2%
  switch (%num%)
   case 1
    %echo% The corpse twitches for a few seconds before finally stopping.
   break
   default
     %echo% The zombie lets out a moan, and stands back up and resumes its shuffling.
     %load% mob 76
   break
  done
~
#15579
Falling Debris~
2 g 45
~
if (%actor.is_pc%)
   wait 1 s
   %echo% An explosion rocks the building, and a piece of debris falls on %actor.name%'s head!
   set %actor.hitp(-50000)%
end
~
#15592
Lost Child2~
0 d 100
yes~
if (%actor.varexists(Childcomplete)%)
   wait 1 s
   emote sits on the ground, sobbing into her arms.
else
    wait 1 s
    smile
    wait 1 s
    say You really... Found her?!
    wait 1 s
    emote wipes tears from her eyes, &1Please... Can you take me to her?! I'm so scared...&2
    follow %actor.name%
end
~
#15593
Lost Child greet~
0 g 100
~
if (%actor.is_pc%)
  if (%actor.varexists(Childcomplete)%)
    wait 1 s
    emote sobs loudly, &1I miss my mommy...&2
       else
         emote looks up at %actor.name% as they enter.
         wait 2 s
         emote speaks to %actor.name% between sobs, &1Have...you...seen...my...mommy?&2
  end
end
~
#15595
Death Loot~
0 f 100
~
%load% obj 15599
set warlorddead
remote warlorddead %actor.id%
~
#15596
Exit Block~
2 q 100
~
if (%direction% == north && %actor.varexists(warlorddead)%)
  return 0
  wait 1
  %send% %actor% That room is destroyed! You can't go in there!
elseif (%direction% == north)
  return 1
  wait 1
  %echo% %actor.name% moves rubble out of %actor.hisher% way, and continues to the north.
else
  return 1  
end
~
#15597
new trigger~
0 d 100
Boop~
%echo% The shadows swirl around %self.name%, and heals %self.name% a bit.
eval healz %self.hitp% + (%self.maxhitp% / 50)
set %self.hitp(-70)%
%echo% Current HP: %self.hitp% Healing: @M%healz%@n Max: %self.maxhitp%
set %self.hitp(%healz%)%
%echo% Current HP: %self.hitp% Healing: @M%healz%@n Max: %self.maxhitp%
~
#15598
undefined~
0 f 100
~
* No Script
~
#15599
Kiel and Warlord~
0 g 100
~
if (%actor.is_pc%)
   wait 2 s
   emote laughs at the kai cowering before him, &1We Makai will rule this world and there is nothing you can do to stop it!&2
   wait 2 s
   %force% Kiel emote cowers before the makai, &1Please... Don't hurt me!&2
   wait 2 s
   emote lets out a roaring laugh, then kicks Kiel in the face, sending him crashing into a wall.
   %damage% Kiel 2500000
   wait 2 s
   emote looks over at %actor.name% and a big, sinnister grin spreads across his face, &1Enjoy the show? How about you be next?!&2
   wait 2 s
   punch %actor.name
end
~
$~
