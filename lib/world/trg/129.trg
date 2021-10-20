#12900
Enter the arena~
2 c 100
pk~
if (%arg% == yes)
 set bottom 12901
 set range 8
 eval room %%random.%range%%%
 eval room %room% + %bottom%
 %teleport% %actor.name% %room%
 %zoneecho% 12900 @C[@YPA Announcement@C]@D: @C%actor.name% @W(Fighter Number: @C%actor.id%@W) has entered the @RL@ra@Yv@Ra @RA@rr@De@rn@Ra@W!@n
 %at% %actor.name% %force% %actor.name% look
else
 %send% %actor% @WYou must type pk yes to enter the arena.@n
end
~
#12901
PK Arena leave~
2 c 100
pk~
if (%arg% == no)
    %teleport% %actor.name% 12900
    %zoneecho% 12900 @C[@YPA Announcement@C]@D: @C%actor.name% @Whas left the Arena!@n
    %at% %actor.name% %force% %actor.name% look
else
  %send% %actor.name% You need to type pk no to leave.
end
~
$~
