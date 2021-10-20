#4000
Cicle hotel clerk trigger part 1~
0 g 100
~
nop %actor.is_thief(on)%
wait 2 s
say Hello there, welcome to Cicle Hotel, finest establishment on Frigid!
wait 3 s
say Rooms are 10 zenni a night, if you would like one, just give me the money and I'll arrange you a room.
~
#4001
Cicle hotel clerk trigger part 2~
0 m 10
10~
say So you do? Fantastic. I'll just arrange your room now, %actor.name%
wait 2 sec
emote fiddles with some keys for a moment before finding the right one.
wait 1 sec
unlock door
wait 1 sec
open door
wait 2 sec
say Enjoy your stay.
wait 10 sec
close door
lock door
~
#4002
Jailing Trigger **Do not touch, testing**~
0 g 100
~
wait 1 sec
if %actor.is_thief%
  say Stop thief!
  wait 1 sec
  mkill %actor%
  wait 1 sec
  emote blows a whistle, summoning help.
%load% mob 4009
%load% mob 4009
%load% mob 4009
set people %self.room.people%
while %people%
if %people.name.contains(guard)%
%force% %people% mkill %actor%
end
set people %people.next_in_room%
done
end
~
#4003
Ice crown city echo~
2 ab 10
~
%zoneecho% 4000 The sounds of the over-crowded streets of Ice Crown city echo through the air.
~
#4004
Jail trigger continued ** do not touch **~
0 k 100
~
eval num ((actor.maxhitp% * 30) /100
if %actor.hitp% > %num%
%teleport% %actor% 4187
%zoneecho% 4000 @R(@YAttention@R)@n This is Ice Crown City Security, %actor.name% has been jailed for stealing!!
%zoneecho% 4100 @R(@YAttention@R)@n This is Ice Crown City Security, %actor.name% has been jailed for stealing!!
%zoneecho% 4200 @R(@YAttention@R)@n This is Ice Crown City Security, %actor.name% has been jailed for stealing!!
nop %actor.is_thief(off)%
(%actor.is_thief(off)%)
end
~
#4005
Jailing Thieves part 3~
0 n 100
~
look
if (%actor.is_pc && actor_is thief%)
  mkill %actor%
return 0
else
return 1
end
~
#4006
Icer Male talks~
0 c 100
talk~
if %cmd% ==  talk
 if %arg% == %self.alias%
  %send% %actor% %self.name% says, 'What would you like to talk about?'
  %send% %actor% ------------------------------------
  %send% %actor% |@ROption @Y1) @WBounty Hunter@n
  %send% %actor% |@ROption @Y2) @WUnwind@n
  %send% %actor% |@ROption @Y3) @WLatest Rumours@n
  %send% %actor% |@ROption @Y4) @WLittle Secret@n
  %send% %actor% |@ROption @Y5) @WAbout me@n
  %send% %actor% |@D @n
  %send% %actor% |@RSyntax@n: @Ytalk %self.alias% <option number>@n
  %send% %actor% ------------------------------------
 elseif %arg% /= %self.alias% && %arg% /= 1
  %send% %actor% %self.name% says, 'The bounty hunters guild is located in the Residential District of Ice Crown City. They are a motley lot that don't care who they hire as long as the job gets done.'
 elseif %arg% /= Male && %arg% /= 2
  %send% %actor% %self.name% says, 'If your looking for a good place to unwind, I would suggest the Warlords & Wenches pub in the Lower level of Ice Crown has a great atmosphere and cheap ale. A true workman's establishment.'
 elseif %arg% /= Male && %arg% /= 3
  %send% %actor% %self.name% says, 'The Bounty Hunter guild has plenty of work going, if you're strapped for cash, you should go check em out.'
 elseif %arg% /= Male && %arg% /= 4
  %send% %actor% %self.name% says, 'Durunkuu's Protectives is the place to go to buy armor if your just starting out on adventures.'
 elseif %arg% /= Male && %arg% /= 5
  %send% %actor% %self.name% says, 'I am %self.name%. Just a commoner trying to make their way in the world.
 end
end
~
#4007
Icer female talks~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% says, 'What would you like to talk about?'
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1)@n Shops
    %send% %actor% @ROption @Y2)@n Latest Rumours
    %send% %actor% @ROption @Y3)@n Little Secret
    %send% %actor% @ROption @Y4)@n About me
    %send% %actor% ------------------------------------
  elseif %arg% /= %self.alias% && %arg% /= Shops
    %send% %actor% %self.name% says, 'Almost all the shops are located in the commercial district. My favorites are Bokuza Accessories and the Coffee Hut.'
  elseif %arg% /= Female && %arg% /= Latest rumors
    %send% %actor% %self.name% says, 'I hear that ruthless bandits are praying on people travelling the Ice Highway.'
  elseif %arg% /= Female && %arg% /= Little secret
    %send% %actor% %self.name% says, 'You can enter the Abandoned level if you have clearance papers from the Chief of Security.'
  elseif %arg% /= Female && %arg% /= About me
    %send% %actor% %self.name% says, 'I am %self.name%. Just a commoner trying to make their way in the world.
  end
end
~
#4008
Frigid Beggar part 1~
0 g 100
~
if !%char.varexists(frigidbeggarpaid)%
  say Please %actor.name% can you spare 10 zenni?
else
  return 0
end
~
#4009
Frigid Beggar part 2~
0 m 100
10~
say Thank you %actor.name% I am ever so grateful
set frigidbeggarpaid
remote frigidbeggerpaid %actor.id%
wait 2 s
emote shoves the coins in a hat and moves on.
~
#4010
Nobles speak~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% says, 'What would you like to talk about?'
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1)@n Complaint
    %send% %actor% @ROption @Y2)@n Frozen Fist Dojo
    %send% %actor% @ROption @Y3)@n Latest Rumours
    %send% %actor% @ROption @Y4)@n Little Secret
    %send% %actor% @ROption @Y5)@n About me
    %send% %actor% ------------------------------------
  elseif %arg% /= %self.alias% && %arg% /= Complaint
    %send% %actor% %self.name% says, 'These guards are really dropping the ball. Don't they know we don't want the lesser filth walking around our district? It seems to be getting worse and worse.'
  elseif %arg% /= Noble && %arg% /= Frozen Fist Dojo
    %send% %actor% %self.name% says, 'The Frozen Fist Dojo is the most prestigious fighting school on the planet. Probaly in the galaxy. Only us Nobles can afford to send our children there. Of course, gifted children of poor families often get a place there too, much to our disgust.'
  elseif %arg% /= Noble && %arg% /= Latest rumors
    %send% %actor% %self.name% says, 'Only Icers are allowed into the Royal Palace. Although I heard the other day that a group of mutants who aren't servants, have found a way to sneak in.'
  elseif %arg% /= Noble && %arg% /= Little secret
    %send% %actor% %self.name% says, 'The guards have opened up the Wyrm Spine mountain region. They say that the mountain passes have been cleared of Wyrms and should be relatively safe to travel.'
  elseif %arg% /= Noble && %arg% /= About me
    %send% %actor% %self.name% says, 'I am %self.name%. I am of noble blood, so you should cease speaking to me, Nwah.
  end
end
~
#4011
Derek the Dodo speech~
0 ab 100
~
eval line %random.3%
switch %line%
case 1
say Hi there %actor.name%, head on over to Amnu-nation for all your weapon needs!
break
case 2
emote flaps his diminutive wings.
break
default
say Remember kids! Derek the Dodo says to observe gun safety. Hold that Barrel away from yourself!
break
done
~
#4012
Leaving Cicle Hotel~
0 g 100
~
wait 1 s
say You looking to be leaving the hotel, %actor.name%?
wait 2 s
say Just say yes and I will escort you out.
%echo% @R(OOC) You could also try to @Cnod@R at the guard.@n
~
#4013
Leaving Cicle Hotel part 2~
0 d 100
yes~
say Splendid, right this way %actor.name%.
wait 1 s
%send% %actor% The clerk leads you back through to the hotel lobby.
%teleport% %actor% 4130
~
#4015
Gimme House Pamphlet~
0 d 100
*~
if %speech% == house
  if %actor.inventory(4005)%
    wait 1 s
    say Sorry, you already have a house pamphlet.
  else
    wait 1 s
    %load% obj 4005
    give house %actor.name%
  end
end
~
#4016
Gimme Ship Pamphlet~
0 d 100
*~
if %speech% == ship
  if %actor.inventory(63)%
    wait 1 s
    say Sorry, you already have a ship pamphlet.
  else
    wait 1 s
    %load% obj 63
    give ship %actor.name%
  end
end
~
#4017
House Pamphlet Commands~
1 c 3
pread~
if (%arg% == one)
    %send% %actor%   @YAre you tired of having no where to sleep? Or having to pay for a hotel room
    %send% %actor% with your hard earned zenni? Do you long for a place to call home? If you have
    %send% %actor% answered yes to any of these, get in contact with an immortal today to begin the
    %send% %actor% process.
    %send% %actor% @n
    %send% %actor% @YLocation Surcharges
    %send% %actor% @W-------------------------
    %send% %actor% @GEarth:    @D0  Percent
    %send% %actor% @CFrigid:   @D15 Percent
    %send% %actor% @RVegeta:   @D10 Percent
    %send% %actor% @gNamek:    @D5  Percent
    %send% %actor% @mSpace:    @D30 Percent
    %send% %actor% @MKonack:   @D10 Percent
    %send% %actor% @bKanassa:  @D20 Percent
    %send% %actor% @cOther:    @D25 Percent
    %send% %actor% @n  
    %send% %actor%    @YA location surcharge will increase house price packages by the percentage
    %send% %actor% listed. Space is defined as anywhere in space (so your house could be a secret
    %send% %actor% asteroid base). Other is defined as any other planet built but not currently
    %send% %actor% listed. 
    %send% %actor% @n
    %send% %actor% @YLand and Building
    %send% %actor% @W-----------------
    %send% %actor% @CSmall House: @W40,000 zenni. Basic five room house.
    %send% %actor% @CRegular House: @W70,000 zenni. Basic ten room house.
    %send% %actor% @CLarge House: @W100,000 zenni. Basic fifteen room house.
    %send% %actor% @CHouse Upgrade: @W30,000 zenni. Upgrades room size, if not already large.
    %send% %actor% @CPrivate Landing Pad: @W20,000 zenni. Adds landing pad to house.
    %send% %actor% @CKey-locked door: @W2,500 zenni. Adds a keylocked door to house.
    %send% %actor% @CLarge House RPP version: @W10 RPP. Includes fifteen rooms, two misc items (Food
    %send% %actor%  within reason, furniture etc. No equipment.), and two misc scripts.
    %send% %actor% @n
    %send% %actor% @YPage One - Type 'pread two' for page two, or 'pread three' for page three.@n
elseif (%arg% == two)
    %send% %actor% @YHome Decor
    %send% %actor% @W-----------------------
    %send% %actor%    @YAbout to buy a home or already have one and want to spruce it up? For
    %send% %actor% additional zenni, we offer a large list of decor items for you to choose from.
    %send% %actor% @n
    %send% %actor% @YThere is a limit of 15 items per house. Includes things like keys also. 
    %send% %actor% @n
    %send% %actor% @CExtra Key: @W500 zenni.
    %send% %actor% @CChair: @W500 Zenni. (Copy of an existing one)
    %send% %actor% @CBed: @W1,000 Zenni. (Copy of an existing one)
    %send% %actor% @CCustom Chair/Bed: @WAdd 500 zenni to above prices.
    %send% %actor% @CSmall Container: @W2,000 zenni. Holds 300 weight.
    %send% %actor% @CMedium Container: @W5,000 zenni. Holds 600 weight.
    %send% %actor% @CLarge Container: @W12,000 zenni. Holds 1000 weight.
    %send% %actor% @CKey-locked container: @WAdd 1,000 zenni to container prices.
    %send% %actor% @CHealing Tank: @W15,000 zenni. (Max 1)
    %send% %actor% @CGravity Chamber: @W10,000 zenni. (Max 1)
    %send% %actor% @CFountain: @W2,500 zenni. (Max 1)
    %send% %actor% @CRefridgerator: @W5,000 zenni (Max 1. Includes 1 food and drink item)
    %send% %actor% @COther Item: @W1,500 zenni.
    %send% %actor% @n
    %send% %actor%    @YNPCs can also be purchased (max 3), though they are basic and will NOT be
    %send% %actor% training NPCs. At a very high cost, basic scripting (greeting and stuff like
    %send% %actor% that) can be purchased for the NPCs. 
    %send% %actor% @n
    %send% %actor% @CNPC: @W2,500 zenni.
    %send% %actor% @CBasic Scripting: @W50,000 zenni.
    %send% %actor% @n
    %send% %actor% @YExtras
    %send% %actor% @W-----------------------
    %send% %actor%   @YLooking for an extra addon not mentioned above anywhere? Here is a
    %send% %actor% list of some things that have been added in.
    %send% %actor% @n
    %send% %actor% @CAnti-IT: @W50,000 per applied room. This will keep anyone in that room from ITing out.
    %send% %actor% @n
    %send% %actor% @YPage Two - Type 'pread one' to go look at page one, or 'pread three' to look at page three.@n
elseif (%arg% == three)
   %send% %actor% @YThings you can't do with a house
   %send% %actor% @W-----------------------
   %send% %actor%   @YHere is a list of things that you CANNOT do with a house.
   %send% %actor% @n
   %send% %actor% @CShops: @WShops will not be added into houses.
   %send% %actor% @CVillages: @WHouses cannot be some form of a village. Having them as an open building
   %send% %actor% like a hotel or a castle is fine, but not a vilalge.
   %send% %actor% @n
   %send% %actor% @YPage One - Type 'pread two' for page two, or 'pread three' for page three.@n
else
  %send% %actor% Read what?
end
~
#4018
Leaving Cicle Hotel Part 3~
0 c 100
nod~
say Splendid, right this way %actor.name%.
wait 1 s
%send% %actor% The clerk leads you back through to the hotel lobby.
%teleport% %actor% 4130
~
#4098
Land mine part 1~
1 h 100
~
attach 4099 %self.id%
~
#4099
Land mine part 2~
2 g 100
~
wait 2 sec
%echo% Kaboooooooooooooooooooom!
%damage% all 1000
%purge% %self%
~
$~
