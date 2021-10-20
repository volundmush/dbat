#2000
Minecart Driver~
0 g 100
~
switch %self.leave%
 case 0
 eval tme Midnight
 break
 case 1
 case 2
 case 3
 case 4
 case 5
 case 6
 case 7
 case 8
 case 9
 case 10
 case 11
 eval tme %self.leave% AM
 break
 case 12
 eval tme Noon
 break
 case 13
 eval tme 1 PM
 break
 case 14
 eval tme 2 PM
 break
 case 15
 eval tme 3 PM
 break
 case 16
 eval tme 4 PM
 break
 case 17
 eval tme 5 PM
 break
 case 18
 eval tme 6 PM
 break
 case 19
 eval tme 7 PM
 break
 case 20
 eval tme 8 PM
 break
 case 21
 eval tme 9 PM
 break
 case 22
 eval tme 10 PM
 break
 case 23
 eval tme 11 PM
 break
done
wait 1
if (%self.room.vnum% == 2017)
wait 1
say Those wanting a ride to the excavation sites, I will be leaving at %tme%.
wait 3 s
say Enter the cart if you want to ride by then.
elseif (%self.room.vnum% == 2046)
wait 1
say Those wanting a ride to the Sub-level One, I will be leaving at %tme%.
wait 3 s
say Enter the cart if you want to ride by then.
end
~
#2001
Minecart Driver Load~
0 n 100
~
if (!%self.room.contents(2002)% && %self.room.vnum% == 2017)
 %load% obj 2002
 drop cart
 wait 1
 mgoto 2069
 %load% obj 2003
 drop exit
 wait 1
 mgoto 2017
end
if (%time.hour% <= 21)
 eval num %time.hour% + 2
 set leave %num%
 remote leave %self.id%
elseif (%time.hour% == 22)
 eval num 0
 set leave %num%
 remote leave %self.id%
elseif (%time.hour% == 23)
 eval num 1
 set leave %num%
 remote leave %self.id%
else
 eval num 2
 set leave %num%
 remote leave %self.id%
end
~
#2002
Minecart Operator Leave~
0 ab 100
~
if (%self.leave% == %time.hour%)
 detach 2001 %self.id%
 if (%self.room.vnum% == 2017)
  wait 2 s
  say No more boarding!
  wait 1 s
  close minecart
  %echo% The minecart driver climbs aboard the cart.
  mgoto 2069
  %echo% The driver climbs aboard and sits down in his seat.
  %purge% exit
  wait 2 s
  %echo% The driver starts the cart up.
  %at% 2017 %echo% The minecart engine starts up loudly.
  wait 2 s
  %echo% The minecart starts moving.
  %at% 2017 %echo% The minecart starts moving north.
  %at% 2017 %purge% cart
  wait 5 s
  %echo% The minecart drives down to Sub-level Two.
  wait 5 s
  %echo% The minecart turns a sharp corner to the west, with its ore buckets clacking in protest behind.
  wait 5 s
  %echo% The minecart turns another sharp corner to the south followed by another turn to the west.
  wait 5 s
  say The mine has been dangerous of late. Glad I am in a minecart most of the time.
  wait 5 s
  %echo% The minecart drives down to Sub-level Three.
  wait 5 s
  say Did you hear about the deaths in Sub-level Six?
  wait 4 s
  say The bossman closed the whole thing off, said it was all an accident.
  wait 5 s
  %echo% The minecart drives down to Sub-level Four.
  wait 5 s
  say I heard though that they broke open some kind of chamber...
  wait 5 s
  say And something came out and killed those miners.
  wait 5 s
  %echo% The minecart drives down to Sub-level Five.
  wait 5 s
  say Don't tell the foreman I said anything about that. He has threatened to fire over it.
  wait 5 s
  %echo% The minecart comes to a halt as it reaches the excavation sites.
  wait 5 s
  say Alright, everyone off.
  %at% 2046 %load% obj 2005
  %at% 2046 %echo% The minecart arrives and comes to a stop.
  %at% 2047 %echo% @GThe minecart has arrived at the excavation sites.@n
  %at% 2048 %echo% @GThe minecart has arrived at the excavation sites.@n
  %at% 2052 %echo% @GThe minecart has arrived at the excavation sites.@n
  %at% 2053 %echo% @GThe minecart has arrived at the excavation sites.@n
  %at% 2057 %echo% @GThe minecart has arrived at the excavation sites.@n
  %at% 2058 %echo% @GThe minecart has arrived at the excavation sites.@n
  %load% obj 2003
  %teleport% all 2070
  wait 2 s
  say I will be leaving in two hours to Sub-level One.
  %echo% The driver disembarks.
  mgoto 2046
  %echo% The minecart driver steps out of the minecart.
  say I will be leaving in two hours to Sub-level One.
  %load% mob 2002
  %purge% %self%
 elseif (%self.room.vnum% == 2046)
  wait 2 s
  say No more boarding!
  wait 1 s
  close minecart
  %echo% The minecart driver climbs aboard the cart.
  mgoto 2070
  %echo% The driver climbs aboard and sits down in his seat.
  %purge% exit
  wait 2 s
  %echo% The driver starts the cart up.
  %at% 2046 %echo% The minecart engine starts up loudly.
  wait 2 s
  %echo% The minecart starts moving.
  %at% 2046 %echo% The minecart starts moving south.
  %at% 2046 %purge% cart
  wait 5 s
  %echo% The minecart drives up to Sub-level Four.
  wait 5 s
  say Have a hard day of work?
  wait 4 s
  say This mine won't ever be as productive with Sub-level Six locked off..
  wait 5 s
  %echo% The minecart drives up to Sub-level Three.
  wait 5 s
  say There has been pressure from above on the foreman to keep everything quiet.
  wait 5 s
  say If you value your job you won't talk about it they say.
  wait 5 s
  say I almost wouldn't mind finding new work... This place feels dangerous anymore.
  wait 5 s
  %echo% The minecart drives up to Sub-level Two.
  wait 5 s
  %echo% The minecart makes a sharp turn to the east.
  wait 5 s
  %echo% The minecart makes a sharp turn to the north and then another to the east.
  wait 5 s
  %echo% The minecart makes a sharp turn to the south.
  wait 5 s
  %echo% The minecart drives up to Sub-level One.
  wait 5 s
  %echo% The minecart comes to a halt as it reaches outside the Foreman's office.
  wait 5 s
  say Alright, everyone off.
  %at% 2017 %load% obj 2002
  %at% 2017 %echo% The minecart arrives and comes to a stop.
  %load% obj 2004
  %teleport% all 2069
  wait 1 s
  say I will be leaving in two hours to the excavation sites.
  %echo% The driver disembarks.
  mgoto 2017
  %echo% The minecart driver steps out of the minecart.
  say I will be leaving in two hours to the excavation sites.
  %load% mob 2002
  %purge% %self%
 end
elseif (%self.leave% > 0 && (%self.leave% - 1) == %time.hour%)
 if (!%self.varexists(announced)%)
 set announced 0
 remote announced %self.id%
 if (%self.room.vnum% == 2017)
   wait 150 s
   say I will be leaving in half an hour to the excavation sites.
   %at% 2018 %echo% The minecart will be leaving for the excavation sites in half an hour.
 elseif (%self.room.vnum% == 2046)
   wait 150 s
   say I will be leaving in half an hour to outside the Foreman's office.
   %at% 2047 %echo% The minecart will be leaving to outside the Foreman's office in half an hour.
   %at% 2048 %echo% The minecart will be leaving to outside the Foreman's office in half an hour.
   %at% 2052 %echo% The minecart will be leaving to outside the Foreman's office in half an hour.
   %at% 2053 %echo% The minecart will be leaving to outside the Foreman's office in half an hour.
   %at% 2057 %echo% The minecart will be leaving to outside the Foreman's office in half an hour.
   %at% 2058 %echo% The minecart will be leaving to outside the Foreman's office in half an hour.
 end
 elseif (%self.announced% <= 0)
  set announced 1
  remote announced %self.id%
  if (%self.room.vnum% == 2017)
   wait 140 s
   say I will be leaving momentarily to the excavation sites.
   %at% 2018 %echo% The minecart will be leaving for the excavation sites momentarily.
  elseif (%self.room.vnum% == 2046)
   wait 140 s
   say I will be leaving momentarily to outside the Foreman's office.
   %at% 2047 %echo% The minecart will be leaving to outside the Foreman's office momentarily.
   %at% 2048 %echo% The minecart will be leaving to outside the Foreman's office momentarily.
   %at% 2052 %echo% The minecart will be leaving to outside the Foreman's office momentarily.
   %at% 2053 %echo% The minecart will be leaving to outside the Foreman's office momentarily.
   %at% 2057 %echo% The minecart will be leaving to outside the Foreman's office momentarily.
   %at% 2058 %echo% The minecart will be leaving to outside the Foreman's office momentarily.
 end
 end
elseif (%self.leave% == 0 && 23 == %time.hour%)
 if (!%self.varexists(announced)%)
 set announced 0
 remote announced %self.id%
 if (%self.room.vnum% == 2017)
   wait 150 s
   say I will be leaving in half an hour to the excavation sites.
   %at% 2018 %echo% The minecart will be leaving for the excavation sites in half an hour.
 elseif (%self.room.vnum% == 2046)
   wait 150 s
   say I will be leaving in half an hour to outside the Foreman's office.
   %at% 2047 %echo% The minecart will be leaving to outside the Foreman's office in half an hour.
   %at% 2048 %echo% The minecart will be leaving to outside the Foreman's office in half an hour.
   %at% 2052 %echo% The minecart will be leaving to outside the Foreman's office in half an hour.
   %at% 2053 %echo% The minecart will be leaving to outside the Foreman's office in half an hour.
   %at% 2057 %echo% The minecart will be leaving to outside the Foreman's office in half an hour.
   %at% 2058 %echo% The minecart will be leaving to outside the Foreman's office in half an hour.
 end
 elseif (%self.announced% <= 0)
  set announced 1
  remote announced %self.id%
  if (%self.room.vnum% == 2017)
   wait 140 s
   say I will be leaving momentarily to the excavation sites.
   %at% 2018 %echo% The minecart will be leaving for the excavation sites momentarily.
  elseif (%self.room.vnum% == 2046)
   wait 140 s
   say I will be leaving momentarily to outside the Foreman's office.
   %at% 2047 %echo% The minecart will be leaving to outside the Foreman's office momentarily.
   %at% 2048 %echo% The minecart will be leaving to outside the Foreman's office momentarily.
   %at% 2052 %echo% The minecart will be leaving to outside the Foreman's office momentarily.
   %at% 2053 %echo% The minecart will be leaving to outside the Foreman's office momentarily.
   %at% 2057 %echo% The minecart will be leaving to outside the Foreman's office momentarily.
   %at% 2058 %echo% The minecart will be leaving to outside the Foreman's office momentarily.
 end
 end
end
~
#2003
Krillin Quest Diamond~
0 f 100
~
if (%actor.varexists(onkq4)% || %actor.varexists(onGohanQ4)% || %actor.varexists(onTenQ4)% || %actor.varexists(onTienQ4)%)
    %load% obj 2013
end
~
#2004
Foreman Greet~
0 g 100
~
if (!%actor.varexists(miner)%)
wait 1
%echo% Kevin looks at a business log on his desk.
wait 2 s
%echo% Kevin scowls at the log.
wait 4 s
%echoaround% %actor% Kevin looks up at %actor.name%.
%send% %actor% Kevin looks up at you.
wait 2 s
say I hope you are here about the mining job. If not then you are wasting my time.
wait 4 s
say If you are interested in the mining job, @Gapply@C for it.
wait 3 s
say Otherwise get out...
else
wait 1
%echoaround% %actor% Kevin looks up from a business log at %actor.name%.
%send% %actor% Kevin looks up from a business log at you.
wait 2 s
say Uh..%actor.name% is it? How's work been?
wait 4 s
say Well don't waste too much of my time, we have money to be making afterall..
end
~
#2005
Apply For Mining Job~
0 c 100
apply~
if (%actor.level% < 15)
wait 1
say You are not going to last five minutes here with your experience.
wait 1 s
eval short 15 - %actor.level%
%send% %actor% You feel like you are %short% levels away from being able to work here.
elseif (!%actor.varexists(miner)%)
wait 1
say Ah, %actor.name%, so you want to do some freelance mining work?
wait 2 s
say Usually we don't do it this way, but this operation is in need.
wait 4 s
say We recently had some accidents on our lowest and newest level that has crippled our output.
wait 5 s
say So we need as many workers as we can get to work the older excavation sites in the meantime.
wait 4 s
say So here you are then, your pickaxe.
set miner
remote miner %actor.id%
wait 1
%load% obj 2008
give pick %actor.name%
wait 3 s
say You will need to go to one of the excavation sites, mine, and then turn in your ore to the minecart when it arrives every two hours.
wait 5 s
say I will pay you depending on what the minecart hauls back. Come to me when you are done mining and depending on how much you loaded and its quality I will give you your pay.
wait 6 s
say Look at the sign here for the values of the ore and what I will pay for them.
else
wait 1
say You are already working for the company, don't be foolish...
end
~
#2006
Mine Trigger~
2 c 100
mine~
if (%actor.fighting%)
return 1
wait 1
%send% %actor% You are fighting!
else
if (%actor.eq(17)%)
 eval obj %actor.eq(17)%
end
if (%obj.vnum% != 2008)
wait 1
%send% %actor% You do not even have a pickaxe equipped!!
elseif (!%actor.varexists(miner)%)
wait 1
%send% %actor% You are not even getting paid by the company, why work for them?
elseif (%actor.move% < (%actor.maxmove% / 25))
wait 1
%send% %actor% You are too tired to mine right now.
elseif (%actor.pos% != Standing)
wait 1
%send% %actor% You must be standing!
else
 if (%self.vnum% <= 2058)
  eval num %random.25%
 else
  eval num %random.20%
 end
eval mm %actor.maxmove% / 25
eval neg -%mm%
wait 1
%echoaround% %actor% %actor.name% slams %actor.hisher% pick into the mine wall.
%send% %actor% You slam your pick into the mine wall and begin mining.
wait 3 s
 if (%actor.room.vnum% == %self.vnum%)
 %echoaround% %actor% %actor.name% continues to mine.
 %send% %actor% You continue mining.
 end
wait 3 s
 if (%actor.room.vnum% == %self.vnum%)
 switch %num%
  case 1
  %echoaround% %actor% %actor.name% strikes some Fullmoon Silver Ore!
  %send% %actor% You lucky S.O.B. you have struck some Fullmoon Silver Ore, better hang onto that.
  %load% obj 2009
  %force% %actor% get fullmoon
  break
  case 2
  case 3
  case 4
  %echoaround% %actor% %actor.name% strikes some ore.
  %send% %actor% You strike some ore.
  %load% obj 2007
  eval ore %self.contents(2007)%
  set %ore.cost(2)%
  %force% %actor% get ore
  break
  case 5
  case 6
  case 7
  case 8
  %echoaround% %actor% %actor.name% strikes some ore.
  %send% %actor% You strike some ore.
  %load% obj 2007
  eval ore %self.contents(2007)%
  set %ore.cost(1)%
  %force% %actor% get ore
  break
  case 9
  case 10
  case 11
  case 12
  case 13
  case 14
  %echoaround% %actor% %actor.name% strikes some ore.
  %send% %actor% You strike some ore.
  %load% obj 2007
  %force% %actor% get ore
  break  
  default
  %echoaround% %actor% %actor.name% breaks loose a dirty rock.
  %send% %actor% You break loose a....dirty rock.
  %load% obj 2006
  %force% %actor% get rock
  break
 done
set %actor.move(%neg%)%
 else
 wait 1
 %send% %actor% You left the ore you started working on...
 end
end
end
~
#2007
Deposit Ore~
0 c 100
Deposit~
if (%self.room.vnum% == 2017)
wait 1
say Loading of ore needs to be done at the excavation sites, not all the way up here.
elseif (%actor.pos% != Standing)
wait 1
%send% %actor You should probably stand up for this.
else
%force% %actor% give all.smark smammy
%force% %actor% give all.rock smammy
%force% %actor% give all.ore smammy
  eval num2 0
 while %self.inventory(2006)%
  eval num2 %num2% + 4
  eval obj %self.inventory(2006)%
  %purge% %obj.name%
 done
 while %self.inventory(2009)%
  eval num2 %num2% + 500
  eval obj %self.inventory(2009)%
  %purge% %obj.name%
 done
 while %self.inventory(2007)%
  eval obj %self.inventory(2007)%
  if (%obj.cost% == 3)
   eval num2 %num2% + 75
  elseif (%obj.cost% == 2)
   eval num2 %num2% + 50
  elseif (%obj.cost% == 1)
   eval num2 %num2% + 25
  end
  %purge% %obj.name%
 done
 if (%num2% > 0)
  if (!%actor.varexists(minepay)%)
   set minepay %num2%
   remote minepay %actor.id%
  else
   eval plus %actor.minepay% + %num2%
   set minepay %plus%
   remote minepay %actor.id%
  end
  wait 1
  say Alright, %actor.name%, go speak to the foreman for your @gpayout@C of @Y%actor.minepay%@C.@n
  wait 1 s
  %echo% %self.name% loads the ore into one of the mine carts behind the main minecart.
 else
  wait 1
  say Don't joke with me...
 end
end
~
#2008
Payout Trigger~
0 c 100
payout~
if (!%actor.varexists(miner)%)
wait 1
say You don't even work for us...
elseif (!%actor.varexists(minepay)%)
wait 1
say You haven't deposited anything in the minecart!
else
 eval pay %actor.minepay%
 wait 1
 say Alright, here is your pay. Do some more work for us sometime.
 set %actor.gold(%pay%)%
 %send% %actor% You receive @Y%pay% @nzenni.
rdelete minepay %actor.id%
end
~
#2009
Technik Guard~
0 g 100
~
wait 1
%echoaround% %actor% %self.name% looks at %actor.name%.
%send% %actor% %self.name% looks at you.
wait 2 s
say You better belong here.
~
#2010
Technik Guard 2~
0 q 100
~
if (%direction% == down)
 if (%actor.varexists(BoughtPrism)%)
  if (%actor.BoughtPrism% == 1)
   wait 1
   %echo% %self.name% allows %actor.name% to go downstairs.
   wait 1
   %send% %actor% %self.name% allows you to pass through since you failed to purchase anything last time.
  else
   return 0
   wait 1
   %echoaround% %actor% %self.name% reaches out and stops %actor.name% with one hand.
   %send% %actor% %self.name% 
   wait 2 s
   say You already purchased a prism. If you want to go in again give me another nano-diode.
  end
 elseif (!%self.varexists(bribed)%)
  return 0
  wait 1
  %echoaround% %actor% %self.name% reaches out and stops %actor.name% with one hand.
  %send% %actor% %self.name% reaches out and stops you with one hand.
  wait 2 s
  say Only Technik members may enter.
  wait 2 s
  say Unless you give me something I want.
  wait 3 s
  say I need a nano-diode to finish something I am working on.
  wait 4 s
  say If you were to happen to give me one, I might overlook your slipping past me once.
 elseif (%self.bribed% != %actor.id%)
  return 0
  wait 1
  %echoaround% %actor% %self.name% reaches out and stops %actor.name% with one hand.
  %send% %actor% %self.name% reaches out and stops you with one hand.
  wait 2 s
  say Only Technik members may enter.
  wait 2 s
  say Unless you give me something I want.
  wait 3 s
  say I need a nano-diode to finish something I am working on.
  wait 4 s
  say If you were to happen to give me one, I might overlook your slipping past me once.
 else
  wait 1
  %echo% %self.name% allows %actor.name% to go downstairs.
  wait 1
  %send% %actor% %self.name% allows you to pass through just this once.
  rdelete bribed %self.id%
 end
end
~
#2011
Buy~
0 c 100
buy~
if (%actor.eq(17)%)
eval obj %actor.eq(17)%
end
if (%obj.vnum% == 2008)
wait 1
say You already have a pickaxe, why buy another?
elseif (%actor.has_item(2008)%)
wait 1
say You already have a pickaxe, why buy another?
elseif (!%actor.varexists(miner)%)
wait 1
say You should only buy a pickaxe if you work for us and have lost the FREE one we give out.
else
return 0
end
~
#2012
Technik Guard 3~
0 j 100
~
time
if (%object.vnum% != 19508)
return 0
wait 1
%echoaround% %actor% %self.name% refuses %object.shortdesc% from %actor.name%.
%send% %actor% %self.name% refuses %object.shortdesc% from you.
wait 1 s
say I don't want that. Why don't you get out of here for wasting my time.
wait 1 s
%force% %actor% leave
elseif (%actor.gold% < 50000)
return 0
wait 1
%echoaround% %actor% %self.name% refuses %object.shortdesc% from %actor.name%.
%send% %actor% %self.name% refuses %object.shortdesc% from you.
wait 1 s
say You don't even have enough money to buy what is in there.
wait 1
eval short 50000 - %actor.gold%
%send% %actor% You feel you are short by %short% zenni.
else
wait 1
say You better hurry through while you have my favor.
set bribed %actor.id%
remote bribed %self.id%
end
~
#2013
Foun Greet~
0 g 100
~
if (!%actor.varexists(discount)%)
  eval cost 10000
else
  eval cost 5000
end
if (!%self.varexists(done)%)
if (!%self.varexists(smithc)% && %actor.varexists(founreceive)% && !%actor.varexists(discount)%)
wait 1
%echoaround% %actor% %self.name% looks up at %actor.name% sadly.
%send% %actor% %self.name% looks up at you sadly.
wait 1 s
say I am sorry, %actor.name%, your ore was lost to a thief.
rdelete founreceive %actor.id%
wait 2 s
say I will give you a discount on your next job, you will only have to pay half.
set discount
remote discount %actor.id%
elseif (!%self.varexists(smithc)% && %actor.varexists(founreceive)% && %actor.varexists(discount)%)
wait 1
%echoaround% %actor% %self.name% looks up at %actor.name% sadly.
%send% %actor% %self.name% looks up at you sadly.
wait 1 s
say I am sorry, %actor.name%, your ore was lost to a thief.
rdelete founreceive %actor.id%
wait 2 s
say I already promised a discount, I have to stay in business. The theft hurts me too..
rdelete founreceive %actor.id%
elseif (%self.varexists(smithc)% && !%actor.varexists(founreceive)%)
wait 1
%echo% %self.name% beats a piece of metal on the anvil carefully.
wait 1 s
%echo% Sparks cascade to the floor with each strike of the hammer.
wait 3 s
%echoaround% %actor% %self.name% looks up at %actor.name% and smiles.
%send% %actor% %self.name% looks up at you and smiles.
wait 4 s
say Sorry I am busy working on something. You will have to try back later.
elseif (%self.varexists(smithc)% && %actor.varexists(founreceive)%)
wait 1
%echo% %self.name% beats a piece of metal on the anvil carefully.
wait 1 s
%echo% Sparks cascade to the floor with each strike of the hammer.
wait 3 s
%echoaround% %actor% %self.name% looks up at %actor.name% and smiles.
%send% %actor% %self.name% looks up at you and smiles.
wait 4 s
say I haven't finished your item yet %actor.name%. Try back later.
elseif (!%actor.varexists(founreceive)%)
wait 1
%echo% %self.name% beats a piece of metal on the anvil carefully.
wait 1 s
%echo% Sparks cascade to the floor with each strike of the hammer.
wait 3 s
%echoaround% %actor% %self.name% looks up at %actor.name% and nods.
%send% %actor% %self.name% looks up at you and nods.
wait 1 s
say You have a job for me, or are you just visiting?
wait 3 s
say I have to say I only work in high quality silver right now.
wait 5 s
say I am concentrating on honing my silver-smithing.
wait 3 s
say If you happen to get a hold of Fullmoon Silver Ore, I can make it into something nice.
wait 4 s
say The ore is sometimes found in the Silver Mine. It is the purest form of silver that can be mined.
wait 5 s
say I will either make you a ring or an earring. It depends on my inspiration... That's just how I work.
wait 4 s
say I will need to be paid %cost% zenni for the work, so make sure to have the cash.
else
  wait 1
  say Ah, %actor.name%, I have finished the job you gave me. Though you still need to pay me %cost% zenni.
end
else
 if (%actor.varexists(founreceive)%)
  wait 1
  say Ah, %actor.name%, I have finished the job you gave me. Though you still need to pay me %cost% zenni.
 else
  wait 1
  say I can't take any work right now, I am busy working for someone else.
  wait 2 s
  say Try back later.
 end
end
~
#2014
Foun Smith Receive~
0 j 100
~
if (%actor.varexists(founreceive)% && %self.varexists(smithc)%)
return 0
wait 1
%echoaround% %actor% %self.name% refuses %obj.shortdesc% from %actor.name%.
%send% %actor% %self.name% refuses %obj.shortdesc% from you.
wait 1 s
say Uh, %actor.name%, I am already working on a job for you.
elseif (%actor.varexists(founreceive)%)
return 0
wait 1
%echoaround% %actor% %self.name% refuses %obj.shortdesc% from %actor.name%.
%send% %actor% %self.name% refuses %obj.shortdesc% from you.
wait 1 s
say Uh, %actor.name%, pay me for the last job first.
elseif (%object.vnum% != 2009)
return 0
wait 1
%echoaround% %actor% %self.name% refuses %object.shortdesc% from %actor.name%.
%send% %actor% %self.name% refuses %object.shortdesc% from you.
wait 1 s
say That isn't what I will work on, so don't waste my time.
else
wait 1
%echoaround% %actor% %self.name% receives %object.shortdesc% from %actor.name%.
%send% %actor% %self.name% receives %object.shortdesc% from you.
%purge% %object%
wait 1 s
say I will begin work immediately. It won't be finished for two hours though.
wait 3 s
say I will inform you when it is ready, you can pay me when it is finished.
wait 2 s
say So feel free to go about your business elsewhere if you like.
set founreceive
remote founreceive %actor.id%
 if (%time.hour% <= 21)
  eval tme %time.hour% + 2
 elseif (%time.hour% == 22)
  eval tme 0
 elseif (%time.hour% == 23)
  eval tme 1
 end
set smithc %tme%
remote smithc %self.id%
set workfor %actor.name%
remote workfor %self.id%
end
~
#2015
Foun Smith Bribe~
0 m 1
~
 if (!%actor.varexists(discount)%)
  eval cost 10000
 else
  eval cost 5000
 end
if (!%actor.varexists(founreceive)%)
 return 0
 wait 1
 say I have not done any work for you, why pay me?
else
 if (%self.varexists(smithc)%)
  return 0
  wait 1
  say Wait till I finish the job for you. I will tell you when it is ready.
  wait 3 s
  say I like to be professional.
 elseif (%amount% < %cost%)
  return 0
  wait 1
  eval short %cost% - %amount%
  say That is not enough. You are short by %short% zenni.
 elseif (%amount% > %cost%)
  wait 1
  say Paying me more after I gave you a discount? Alright.
  wait 2 s
  say Alright here you go, just like I promised.
  if (%random.2% == 2)
   %load% obj 2012
  else
   %load% obj 2011
  end
  give fullmoon %actor.name%
  rdelete founreceive %actor.id%
  rdelete done %self.id%
  rdelete discount %actor.id%
 elseif (%amount% == %cost%)
  wait 1
  say Thanks, %actor.name%.
  wait 2 s
  say Next time I will charge full price I hope you know.
  wait 2 s
  say Alright here you go, just like I promised.
  if (%random.2% == 2)
   %load% obj 2012
  else
   %load% obj 2011
  end
  give fullmoon %actor.name%
  rdelete founreceive %actor.id%
  rdelete done %self.id%
  redelete discount %actor.id%
 end
end
~
#2016
Foun Time Trigger~
0 ab 100
~
if (%self.varexists(smithc)%)
 if (%self.smithc% == %time.hour%)
  rdelete smithc %self.id%
  tell %self.workfor% I have finished my work, come and pay me to receive the product.
  rdelete workfor %self.id%
  set done
  remote done %self.id%
 end
end
~
#2017
Load VR World~
2 c 100
Load~
if (!%arg%)
wait 1
%send% %actor% Load what?
%send% %actor% --------------
%send% %actor% Zelda
%send% %actor% Mario
%send% %actor% Megaman
%send% %actor% --------------
%echoaround% %actor% %actor.name% looks at the load menu.
elseif (%game_loaded% && %game_loaded% != none)
   if (%game_time% == 0)
    set tme Midnight
   elseif (%game_time% == 13)
    set tme 1 PM
   elseif (%game_time% == 14)
    set tme 2 PM
   elseif (%game_time% == 15)
    set tme 3 PM
   elseif (%game_time% == 16)
    set tme 4 PM
   elseif (%game_time% == 17)
    set tme 5 PM
   elseif (%game_time% == 18)
    set tme 6 PM
   elseif (%game_time% == 19)
    set tme 7 PM
   elseif (%game_time% == 20)
    set tme 8 PM
   elseif (%game_time% == 21)
    set tme 9 PM
   elseif (%game_time% == 22)
    set tme 10 PM
   elseif (%game_time% == 23)
    set tme 11 PM
   else
    set tme %game_time% AM
   end
   wait 1
   %send% %actor% There is already a game loaded, try 'enter game' to play.
   %send% %actor% The current game is %game_loaded% and will be on until %tme%.
else
 if (%arg% == Zelda)
   wait 1
   %send% %actor% You push the button labled "Zelda" on the screen.
   %echoaround% %actor% %actor.name% pushes the button labled "Zelda" on the screen.
   wait 1 s
   %echo% Through the window the old dusty NES opens as a mechanical arms places the Zelda game in it.
   wait 3 s
   %echo% The mechanical arm pushes the power button to the NES.
   wait 4 s
   %echo% The power light flashes on and off, and nothing else happens.
   wait 5 s
   %echo% The mechanical arm raises above the NES and forms a fist, than slams that fist into the top of the NES.
   wait 3 s
   %echo% The NES starts working right.
   wait 1
   set game_loaded Zelda
   global game_loaded
   if (%time.hour% <= 17)
    eval timy %time.hour% + 6
    set game_time %timy%
    global game_time
   elseif (%time.hour% == 18)
    set game_time 0
    global game_time
   elseif (%time.hour% == 19)
    set game_time 1
    global game_time
   elseif (%time.hour% == 20)
    set game_time 2
    global game_time
   elseif (%time.hour% == 21)
    set game_time 3
    global game_time
   elseif (%time.hour% == 22)
    set game_time 4
    global game_time
   elseif (%time.hour% == 23)
    set game_time 5
    global game_time
   end
    eval count 100
    while %count% > 0
     eval num 2074 + %random.50%
     eval num2 2015 + %random.3%
     if (%random.2% == 2)
      eval num2 2016
     end
     %at% %num% %load% mob %num2%
     eval count %count% - 1
    done
    eval rrm 2074 + %random.50%
    %at% %rrm% %load% obj 2107
    eval rrm 2074 + %random.50%
    %at% %rrm% %load% obj 2108
    eval rrm 2074 + %random.50%
    %at% %rrm% %load% obj 2108
    eval rrm 2074 + %random.50%
    %at% %rrm% %load% obj 2108
   %echo% The game has started, you may enter it with "enter game" as long as it is still running.
 elseif (%arg% == Mario)
   wait 1
   %send% %actor% You push the button labled "Mario" on the screen.
   %echoaround% %actor% %actor.name% pushes the button labled "Mario" on the screen.
   wait 1 s
   %echo% Through the window the old dusty NES opens as a mechanical arms places the Mario game in it.
   wait 3 s
   %echo% The mechanical arm pushes the power button to the NES.
   wait 4 s
   %echo% The power light flashes on and off, and nothing else happens.
   wait 5 s
   %echo% The mechanical arm raises above the NES and forms a fist, than slams that fist into the top of the NES.
   wait 3 s
   %echo% The NES starts working right.
   wait 1
   set game_loaded Mario
   global game_loaded
   if (%time.hour% <= 17)
    eval timy %time.hour% + 6
    set game_time %timy%
    global game_time
   elseif (%time.hour% == 18)
    set game_time 0
    global game_time
   elseif (%time.hour% == 19)
    set game_time 1
    global game_time
   elseif (%time.hour% == 20)
    set game_time 2
    global game_time
   elseif (%time.hour% == 21)
    set game_time 3
    global game_time
   elseif (%time.hour% == 22)
    set game_time 4
    global game_time
   elseif (%time.hour% == 23)
    set game_time 5
    global game_time
   end
    eval count 100
    while %count% > 0
     eval num 2074 + %random.50%
     eval num2 2018 + %random.3%
     if (%random.2% == 2)
      eval num2 2019
     end
     %at% %num% %load% mob %num2%
     eval count %count% - 1
    done
    eval rrm 2074 + %random.50%
    %at% %rrm% %load% obj 2110
    eval rrm 2074 + %random.50%
    %at% %rrm% %load% obj 2111
    eval rrm 2074 + %random.50%
    %at% %rrm% %load% obj 2111
    eval rrm 2074 + %random.50%
    %at% %rrm% %load% obj 2111
    eval rrm 2074 + %random.50%
    %at% %rrm% %load% obj 2111
   %echo% The game has started, you may enter it with "enter game" as long as it is still running.
 elseif (%arg% == Megaman)
   wait 1
   %send% %actor% You push the button labled "Megaman" on the screen.
   %echoaround% %actor% %actor.name% pushes the button labled "Megaman" on the screen.
   wait 1 s
   %echo% Through the window the old dusty NES opens as a mechanical arms places the Mario game in it.
   wait 3 s
   %echo% The mechanical arm pushes the power button to the NES.
   wait 4 s
   %echo% The power light flashes on and off, and nothing else happens.
   wait 5 s
   %echo% The mechanical arm raises above the NES and forms a fist, than slams that fist into the top of the NES.
   wait 3 s
   %echo% The NES starts working right.
   wait 1
   set game_loaded Megaman
   global game_loaded
   if (%time.hour% <= 17)
    eval timy %time.hour% + 6
    set game_time %timy%
    global game_time
   elseif (%time.hour% == 18)
    set game_time 0
    global game_time
   elseif (%time.hour% == 19)
    set game_time 1
    global game_time
   elseif (%time.hour% == 20)
    set game_time 2
    global game_time
   elseif (%time.hour% == 21)
    set game_time 3
    global game_time
   elseif (%time.hour% == 22)
    set game_time 4
    global game_time
   elseif (%time.hour% == 23)
    set game_time 5
    global game_time
   end
   set someit none
   global someit
   %echo% The game has started, you may enter it with "enter game" as long as it is still running.
 else
 wait 1
 %echoaround% %actor% %actor.name% presses an empty spot on the menu.
 %send% %actor% You press an empty spot on the menu where you imagine that choice might be...
 end
end
~
#2018
VR World Running~
2 ab 100
~
if (!%game_loaded% || %game_loaded% == none)
 eval cnt 2074
 while (!%fin% || %fin% != finished)
  eval cnt %cnt% + 1
  if (%cnt% < 2125)
   if (%people.%cnt%% > 0)
    %at% %cnt% %purge%
    %at% %cnt% %teleport% all 2126
   end
  else
   set fin finished
  end
 done
else
if (!%warn% || %warn% != warned)
   if (%game_time% - 1 == %time.hour%)
    set ann 1
   elseif (%game_time% == 0 && %time.hour% == 23)
    set ann 1
   end
 if (%ann% && %ann% == 1)
   wait 1
   eval count 0
   eval rrm 2074
   while (%count% < 50)
    eval count %count% + 1
    eval rrm %rrm% + 1
    %at% %rrm% %echo% @RThe game will be ending in one hour!@n
   done
   set warn warned
   global warn
 end
 elseif (%game_time% == %time.hour%)
   eval count 0
   eval rrm 2074
   while (%count% < 50)
    eval count %count% + 1
    eval rrm %rrm% + 1
    %at% %rrm% %purge%
    %at% %rrm% %teleport% all 2126
   done
   if (%people.2126% < 3)
    set game_loaded none
    global game_loaded
    set nowinner none
   else
    set target_char %self.people%
    set top_score 0
    while %target_char%
     set tmp_target %target_char.next_in_room%
      if (!%target_char.varexists(points)%)
       set target_char %tmp_target%
      elseif (%target_char.points% > %top_score%)
       set top_score %target_char.points%
       set top_pla %target_char%
       set target_char %tmp_target%
      elseif (%target_char.points% > %rup_score%)
       set rup_score %target_char.points%
       set rup_pla %target_char%
       set target_char %tmp_target%
      elseif (%target_char%)
       %send% %target_char% You didn't win, but you receive a consolation prize.
       %send% %target_char% You get 100 zenni in the bank!
       set %actor.bank(100)
       set target_char %tmp_target%
      else
       set target_char %tmp_target%
      end
    done
    if (%nowinner% != none && !%nowinner%)
    if (%top_pla%)
    %send% %top_pla% You are the winner of the last round!
    %send% %top_pla% You receive the NES %game_loaded% trophy and 800 zenni in the bank!
    set %top_pla.bank(800)%
     if (%game_loaded% == Zelda)
      %load% obj 2100
      %force% %top_pla% get trophy
     elseif (%game_loaded% == Mario)
      %load% obj 2101
      %force% %top_pla% get trophy
     elseif (%game_loaded% == Megaman)
      %load% obj 2102
      %force% %top_pla% get trophy
     end
    end
    if (%rup_pla%)
    %send% %rup_pla% You are the runner-up winner of the last round!
    set %rup_pla.bank(500)%
     if (%game_loaded% == Zelda)
      %send% %rup_pla% You receive a Rupee and 500 zenni in the bank!
      %load% obj 2103
      %force% %rup_pla% get rupee
     elseif (%game_loaded% == Mario)
      %send% %rup_pla% You receive a Super Mushroom and 500 zenni in the bank!
      %load% obj 2104
      %force% %rup_pla% get mushroom
     elseif (%game_loaded% == Megaman)
      %send% %rup_pla% You receive an Energy Can and 500 zenni in the bank!
      %load% obj 2105
      %force% %rup_pla% get energy
     end
    end
    end
    if (%people.2126% < 3)
     %echo% There was not enough players last round for anyone to win. There needs to be at least 3.
    else
     wait 1 s
     %echo% The game is over, last rounds winners were; First Place: %top_pla.name% with (%top_score%) points and Runner-up: %rup_pla.name% with (%rup_score%) points.
    end
    set game_loaded none
    global game_loaded
    set game_time -1
    global game_time
   end
 end
end
~
#2019
VR World Enter~
2 c 100
enter~
if (%arg% == game)
 if (!%game_loaded% || %game_loaded% == none)
  wait 1
  %send% %actor% There isn't a game loaded.
 elseif (%actor.gold% < 50)
  wait 1
  %send% %actor% It costs 50 zenni to enter the game.
 else
  set %actor.gold(-50)%
  if (%game_time% == 0)
   eval tme 23
  else
   eval tme %game_time% - 1
  end
  if (%tme% == %time.hour%)
   wait 1
   %send% %actor% The game will be ending shortly, there is no point in joining.
  elseif (%game_loaded% == Zelda)
   set points 0
   remote points %actor.id%
   set life 3
   remote life %actor.id%
   wait 1
   %send% %actor% You enter the game!
   %echoaround% %actor% %actor.name% enters %game_loaded% game currently running.
   set cnt 2074
   while (!%fin% || %fin% != finished)
    eval cnt %cnt% + 1
    if (%cnt% < 2125)
     %at% %cnt% %echo% @Y%actor.name% has entered the %game_loaded% game.@n
    else
     set fin finished
    end
   done
   eval num %random.4%
    switch %num%
     case 1
      %teleport% %actor% 2075
     break
     case 2
      %teleport% %actor% 2088
     break
     case 3
      %teleport% %actor% 2109
     break
     default
      %teleport% %actor% 2097
     break
    done
  %force% %actor% look
  wait 1
  %at% %actor% %load% obj 2106
  %force% %actor% get sword
   eval count 50
    while %count% > 0
     eval num 2074 + %random.50%
     eval num2 2015 + %random.3%
     %at% %num% %load% mob %num2%
     eval count %count% - 1
    done
  elseif (%game_loaded% == Mario)
   set points 0
   remote points %actor.id%
   wait 1
   %send% %actor% You enter the game!
   %echoaround% %actor% %actor.name% enters %game_loaded% game currently running.
   set cnt 2074
   while (!%fin% || %fin% != finished)
    eval cnt %cnt% + 1
    if (%cnt% < 2125)
     %at% %cnt% %echo% @Y%actor.name% has entered the %game_loaded% game.@n
    else
     set fin finished
    end
   done
   eval num %random.4%
    switch %num%
     case 1
      %teleport% %actor% 2075
     break
     case 2
      %teleport% %actor% 2088
     break
     case 3
      %teleport% %actor% 2109
     break
     default
      %teleport% %actor% 2097
     break
    done
  %force% %actor% look
  wait 1
  %at% %actor% %load% obj 2109
  %force% %actor% get hat
   eval count 50
    while %count% > 0
     eval num 2074 + %random.50%
     eval num2 2018 + %random.3%
     if (%random.2% == 2)
      eval num2 2019
     end
     %at% %num% %load% mob %num2%
     eval count %count% - 1
    done
  elseif (%game_loaded% == Megaman)
   set points 0
   remote points %actor.id%
   wait 1
   %send% %actor% You enter the game!
   %echoaround% %actor% %actor.name% enters %game_loaded% game currently running.
   set cnt 2074
   if (%actor.varexists(tagged)%)
    rdelete tagged %actor.id%
   end
   if (%someit% == none && %random.2% == 2)
    set someit yes
    global someit
    set tagged
    remote tagged %actor.id%
   end
   while (!%fin% || %fin% != finished)
    eval cnt %cnt% + 1
    if (%cnt% < 2125)
     %at% %cnt% %echo% @Y%actor.name% has entered the %game_loaded% game.@n
     if (%actor.varexists(tagged)%)
      %at% %cnt% %echo% @R%actor.name% is it!@n
     end
    else
     set fin finished
    end
   done
   eval num %random.4%
    switch %num%
     case 1
      %teleport% %actor% 2075
     break
     case 2
      %teleport% %actor% 2088
     break
     case 3
      %teleport% %actor% 2109
     break
     default
      %teleport% %actor% 2097
     break
    done
  %force% %actor% look
  wait 1
  %at% %actor% %load% obj 2112
  %force% %actor% get cannon
  if (%actor.varexists(tagged)%)
   wait 1
   %send% %actor% @RYou are it!@n
  end
  else
   %send% %actor% There isn't a game loaded.
  end
 end
else
%send% %actor% Try enter game.
end
~
#2020
VR World Game Master Load~
2 c 100
gtime~
if (!%game_time% || %game_time% == -1)
 wait 1
 %send% %actor% There is currently no game running.
else
 if (%game_time% == 0)
  set tme Midnight
 elseif (%game_time% >= 13)
  eval adj %game_time% - 12
  set tme %adj% PM
 elseif (%game_time% < 12)
  set tme %game_time% AM
 elseif (%game_time% == 12)
  set tme Noon
 end
 wait 1
 %send% %actor% The current game is, %game_loaded%, and it will be ending at %tme%.
end
~
#2021
VR World Sword~
1 c 3
slash~
If (%self.room.vnum% < 2075 || %self.room.vnum% > 2124)
 return 0
 wait 1
 %purge% %self%
else
   set target_char %actor.room.people%
    while %target_char%
     set tmp_target %target_char.next_in_room%
     if (%target_char.name% /= %arg%)
      if (%target_char% == %actor%)
       wait 1
       %send% %actor% What good would slashing yourself do?
       unset target_char
      elseif (%target_char.is_pc%)
       wait 1
       %send% %actor% You slash at %target_char.name%!
       %send% %target_char% %actor.name% slashes at you!
       %echo% There is battle between %actor.name% and %target_char.name%!       
       set found 1
       wait 1
       eval num %random.3%
       switch %num%
        case 1
         if (%target_char.life% > 1)
          eval loss %target_char.life% - 1
          %send% %target_char% ...and manages to hit you! You now have %loss% hearts left!
          %echoaround% %target_char% ...and manages to hit! %target_char.name% now has %loss% hearts left!
          set life %loss%
          remote life %target_char.id%
         else
          eval gain %target_char.points% / 4
          eval loss %target_char.points% - %gain%
          eval tgain %actor.points% + %gain%
          %send% %target_char% ...and manages to hit you with the killing blow! You have lost %loss% rupees!
          %echoaround% %target_char% ...and manages to hit with the killing blow!
          %send% %actor% You gain (%gain%) rupees!
          set points %tgain%
          remote points %actor.id%
          set points %loss%
          remote points %target_char.id%
          set life 3
          remote life %target_char.id%
          eval ran %random.4%
           if (%ran% == 1)
            %teleport% %target_char% 2075
           elseif (%ran% == 2)
            %teleport% %target_char% 2088
           elseif (%ran% == 3)
            %teleport% %target_char% 2097
           else
            %teleport% %target_char% 2109
           end
           wait 1 s
         end
        break
        case 2
          %send% %target_char% ...and you manage to dodge!
          %echoaround% %target_char% ...and %target_char.name% manages to dodge!
          wait 1 s
        break
        case 3
          %send% %target_char% ...and you manage to parry!
          %echoaround% %target_char% ...and %target_char.name% manages to parry the slash with %target_char.hisher% own sword!
          wait 1 s
        break
       done
      unset target_char
      else
       wait 1
       %send% %actor% You slash at %target_char.name%!
       %echoaround% %actor% %actor.name% slashes at %target_char.name%!
       set found 1
       wait 1
       eval num %random.2%
       switch %num%
        case 1
         %send% %actor% ...and manage to hit! %target_char.name% dies!
         eval ran %random.10%
         if (%target_char.vnum% == 2016)
          if (%ran% >= 9)
           eval gain 20
          elseif (%ran% >= 6)
           eval gain 5
          elseif (%actor.life% < 3)
           eval gain -1
          else
           eval gain 1
          end
         elseif (%target_char.vnum% == 2017)
          if (%ran% >= 7)
           eval gain 20
          elseif (%ran% >= 4)
           eval gain 5
          elseif (%actor.life% < 3)
           eval gain -1
          else
           eval gain 1
          end
         elseif (%target_char.vnum% == 2018)
          if (%ran% >= 6)
           eval gain 20
          elseif (%ran% >= 3)
           eval gain 5
          elseif (%actor.life% < 3)
           eval gain -1
          else
           eval gain 1
          end
         end
         if (%gain% > 0)
         %send% %actor% You gain (%gain%) rupees!
         %echoaround% %actor% ...and manages to hit! %target_char.name% dies!
         eval gain %gain% + %actor.points%
         set points %gain%
         remote points %actor.id%
         else
         %send% %actor% You get a heart!
         %echoaround% %actor% ...and manages to hit! %target_char.name% dies!
         eval gain %actor.life% + 1
         set life %gain%
         remote life %actor.id%
         end
         if (%random.2% == 2)
          eval rrm 2074 + %random.50%
          %at% %rrm% %load% mob %target_char.vnum%
         end
         %purge% %target_char%
        break
        case 2
         %send% %actor% ...and miss! %target_char.name% scurries out of the way!
         %echoaround% %actor% ...and misses! %target_char.name% scurries out of the way!
         wait 1 s
        break
       done
      unset target_char
      end
     else
     set target_char %tmp_target%
     end
    done
   if (!%found%)
    %send% %actor% Slash at who?
   end
end
~
#2022
VR World Item Purge~
1 b 100
~
If (%self.room.vnum% < 2075 || %self.room.vnum% > 2124)
 wait 1
 if (%self.carried_by%)
  set user %self.carried_by%
  if (%user.varexists(points)%)
  rdelete points %user.id%
  end
  if (%user.varexists(life)%)
  rdelete life %user.id%
  end
 end
 %purge% %self%
elseif (%self.vnum% == 2112)
 if (%self.carried_by%)
  set user %self.carried_by%
   if (%user.varexists(tagged)%)
    eval gain %user.points% - 10
    if (%gain% > 0)
     %send% %user% -10 points...
     set points %gain%
     remote points %user.id%
    else
     set points 0
     remote points %user.id%
     %send% %user% Your points will never get higher while you are it.
    end
   else
    eval gain %user.points% + 10
    set points %gain%
    remote points %user.id%
    %send% %user% +10 points.
   end
 end
end
~
#2023
Zelda Game Boomerang~
1 c 3
throw~
If (%actor.room.vnum% < 2075 || %actor.room.vnum% > 2125)
 return 0
 wait 1
 %purge% %self%
else
   set oroom %actor.room.vnum%
   eval rroom 2074
   eval broom 2074 + %random.50%
   while (!%targ% && %dne% != done)
    eval rroom %rroom% + 1
    if (%rroom% < 2125)
    %teleport% %actor% %rroom%
    set target_char %actor.room.people%
     while %target_char%
      set tmp_target %target_char.next_in_room%
     if (!%target_char.is_pc%)
      set target_char %tmp_target%
     elseif (%target_char.name% /= %arg%)
      set targ %target_char%
      unset target_char
     else
      set target_char %tmp_target%      
     end
    done
    else
     %teleport% %actor% %oroom%  
     set dne done
    end
   done
  if (%targ%)
   %teleport% %actor% %oroom%  
   if (%targ.room.vnum% == %actor.room.vnum%)
    wait 1
    %send% %actor% They are in the same room! Try it from further away.
   else
    wait 1
    %send% %actor% You throw the boomerang and watch as it flies into the distance!
    %echoaround% %actor% %actor.name% Throws the boomerang and watches as it flies into the distance!
    wait 2 s
    switch %random.2%
     case 1
     eval score %targ.points% / 4
     eval loss %targ.points% - %score%
     set points %loss%
     remote points %targ.id%
     %send% %actor% The boomerang hits! %targ.name% loses %score% rupees!
     %send% %targ% A boomerang comes chopping into the room and slams into you, knocking %score% rupees out of your pocket!
     %echoaround% %targ% A boomerang comes chopping into the room and slams into %targ.name%, knocking %score% rupees out of %targ.hisher% pocket!
     %at% %broom% %load% obj %self.vnum%
     %purge% %self%
     break
     case 2
     %send% %actor% The boomerang misses! Dang...
     %send% %targ% A boomerang comes chopping into the room but you manage to avoid it!
     %echoaround% %targ% A boomerang comes chopping into the room at %targ.name%, but %targ.heshe% avoids it!
     %at% %broom% %load% obj %self.vnum%
     %purge% %self%
     break
    done
   end
  else
   wait 1
   %send% %actor% That person is not playing the game!
  end
end
~
#2024
Zelda Game Bomb~
1 c 3
toss~
If (%actor.room.vnum% < 2075 || %actor.room.vnum% > 2125)
 return 0
 wait 1
 %purge% %self%
else
 set target_char %actor.room.people%
 wait 1
 %send% %actor% You toss a lit bomb into the room and take cover!
 %echoaround% %actor% %actor.name% tosses a lit bomb into the room!
 eval count 0
 while %target_char%
  set tmp_target %target_char.next_in_room%
  if (!%target_char.is_pc%)
   %echoaround% %target_char% %target_char.name% is caught in the explosion and killed!
    switch %random.2%
     case 1
      eval score 1
      %send% %actor% You pick up %score% rupees!
      eval gain %actor.points% + %score%
      set points %gain%
      remote points %actor.id%
     break
     default
      eval score 5
      %send% %actor% You pick up %score% rupees!
      eval gain %actor.points% + %score%
      set points %gain%
     break
    done
   if (%random.2% == 2)
    eval rrm 2074 + %random.50%
    %at% %rrm% %load% mob %target_char.vnum%
   end
   %purge% %target_char%
   set target_char %tmp_target%
   eval count %count% + 1
  elseif (%target_char% != %actor% && %random.3% >= 2)
   eval score %target_char.points% / 6
   eval loss %target_char.points% - %score%
   set points %loss%
   remote points %target_char.id%
   %send% %target_char% You are caught in the explosion and drop %score% rupees!
   %echoaround% %target_char% %target_char.name% is caught in the explosion!
   %send% %actor% You pick up %score% rupees!
   eval gain %actor.points% + %score%
   set points %gain%
   remote points %actor.id%
   set target_char %tmp_target%
   eval count %count% + 1
  else
   set target_char %tmp_target%
  end
 done
 if (%count% < 1)
  wait 1 s
  %echo% Everyone avoided the explosion!
  eval rroom 2074 + %random.50%
  %at% %rroom% %load% obj %self.vnum%
  %purge% %self%
 else
  wait 1 s
  %echo% The flames and heat subside...
  eval rroom 2074 + %random.50%
  %at% %rroom% %load% obj %self.vnum%
  %purge% %self%
 end
end
~
#2025
Mario Game Stomp~
1 c 3
stomp~
If (%actor.room.vnum% < 2075 || %actor.room.vnum% > 2125)
 return 0
 wait 1
 %purge% %self%
else
 set target_char %actor.room.people%
 while %target_char%
  set tmp_target %target_char.next_in_room%
  if (%target_char.is_pc%)
   set target_char %tmp_target%
  elseif (%target_char.name% /= %arg%)
   set targ %target_char%
   unset target_char
  else
   set target_char %tmp_target%
  end
 done
 if (!%targ%)
  wait 1
  %send% %actor% Who do you want to stomp?
 elseif (%target_char.varexists(shelled)%)
  wait 1
  %send% %actor% You need to 'kick' a shelled enemy!
 else
  wait 1
  %send% %actor% You leap into the air and aim to stomp on %targ.name%!
  %echoaround% %actor% %actor.name% leaps into the air and aims to stomp on %targ.name%!
  wait 1 s
  switch %random.3%
   case 1
   case 2
    if (%targ.vnum% == 2019)
     %send% %actor% You smash down on %targ.name%, and lay them flat!
     %send% %actor% 25 Points!
     %echoaround% %actor% %actor.name% smashes %targ.name%, and lays them flat!
     eval gain %actor.points% + 25
     set points %gain%
     remote points %actor.id%
     if (%random.2% == 2)
      eval rrm 2074 + %random.50%
      %at% %rrm% %load% mob %targ.vnum%
     end
     %purge% %targ%
    else
     %send% %actor% You smash down on %targ.name%, and force them into their shell!
     set %targ.pos(Sitting)%
      if (%targ.vnum% == 2020)
       eval score 50
       %send% %actor% %score% Points!
       %force% %targ% mtransform 2022
       set shelled
       remote shelled %targ.id%
      else
       eval score 100
       %send% %actor% %score% Points!
       %force% %targ% mtransform 2023
       set shelled
       remote shelled %targ.id%
      end
     %echoaround% %actor% %actor.name% smashes down on %targ.name%, and forces them into their shell!
     eval gain %actor.points% + %score%
     set points %gain%
     remote points %actor.id%
    end
   break
   case 3
    %send% %actor% You miss! How unlucky!
    %echoaround% %actor% %actor.name% misses! How unlucky!
   break
  done
 end
end
~
#2026
Mario Game Kick~
1 c 3
kick~
If (%actor.room.vnum% < 2075 || %actor.room.vnum% > 2125)
 return 0
 wait 1
 %purge% %self%
else
 set target_char %actor.room.people%
 while %target_char%
  set tmp_target %target_char.next_in_room%
  if (%target_char.is_pc%)
   set target_char %tmp_target%
  elseif (%target_char.name% /= %arg%)
   set targ %target_char%
   unset target_char
  else
   set target_char %tmp_target%
  end
 done
 if (!%targ%)
  wait 1
  %send% %actor% Kick who?
 elseif (!%targ.varexists(shelled)%)
  wait 1
  %send% %actor% That target isn't in its shell!
 else
  wait 1
  set target_char %actor.room.people%
  %echoaround% %actor% %actor.name% runs and kicks %targ.name% sending it sliding!
  %send% %actor% You run and kick %targ.name% sending it sliding!
  eval count 0
  wait 1
  while %target_char%
   set tmp_target %target_char.next_in_room%
   if (%target_char% == %actor%)
    set target_char %tmp_target%
   elseif (%target_char% == %targ%)
    set target_char %tmp_target%
   elseif (%target_char.is_pc%)
    set target_char %tmp_target%
   else
    if (!%mult%)
     eval mult 50
    else
     eval mult %mult% + 50
    end
    eval count %count% + 1
    %echo% %target_char.name% gets hit by the shell and falls off screen!
    if (%random.2% == 2)
     eval rrm 2074 + %random.50%
     %at% %rrm% %load% mob %target_char.vnum%
    end
    %send% %actor% %mult% points!
    %purge% %target_char%
    set target_char %tmp_target%
   end 
  done 
  if (%count% < 1)
  wait 1
  %send% %actor% Nothing was hit, and the shell is now off screen.
  %echoaround% %actor% %actor.name% watches as the kicked shell slides off into the distance.
   %purge% %targ%
  else
  wait 1
  eval gain %actor.points% + %mult%
  set points %gain%
  remote points %actor.id%
  %send% %actor% You watch as the shell slides off into the distance.
  %echoaround% %actor% %actor.name% watches as the kicked shell slides off into the distance.
   if (%count% > 10)
    %send% %actor% You get a bonus of 1000 points!
    eval gain %actor.points% + 1000
    set points %gain%
    remote points %actor.id%
   elseif (%count% > 5)
    %send% %actor% You get a bonus of 500 points!
    eval gain %actor.points% + 500
    set points %gain%
    remote points %actor.id%
   end
   %purge% %targ%
  end
 end  
end
~
#2027
Mario Game Grab~
1 c 3
grab~
If (%actor.room.vnum% < 2075 || %actor.room.vnum% > 2125)
 return 0
 wait 1
 %purge% %self%
else
 if (!%actor.room.contents(2110)%)
  wait 1
  %send% %actor% There is no flag in this room!
 else
  set flag %actor.room.contents(2110)%
  wait 1
  %send% %actor% You leap up and grab the flag!
  %echoaround% %actor% %actor.name% leaps up and grabs the flag!
   switch %random.4%
    case 1
     eval score 800
    break
    case 2
     eval score 5000
    break
    default
     eval score 2000
    break
   done
  %send% %actor% %score% Points!
  eval gain %actor.points% + %score%
  set points %gain%
  remote points %actor.id%
  %purge% %flag%
  eval rrm 2074 + %random.50%
  %at% %rrm% %load% obj 2110
 end
end
~
#2028
Mario Game Bash~
1 c 3
bash~
If (%actor.room.vnum% < 2075 || %actor.room.vnum% > 2125)
 return 0
 wait 1
 %purge% %self%
else
 if (!%actor.room.contents(2111)%)
  wait 1
  %send% %actor% There is no box in this room!
 else
  set box %actor.room.contents(2111)%
  wait 1
  %send% %actor% You leap up and bash the coin box from underneath!
  %echoaround% %actor% %actor.name% leaps up and bashes the coin box from underneath!
   switch %random.4%
    case 1
     eval score 100
    break
    case 2
     eval score 500
    break
    default
     eval score 200
    break
   done
  %send% %actor% %score% Points!
  eval gain %actor.points% + %score%
  set points %gain%
  remote points %actor.id%
  %purge% %box%
  eval rrm 2074 + %random.50%
  %at% %rrm% %load% obj 2111
 end
end
~
#2029
Mario Game Become Unshelled~
0 ab 25
~
if (%self.varexists(shelled)%)
 wait 1
 %echo% %self.name% pops out of their shell!
 rdelete shelled %self.id%
  if (%self.vnum% == 2020)
   mtransform 2020
  else
   mtransform 2021
  end
 stand
end
~
#2030
VR World HUD~
1 c 3
hud~
if (%actor.room.vnum% < 2075 || %actor.room.vnum% > 2125)
 return 0
 wait 1
 %purge% %self%
else
 if (%self.vnum% == 2106)
  wait 1
  %send% %actor% Rupees: %actor.points%
  %send% %actor% Hearts: %actor.life%
 elseif (%self.vnum% == 2109)
  wait 1
  %send% %actor% Points: %actor.points%
 else
  wait 1
  %send% %actor% Points: %actor.points%
  if (%actor.varexists(tagged)%)
   %send% %actor% @RYou are it. Tag someone!@n
  else
   %send% %actor% @BYou are NOT it. Hide!@n
  end
 end
end
~
#2031
Megaman Tag Trigger~
1 c 3
tag~
If (%self.room.vnum% < 2075 || %self.room.vnum% > 2125)
 return 0
 wait 1
 %purge% self
else
  wait 1
  %echoaround% %actor% %actor.name% raises %actor.hisher% armcannon and fires...
  %send% %actor% You raise your arm cannon and fire...
  wait 1
  set target_char %actor.room.people%
  while %target_char%
   set tmp_target %target_char.next_in_room%
   if (%target_char.name% /= %arg%)
    set targ %target_char%
    unset target_char
   else
    set target_char %tmp_target%
   end
  done
  if (!%actor.varexists(tagged)%)
   %send% %actor% You are not even it!
  elseif (!%targ%)
   %send% %actor% You miss whoever the target had been
   %echoaround% %actor% and misses!
  else
   %send% %targ% and tags @RYOU@n with a plasma bolt!
   %echoaround% %targ% and %targ.name% is hit with a plasma bolt!
   rdelete tagged %actor.id%
   set tagged
   remote tagged %targ.id%
   wait 1 s
   eval rrm 2074
   while (%rrm% < 2125)
    eval rrm %rrm% + 1
    %at% %rrm% %echo% @R%targ.name% is now it!@n
   done
  end
end
~
#2032
Guard Trigger 4~
0 q 100
~
if (%direction% == down)
 if (!%actor.varexists(BoughtPrism)%)
  if (!%self.varexists(bribed)% || %self.varexists(bribed)% != %actor.id%)
   return 0
  end
 elseif (%actor.BoughtPrism% == 0)
  if (!%self.varexists(bribed)% || %self.varexists(bribed)% != %actor.id%)
   return 0
end
 end
end
~
#2033
Bought Prism~
2 g 100
~
if (!%actor.varexists(BoughtPrism)%)
 set BoughtPrism 1
 remote BoughtPrism %actor.id%
end
~
$~
