#4900
Pick Berry~
1 c 100
harvest~
if %arg% == berry
  if !%actor.varexists(pickingberry)%
    wait 2 s
    %send% %actor% You search the bush, looking for berries.
    set pickingberry
    remote pickingberry %actor.id%
    wait 60 s
    eval success %random.3%
    switch %success%
      case 1
        %send% %actor% After searching the berry bush, you have found nothing!
        rdelete pickingberry %actor.id%
      break
      case 2
        %send% %actor% After searching the berry bush, you have found a good berry!
        %load% %obj% 4902
        rdelete pickingberry %actor.id%
      break
      default
        %send% %actor% After searching the berry bush, you have found a berry that is no good.
        rdelete pickingberry %actor.id%
      break
    done
  else
    wait 2 s
    %send% %actor% You are already searching the busy for berries.
  end
else
  wait 2 s
  %send% %actor% Try typing harvest berry!
end
~
#4901
Dodoria/Ginyu Q2 Drop~
0 f 100
~
if (%actor.varexists(onDodQ2)%)
    %load% obj 4903
elseif (%actor.varexists(onGinyuQ2)%)
    %load% obj 4904
end
~
$~
