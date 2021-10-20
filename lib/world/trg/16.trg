#1600
Piccolo Greet~
0 g 100
~
if (%actor.level% <= 2)
wait 1
say You are following me now %actor.name%. If you need any help getting started speak with Gohan. He's the small child just outside this clearing. Right now I consider him a better warrior than you, so you had best listen to what advice he has for getting started.
 if !%actor.varexists(piccoloeq)%
  wait 2 sec
  say Now %actor.name% here is the official symbol of my students, wear it with pride even though you may not deserve it yet.
  wait 1 sec
  mload obj 1602
  give symbol %actor.name%
  set piccoloeq
  remote piccoloeq %actor.id%
  wait 2 s
  say Also, if you need some food I will give you some... But don't rely too much on my kindness, I am not going to baby you. Just say 'feed me'.
 end
elseif (%actor.level% < 5)
wait 1
say %actor.name% I see you are not so pathetic now, but you are still weaker than Gohan. You better stay dedicated to your training.
 if %actor.varexists(piccoloeq)%
  rdelete piccoloeq %actor.id%
end
elseif (%actor.level% < 10)
wait 1
say %actor.name% I am no longer embarrassed by your skills, but you still need a lot more training.
elseif (%actor.level% < 20)
wait 1
say %actor.name% I have a small bit of pride for you, but not a lot.
elseif (%actor.level% < 40)
wait 1
say %actor.name% I am proud that you have followed my teachings, you are a strong warrior now.
elseif (%actor.level% < 75)
wait 1
say %actor.name% you are one of my strongest students, never cease your training.
elseif (%actor.level% < 100)
wait 1
say %actor.name% you are nearly beyond my abilities to teach. I hate to admit it...
elseif (%actor.level% == 100)
wait 1
say %actor.name% what are you doing here still? You are a first class warrior, you should be out on your own.
%send% %actor% Piccolo crosses his arms and scowls at you.
%echoaround% %actor% Piccolo crosses his arms and scowls at %actor.name%.
else
wait 1
say %actor.name% I don't know what to say about you except LEAVE!
end
~
#1601
Gohan Guard~
0 q 100
~
if (%direction% == west && %actor.class% != Piccolo)
 return 0
 wait 1
 say You may not go on..Mr. Piccolo has told me to only let the few worthy students he has through.
 wait 3 sec
 say I'm not trying to be mean or anything...
* elseif (%actor.carry%)
* return 0
* wait 1
* say You may not carry anyone inside.
* elseif (%actor.drag%)
* return 0
* wait 1
* say You may not drag anyone inside.
elseif (%direction% == west && %actor.class% == Piccolo)
 wait 1
 say Welcome in, Mr. Piccolo is waiting for you %actor.name%
 %send% %actor% %self.name% @wsays, '@CWelcome in, Mr. Piccolo is waiting for you %actor.name%@w'@n
end
~
#1602
Gohan Greet~
0 g 100
~
if (%actor.class% == Piccolo)
  if (%actor.level% <= 2)
    wait 1
    emote walks over to %actor.name%, &1Oh hi there! Are you training under Mister Piccolo too?&2
    wait 2 s
    emote lights up and a big smile crosses his face, &1I've been training under him for a while now. He's not as bad as he seems.&2
    wait 2 s
    say In fact, he told me to tell you where you can get your first bit of training. And that would be Rosewater Park in Nexus.
    wait 2 s
    emote points east, &1Just go east, then north, then east, then north again, then east one more time to get to Nexus City. Then head south until you get to @GEvergreen Street@C and @GEagle Street.@n&2
    wait 1 s
    say From there head east until you can't anymore, then go south until you see a sign. Then just go east form there, and you're there!
    wait 2 s
    emote chuckles, &1It's easier than it sounds, trust me. If you're @Glost@C, just let me know and I'll tell you how to get there again.@n&2
    
  elseif (%actor.level% >= 5 && %actor.level% <= 9 && !%actor.varexists(GohanQ1Complete)% && !%actor.varexists(onGohanQ1) && !%actor.varexists(GohanQComplete)%)
    wait 1
    emote walks over to %actor.name%, &1Wow, you're getting a lot stronger! Maybe someday you'll be as strong as my dad or Mister Piccolo!&2
    wait 2 s
    emote laughs, &1My dad is the strongest in the universe! But enough about my dad, maybe you can help me out with something. An animal in the woods has run off with my basket of food, but I can't stop my training to find it. I didn't get a good look at    wait 2 s
    say They might be too strong for you, so you might need some more training. You could try training in the @GHeaven's Gate Dojo@C until you can protect yourself from the animals in the woods.@n
    wait 2 s
    say You may have passed it if you went to Rosewater Park. It's on @GEagle Street@C, a bit east of the intersection, and west of the intersection of Terah and Eagle.@n
    wait 2 s
    emote rubs his stomach, &1I hope you can find my food soon.&2
    set onGohanQ1
    remote onGohanQ1 %actor.id%
    
  elseif (%actor.level% >= 10 && %actor.level% <= 14 && !%actor.varexists(GohanQ2Complete)% && !%actor.varexists(onGohanQ2)% && !%actor.varexists(GohanQComplete)%)
    wait 1
    emote gestures for %actor.name% to come over to him, &1Hey there! You seem to be growing stronger all the time. I'm glad your training is working out for you!&2
    wait 2 s
    say Oh, and before I forget. I have an idea of where you can train, and help me out as well! The @GNorthern Plains@C. I've heard from Krillin that the meat from the Buffalo up there is really tasty, so I'd like you to get me some.@n
    wait 2 s
    emote pauses to think, &1To get there, you have to go into Nexus and head north, through the @GNexus Plains@C, climb up @GCherry Blossom Mountain@C, and then cross the @GSandy Desert@n. If you've meditated for @G30@C minutes and @Gfocused@C your mind,    wait 2 s
    say The creatures on the way can be sort of aggressive, so being able to fly can be really useful. I've heard people use it to @Gfly@C into @Gspace@C to get around on a planet, though I've never tried it.@n
    wait 2 s
    %echo% %self.name%'s stomach lets off a loud growl, '@CAh, I'm getting hungry just thinking about that @GBuffalo meat@C. Remember, it's in the @GNorthern Plains@C to the north of Nexus and Cherry Blossom Mountain!@n'
    set onGohanQ2
    remote onGohanQ2 %actor.id%
    
  elseif (%actor.level% >= 15 && %actor.level% <= 19 && !%actor.varexists(GohanQ3Complete)% && !%actor.varexists(onGohanQ3)% && !%actor.varexists(GohanQComplete)%)
    wait 1
    emote walks over to %actor.name%, &1Wow, you've gotten so much stronger than you were when I first met you!&2
    wait 2 s
    say I have something that I'd like you to do. When I was younger Mister Piccolo had me train in the wilds, and I used to eat the tail of a Tyrannosaurus Rex's that lived there. It's been so long since then, and I'd like a tail again.
    wait 2 s
    emote points to the north east, &1You may have seen some if you walked to the Northern Plains, but some live up on @GCherry Blossom Mountain@C. I'll give you something in return if you manage to get me a tail.@n&2
    set onGohanQ3
    remote onGohanQ3 %actor.id%
    
  elseif (%actor.level% >= 20 && !%actor.varexists(GohanQ4Complete)% && !%actor.varexists(onGohanQ4)% && !%actor.varexists(GohanQComplete)%)
    wait 1
    emote waves over %actor.name%, &1I'm amazed at how fast you've been progressing!&2
    wait 2 s
    emote grins, &1I have one more location for you to try out. It's in @GWest City@C, called the @GSilver Mine@C. I've heard from Krillin that some Silver Worms there sometimes swallow diamonds.@n&2
    wait 3 s
    say I'm not going to tell you exactly how to get there, as you'll need to figure things out on your own.
    wait 2 s
    emote smiles, &1Remember, goto the Silver Mines in West City and get a diamond from the Silver Worms. And the Foreman there might have some work for you if you need some cash.&2
    set onGohanQ4
    remote onGohanQ4 %actor.id%     
    
  else
    if (!%actor.varexists(onGohanQ1)% && !%actor.varexists(onGohanQ2)% && !%actor.varexists(onGohanQ3)% && !%actor.varexists(onGohanQ4)%)
      wait 1
      say Hi, %actor.name%! I hope you get as strong as my dad some day!
    end
  end 
else
  wait 1
  say Hi there. I'm sorry but Mister Piccolo doesn't want to be disturbed right now.
end
~
#1603
Piccolo Free Food~
0 d 100
Feed Me~
if (!%actor.varexists(piccolofood1)% && !%actor.varexists(piccolofood)%)
  wait 1 sec
  say Alright, I'll give you some food.
  wait 3 sec
  emote points up into the air and conjures up some food, &1Just don't expect this treatment all the time.&2
  %load% obj 1601
  %load% obj 1601
  %load% obj 1601
  %load% obj 1601
  %load% obj 1614
  give all.apple %actor.name%
  give all.water %actor.name%
  set piccolofood1
  remote piccolofood1 %actor.id%
elseif (!%actor.varexists(piccolofood2)% && !%actor.varexists(piccolofood)%)
  wait 1 sec
  emote a small vein appears on his forehead, &1Want more do you? I can't believe I'm training a wimp like you.&2
  wait 3 sec
  emote points up into the air and conjures up some food, &1You should really think about figuring out how to feed yourself.&2
  %load% obj 1601
  %load% obj 1601
  %load% obj 1601
  %load% obj 1601
  %load% obj 1614
  give all.apple %actor.name%
  give all.water %actor.name%
  set piccolofood2
  remote piccolofood2 %actor.id%
elseif (!%actor.varexists(piccolofood3)% && !%actor.varexists(piccolofood)%)
  wait 1 sec
  emote growls, &1If you want to be a baby about it, fine! More food for you!&2
  wait 3 sec
  emote points up into the air and conjures up some food, &1No more after this! I'm done babying you!&2
  %load% obj 1601
  %load% obj 1601
  %load% obj 1601
  %load% obj 1601
  %load% obj 1614
  give all.apple %actor.name%
  give all.water %actor.name%
  rdelete piccolofood1 %actor.id%
  rdelete piccolofood2 %actor.id%
  set piccolofood
  remote piccolofood %actor.id%
else
  wait 1 sec
  say No! You're on your own now.
end
~
#1604
Haunted Cabin Ghost~
2 b 10
~
wait 30 s
eval num %random.4%
switch %num%
case 1
wait 10 s
%echo% A chill goes through the room as the air gets colder.
wait 5 s
%echo% A knocking sound can faintly be heard starting up out of nowhere.
wait 6 s
%echo% An old herb knife floats up off a shelf and towards empty air.
wait 5 s
%echo% The knife starts chopping in mid air above marks on the floor where a table used to be.
wait 6 s
%echo% Suddenly the chopping stops as a faint bluish outline of an old woman appears holding the knife.
wait 4 s
%echo% The woman slowly turns towards you.
wait 3 s
%echo% She looks into your eyes and says, "Get out...." before turning back and continuing her chopping.
wait 5 s
%echo% The woman slowly disappears and the knife floats back to one of the shelves.
wait 6 s
%echo% The knife is laid to rest on the shelf again.
wait 10 s
%echo% Everything is errily quiet...
break
case 2
wait 10 s
%echo% A jar on one of the shelves begins to rattle violently!
wait 8 s
%echo% The jar rattles louder and louder before falling onto the floor and shattering.
wait 5 s
%echo% A bluish white form of an old woman appears next to the shattered jar holding a broom.
wait 8 s
%echo% The woman slowly begins to sweep the shattered glass into a pile.
wait 8 s
%echo% As every piece joins the pile the woman looks up at you.
wait 6 s
%echo% "You are not welcome, this is MY house...." The woman says.
wait 6 s
%echo% The pile of shattered jar reforms into a solid, yet dirty, jar as the woman slowly disappears.
wait 8 s
%Echo% The jar slowly floats back and rests on the shelf again.
break
case 3
wait 10 s
%echo% A bluish white form of an old woman begins to form bent over in front of the decrepit fireplace.
wait 8 s
%echo% The woman appears to be stoking a fire that is no longer there.
wait 6 s
%echo% Suddenly a bright fire forms in the fireplace, burning bright and strong.
wait 8 s
%echo% The cabin begins to feel warm and toasty.
wait 6 s
%echo% The woman looks up from the fire at you and scowls.
wait 5 s
%echo% "I didn't invite you..." the woman says before disappearing slowly.
wait 6 s
%echo% After she has completely faded the roaring fire disappears as if it had never been there.
wait 8 s
%echo% The room is again cold and lifeless.
break
default
wait 10 s
%echo% Footsteps can faintly be heard moving around the cabin.
wait 8 s
%echo% The footsteps fade into nothingness, leaving an eerie silence.
break
done
~
#1605
Knife Grab~
2 c 100
grab~
if (%arg% == knife)
detach 1604 %self.id%
wait 1
%echoaround% %actor% %actor.name% slowly reaches to a knife on the bottom most shelf and grabs it.
%send% %actor% You reach down and grab the herb knife on the bottom most shelf.
wait 3 s
%echo% Suddenly a bluish white form of an old woman appears out of nowhere.
wait 5 s
%echo% "DON'T TOUCH!" She shouts loud enough to shake the whole cabin.
wait 4 s
%echoaround% %actor% %actor.name% is suddenly thrown down and lets go of the knife, letting it rattle to the floor.
%send% %actor% You are suddenly thrown down and let go of the knife, letting it rattle to the floor.
%force% %actor% sit
wait 5 s
%echo% The woman disappears and shortly after the knife floats back to its resting place.
attach 1604 %self.id%
else
return 0
end
~
#1606
Hunter's Greet~
0 g 100
~
wait 2 s
%echo% The hunter looks up from a pelt he is admiring.
wait 1 s
say Ah hello there.
wait 3 s
say You looking to buy some pelts?
wait 3 s
say Well I am not selling them even if you are, I like to collect them personally, but I do give some out occasionaly.
wait 5 s
say I give them to friends sometimes.
wait 3 s
say But ya know, I might be willing to part with this here deer pelt.
wait 5 s
%echo% The hunter holds up the pelt, showing off its obvious quality.
wait 3 s
say I won't sell it, but if you can get me a stack of firewood I will trade it to you.
wait 4 s
say I have a friend who usually gives me all the firewood I want, but he has fallen ill lately.
wait 5 s
say He might let you borrow his axe if you ask him. I am too busy myself to do it.
wait 4 s
say He lives to the southeast of here in the forest.
set findaxe
remote findaxe %actor.id%
~
#1607
Hunter Receive~
0 j 100
~
if (%object.vnum% == 1603)
detach 1606 %self.id%
%purge% firewood
wait 1 s
say Ah, thanks for the firewood.
wait 3 s
say Well in return have this deer pelt. It is more than worth it.
mload obj 1604
give pelt %actor.name%
wait 4 s
say You might even say I was ripping myself off, but I need this firewood something terrible.
wait 3 s
%echo% The hunter takes some of the firewood and places it in the fireplace.
wait 2 s
%echo% The hunter lights a fire with a zippo lighter and gently stokes it.
attach 1606 %self.id%
else
detach 1606 %self.id%
wait 1 s
say I don't want this...
drop %object.name%
attach 1606 %self.id%
end
~
#1608
Lumberjack Greet~
0 g 100
~
if !%actor.varexists(findaxe)%
wait 1 s
%echo% The lumberjack coughs loudly.
wait 3 s
say Oh man, please leave... I feel like I am dying.
wait 4 s
say You will probably get sick if you stick around...
wait 2 s
%echo% The lumberjack rolls over and pulls his covers up higher.
else
wait 1 s
%echo% The lumberjack coughs loudly.
wait 3 s
say Oh man, please leave... I feel like I am dying.
wait 2 s
say What is that? My friend told you I might lend you my axe?
wait 4 s
say Well I guess I could... but first I want you to do something for me
wait 4 s
say I know, I know. Big surprise, you come for something and are told to bring something.
wait 6 s
say But I am miserable right now, and what I want could make me better
wait 6 s
say I want a pearl shard. Not a pearl as in from shellfish, it looks like that but...
wait 6 s
say It is said to have that ability to heal sickness if swallowed.
wait 4 s
say I don't know where exactly you can find one, but I heard sometimes they could be found falling from the sky on the northern plains.
wait 8 s
say Well give me one and you can borrow my axe.
rdelete findaxe %actor.id%
end
~
#1609
Lumberjack Receive~
0 j 100
~
if (%object.vnum% == 1452)
%purge% %object%
detach 1608 %self.id%
wait 1 s
say Oh man, I can't believe you found one.
wait 3 s
say Well I am a man of my word. Here you go.
wait 1
mload obj 1605
give axe %actor.name%
wait 2 s
say Now this is what you need to do to make a nice bundle of firewood.
wait 4 s
say Find a pine tree in the forest, and @Gchop tree@w
wait 3 s
say If you ruin the bundle you will have to find another tree to try it on, since you are not an expert like I am, just keep at it.
wait 6 s
say Now back to my precious pearl.
wait 2 s
%echo% The lumberjack swallows the pearl shard.
wait 1 s
%echo% The lumberjack feels slightly better.
wait 4 s
say Oh man...I hope I wasn't lied to... I barely feel better at all.
wait 2 s
say Oh, by the way just leave that axe wherever you finish your bundle, I hopefully will be able to get out there and start cutting again soon
attach 1608 %self.id%
else
wait 1 s
detach 1608 %self.id%
say What can I do with this? Oh well, if you don't want it, I guess I will take it.
wait 1 s
smile
end
~
#1610
Axe Chop~
1 c 2
chop~
if (%arg% == tree)
if !%self.room.contents(1606)%
wait 1
%send% %actor% You are not near the right tree.
else
eval num %random.6%
wait 1
%send% %actor% You begin to chop at the tree.
%echoaround% %actor% %actor.name% begins chopping the Pine Tree.
switch %num%
  case 1
   wait 3 s
   %send% %actor% You whack the tree with the axe, tearing bits and pieces loose.
   %echoaround% %actor% %actor.name% whacks the tree with an axe, tearing bits and pieces loose.
   wait 5 s
   %send% %actor% You continue chopping the tree, cutting a large wedge out of its trunk.
   %echoaround% %actor% %actor.name% continues chopping the tree, cutting a large wedge out of its trunk.
   wait 6 s
   %send% %actor% You stand back as the tree begins to fall to the ground.
   %echoaround% %actor% %actor.name% steps back as the tree begins falling to the ground.
   wait 3 s
   %echo% The tree slams into the ground.
   wait 5 s
   %send% %actor% You begin chopping some of the tree into logs, and adding them to a bundle.
   %echoaround% %actor% %actor.name% begins chopping some of the tree into logs, and adding them to a bundle.
   wait 8 s
   %send% %actor% You have made a bundle of firewood! You leave the axe with the felled tree.
   %echoaround% %actor% %actor.name% finishes a bundle of firewood.
   %load% obj 1603
   %purge% %self.room.contents(1606)%
   %purge% self
   break
  case 2
   wait 1 s
   %send% %actor% You take a swing at the tree and your axe buries its head almost through the whole trunk.
   %echoaround% %actor% %actor.name% swings an axe at the Pine Tree and the axe head is almost buried through the whole trunk.
   wait 5 s
   %send% %actor% You realise this tree is rotten and no good, and stop chopping...
   %echoaround% %actor% %actor.name% stops cutting the tree because it is rotten.
   %purge% %self.room.contents(1606)%
   %load% obj 1607
   break
  default
     wait 3 s
     %send% %actor% You swing the axe at the tree but miss.... oops.
     %echoaround% %actor% %actor.name% swings an axe at the Pine Tree, but misses.
     wait 5 s
     %send% %actor% This time you swing at the tree with more force and aiming, and get the axe stuck in it.
     %echoaround% %actor% %actor.name% manages to get the axe stuck in the tree this time.
     wait 3 s
     %send% %actor% You struggle to free the axe.
     %echoaround% %actor% %actor.name% struggles to free the axe.
     wait 8 s
     %send% %actor% You free the axe, but have failed to cut down the tree.
     %echoaround% %actor% %actor.name% manages to free the axe.
     break
done
end
else
wait 1
%send% %actor% You want to chop what?
end
~
#1611
Jim Vaus Greet~
0 g 100
~
wait 2 s
%echo% Jim the farmer looks up from a notebook he is scribling in.
wait 3 s
say Ah, are you responding to the help wanted ad I put in the Nexus Chronicle?
wait 4 s
say Well either way you may be interested in the job, I need someone to help me harvest some of my turnips.
wait 6 s
say I recently lost one of my farmhands, and so I am willing to pay for some temp work.
wait 6 s
say I will pay you 25 zenni for every turnip you harvest and put in the silo.
wait 5 s
say To harvest, go to one of the unharvested sections of a turnip row and '@Gharvest turnip@C'
wait 6 s
say Then when you are done harvesting the turnips go into the silo and '@Gdeposit turnip@C'
wait 6 s
say After that come to me and '@Greport@C'. I will then pay you based on the number of turnips.
wait 6 s
say If you find any bad turnips just throw them away, they will ruin the rest of the silo otherwise.
~
#1612
Harvest Turnip~
2 c 100
harvest~
if %arg% != turnip
wait 1
%send% %actor% Harvest what?
else
if !%actor.varexists(vauspaid)%
 if !%self.contents(1609)% 
  wait 1
  %send% %actor% All the mature turnips have been harvested from this room.
  else
  wait 1 s
  %send% %actor% You start to harvest a turnip.
  %echoaround% %actor% %actor.name% starts to harvest a turnip.
  wait 2 s
  eval num %random.5%
  switch %num%
   case 1
   case 2
   case 3
    %echoaround% %actor% %actor.name% harvests a healthy turnip!
    %send% %actor% You harvest a healthy turnip
    %purge% rimvog
    if (!%actor.varexists(turnipc)%)
     set turnipc 1
     remote turnipc %actor.id%
    else
     eval count %actor.turnipc% + 1
     rdelete turnipc %actor.id%
     set turnipc %count%
     remote turnipc %actor.id%
    end
   break
   case 4
    %echoaround% %actor% %actor.name% harvests a bad turnip.
    %send% %actor% You harvest a bad turnip.
    %purge% rimvog
   break
   case 5
    %echoaround% %actor% %actor.name% harvests a turnip that appears to have been eaten by a burrowing animal.
    %send% %actor% You harvest a turnip that appears to have been half eaten by burrowing animals.
    %purge% rimvog
   break
  done
 end
 else
 wait 1
 %send% %actor% You are not being paid to do that, so why bother?
 end
end
~
#1613
Deposit Turnip~
2 c 100
deposit~
if %arg% == turnip
 wait 1
 if %actor.varexists(depositc)%
  if %actor.varexists(turnipc)%
   %send% %actor% You drop a turnip into the silo.
   %echoaround% %actor% %actor.name% drops a turnip into the silo.
   eval count %actor.turnipc% - 1
   rdelete turnipc %actor.id%
   set turnipc %count%
   remote turnipc %actor.id%
   eval depc %actor.depositc% + 1
   rdelete depositc %actor.id%
   set depositc %depc%
   remote depositc %actor.id%
   if %actor.turnipc% <= 0
       rdelete turnipc %actor.id%
   end
  else
   %send% %actor% You do not have a turnip to deposit.
  end
 else
  if %actor.varexists(turnipc)%
   %send% %actor% You drop a turnip into the silo.
   %echoaround% %actor% %actor.name% drops a turnip into the silo.
   eval count %actor.turnipc% - 1
   rdelete turnipc %actor.id%
   set turnipc %count%
   remote turnipc %actor.id%
   set depositc 1
   remote depositc %actor.id%
   if %actor.turnipc% <= 0
       rdelete turnipc %actor.id%
   end
  else
   %send% %actor% You do not have a turnip to deposit.
  end
 end
end
~
#1614
Report~
0 c 100
report~
detach 1611 %self.id%
if !%actor.varexists(vauspaid)%
 if !%actor.varexists(depositc)%
  wait 1
  %echoaround% %actor% %actor.name% whispers to %self.name%
  %send% %actor% you whisper to %self.name% how many turnips you have deposited.
  wait 1 s
  say %actor.name%, you want paid for nothing!? I think not...
  attach 1611 %self.id%
 else
  wait 1
  %echoaround% %actor% %actor.name% whispers to %self.name%
  %send% %actor% you whisper to %self.name% how many turnips you have deposited.
  wait 1 s
  eval money %actor.depositc% * 25
  say %actor.name%, here you go. You have earned %money% zenni.
  wait 2
  %echo% %self.name% hands %actor.name% %money% zenni.
set %actor.gold(%money%)%
  rdelete depositc %actor.id%
  set vauspaid
  remote vauspaid %actor.id%
  wait 3 s
  say Well now I am going to truck those turnips to Nexus. Good-bye.
  wait 2 s
  %echo% %self.name% leaves, and a truck is heard driving away shortly after.
  %purge% self
 end
else
wait 1
say %actor.name%, you have already worked for me and been paid.
attach 1611 %self.id%
end
~
#1615
Vaus Load~
0 n 100
~
mgoto 1674
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
s
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
s
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
s
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
s
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
e
wait 1
e
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
n
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
n
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
n
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
n
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
e
wait 1
e
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
s
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
s
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
s
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
s
wait 1 s
%echo% %self.name% checks the turnips carefully.
 if !%self.room.contents(1609)%
  %load% obj 1609
 end
wait 1
mgoto 1672
~
#1616
Already Chopping~
1 c 3
chop~
wait 1
%send% %actor% You are already chopping down a tree!
~
#1617
This is just for a variable~
1 g 100
~
%echo% This trigger commandlist is not complete!
~
#1618
Gohan Guard~
0 q 100
~
if (%direction% == west && %actor.class% != Piccolo)
return 0
end
~
#1619
Lumberjack Anti-Jackass Trigger~
0 b 100
~
if (%self.room.vnum% != 1667)
 mgoto 1667
end
~
#1620
Gohan Newbie Quest Complete~
0 j 100
~
if (%actor.varexists(onGohanQ1)% && %object.vnum% == 1615)
    %echoaround% %actor% %actor.name% gives Gohan a large basket.
    %purge% %object%
    wait 1
    emote takes the basket with a large smile on his face, &1Thanks! I hope the animals didn't actually get into it.&2
    wait 2 s
    emote starts to open the basket and take out food but stops.
    wait 2 s
    say Oh! I should reward you for helping me.
    wait 1 s
    emote hands %actor.name% a small bag of zenni, &1It's not much, but it's all I have on me right now. I might have something else for you to do if you get stronger.&2
    nop %actor.gold(500)%
    rdelete onGohanQ1 %actor.id%
    set GohanQ1Complete
    remote GohanQ1Complete %actor.id%
    
elseif (%actor.varexists(onGohanQ2)% && %object.vnum% == 1410)
    %echoaround% %actor% %actor.name% gives Gohan some Buffalo Meat.
    %purge% %object%
    wait 1
    emote takes the meat, stomach growling, &1Ah! You got it.&2 He runs off to a campfire he seems to have set up just for this and returns with a pouch a few seconds later.
    wait 2 s
    say I got this while you were getting it for me. They're tickets for the spaceships that fly out of the @GNexus Spaceport@C. They can take you to a space station where I've heard has ships landing on it from all sorts of other planets.@n
    wait 1 s
    say And I have some more zenni for you too.
    nop %actor.gold(500)%
    %load% obj 3999
    %load% obj 3999
    %load% obj 3999
    %load% obj 3999
    give all.ticket %actor.name%
    wait 1
    say Oh yeah, the spaceport is on @GShale Boulevard@C. And stop by later! I might have something else for you@n
    rdelete onGohanQ2 %actor.id%
    set GohanQ2Complete
    remote GohanQ2Complete %actor.id%
    
elseif (%actor.varexists(onGohanQ3)% && %object.vnum% == 1204)
    %echoaround% %actor% %actor.name% gives Krillin a T-Rex tail.
    %purge% %object%
    wait 1
    emote takes the tail, &1Ooo, you got it for me! Thanks! It's been a long time since I had this.&2
    wait 2 s
    emote hands over a large bag of Zenni to %actor.name%, &1Here take this. I managed to save up some more for you this time.&2a
    nop %actor.gold(2000)%
    rdelete onGohanQ3 %actor.id%
    set GohanQ3Complete
    remote GohanQ3Complete %actor.id%
    
elseif (%actor.varexists(onGohanQ4)% && %object.vnum% == 2013)
    %echoaround% %actor% %actor.name% gives Gohan a diamond.
    %purge% %object%
    wait 1
    emote takes the Diamond from %actor.name% and stares at it, &1Thanks! I hope my mom likes it.&2
    wait 3 s
    emote puts the diamond away and hands %actor.name% a purple gi with Piccolo's symbol on the front, &1Here, you can have this. It's my backup Gi thats a replica of what Mister Piccolo wears.&2
    %load% obj 1616
    give gi %actor.name%
    rdelete onGohanQ4 %actor.id%
    rdelete kq1complete %actor.id%
    rdelete GohanQ2Complete %actor.id%
    rdelete GohanQ3Complete %actor.id%
    set GohanQComplete
    remote GohanQComplete %actor.id%
    
else
    return 0
    wait 1
    %echoaround% %actor% %actor.name% attempts to give Gohan something.
    say I don't want this.
end
~
#1621
Gohan Quest1 Mob Drop~
0 f 100
~
if (%actor.varexists(onGohanq1)%)
    %load% obj 1615
end
~
#1622
Gohan Quest Checkers~
0 g 100
~
if (%actor.varexists(onGohanQ1)%)
  wait 1
  say Have you found my basket of food yet?
  wait 2 s
  say If you haven't please hurry. If you're not strong enough to defend yourself in the woods, remember you can train at the @GHeaven's Gate Dojo@C on @GEagle Street@C.@n
elseif (%actor.varexists(onGohanQ2)%)
  wait 1
  say You're back? Did you forget where to go?
  wait 2 s
  emote points north, &1Remember, go north of @GNexus@C to @GCherry Blossom Mountain@C and follow the path through it, into the @GDesert@C while continuing north and then you are there. And don't forget the Buffalo Meat!@n&2
elseif (%actor.varexists(onGohanQ3)%)
  wait 1
  say You're back? Did you forget where to go?
  wait 2 s
  emote points north, &1Just head north and climb @GCherry Blossom Mountain@C.@n&2
elseif (%actor.varexists(onGohanQ4)%)
  wait 1
  say You're back? Did you forget where to go?
  wait 2 s
  say Remember, the Silver Mine is in West City.
end
~
#1623
Piccolo Mute Food~
0 c 3
point~
if %arg% == food
    if (!%actor.varexists(piccolofood1)% && !%actor.varexists(piccolofood)%)
        wait 1 sec
        say Alright, I'll give you some food.
        wait 3 sec
        emote points up into the air and conjures up some food, &1Just don't expect this treatment all the time.&2
        %load% obj 1601
        %load% obj 1601
        %load% obj 1601
        %load% obj 1601
        %load% obj 1614
        give all.apple %actor.name%
        give all.water %actor.name%
        set piccolofood1
        remote piccolofood1 %actor.id%
    elseif (!%actor.varexists(piccolofood2)% && !%actor.varexists(piccolofood)%)
        wait 1 sec
        emote a small vein appears on his forehead, &1Want more do you? I can't believe I'm training a wimp like you.&2
        wait 3 sec
        emote points up into the air and conjures up some food, &1You should really think about figuring out how to feed yourself.&2
        %load% obj 1601
        %load% obj 1601
        %load% obj 1601
        %load% obj 1601
        %load% obj 1614
        give all.apple %actor.name%
        give all.water %actor.name%
        set piccolofood2
        remote piccolofood2 %actor.id%
    elseif (!%actor.varexists(piccolofood3)% && !%actor.varexists(piccolofood)%)
        wait 1 sec
        emote growls, &1If you want to be a baby about it, fine! More food for you!&2
        wait 3 sec
        emote points up into the air and conjures up some food, &1No more after this! I'm done babying you!&2
        %load% obj 1601
        %load% obj 1601
        %load% obj 1601
        %load% obj 1601
        %load% obj 1614
        give all.apple %actor.name%
        give all.water %actor.name%
        rdelete piccolofood1 %actor.id%
        rdelete piccolofood2 %actor.id%
        set piccolofood
        remote piccolofood %actor.id%
    else
        wait 1 sec
        say No! You're on your own now.
    end
else
    %send% %actor% Point at what?
end
~
#1624
Dead Campfire Clear~
1 c 4
clean~
%send% %actor% You clear away the dead fire.
%echoaround% %actor% %actor.name% clears away the dead fire.
%purge% self
~
$~
