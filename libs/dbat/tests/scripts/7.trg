#700
room echoes~
2 g 100
~
%echo% room echo
wecho
%send% %actor% room to actor
%send% %actor%
%send% nobody hi
wsend
%echoaround% %actor% room around
wechoaround
%zoneecho% 300 room zone
%zoneecho% 99999 nowhere
wzoneecho
%asound% room sound
wasound
%recho% 300 310 room ranged
wrecho
~
#701
room loads and purges~
2 g 100
~
%load% obj 300
%load% obj 300 %actor%
%load% mob 300
%load% mob 99999
%load% obj 99999
%load% mob 300 99999
%load% thing 300
%load%
%purge% nobody
%purge% %actor%
%purge%
%transform% 301
~
#702
room moves~
2 g 100
~
%teleport% %actor% 310
%teleport% all 310
%teleport% nobody 310
%teleport% %actor% 99999
%teleport%
%at% 310 %echo% room at
%at% 99999 %echo% nowhere
%at%
%force% %actor% say room forced
%force% nobody say x
%force%
%damage% %actor% 5
%damage% nobody 5
%damage%
%door% 300 north room 310
%door% 300 xx room 310
%door%
weffect lava 300
weffect gravity 5
weffect
weffect lava
weffect lava 20000
nosuchroomcommand
~
$~
