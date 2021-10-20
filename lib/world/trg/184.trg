#18400
Kame Vault Password~
2 d 100
Shishiniohame~
if (%speech% == Shishiniohame)
 wait 1
 %echo% A security panel starts lighting up and a row of lights flash green.
 wait 1 s
 if (%self.vnum% == 18405)
 %at% 18415 %echo% The vault door bangs loudly then opens with a hiss.
 else
  %at% 18405 %echo% The vault door bangs loudly then opens with a hiss.
 end
 %door% 18405 10 flags a
 %door% 18415 11 flags a
 wait 3 s
 if (%self.vnum% == 18405)
  %at% 18415 %echo% The vault door hisses and slowly closes, resulting in a loud bang.
 else
  %at% 18405 %echo% The vault door hisses and slowly closes, resulting in a loud bang.
 end
 %door% 18405 10 flags abc
 %door% 18415 11 flags abc
else
 wait 1
 %echo% The hatch security panel flashes as the password is analyzed.
 wait 1 s
 %echo% The security panel flashes a failure message and the hatch remains closed.
end
~
#18401
Kame Csec Password~
2 d 100
Yishitakura~
if (%speech% == Yishitakura)
 wait 1
 %echo% A security panel starts lighting up and a row of lights flash green.
 wait 1 s
 if (%self.vnum% == 18452)
 %at% 18452 %echo% A metal door splits down the middle and retracts into the walls.
 else
  %at% 18455 %echo% A metal door splits down the middle and retracts into the walls.
 end
 %door% 18452 2 flags a
 %door% 18455 0 flags a
 wait 3 s
 if (%self.vnum% == 18452)
  %at% 18452 %echo% Two metal sheets close to form a solid door.
 else
  %at% 18455 %echo% Two metal sheets close to form a solid door.
 end
 %door% 18452 2 flags abc
 %door% 18455 0 flags abc
else
 wait 1
 %echo% A security panel starts to flash.
 wait 1 s
 %echo% The panel starts flashing red and denies the password.
end
~
#18402
Guard Greeting~
0 g 100
~
if (%actor.clan(Fumetsu Organization)%)
 wait 1
 %echo% %self.name% nods %self.hisher% head at %actor.name%.
else
 wait 1
 %echo% %self.name% yells out, '@CIntruder spotted!@n' then begins attacing %actor.name%!
 wait 1
 punch %actor.name%
end
~
#18403
Keeper Greeting~
0 g 100
~
if (%actor.clan(Fumetsu Organization)%)
 wait 1
 %echo% %self.name% ignores %actor.name% as it performs it's tasks.
else
 wait 1
  %echo% %self.name% emits a strange sequence of clicks then begins attacking %actor.name%!
 wait 1
 punch %actor.name%
end
~
#18404
IT Prevention~
2 c 100
inst~
if (%cmd% == inst || %cmd% == insta || %cmd% == instan || %cmd% == instant)
return 1
%send% %actor% You can't sense anything from within here.
end
~
$~
