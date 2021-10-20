#6300
Tsei load~
0 n 100
~
wait 1 s
sit throne
~
#6301
First Maxim Greet~
0 g 100
~
wait 2 s
say I am the first of five Maxim, to see the reaper, knowledge you must possess.
wait 3 s
say The unholy Book of Deception you must read, to pass our tests.
wait 3 s
say Complete the core verses if you wish to progress.
wait 3 s
say Truth is a cage that confines us
~
#6302
First Maxim answer~
0 d 100
Deceit~
if (%speech% == Deceit is the key that unlocks the cage)
  wait 2 s
  say Knowing the first verse is only the beginning, but you have passed my test.
  wait 2 s
  say A deal is a deal, so now you progress.
  wait 1 s
  %send% %actor% The crackling energy from the runes on the massive gate envelop you.
  %echoaround% %actor% %actor.name% is enveloped by purple energy.
  wait 1 s
  %teleport% %actor% 6382
  %send% %actor% The energy dissipates and you realise you are in a new location.
else
  wait 1 s
  say You must speak it correctly or I cannot allow you to pass.
end
~
#6303
Second Maxim Greet~
0 g 100
~
wait 2 s
say I am the second of five Maxim, to see the reaper, knowledge you must possess.
wait 3 s
say The unholy Book of Deception you must read, to pass our tests.
wait 3 s
say Complete the core verses if you wish to progress.
wait 3 s
say Without conflict
~
#6304
Second Maxim Answer~
0 d 100
we~
if (%speech% == we grow weak)
  wait 2 s
  say You know the second verse, some wisdom you do possess.
  wait 2 s
  say A deal is a deal, so now you progress.
  wait 1 s
  %send% %actor% The crackling energy from the runes on the massive gate envelop you.
  %echoaround% %actor% %actor.name% is enveloped by purple energy.
  wait 1 s
  %teleport% %actor% 6385
  %send% %actor% The energy dissipates and you realise you are in a new location.
else
  wait 1 s
  say You must speak it correctly or I cannot allow you to pass.
end
~
#6305
Third Maxim Greet~
0 g 100
~
wait 2 s
say I am the third of five Maxim, to see the reaper, knowledge you must possess.
wait 3 s
say The unholy Book of Deception you must read, to pass our tests.
wait 3 s
say Complete the core verses if you wish to progress.
wait 3 s
say Nothing else matters
~
#6306
Third Maxim answer~
0 d 100
if~
if (%speech% == if you achieve victory)
  wait 2 s
  say You know the third verse, some wickedness you do possess.
  wait 2 s
  say A deal is a deal, so now you progress.
  wait 1 s
  %send% %actor% The crackling energy from the runes on the massive gate envelop you.
  %echoaround% %actor% %actor.name% is enveloped by purple energy.
  wait 1 s
  %teleport% %actor% 6388
  %send% %actor% The energy dissipates and you realise you are in a new location.
else
  wait 1 s
  say You must speak it correctly or I cannot allow you to pass.
end
~
#6307
Fourth Maxim Greet~
0 g 100
~
wait 2 s
say I am the fourth of five Maxim, to see the reaper, knowledge you must possess.
wait 3 s
say The unholy Book of Deception you must read, to pass our tests.
wait 3 s
say Complete the core verses if you wish to progress.
wait 3 s
say Faith is their shield
~
#6308
Fourth maxim answer~
0 d 100
treachery~
if (%speech% == Treachery is our weapon)
  wait 2 s
  say You know the fourth verse, some wisdom you do possess.
  wait 2 s
  say A deal is a deal, so now you progress.
  wait 1 s
  %send% %actor% The crackling energy from the runes on the massive gate envelop you.
  %echoaround% %actor% %actor.name% is enveloped by purple energy.
  wait 1 s
  %teleport% %actor% 6391
  %send% %actor% The energy dissipates and you realise you are in a new location.
else
  wait 1 s
  say You must speak it correctly or I cannot allow you to pass.
end
~
#6309
Fifth Maxim Greet~
0 g 100
~
wait 2 s
say I am the final Maxim, to see the reaper, knowledge you must possess.
wait 3 s
say The unholy Book of Deception you must read, to pass our tests.
wait 3 s
say Complete the core verses if you wish to progress.
wait 3 s
say Good and evil are foolish notions of the weak
~
#6310
Fifth Maxim answer~
0 d 100
we~
if (%speech% == we are strong, we do as we please)
  wait 2 s
  say You know the final verse, much wisdom you do possess.
  wait 2 s
  say A deal is a deal, so now you progress.
  wait 1 s
  %send% %actor% The crackling energy from the runes on the massive gate envelop you.
  %echoaround% %actor% %actor.name% is enveloped by purple energy.
  wait 1 s
  %teleport% %actor% 6398
  %send% %actor% The energy dissipates and you realise you are in a new location.
else
  wait 1 s
  say You must speak it correctly or I cannot allow you to pass.
end
~
#6315
Reaper Greet~
0 g 100
~
if (%actor.align% > 0)
 wait 1 s
 say Ah what brings a goody goody like yourself to me?
 wait 3 s
 say Your soul is wretched enough. Go away.
 wait 1 s
 %send% %actor% %self.name% holds up a hands and a swirl of flames teleports you!
 %teleport% %actor% 6349
 wait 1
 %force% %actor% look
elseif (%actor.inventory(6301)%)
 wait 1 s
 say You have brought the skull, good. Give it to me.
else
 wait 1 s
 say Welcome, %actor.name%, I was expecting you.
 wait 3 s
 %echo% %self.name% takes a deep breath and sighs with escatsy.
 wait 3 s
 say I can taste the wretchedness of your soul tainted by your deeds.
 wait 4 s
 say So you wish to return to the living do you?
 wait 3 s
 say You have proven yourself to me concerning your worth.
 wait 4 s
 say However I require something more of you for revival.
 wait 4 s
 say Bring me the skull of that worm King Kai's pet monkey.
 wait 3 s
 say It shall make a great addition to my throne.
 wait 3 s
 if (%actor.room.vnum% == %self.room.vnum%)
  %send% %actor% %self.name% holds up a hands and a swirl of flames teleports you!
  %teleport% %actor% 6349
 end
 wait 1
 %force% %actor% look
end
~
#6316
Reaper Receive~
0 j 100
~
if (%object.vnum% == 6301)
 eval num %actor.death%
 if (%num% <= %ctime%)
  %purge% %object%
  %echo% %self.name% takes Bubble's skull and fuses it with his throne.
  wait 2 s
  say Good work.
  wait 1 s
  say I will now send you back to the world of the living to rain your vengeance upon those who killed you!
  wait 3 s
  raise %actor.name%
 elseif (%num% > %ctime%)
  return 0
  wait 1
  say You have proven yourself worthy of returning to life %actor.name%, however...
  wait 2 s
  %echo% %self.name% looks at his book.
  wait 2 s
  eval coun %num% - %ctime%
  eval count %coun% / 3600
say I can't send you back just yet. You will need to wait about %count% hours (In Real Life)
  wait 4 s
  say Sorry it has to be that way or else the universe would descend into chaos (And death would be a slap on the wrist).
%echo% @D(@ROOC@D: @RIf you wish to return to hell, give me any item that is not the skull. It will stay in your inventory.@D)@n
  wait 1
 end
else
return 0
 wait 1
 say This is not something I want.
 wait 1 s
 %send% %actor% %self.name% holds up a hands and a swirl of flames teleports you!
 %teleport% %actor% 6349
 wait 1
 %force% %actor% look
end
~
#6317
First Maxim Receive~
0 j 100
~
if (%object.vnum% != 6413)
%echo% %self.name% doesn't seem to be interested in that item.
return 0
else
wait 1 s
%echo% %self.name% grasps the goblet and swallows the contents greedily.
wait 2 s
say The offering pleases, I shall allow you to continue forward.
wait 2 s
%send% %actor% The crackling energy from the runes on the massive gate envelop you.
%echoaround% %actor% %actor.name% is enveloped by purple energy.
wait 1 s
%teleport% %actor% 6382
%send% %actor% The energy dissipates and you realise you are in a new location.
end
~
#6318
Second Maxim Receive~
0 j 100
~
if (%object.vnum% != 6413)
%echo% %self.name% doesn't seem to be interested in that item.
return 0
else
wait 1 s
%echo% %self.name% grasps the goblet and swallows the contents greedily.
wait 2 s
say The offering pleases, I shall allow you to continue forward.
wait 2 s
%send% %actor% The crackling energy from the runes on the massive gate envelop you.
%echoaround% %actor% %actor.name% is enveloped by purple energy.
wait 1 s
%teleport% %actor% 6385
%send% %actor% The energy dissipates and you realise you are in a new location.
end
~
#6319
Third Maxim receive~
0 j 100
~
if (%object.vnum% != 6413)
%echo% %self.name% doesn't seem to be interested in that item.
return 0
else
wait 1 s
%echo% %self.name% grasps the goblet and swallows the contents greedily.
wait 2 s
say The offering pleases, I shall allow you to continue forward.
wait 2 s
%send% %actor% The crackling energy from the runes on the massive gate envelop you.
%echoaround% %actor% %actor.name% is enveloped by purple energy.
wait 1 s
%teleport% %actor% 6388
%send% %actor% The energy dissipates and you realise you are in a new location.
end
~
#6320
Fourth Maxim receive~
0 j 100
~
if (%object.vnum% != 6413)
%echo% %self.name% doesn't seem to be interested in that item.
return 0
else
wait 1 s
%echo% %self.name% grasps the goblet and swallows the contents greedily.
wait 2 s
say The offering pleases, I shall allow you to continue forward.
wait 2 s
%send% %actor% The crackling energy from the runes on the massive gate envelop you.
%echoaround% %actor% %actor.name% is enveloped by purple energy.
wait 1 s
%teleport% %actor% 6391
%send% %actor% The energy dissipates and you realise you are in a new location.
end
~
#6321
Fifth Maxim Receive~
0 j 100
~
if (%object.vnum% != 6413)
%echo% %self.name% doesn't seem to be interested in that item.
return 0
else
wait 1 s
%echo% %self.name% grasps the goblet and swallows the contents greedily.
wait 2 s
say The offering pleases, I shall allow you to continue forward.
wait 2 s
%send% %actor% The crackling energy from the runes on the massive gate envelop you.
%echoaround% %actor% %actor.name% is enveloped by purple energy.
wait 1 s
%teleport% %actor% 6398
%send% %actor% The energy dissipates and you realise you are in a new location.
end
~
#6322
Prophet 1~
0 g 100
~
if !%actor.varexists(evil_dedras)%
say Sooon, soon the great and mighty Samael will return and unleash the End Times upon us!
wait 2 s
say Skies shall rain blood, oceans will boil and brother will fight brother!
wait 2 s
say There is no escaping the inevitable. Your soul is his no matter how much you run!
wait 2 s
say But our Lord can be appeased.. with sacrifice.
wait 2 s
say Bring me the heart of Dedras and he shall reward you.
else
wait 2 s
say You have served our Lord well! Go, spread the word, the End Times cometh.
end
~
#6323
Prophet 2~
0 j 100
~
if !%actor.varexists(evil_dedras)%
if (%object.vnum% == 7305 && %actor.align% > 0)
wait 1 s
say It is good that you brought the heart..
wait 2 s
say But there is no evil in your soul.. so my Lord will not help you.
wait 2 s
give heart %actor.name%
elseif (%object.vnum% == 7305 && %actor.align% < 0)
wait 1 s
%purge% %object%
say Well done child.. you have served your Lord well. For your reward...
%load% obj 89
wait 2 s
emote chants for a short while, causing dark energy to form around him.
wait 2 s
emote stops chanting and reveals ink his left palm. It appears life like and currently forming the shape of a spider.
wait 1 s
say This ink, shall mark you as favoured.
drop tattoo
%force% %actor% get tattoo
set evil_dedras
remote evil_dedras %actor.id%
else
return 0
say Get away from me, you snivelling mortal!
end
else
say You've already been rewarded. Begone.
drop %object%
end
~
#6324
Reaper skip~
2 c 100
skip~
if (%actor.death% > 1)
wait 2 s
%echo% A swirl of flame surrounds %actor.name%.
wait 5 s
%send% %actor% You are teleported before Tsei the Reaper himself!
%echoaround% %actor% %actor.name% is consumed by the flames!
%teleport% %actor% 6397
wait 1 s
%force% %actor% look
else
%send% %actor% You have not passed the gauntlet!
end
~
$~
