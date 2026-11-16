#600
obj echoes~
1 h 100
~
%echo% obj echo
oecho
%send% %actor% obj to actor
%send% %actor%
%send% nobody hi
osend
%echoaround% %actor% obj around
oechoaround
%zoneecho% 300 obj zone
%zoneecho% 99999 nowhere
ozoneecho
%asound% obj sound
oasound
%recho% 300 310 obj ranged
orecho
~
#601
obj loads and purges~
1 h 100
~
%load% obj 300
%load% obj 300 %actor%
%load% obj 300 %actor% wield
%load% mob 300
%load% mob 99999
%load% obj 99999
%load% thing 300
%load%
%purge% nobody
%purge% %actor%
%purge%
otimer 5
otimer x
otimer
osetval 0 5
osetval 99 5
osetval x
osetval
%transform% 301
%transform% 99999
%transform% x
%transform%
odupe
~
#602
obj moves~
1 h 100
~
%teleport% %actor% 310
%teleport% all 310
%teleport% nobody 310
%teleport% %actor% 99999
%teleport%
%at% 310 %echo% obj at
%at% 99999 %echo% nowhere
%at%
ogoto 310
ogoto 99999
ogoto
%force% %actor% say obj forced
%force% nobody say x
%force%
%damage% %actor% 5
%damage% nobody 5
%damage%
%door% 300 north room 310
%door% 300 xx room 310
%door%
nosuchobjcommand
~
#603
obj purges itself~
1 h 100
~
%echo% obj before purge
%purge% %self%
%echo% obj after purge, never
~
#604
obj argument corners~
1 h 100
~
%load% obj 300
%load% mob 300 abc
%load% obj 301 %actor% wield
%load% obj 302 chest
%load% obj 302 nothing
%asound% a sound from an obj
%door% nosuchroom north description x
%door% 300 north nosuchfield x
%door% 300 north room 99999
%door% 300 north description a carved door
%door% 300 north flags ab
%door% 300 north key 301
%door% 300 north name gate
%door% 300 north purge
%at% %actor.name% %echo% at the actor
%at% 310
%force% all say all forced by an obj
%zoneecho% 99999 nowhere
dg_cast 'heal' %actor%
%send% %actor% |%actor.name% ^%actor.name% &%actor.name% *%actor.name% and _chest
~
$~
