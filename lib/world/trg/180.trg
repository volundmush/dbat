#18000
UDF Password~
2 d 100
ketchup~
if (%speech% == Ketchup)
 wait 1
 %echo% The airlock security panel flashes as the password is analyzed.
 wait 1 s
 %echo% The airlock opens with a hiss.
 if (%self.vnum% == 18000)
  %at% 18001 %echo% The airlock opens with a hiss.
 else
  %at% 18000 %echo% The airlock opens with a hiss.
 end
 %door% 18001 5 flags a
 %door% 18000 4 flags a
 wait 3 s
 %echo% The airlock closes with a hiss.
 if (%self.vnum% == 18000)
  %at% 18001 %echo% The airlock closes with a hiss.
 else
  %at% 18000 %echo% The airlock closes with a hiss.
 end
 %door% 18000 4 flags abc
 %door% 18001 5 flags abc
else
 wait 1
 %echo% The airlock security panel flashes as the password is analyzed.
 wait 1 s
 %echo% The security panel flashes a failure message and the airlock remains closed.
end
~
#18001
Guard Trigger~
0 g 100
~
if (%actor.clan(Universal Defense Force)%)
 wait 1
 %echo% %self.name% nods %self.hisher% head in greeting.
else
 wait 1
 %echo% %self.name% spots an intruder!
 wait 1
 punch %actor.name%
end
~
#18002
UDF Password~
2 d 100
cranium~
if (%speech% == cranium)
 wait 1
 %echo% The hatch security panel flashes as the password is analyzed.
 wait 1 s
 %echo% The hatch opens with a hiss.
 if (%self.vnum% == 18002)
  %at% 18001 %echo% The hatch opens with a hiss.
 else
  %at% 18002 %echo% The hatch opens with a hiss.
 end
 %door% 18002 2 flags a
 %door% 18001 0 flags a
 wait 3 s
 %echo% The hatch closes with a hiss.
 if (%self.vnum% == 18002)
  %at% 18001 %echo% The hatch closes with a hiss.
 else
  %at% 18002 %echo% The hatch closes with a hiss.
 end
 %door% 18002 2 flags abc
 %door% 18001 0 flags abc
else
 wait 1
 %echo% The hatch security panel flashes as the password is analyzed.
 wait 1 s
 %echo% The security panel flashes a failure message and the hatch remains closed.
end
~
$~
