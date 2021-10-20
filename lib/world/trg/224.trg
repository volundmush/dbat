#64000
Getting Lost~
2 g 100
~
eval num %random.20%
if %num% == 20
  %send% %actor% You suddenly realize you've lost sight of the building in the center of the Time Chamber!  You'd better go find it!
  %teleport% %actor% 64097
  eval num2 %random.10%
    if (%num2% >= 6)
      wait 2 s
      %at% %actor% %load% mob 64022
    else
    end
else
end
~
#64001
Getting Found~
2 g 100
~
eval num %random.10%
if %num% == 10
  %send% %actor% @CYou finally catch sight of the building in the center of the Hyperbolic Time Chamber! You're no longer lost, and have found your way!@n
  eval num2 %random.90%
  eval num3 64005 + %num2%
  %teleport% %actor% %num3%
else
end
~
#64002
Layer 1 Spawns~
2 g 100
~
eval num %random.8%
if (%num% == 1)
  %load% mob 64011
  %echo% @CA strange mirage appears before %actor.name%, slowly revealing the shape of a person.@n
  wait 1s
  %echo% @CThe mirage begins to solidify before Goten appears through the veil.@n
elseif (%num% == 2)
  %load% mob 64013
  %echo% @CA strange mirage appears before %actor.name%, slowly revealing the shape of a person.@n
  wait 1s
  %echo% @CThe mirage begins to solidify before Kid Trunks appears through the veil.@n
elseif (%num% == 3)
  %load% mob 64012
  %echo% @CA strange mirage appears before %actor.name%, slowly revealing the shape of a person.@n
  wait 1s
  %echo% @CThe mirage begins to solidify before Super Saiyan Goten appears through the veil.@n
elseif (%num% == 4)
  %load% mob 64014
  %echo% @CA strange mirage appears before %actor.name%, slowly revealing the shape of a person.@n
  wait 1s
  %echo% The mirage begins to solidify before Super Saiyan Kid Trucks apears through the veil.@n
elseif (%num% >= 5)
end
~
#64003
Exit purge~
0 q 100
~
wait 1 s
%echo% The mirages slowly begin to fade out of this reality and into the next, soon vanishing and leaving no trace, as if they were never there to begin with.
%purge% self
~
#64004
Layer 2 Spawns~
2 g 100
~
eval num %random.8%
if (%num% == 1)
%load% mob 64015
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Piccolo appears through the veil.@n
elseif (%num% == 2)
 %load% mob 64007
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Future Trunks appears through the veil.@n
elseif (%num% == 3)
 %load% mob 64005
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Vegeta appears through the veil.@n
elseif (%num% == 4)
 %load% mob 64000
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Gohan appears through the veil.@n
elseif (%num% >= 5)
end
~
#64005
Layer 3 Spawns~
2 g 100
~
eval num %random.8%
if (%num% == 1)
 %load% mob 64003
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Goku appears through the veil.@n
elseif (%num% == 2)
 %load% mob 64016
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Gotenks appears through the veil.@n
elseif (%num% == 3)
 %load% mob 64008
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Super Saiyan Future Trunks appears through the veil.@n
elseif (%num% == 4)
 %load% mob 64006
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Super Saiyan Vegeta appears through the veil.@n
elseif (%num% >= 5)
end
~
#64006
Layer 4 Spawns~
2 g 100
~
eval num %random.8%
if (%num% == 1)
 %load% mob 64010
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Super Vegeta appears through the veil.@n
elseif (%num% == 2)
 %load% mob 64009
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Super Trunks appears through the veil.@n
elseif (%num% == 3)
 %load% mob 64004
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Super Saiyan Goku appears through the veil.@n
elseif (%num% == 4)
 %load% mob 64001
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Super Saiyan Gohan appears through the veil.@n
elseif (%num% >= 5)
end
~
#64007
Layer 5 Spawns~
2 g 100
~
eval num %random.8%
if (%num% == 1)
 %load% mob 64002
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Super Saiyan Second Gohan appears through the veil.@n
elseif (%num% == 2)
 %load% mob 64017
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Super Saiyan Gotenks appears through the veil.@n
elseif (%num% == 3)
 %load% mob 64019
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Super Buu appears through the veil.@n
elseif (%num% == 4)
 %load% mob 64018
  %echo%@C A strange mirage appears before %actor.name%, slowly revealing the shape of a person.  The mirage begins to solidify before Super Saiyan Third Gotenks appears through the veil.@n
elseif (%num% >= 5)
end
~
#64008
Layer 6 Spawns~
2 g 100
~
eval num %random.100%
if (%num% >=80)
 load mob 64020
 %echo% @CA strange mirage appears before %actor.name%, slowly revealing the shape of a person. The mirage begins to solidify before Kvothe appears through the veil.@n
elseif (%num% >= 95)
 %load% mob 64021
 %echo% @CA strange mirage appears before %actor.name%, slowly revealing the shape of a person. The mirage begins to solidify before Legendary Super Saiyan Kvothe appears through the veil.@n
end
~
#64009
Ambient HBTC trigger 1~
2 b 100
~
%echo% This trigger commandlist is not complete!
~
#64010
Lost Human Load~
0 n 100
~
emote blinks as he sees someone come towards him.  "@CYou too, huh?@n" He shakes his head. "@CLost sight of the chamber in the center...@n"
wait 3 s
say Must... keep searching...
wait 4 s
emote leaves north.
%purge% %self%
~
#64011
Cooking workaround~
2 c 100
pack~
%send% %actor% You feel like taking this stove would incur the @rW@RR@YA@rT@RH@n of @DPopo.
~
#64012
HBTC yes trigger~
2 c 100
yes~
eval %actor.rpp%
if (%actor.rpp% >= 5)
  %send% %actor% @D5 RPP deducted.@n
  nop %actor.rpp(-5)%
  wait 2 s
  %teleport% %actor% 1574
  %force% %actor% l
  wait 1 s
  %send% %actor% You open the door and enter the Hyperbolic Time Chamber.  You feel this place opperates on a different time frame from the rest of the universe.
elseif (%actor.rpp% < 5)
  %send% %actor% @RYou don't have enough RPP.@n
end
~
#64013
Ambient HBTC 2~
2 b 100
~
%echo% This trigger commandlist is not complete!
~
#64014
undefined~
0 n 100
~
* oops this is extra
~
#64015
troll restocks fridge~
0 n 100
~
get fridge
junk fridge
get cabinet
junk cabinet
%purge% self
~
#64016
y~
2 c 100
*~
If (%cmd% == p || %cmd% == pa || %cmd% == pac || %cmd% == pack || )
 Eval name %actor.name% 
 Set StoveThief %name% 
 Remote StoveThief 335 
Else 
End
~
$~
