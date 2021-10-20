#200
Karl's Fishing Pond~
2 q 100
~
if (%direction% == east && !%actor.varexists(karlperm)%)
return 0
wait 1
%send% %actor% The gate will only open for paying customers.
elseif (%direction% == east)
return 1
wait 1
%send% %actor% The gate opens slowly as your invisble ink is detected.
%echo% The gate opens for %actor.name% after scanning the invisible ink on %actor.himher%. 
else
return 1
end
~
#201
Karl's Greet~
0 g 100
~
wait 1 s
say Ah hello there, %actor.name%, would you be interested in fishing?
wait 4 s
say If you wish to fish, give me 20 zenni and I will stamp your hand with some of my invisible ink
wait 5 s
say That way you can enter the fishing area of my pond. Don't forget to buy a pole too.
~
#202
Karl's Payment~
0 m 0
~
if (%amount% < 20)
wait 1
say This is not enough...
wait 1 s
give %amount% zenni %actor.name%
elseif (%amount% > 20)
wait 1
say This is more than needed, but alright. Here is your change.
wait 1 s
eval change %amount% - 20
give %change% zenni %actor.name%
wait 1 s
%echoaround% %actor% %self.name% stamps %actor.name%'s hand.
%send% %actor% %self.name% stamps your hand.
set karlperm
remote karlperm %actor.id%
wait 3 s
say When you leave the pond after entering, that stamp will be removed. So remember that.
else
wait 1 s
%echoaround% %actor% %self.name% stamps %actor.name%'s hand.
%send% %actor% %self.name% stamps your hand.
set karlperm
remote karlperm %actor.id%
wait 3 s
say When you leave the pond after entering, that stamp will be removed. So remember that.
end
~
#203
Karl's Pond Leave~
2 q 100
~
if (%actor.varexists(karlperm)% && %direction% == west)
rdelete karlperm %actor.id%
return 1
wait 1
%send% %actor% Your ink has faded from your hand.
else
return 1
end
~
#204
Fishing Trigger~
1 c 3
fish~
if (%actor.varexists(fishing)%)
  wait 1
  %echoaround% %actor% %actor.name% reels in %actor.hisher% line and stops fishing.
  %send% %actor% You reel in your line and stop fishing.
  rdelete fishing %actor.id%
  rdelete caught %actor.id%
  rdelete fishd %actor.id%
elseif (%actor.room.vnum% == 244 && %self.worn_by% == %actor%)
  wait 1
  %echoaround% %actor% %actor.name% casts out %actor.hisher% fishing line.
  %send% %actor% You cast out your fishing line.
  set fishing
  remote fishing %actor.id%
  set fishd 30
  remote fishd %actor.id%
elseif (%self.worn_by% != %actor%)
  wait 1
  %send% %actor% You do not have a fishing pole in your hand.
elseif (%actor.room.vnum% != 244)
  wait 1
  %send% %actor% You are not where you can fish, sorry.
else
  wait 1
  %send% %actor% Something doesn't seem right...
end
~
#205
Catch Trigger~
1 b 100
~
if (%self.worn_by%)
 eval ch %self.worn_by%
if (%ch.varexists(fishd)% && %ch.varexists(fishing)%)
if (%ch.room.vnum% != 244)
 rdelete fishd %ch.id%
 rdelete fishing %ch.id%
 rdelete caught %ch.id%
elseif (%random.20% >= 17)
 rdelete fishd %ch.id%
 rdelete fishing %ch.id%
 rdelete caught %ch.id%
 wait 1
 %send% %ch% You feel like the fish got away.
 %echoaround% %ch% %ch.name% frowns, it looks like the fish got away.
else
if (%ch.fishd% > 0)
 eval change %ch.fishd% - 5
 set fishd %change%
 remote fishd %ch.id%
end
if (!%ch.varexists(caught)%)
 eval num %random.15%
 switch %num%
  case 1
   wait 1 s
   %send% %ch% Your line jerks, and you can feel a large fish on the line.
   %echoaround% %ch% %ch.name%'s fishing line jerks mightly.
   set caught 3
   remote caught %ch.id%
  break
  case 2
  case 3
   wait 1 s
   %send% %ch% Your line jerks, you guess a medium fish is on the line.
   %echoaround% %ch% %ch.name%'s fishing line jerks.
   set caught 2
   remote caught %ch.id%
  break
  case 4
  case 5
  case 6
  case 7
  case 8
   wait 1 s
   %send% %ch% Your line tugs, and you guess a small fish is on the line.
   %echoaround% %ch% %ch.name%'s fishing line tugs.
   set caught 1
   remote caught %ch.id%
  break
  default
   if %random.2% == 1
    %send% %ch% Your line drifts slowly.
   end
  break
 done
elseif (%ch.varexists(caught)% && %ch.fishd% > 0)
 wait 1
  %send% %ch% You feel the fish fighting the line!
  %echoaround% %ch% %ch.name% struggles against the fish on the line!
elseif (%ch.varexists(caught)% && %ch.fishd% == 0)
 rdelete fishd %ch.id%
 rdelete fishing %ch.id%
 eval type %ch.caught%
 rdelete caught %ch.id%
 wait 1
 switch %type%
  case 1
   eval length 1 + %random.5%
   eval weight 1 + %random.2%
   %send% %ch% You reel in your line and have caught a small bass (+25 exp)!
   %send% %ch% The bass measures %length% and has a weight of %weight%
   if (!%ch.varexists(reclength)%)
    %send% %ch% You have a new record in length!
    set reclength %length%
    remote reclength %ch.id%
   elseif (%ch.reclength% < %length%)
    %send% %ch% You have a new record in length!
    set reclength %length%
    remote reclength %ch.id%
   end
   if (!%ch.varexists(recweight)%)
    %send% %ch% You have a new record in weight!
    set recweight %weight%
    remote recweight %ch.id%
   elseif (%ch.recweight% < %weight%)
    %send% %ch% You have a new record in weight!
    set recweight %weight%
    remote recweight %ch.id%
   end
   %echoaround% %ch% %ch.name% reels in %ch.hisher% line and has caught a small bass!
   set %ch.exp(25)%
   %load% obj 201
   %force% %ch% get bass
  break
  case 2
   eval length 3 + %random.10%
   eval weight 3 + %random.10%
   %send% %ch% You reel in your line and have caught a medium bass (+150 exp)!
   %send% %ch% The bass measures %length% and has a weight of %weight%
   %echoaround% %ch% %ch.name% reels in %ch.hisher% line and has caught a medium bass!
   if (!%ch.varexists(reclength)%)
    %send% %ch% You have a new record in length!
    set reclength %length%
    remote reclength %ch.id%
   elseif (%ch.reclength% < %length%)
    %send% %ch% You have a new record in length!
    set reclength %length%
    remote reclength %ch.id%
   end
   if (!%ch.varexists(recweight)%)
    %send% %ch% You have a new record in weight!
    set recweight %weight%
    remote recweight %ch.id%
   elseif (%ch.recweight% < %weight%)
    %send% %ch% You have a new record in weight!
    set recweight %weight%
    remote recweight %ch.id%
   end
   set %ch.exp(150)%
   %load% obj 202
   %force% %ch% get bass
  break
  case 3
   eval length 6 + %random.20%
   eval weight 6 + %random.20%
   %send% %ch% You reel in your line and have caught a large bass  (+200 exp)!
   %send% %ch% The bass measures %length% and has a weight of %weight%
   %echoaround% %ch% %ch.name% reels in %ch.hisher% line and has caught a large bass!
   if (!%ch.varexists(reclength)%)
    %send% %ch% You have a new record in length!
    set reclength %length%
    remote reclength %ch.id%
   elseif (%ch.reclength% < %length%)
    %send% %ch% You have a new record in length!
    set reclength %length%
    remote reclength %ch.id%
   end
   if (!%ch.varexists(recweight)%)
    %send% %ch% You have a new record in weight!
    set recweight %weight%
    remote recweight %ch.id%
   elseif (%ch.recweight% < %weight%)
    %send% %ch% You have a new record in weight!
    set recweight %weight%
    remote recweight %ch.id%
   end
   set %ch.exp(200)%
   %load% obj 203
   %force% %ch% get bass
  break
  default
   %send% %ch% You reel in your line and realize nothing was on it!
   %echoaround% %ch% %ch.name% reels in %ch.hisher% line and nothing is on it!
  break
 done
elseif (!%ch.varexists(caught)% && %ch.fishd% == 0)
 rdelete fishd %ch.id%
 rdelete fishing %ch.id%
 wait 1
 %send% %ch% You reel in your line, giving up for now.
 %echoaround% %ch% %ch.name% reels in %ch.hisher% line, apparently giving up.
   end
  end
 end
end
~
#206
Record Plaque~
1 c 3
display~
if %arg% == record
wait 1
%echoaround% %actor% %actor.name% holds up %actor.hisher% Record Plaque.
%send% %actor% You show everyone your record.
wait 1
if %actor.varexists(reclength)%
%echo% %actor.name%'s Record Catch Length: %actor.reclength%
else
%echo% %actor.name%'s Record Catch Length: None
end
if %actor.varexists(recweight)%
%echo% %actor.name%'s Record Catch Weight: %actor.recweight%
else
%echo% %actor.name%'s Record Catch Weight: None
end
else
wait 1
%send% %actor% display what?
end
~
#207
Record Plaque 2~
1 c 3
check~
if %arg% == record
wait 1
if %actor.varexists(reclength)%
%send% %actor% Your record catch length is, %actor.reclength%
else
%send% %actor% You do not have a record catch length yet.
end
if %actor.varexists(recweight)%
%send% %actor% Your record catch weight is, %actor.recweight%
else
%send% %actor% You do not have a record catch weight yet.
end
else
return 0
end
~
#208
Karl's Receive~
0 j 100
~
if %object.vnum% == 201
wait 1
%actor.gold(100)%
%send% %actor% You receive 100 zenni for the fish.
%purge% %object.name%
end
if %object.vnum% == 202
wait 1
%actor.gold(250)%
%send% %actor% You receive 250 zenni for the fish.
%purge% %object.name%
end
if %object.vnum% == 203
wait 1
%actor.gold(800)%
%send% %actor% You receive 800 zenni for the fish.
%purge% %object.name%
end
~
#209
Goku Greet~
0 g 100
~
set target_char %self.room.people%
while %target_char%
 set tmp_tar %target_char.next_in_room%
 if %target_char.vnum% == 202
  eval chi %target_char%
 end
 set target_char %tmp_tar%
done
if (!%chi%)
 wait 1 s
 say Where is Chi Chi?
 wait 3 s
 say It isn't like her to not be here.
else
 wait 1 s
 say Sorry Chi Chi, but I think Gohan needs to train.
 wait 3 s
 %force% %chi% say How is he ever going to compete in the real world if he doesn't study!?
 wait 5 s
 say He can study later...
 wait 3 s
 %force% %chi% say Later never comes with you Goku!
 wait 4 s
 say Ha ha ha
 wait 1 s
 %echo% Goku pats the back of his head while laughing.
 wait 3 s
 %force% %chi% say Well anyway I need to get dinner started.
 if (!%actor.varexists(gokuknow)%)
  wait 5 s
  if (%actor.hisher% == his)
   say Oh hello there Mr.
   wait 2 s
   say I'm not sure if you know me yet, but I'm Goku. Nice to meet you... what's your name again (@Gintroduce@C)?
  elseif (%actor.hisher% != her)
   say Oh hello there.
   wait 2 s
   say I'm not sure if you know me yet, but I'm Goku. Nice to meet you... what's your name again (@Gintroduce@C)?
  else
   say Oh hello there Miss.
   wait 2 s
   say I'm not sure if you know me yet, but I'm Goku. Nice to meet you... what's your name again (@Gintroduce@C)?
  end
 else
  wait 4 s
  say Ah nice to see you again %actor.name%
 end
end
~
#210
Goku Introduce~
0 c 100
introduce~
if (!%actor.varexists(gokuknow)%)
wait 1
%echoaround% %actor% %actor.name% introduces %actor.himher%self to Goku.
%send% %actor% You introduce yourself to Goku.
wait 3 s
say Nice to meet you, %actor.name%.
set gokuknow
remote gokuknow %actor.id%
 if (!%self.varexists(full)%)
  wait 4 s
  say I am kinda hungry and Chi Chi won't have dinner ready for a bit. Do you happen to have a black bass?
  wait 6 s
  say Even a black bass will just be a snack to me, ha ha.
  wait 4 s
  say If you give me a black bass I will give you something interesting.
 else
  wait 4 s
  say Well I am good till dinner, stay as long as you want %actor.name%
  wait 2 s
  laugh
 end
else
wait 1
%echoaround% %actor% %actor.name% introduces %actor.himher%self to Goku.
%send% %actor% You introduce yourself to Goku.
wait 2 s
say I already know you, %actor.name%, ha ha.
end
~
#211
Goku Receive~
0 j 100
~
if (!%actor.varexists(gokuknow)%)
 return 0
 wait 1
 say I am not going to accept food from a stranger, sorry.
else
 if (%object.vnum% != 1000)
  return 0
  wait 1
  say I don't really want this, %actor.name%, you take it back.
 elseif (%self.varexists(full)%)
  return 0
  wait 1
  say I am already good until dinner, ha ha.
 elseif (%actor.varexists(gokugift)%)
  return 0
  wait 1
  say I already gave you something, I have nothing else to give you.
  wait 4 s
  say Sorry.
 else
  wait 1
  %purge% %object.name%
  say Thanks, %actor.name%!
  wait 2 s
  %echo% Goku eats the large bass raw at an astonishing speed, and then tosses the bones into a trashcan.
  wait 5 s
  say Wow that was good..
  wait 3 s
  say Well here you go then, take care of it.
  %load% obj 205
  drop pole
  set gokugift
  remote gokugift %actor.id%
  set full
  remote full %self.id%
 end
end
~
#212
Fishing New~
1 c 3
fish~
if (%actor.varexists(fishing)%)
  wait 1
  %echoaround% %actor% %actor.name% reels in %actor.hisher% line and stops fishing.
  %send% %actor% You reel in your line and stop fishing.
  rdelete fishing %actor.id%
  rdelete caught %actor.id%
  rdelete fishd %actor.id%
elseif (%actor.room.fishing% == 1 && %self.worn_by% == %actor%)
  wait 1
  %echoaround% %actor% %actor.name% casts out %actor.hisher% fishing line.
  %send% %actor% You cast out your fishing line.
  set fishing
  remote fishing %actor.id%
  set fishd 30
  remote fishd %actor.id%
elseif (%self.worn_by% != %actor%)
  wait 1
  %send% %actor% You do not have a fishing pole in your hand.
else
  wait 1
  %send% %actor% You are not where you can fish, sorry.
end
~
#213
Power Pole Extend~
1 c 1
extend~
if %arg% = Extend
  wait 1s
  %send% %actor% @WYou shout, @C'@YPower Pole:  Extend!@C'@W  The %self.name% extends itself!@n
  %echoaround% %actor% @W%actor.name% shouts, @C'@YPower Pole:  Extend!@C'@W  The %self.name% extends itself!@n
  wait 300s
  %send% %actor% @RThe %self.name% reverts back to its normal length.@n
  %echoaround% %actor% @R%actor.name%'s %self.name% reverts back to its normal length.@n
end
~
#250
Senzu Plant Random~
1 ab 100
~
if (!%self.carried_by%)
 if (%self.weight% < 4)
  if (%self.cost_per_day% == %time.hour% && %self.weight% <= 2)
   nop %self.weight(1)%
   %echo% The beans on %self.shortdesc% grow a little more.
   if (%time.hour% + 8 <= 23)
    eval hor %time.hour% + 8
   else
    eval hor (24 - %time.hour%) + 8
   end
   nop %self.cost_per_day(-%self.cost_per_day%)%
   nop %self.cost_per_day(%hor%)%
  elseif (%self.cost_per_day% == %time.hour% && %self.weight% == 3)
   nop %self.weight(1)%
   nop %self.cost_per_day(-%self.cost_per_day%)%
   nop %self.cost_per_day(30)%
   %echo% The beans on %self.shortdesc% seemed to have finished growing!
  end
 end
end
~
#251
Senzu Plant Water~
1 c 6
water~
return 1
if (!%actor.inventory(251)%)
 %send% %actor% You don't have the right stuff to water it with!
elseif (%self.weight% >= 4)
 %send% %actor% It already has three senzu beans on it. It won't grow anymore until they have been picked.
elseif (%self.cost_per_day% != 30)
 %send% %actor% It has already been watered and is growing a bean.
else
 %force% %actor% get plant
 wait 1 s
 %send% %actor% You empty the bottle of grow water on the %self.shortdesc%.
 %echoaround% %actor% %actor.name% empties a bottle of grow water on the %self.shortdesc%
 %purge% %actor.inventory(251)%
 if (%time.hour% + 8 <= 23)
  eval hor %time.hour% + 8
 else
  eval hor (24 - %time.hour%) + 8
 end
 if (%hor% == 0)
  eval hor 1
 end
 nop %self.cost_per_day(-%self.cost_per_day%)%
 nop %self.cost_per_day(%hor%)%
 %force% %actor% drop plant
end
~
#252
Senzu Plant Pick~
1 c 6
pick~
if (%arg% != bean)
 %send% %actor% Try pick bean.
elseif (%self.cost_per_day% == 30 && %self.weight% == 1)
 %send% %actor% There are no beans growing on %self.shortdesc% yet.
elseif (%self.weight% <= 3)
 %send% %actor% The beans on %self.shortdesc% are not ready.
else
 %force% %actor% get plant
 wait 1 s
 %send% %actor% You pick three senzu beans off the %self.shortdesc%!
 %echoaround% %actor% %actor.name% picks a bean off the %self.shortdesc%!
 wait 1 s
 %load% obj 1
 %load% obj 1
 %load% obj 1
 %force% %actor% get all.bean
 nop %self.weight(-3)%
 if (%self.cost_per_day% < 30)
  nop %Self.cost_per_day(-%self.cost_per_day%)%
  nop %self.cost_per_day(30)%
 end
 %force% %actor% drop plant
end
~
#253
Plant Load~
1 n 100
~
nop %self.cost_per_day(-1)%
~
#256
new trigger~
0 g 100
~
eval dnum %random.2%
if (%self.room% == 200)
 wait 1
 %echo% %self.shortdesc% speeds along the the highway to the east (room200).
 ogoto %self.room.east(vnum)%
else
 switch %dnum%
  case 1
   if (%self.room.north(vnum)%) 
    wait 1
    %echo% %self.shortdesc% speeds along the highway to the north.
    ogoto %self.room.north(vnum)%
   elseif (%self.room.east(vnum)%)
    wait 1
    %echo% %self.shortdesc% speeds along the highway to the east.
    ogoto %self.room.east(vnum)%
   elseif (%self.room.south(vnum)%)
    wait 1
    %echo% %self.shortdesc% speeds along the highway to the south.
    ogoto %self.room.south(vnum)%
   elseif (%self.room.west(vnum)%)
    wait 1
    %echo% %self.shortdesc% speeds along the highway to the west.
    ogoto %self.room.west(vnum)%
   end 
   break
   default
    %echo% %self.shortdesc% speeds along the highway.
    break
   done
end
~
#299
Augmenter 9001 Trigger~
1 b 15
~
%echo% %self.name%'s speakers crackle to life with music.
wait 1 s
%echo% @D'@RAuuuugmenter 9001 has your fix! It will make you better with its grand bag of tricks!@D'@n
wait 2 s
%echo% @D'@RStep right to it and don't be shy! Grow muscles while you sit and achieve dreams beyond the sky!@D'@n
~
$~
