#16700
Targgo Greet~
0 g 100
~
wait 1 s
say Welcome! Welcome! You come and buy, yes?
~
#16701
Kryikia Greet~
0 g 100
~
if (!%actor.varexists(kilnak)%)
 wait 1 sec
 %echo% %self.name% looks up.
 wait 2 s
 say If you want fight Toorg'ak, you need chief permission.
elseif (%actor.kilnak% == Permission)
 wait 1 sec
 say You need talk to chief, get permission.
elseif (!%actor.has_item(16709)%)
 wait 1 sec
 say Ah, you now honary member of tribe...take this.
 wait 1
 %load% o 16709
 drop key
 %force% %actor% get key
 wait 1 sec
 say Have fun with Toorg'ak, but not too much fun! Ha ha ha!
end
~
#16702
Royal Jelly Eat~
1 c 3
eat~
eval num %actor.level% * 10000000
if (%num% + %actor.exp% <= %actor.tnl% * 4)
set %actor.exp(%num%)%
%send% %actor% @GYou feel the royal jelly's energies clarify all the memories in your mind!@n @D[@WExp: @Y+%num%@D]@n
else
 eval minus %actor.tnl% * 4 - %actor.exp%
 set %actor.exp(%minus%)%
 %send% %actor% @GYou feel the royal jelly's energies clarify all the memories in your mind! @D[@WExp@D: @Y+%minus%@D]@n
 %send% %actor% @RDue to your low level you did not receive the maximum experience this would have provided.@n
end
return 0
~
#16703
Chief Greet~
0 g 100
~
if (!%actor.varexists(kilnak)%)
 wait 20
 say You dare invade Kilnak territory uninvited!?
 wait 2 s
 seishou %actor.name%
elseif (%actor.kilnak% == Enemy)
 wait 10
 say An enemy in MY chambers!? Die!
 wait 2
 seishou %actor.name%
elseif (%actor.kilnak% == Friend)
 wait 20
 say Ah, welcome friend. Kilnak home is your home!
elseif (%actor.kilnak% == Permission)
 wait 1 sec
 %echo% %self.name% looks up.
 wait 2 s
 say I hear of you from guards. You seek be friend of Kilnak?
 wait 4 s
 say If you be Kilnak friend, bring me fifteen Gaxzixite eggs, then I consider friendship.
 wait 1 s
 if (%actor.varexists(eggcount)%)
  say You still need to give me %actor.eggcount% Gaxzixite eggs.
 else
  set eggcount 15
  remote eggcount %actor.id%
 end
end
~
#16704
Kilnak Guard~
0 g 100
~
if (!%actor.varexists(kilnak)%)
 wait 1
 say Intruder! Die!
 wait 1 s
 head %actor.name%
elseif (%actor.kilnak% == Enemy)
 wait 1
 say I must defend my people!
 wait 1 s
 seishou %actor.name%
else
 wait 1
 %echo% %self.name% salutes.
end
~
#16705
Fixi greet~
0 g 100
~
wait 1 s
say Children are the lifeblood of our people, and should be well fed.
wait 2 s
%echo% %self.name% slowly stirs the Toorg'ak stew.
wait 4 s
say Arlians on the surface are so barbaric, they barely care for their young at all.
wait 5 s
%echo% %self.name% tastes the stew, seems to think to herself, and then adds some spices from one of the silk pouches hanging from her waist.
~
#16706
Chief Receive~
0 j 100
~
if (%object.vnum% == 16900)
 say Ah, another delicious egg!
 set eggcount 0
 if (%actor.varexists(eggcount)%)
  if (%actor.eggcount% == 1)
   set eggcount -1
   remote eggcount %actor.id%
   wait 1 s
%purge% %object%
   say Well, you complete your end of bargain. You are now friend of tribe!
   set kilnak Friend
   remote kilnak %actor.id%
  else
   eval eggcount %actor.eggcount% - 1
   wait 1 s
   remote eggcount %actor.id%
%purge% %object%
   say You need only bring me %actor.eggcount% more eggs now.
  end
 else
  set eggcount 14
  remote eggcount %actor.id%
  wait 1 s
%purge% %object%
  say You need only bring me %actor.eggcount% more eggs now.
 end
else
 return 0
 wait 1
 say I don't want this!
end
~
#16707
Fiixora Greet~
0 g 100
~
if (!%actor.varexists(fiixora)%)
wait 2 s
say Hmm, who are you?
wait 3 s
say Hmm, doesn't matter.
wait 2 s
say Hmm, you be interested in fine silk?
wait 3 s
emote holds up a fine silk cape in her hands.
wait 4 s
say Bring me five deeshin mushrooms, this cape then be yours.
elseif (%actor.fiixora% == 0)
 wait 2 s
 say Hmm, enjoying that cape?
 wait 5
 smile
else
 wait 2 s
 say Hmm, got the mushrooms yet?
end
~
#16708
Fiixora Receive~
0 j 100
~
if (%object.vnum% == 16901)
 if (!%actor.varexists(fiixora)%)
  set fiixora 4
  remote fiixora %actor.id%
  wait 1
  say Hmm, just 4 more.
elseif (%actor.fiixora% == 0)
return 0
wait 1
say Hmm, I already traded you cape, leave me alone.
 elseif (%actor.fiixora% > 1)
  eval fiixora %actor.fiixora% - 1
  remote fiixora %actor.id%
  wait 1
%purge% %object%
  say Hmm, just %actor.fiixora% more.
 elseif (%actor.fiixora% == 1)
  set fiixora 0
  remote fiixora %actor.id%
  wait 1
%purge% %object%
  say Hmm, that's all the mushroom. Here...
  wait 1 s
  %load% o 16724
  drop cape
  %force% %actor% get cape
 end
else
 return 0
 wait 1
 say Hmm, I have no use for that. Keep it young one.
end
~
#16709
Kryikia Block~
2 q 100
~
if (!%actor.has_item(16709)% && %direction% == north)
 return 0
wait 1
 %echo% Kryikia blocks the way. You need the key to pass by.
end
~
#16710
Gaxira Greet~
0 g 100
~
if (!%actor.varexists(gaxira)%)
wait 2 s
emote looks up.
wait 1 s
sigh
wait 1 s
say If you want scalvis bracers, I can make you deal. Bring me ten scalvis shells, me give you bracer.
elseif (%actor.gaxira% != -1)
 wait 2 s
 emote looks up.
 wait 1 s
 say You have shell?
else
 wait 2 s
 emote looks up.
 wait 1 s
 say You enjoy bracer?
end
~
#16711
Gaxira Receive~
0 j 100
~
if (!%actor.varexists(gaxira)%)
 set gaxira 10
 remote gaxira %actor.id%
end
if (%object.vnum% == 16800 && %actor.gaxira% > -1)
 return 1
 wait 1
 eval gaxira %actor.gaxira% - 1
 remote gaxira %actor.id%
wait 1
%purge% %object%
 if (%actor.gaxira% == 0)
  set gaxira -1
  remote gaxira %actor.id%
  wait 1
  say Ah, this enough.  'Here bracer for you.
  %load% o 16725
  wait 1
  drop bracer
  %force% %actor% get bracer
 else
  say Only %actor.gaxira% more shells needed.
 end
elseif (%actor.gaxira% == -1)
 return 0
 wait 1
 say I already give you bracer, you not Kilnak, you not get more.
else
 return 0
 wait 1
 say I not want that.
end
~
$~
