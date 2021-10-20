#13400
Scraps greets~
0 g 20
~
eval greet %random.5%
switch %greet%
  case 1
    %send% %actor.name% Scraps sniffs the ground and ingores your presence.
    break
  case 2
    %send% %actor.name% Scraps growls at you menacingly 
    break
  case 3 
    %send% %actor.name% Scraps starts barking at you!
    break
  case 4
    %send% %actor.name% Scraps licks you playfully.
    break
  default
    %send% %actor.name% Scraps pees on your leg!
    break
done
~
$~
