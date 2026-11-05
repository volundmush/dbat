#300
Humming floor~
2 g 100
~
%echo% The floor hums.
~
#301
Vanishes when greeted~
0 g 100
~
%echo% The stranger vanishes.
%purge% %self%
~
#310
Type a of kind 0~
0 a 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#311
Type b of kind 0~
0 b 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#312
Type c of kind 0~
0 c 100
dance~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#313
Type d of kind 0~
0 d 100
hello~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#314
Type e of kind 0~
0 e 100
hello~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#315
Type f of kind 0~
0 f 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#316
Type g of kind 0~
0 g 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#317
Type h of kind 0~
0 h 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#318
Type i of kind 0~
0 i 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#319
Type j of kind 0~
0 j 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#320
Type k of kind 0~
0 k 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#321
Type l of kind 0~
0 l 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#322
Type m of kind 0~
0 m 1
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#323
Type n of kind 0~
0 n 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#324
Type o of kind 0~
0 o 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#325
Type p of kind 0~
0 p 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#326
Type q of kind 0~
0 q 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#327
Type r of kind 0~
0 r 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#328
Type s of kind 0~
0 s 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#329
Type t of kind 0~
0 t 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#340
Type a of kind 1~
1 a 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#341
Type b of kind 1~
1 b 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#342
Type c of kind 1~
1 c 100
dance~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#343
Type d of kind 1~
1 d 100
hello~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#344
Type e of kind 1~
1 e 100
hello~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#345
Type f of kind 1~
1 f 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#346
Type g of kind 1~
1 g 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#347
Type h of kind 1~
1 h 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#348
Type i of kind 1~
1 i 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#349
Type j of kind 1~
1 j 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#350
Type k of kind 1~
1 k 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#351
Type l of kind 1~
1 l 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#352
Type m of kind 1~
1 m 1
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#353
Type n of kind 1~
1 n 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#354
Type o of kind 1~
1 o 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#355
Type p of kind 1~
1 p 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#356
Type q of kind 1~
1 q 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#357
Type r of kind 1~
1 r 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#358
Type s of kind 1~
1 s 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#359
Type t of kind 1~
1 t 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#360
Type a of kind 2~
2 a 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#361
Type b of kind 2~
2 b 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#362
Type c of kind 2~
2 c 100
dance~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#363
Type d of kind 2~
2 d 100
hello~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#364
Type e of kind 2~
2 e 100
hello~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#365
Type f of kind 2~
2 f 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#366
Type g of kind 2~
2 g 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#367
Type h of kind 2~
2 h 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#368
Type i of kind 2~
2 i 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#369
Type j of kind 2~
2 j 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#370
Type k of kind 2~
2 k 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#371
Type l of kind 2~
2 l 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#372
Type m of kind 2~
2 m 1
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#373
Type n of kind 2~
2 n 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#374
Type o of kind 2~
2 o 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#375
Type p of kind 2~
2 p 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#376
Type q of kind 2~
2 q 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#377
Type r of kind 2~
2 r 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#378
Type s of kind 2~
2 s 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#379
Type t of kind 2~
2 t 100
~
* exercises variables, expressions and control flow
eval x %random.10% + 2 * 3
set y %actor.name%
if %x% > 5 && %y% != nobody
  %echo% x is %x%, %actor.level% %actor.gold% %actor.room% %self.name% %self.vnum%
elseif %x% == 3 || %actor.is_pc%
  %send% %actor% hello %actor.name%
else
  %echoaround% %actor% around %actor.heshe%
end
set n 0
while %n% < 3
  eval n %n% + 1
  nop %actor.gold(1)%
done
switch %random.4%
  case 1
    %load% obj 302
  break
  case 2
    %load% mob 300
  break
  case 3
    %force% %actor% look
  break
  default
    %at% 301 %echo% far away
done
global x
context 1
remote y %actor.id%
rdelete y %actor.id%
makeuid who %actor.id%
extract w 1 one two three
eval s %y.strlen% + %y.toupper.strlen%
%zoneecho% 300 hello zone
%asound% boom
%door% 300 north description x
%damage% %actor% 1
dg_cast 'bless' %actor%
dg_affect %actor% str 1 1
wait 1 s
%teleport% %actor% 300
return 1
~
#380
Purges itself~
0 g 100
~
%echo% going
%purge% %self%
%echo% still here
~
#381
Object purges itself~
1 j 100
~
%echo% going
%purge% %self%
%echo% still here
~
#382
Forces the carrier away~
1 c 2
zap~
%force% %actor% quit
%echo% still running
~
#383
Purges itself at random~
1 b 100
~
%purge% %self%
~
#384
Purges itself at five~
1 t 5
~
%purge% %self%
~
#385
Clears the room when its timer runs out~
1 f 100
~
%purge%
~
#386
Purges itself when a spell is aimed at it~
1 p 100
~
%purge% %self%
~
#387
Purges itself as it loads~
1 n 100
~
%purge% %self%
~
#388
Purges itself when taken off~
1 l 100
~
%purge% %self%
~
#389
Purges whatever it is given~
0 j 100
~
%purge% %object%
~
#390
Forces whoever types this away, then lets the command through~
1 c 4
zop~
%force% %actor% quit
return 0
~
$~
