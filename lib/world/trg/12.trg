#1200
Krillin Quest Rex Skull~
0 f 100
~
if (%actor.varexists(onkq3)% || %actor.varexists(onTienQ3)% || %actor.varexists(onDodQ3)%)
 if (!%actor.has_item(1203)%)
    %load% obj 1203
 end
elseif (%actor.varexists(onGohanQ3)%)
  if (!%actor.has_item(1204)%)
    %load% obj 1204
  end
elseif (%actor.varexists(onTenQ3)% || %actor.varexists(onGinyuQ3)%)
  if (!%actor.has_item(1205)%)
    %load% obj 1205
  end
end
~
$~
