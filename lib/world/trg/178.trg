#17800
Tim the Scalper Say~
0 g 50
~
wait 2 s
say Galaxy's Strongest Martial Arts Tournament coming soon! 50 Zenni a ticket!
wait 5 s
say Tickets! Tickets! Get your tickets right here!
wait 5 s
say For 50 zenni, a ticket can be yours!
~
#17801
Tim the Scalper sells his tickets~
0 m 1
~
if %amount% == 50
  wait 1 s
  say Great. One ticket coming up.
  wait 2 s
  emote reaches into a pocket and pulls out a ticket.
  %load% obj 17800
  wait 1 s
  give ticket %actor.name%
  wait 3 s
  say I hope you enjoy the tournament.
else
  say You've given me %amount% zenni, I need exactly 50 zenni.
end
~
#17802
Tournament Pilot from Planet Earth~
0 j 100
~
if %object.vnum% == 17800
  %purge% %object%
  wait 1 s
  say All aboard!
  wait 1 sec
  mteleport %actor.name% 48
  %send% %actor% The ship roars as you take your seat and begins its journey.
  wait 2 sec
  %send% %actor% Stars begin to fly by as the ship reaches full speed.
  wait 3 sec
  %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
  wait 2 sec
  %send% %actor% The ship begins to shake as it settles down.
  wait 1 sec
  mteleport %actor% 17876
  %force% %actor% look
  %send% %actor% You have arrived, and disembark.
  %echoaround% %actor% %actor.name% disembarks from a ship.
else
  say I don't want that.
  give %object% %actor.name%
end
~
#17803
Tournament Pilot from Tournament Area~
0 d 100
*~
if %speech% = leave
  wait 1 s
  say All aboard!
  wait 1 sec
  mteleport %actor.name% 48
  %send% %actor% The ship roars as you take your seat and begins its journey.
  wait 2 sec
  %send% %actor% Stars begin to fly by as the ship reaches full speed.
  wait 3 sec
  %send% %actor% Stars are flying by too fast to be seen as the ship continues on.
  wait 2 sec
  %send% %actor% The ship begins to shake as it settles down.
  wait 1 sec
  mteleport %actor% 409
  %force% %actor% look
  %send% %actor% You have arrived, and disembark.
  %echoaround% %actor% %actor.name% disembarks from a ship.
else
  say You need to say leave if you want to leave the tournament area, %actor.name%.
end
~
#17804
Enter the arena~
2 c 100
pk~
if (%arg% == yes)
 set bottom 17825
 set range 25
 eval room %%random.%range%%%
 eval room %room% + %bottom%
 %teleport% %actor.name% %room%
 %zoneecho% 17825 @C[@YPA Announcement@C]@D:@W %actor.name% (Fighter Number: %actor.id%) has entered the Arena!
 %at% %actor.name% %force% %actor.name% look
else
 %send% %actor% You must type pk yes to enter the arena.
end
~
#17805
PK Arena leave~
2 c 100
pk~
if (%arg% == no)
    %teleport% %actor.name% 17875
    %zoneecho% 17825 @C[@YPA Announcement@C]@Y: @W%actor.name% has left the Arena!@n
    %at% %actor.name% %force% %actor.name% look
else
  %send% %actor.name% You need to type pk no to leave.
end
~
#17806
Strength testing device~
1 c 100
test~
set mypower  %actor.hitp%
set mystr  %actor.str%
eval mystr %mystr% / 7
set myrank 0
if %mypower% >= 1000000000
  eval mypower ERROR
  set myrank SSS
elseif %mypower% >= 500000000
  eval mypower %mypower% - 500000000
  eval mypower %mypower% / 5000000
  eval mypower %mypower% * %mystr%
  set myrank SS
elseif %mypower% >= 10000000
  eval mypower %mypower% - 10000000
  eval mypower %mypower% / 100000
  eval mypower %mypower% * %mystr%
  set myrank S
elseif %mypower% >= 600000
  eval mypower %mypower% - 600000
  eval mypower %mypower% / 6000
  eval mypower %mypower% * %mystr%
  set myrank B
elseif %mypower% >= 100000
  eval mypower %mypower% - 100000
  eval mypower %mypower% - 1000
  eval mypower %mypower% * %mystr%
  set myrank C
elseif %mypower% >= 20000
  eval mypower %mypower% - 20000
  eval mypower %mypower% - 200
  eval mypower %mypower% * %mystr%
  set myrank D
elseif %mypower% >= 1000
  eval mypower %mypower% - 1000
  eval mypower %mypower% / 10
  eval mypower %mypower% * %mystr%
  set myrank F
elseif %mypower% >= 200
  eval mypower %mypower% / 200
  set myrank LOL
else
  set myrank TURTLE
end
if %arg% == strength
  wait 1 s
  %send% %actor% @YAfter a moment to gather yourself, you punch the device!@n
  %echoaround% %actor% %actor.name% @Ytakes a moment to gather %actor.hisher%self, then punches the machine!@n
  wait 1
  %echo% @RCombatant ID:           @Y%actor.name%@n
  %echo% @RCombatant Combat Rank:  @Y%myrank%@n
  %echo% @RCombatant Score:        @Y%mypower%@n
else
  %send% %actor% Try typing test strength instead!
end
~
#17813
Plushy Grab~
1 j 100
~
wait 1 s
%echoaround% %actor% %actor.name% grabs the %self.shortdesc%, making the %self.shortdesc% exclaim, "I'm the Champ and I know your tricks. See, I can make lights too!".
%send% %actor% You grab the %self.shortdesc%, making it exclaim, "I'm the champ and I know your tricks. See, I can make lights too!"
~
#17814
Plushy Remove~
1 l 100
~
wait 1 s
%echoaround% %actor% %actor.name% stops using their %self.shortdesc%, causing it to exclaim, "I didn't want to fight anyway. It would have been too easy for the Champ!"
%send% %actor% You stop using your %self.shortdesc%, causing it to exclaim, "I didn't want to fight anyway. It would have been too easy for the Champ!"
~
#17899
pk yes~
2 c 100
pk~
If (%arg% == yes)
 Eval num %random.4%
 If (%num% == 4)
  %teleport% %actor.name% 17838
 Elseif (%num% == 3)
  %teleport% %actor.name% 17832
 Elseif (%num% == 2)
  %teleport% %actor.name% 17836
 Elseif (%num% == 1)
  %teleport% %actor.name% 17842
 End
 %zoneecho% 17825 @C[@YPA Announcement@C]@D: @C%actor.name% @W(Fighter Number: @C%actor.id%@W) has entered the Arena!@n
 %at% %actor.name% %force% %actor.name% look
Else
 %send% %actor% You must type pk yes to enter the arena!
end
~
$~
