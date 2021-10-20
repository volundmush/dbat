#15300
Water Elemental~
0 n 100
~
eval chance %random.100%
%teleport% all 21000
mgoto 15308
get ancient
junk ancient
if (%chance% >= 95)
 %load% o 15301
 %load% o 15302
 open chest
 put mace chest
 close chest
 mgoto 15308
else
 %load% o 15301
 %load% o 15303
 open chest
 put note chest
 close chest
 mgoto 15308
end
%at% 21000 %teleport% all 15308
~
#15301
Water Elemental 2~
0 f 100
~
%teleport% all 21000
mgoto 15308
drop chest
%at% 21000 %teleport% all 15308
%echo% As the elemental dies a rush of water shifts the sand and reveals an ancient marble chest!
~
#15302
Ice Elemental~
0 n 100
~
eval chance %random.100%
%teleport% all 21000
mgoto 15315
get chest
junk chest
if (%chance% >= 95)
 %load% o 15305
 %load% o 15304
 open chest
 put armor chest
 close chest
 mgoto 15315
else
 %load% o 15305
 %load% o 15303
 open chest
 put note chest
 close chest
 mgoto 15315
end
%at% 21000 %teleport% all 15315
~
#15303
Ice Elemental 2~
0 f 100
~
%teleport% all 21000
mgoto 15315
drop chest
%at% 21000 %teleport% all 15315
%echo% As the elemental dies off a column of ice melts away and reveals a chest!
~
#15310
Inscription Trigger~
2 c 100
inscribe~
if (!%actor.varexists(translate)%)
 wait 1
 %send% %actor% You copy down the inscription on a nearby tablet.
 %echoaround% %actor% %actor.name% copies down the inscription on the altar.
 %load% o 15310
 %force% %actor% get all
 set translate 0
 remote translate %actor.id%
else
 if (%actor.translate% == 1)
  %send% %actor% You have already copied down the inscription and had it translated for you.
 else
  %send% %actor% You can't seem to find a tablet to copy it onto.
 end
end
~
#15311
Sacrifice~
2 c 100
sacrifice~
if (!%actor.varexists(saevu)%)
 if (%actor.has_item(2009)% && %actor.has_item(11704)% && %actor.has_item(14016)% && %actor.has_item(12501)%)
  wait 1
  %send% %actor% You take out the four ores and place them upon the emerald altar.
  %echoaround% %actor% %actor.name% places four types of ore upon the emerald altar.
  wait 1 s
  %purge% %actor.inventory(2009)%
  %purge% %actor.inventory(11704)%
  %purge% %actor.inventory(14016)%
  %purge% %actor.inventory(12501)%
  %echo% Each ore disolves instantly into a pool of metal as it touches the surface of the altar.
  wait 4 s
  %echo% Green and blue energy begins to ebb from the altar.
  wait 4 s
  %echo% The pool of metal begins to absorb the energy and take the shape of an ancient kanassan rune!
  wait 4 s
  %send% %actor% The rune finishes taking shape and you remove it from the now silent altar.
  %echoaround% %actor% As the rune finishes taking shape %actor.name% removes it from the now silent altar.
  %load% o 15312
  %force% %actor% get rune
  set saevu
  remote saevu %actor.id%
 else
  wait 1
  %send% %actor% You get the feeling that you do not have the necessary things...
 end
else
 wait 1
 %send% %actor% You have already been blessed with a rune by Saevu's Altar.
end
~
#15312
Rozo Greet~
0 g 100
~
if (!%actor.varexists(metrozo)%)
 wait 1
 %echo% %self.name% looks around with a surly expression on his face.
 wait 2 s
 say Now what do you think you are doing?
 wait 2 s
 say Everything of value in this temple has already been claimed by the Guntar Brotherhood of Pirates. So you had best get lost as quickly as possible.
 set metrozo
 remote metrozo %actor.id%
else
 wait 1
 say Now didn't I tell you to get lost already?
end
~
#15313
Tuslo's Greet~
0 g 100
~
eval num %random.4%
eval numa %random.3%
wait %numa% s
switch (%num%)
 case 1
  %echo% %self.name% flips a page.
  break
 case 2
  %echo% %self.name% finishes reading one book and begins reading another.
  break
 case 3
  %echo% %self.name% stops reading to rub his eyes for a second and then continues reading.
  break
 default
  say Hmmm, yes, very interesting...
  break
done
~
#15314
Tuslo Receive~
0 j 100
~
if (!%actor.varexists(tuslo)%)
 if (%object.vnum% == 15310)
   wait 1
   %echo% %self.name% takes a look at %object.shortdesc%.
   wait 2 s
   say My word! This is a fabulous find. Where ever did you find it?
   wait 3 s
   say I see from these markings that you must have copied down this inscription fairly recently..
   wait 4 s
   say Hmm, well. I can translate it for you. I don't work for free though. The temple doesn't pay me very much...
   wait 4 s
   say Well due to the interesting nature of this find I won't charge you full price, but it will be close considering this is a second rate copy job you did.
   wait 4 s
   say Give me 60,000 zenni and I will translate it for you. I can assure you that no one else is capable.
   wait 1 s
   %echo% %self.name% grins greedily.
   give tablet %actor.name%
   drop tablet
   set tuslo 0
   remote tuslo %actor.id%
 else
   wait 1
   say I do not want this piece of junk. I'm only interested in things of kanassan historical value.
   give tablet %actor.name%
   drop tablet
 end
elseif (%actor.tuslo% == 0)
 if (%self.varexists(paid)%)
  wait 1
  say Alright, give me a moment.
  wait 1 s
  %echo% %self.name% examines %object.shortdesc% closely.
  wait 5 s
  say The first line says basically, 'To acquire blessing from great Saevu.'
  wait 4 s
  say The next line says, 'obtain four of the most rare ores and sacrifice.'
  wait 5 s
  say The last line says, 'moon, crystal, fangol, and sapphire at the Altar.'
  set tuslo 1
  remote tuslo %actor.id%
  mgoto 49
  junk tablet
  mgoto 15028
  wait 3 s
  say That's all. I'm going to keep this tablet for my own personal records. Even though it's no doubt pagan shrimp droppings. Saevu worship was overthrown by Aquisa centuries ago.
  rdelete paid %self.id%
 else
  wait 1
  say You still have to pay me the 60,000 zenni!
  give tablet %actor.name%
  drop tablet
 end
elseif (%actor.tuslo% == 1)
 wait 1
 say I have already translated this for you. I never work on the same project twice.
 give tablet %actor.name%
 drop tablet
end
~
#15315
Tuslo Paid~
0 m 1
~
if (!%actor.varexists(tuslo))
 return 0
 wait 1
 say I'm as much a fan of free money as anyone but I can't accept.
elseif (%actor.tuslo% == 0)
 if (%amount% == 60000)
  wait 1
  say Alright! ...I mean, good. Now I can translate your tablet with a mind free of financial worries.
  wait 2 s
  say Give me your tablet again.
  set paid
  remote paid %self.id%
  nop %self.gold(-60000)%
 elseif (%amount% > 60000)
  return 0
  wait 1
  say This is too much money.
 else
  return 0
  wait 1
  say This is not the amount we agreed upon.
 end
else
 return 0
 wait 1
 say I've already translated for you. I'm not so greedy that I would rob you of a fortune.
end
~
#15316
Pirate Random~
0 ab 15
~
eval num %random.4%
eval numa %random.3%
wait %numa% s
switch (%num%)
 case 1
  %echo% %self.name% shifts some rubble.
  break
 case 2
  say I found treasure!
  wait 1 s
  say No wait it's just a rock...
  break
 case 3
  %echo% %self.name% scratches his head.
  break
 default
  %echo% %self.name% inspects a crevice closely.
  break  
done
~
#15317
Search Rubble~
1 c 4
shift~
set rm %self.room.vnum%
eval pause %random.3% + 4
eval num %random.10%
wait 1
%send% %actor% You start to shift some of the rubble while looking for anything important.
%echoaround% %actor% %actor.name% starts to shift some of the rubble.
wait %pause% s
if (%actor.room.vnum% == %rm%)
 switch (%num%)
  case 1
   %send% %actor% You didn't seem to find anything this time.
   %echoaround% %actor% %actor.name% gives up %actor.hisher% search.
   break
  case 2
   %send% %actor% You find a silver rune! @D[@gEXP@W: @G+150@D]@n
   %echoaround% %actor% %actor.name% finds a silver rune.
   nop %actor.exp(150)%
   %load% o 15316
   %force% %actor% get rune
   break
  case 3
   %send% %actor% You pull out what appears to be something valuable but after examining it closely you see that it is not.
   %echoaround% %actor% %actor.name% gets excited about something they pull from the rubble before realizing that it's worthless.
   break
  case 4
   %send% %actor% You find a gold rune! @D[@gEXP@W: @G+200@D]@n
   %echoaround% %actor% %actor.name% finds a gold rune!
   nop %actor.exp(200)%
   %load% o 15317
   %force% %actor% get rune
   break
  default
   %send% %actor% You stop searching the rubble and were unable to find anything.
   %echoaround% %actor% %actor.name% stops searching the rubble.
   break
 done
else
 %send% %actor% You can't very well search the rubble in a different room...
end
~
#15318
Guard Redundancy~
0 q 100
~
return 0
~
$~
