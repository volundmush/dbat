#500
mob echoes~
0 g 100
~
%echo% plain echo
mecho
%send% %actor% to actor
%send% %actor%
%send% nobody hello
msend
%echoaround% %actor% around actor
%echoaround% nobody hi
mechoaround
%zoneecho% 300 zone wide
%zoneecho% 99999 nowhere
%zoneecho% 300
mzoneecho
%asound% a sound around
masound
%recho% 300 310 ranged echo
%recho% 300
mrecho
%echo% %actor.name% says @Rcolour@n and $n $N
~
#501
mob loads and purges~
0 g 100
~
%load% obj 300
%load% obj 300 %actor%
%load% obj 300 %actor% wield
%load% obj 300 %actor% nosuchplace
%load% obj 300 300
%load% obj 99999
%load% mob 300
%load% mob 99999
%load% mob 300 99999
%load% thing 300
%load%
mload obj
%purge% %actor%
%purge% nobody
%purge% %self.room.contents%
%purge%
mjunk all
mjunk nothing
mjunk
%echo% after purges
~
#502
mob moves~
0 g 100
~
%teleport% %actor% 310
%teleport% %actor% 99999
%teleport% all 310
%teleport% nobody 310
%teleport%
%at% 310 %echo% at 310
%at% 99999 %echo% nowhere
%at%
mgoto 310
mgoto 99999
mgoto
mgoto 300
%force% %actor% say forced
%force% all say all forced
%force% nobody say no
%force%
mfollow %actor%
mfollow nobody
mfollow self
mfollow
~
#503
mob fights~
0 g 100
~
%damage% %actor% 5
%damage% %actor% -5
%damage% %actor% x
%damage% nobody 5
%damage%
mheal %actor% pl 10
mheal %actor% xx 10
mheal
mkill %actor%
mkill nobody
mkill %self%
mkill
mremember %actor% say remembered
mremember nobody
mremember
mforget %actor%
mforget nobody
mforget
%transform% 301
%transform% 99999
%transform%
~
#504
mob doors~
0 g 100
~
%door% 300 north purge
%door% 300 north description a new door
%door% 300 north flags ab
%door% 300 north key 300
%door% 300 north name gate
%door% 300 north room 310
%door% 300 xx room 310
%door% 300 north nosuchfield 1
%door% 99999 north room 310
%door% 300 north room 99999
%door%
~
#505
mob command trigger star~
0 c 100
*~
%echo% any command: %cmd% %arg%
~
$~
