#18200
League of Shadows front door~
2 d 100
pondero~
if (%speech% == pondero)
 wait 1
 %echo% The sound of stone grinding can be heard.
 wait 1 s
 %echo% The wall here starts turning slowly.
 if (%self.vnum% == 18203)
  %at% 18205 %echo% The wall here is now fully open, creating an open passage.
 else
  %at% 18203 %echo% The wall here is now fully open, creating an open passage.
 end
 %echo% Welcome %actor.name%.
 %door% 18205 8 flags a
 %door% 18203 6 flags a
 wait 3 s
 %echo% The wall grinds shut.
 if (%self.vnum% == 18203)
  %at% 18205 %echo% The wall grinds shut.
 else
  %at% 18203 %echo% The wall grinds shut.
 end
 %door% 18203 6 flags abc
 %door% 18205 8 flags abc
else
 wait 1
 %echo% Nothing seems to happen.
 wait 1 s
end
~
#18201
Drone Guard Trigger~
0 g 100
~
if (%actor.clan(League of Shadows)%)
 wait 1
 %echo% %self.name% beeps in acknowledgement that %actor.name% is not an intruder.
else
 wait 1
 %echo% %self.name% spots an intruder and goes into attack mode
 wait 1
 punch %actor.name%
end
~
#18202
Merc Guard Trigger~
0 g 100
~
if (%actor.clan(League of Shadows)%)
 wait 1
 %echo% %self.name% nods %self.hisher% head at %actor.name%
else
 wait 1
 %echo% %self.name% spots an intruder and prepares to repel them!
 wait 1
 punch %actor.name%
end
~
#18203
Altar trapdoor trigger~
2 d 100
conflagration~
if (%speech% == conflagration)
  wait 1 sec
  %echo% A small rumbling sound can be heard.
  wait 2 s
  %echo% A trapdoor on the floor opens with a creak.
  if (%self.vnum% == 18266)
    %at% 18267 %echo% A trapdoor on the roof opens with a creak.
  else
    %at% 18266 %echo% A trapdoor on the floor opens with a creak.
  end
  %door% 18267 4 flags a
  %door% 18266 5 flags a
  wait 5 s
  %echo% The trapdoor closes.
  if (%self.vnum% == 18266)
    %at% 18267 %echo% The trapdoor closes.
  else
    %at% 18266 %echo% The trapdoor closes.
  end
  %door% 18266 5 flags abc
  %door% 18267 4 flags abc
end
~
#18204
Landing Pad door~
2 d 100
ira~
if (%speech% == ira)
 wait 1
 %echo% The security panel to the left of the door flashes as the password is analyzed.
 wait 1 s
 %echo% The door automatically swings open.
 if (%self.vnum% == 18211)
  %at% 18212 %echo% The door automatically swings open.
 else
  %at% 18211 %echo% The door automatically swings open.
 end
 %echo% Welcome %actor.name%.
 %door% 18212 2 flags a
 %door% 18211 0 flags a
 wait 3 s
 %echo% The door swings shut.
 if (%self.vnum% == 18211)
  %at% 18212 %echo% The door swings shut.
 else
  %at% 18211 %echo% The door swings shut.
 end
 %door% 18211 0 flags abc
 %door% 18212 2 flags abc
else
 wait 1
 %echo% The security panel to the left of the door flashes as the password is analyzed.
 wait 1 s
 %echo% The security panel flashes a failure message and the door remains closed.
end
~
#18205
Intruder Alarm~
2 g 100
~
if (%actor.clan(League of Shadows)%)
 wait 1 s
 %echo% A light flashes green.
else
 wait 1 s
 %echo% A red light flashes here.
 %zoneecho% %self.vnum% Intercom: Intruder alert. All personnel to designated stations.
end
~
#18206
Mansion door~
2 d 100
superbia~
if (%speech% == superbia)
 wait 1
 %echo% The security panel to the left of the door flashes as the password is analyzed.
 wait 1 s
 %echo% The door automatically swings open.
 if (%self.vnum% == 18292)
  %at% 18201 %echo% The door automatically swings open.
 else
  %at% 18292 %echo% The door automatically swings open.
 end
 %door% 18201 2 flags a
 %door% 18292 0 flags a
 wait 3 s
 %echo% The door swings shut.
 if (%self.vnum% == 18292)
  %at% 18201 %echo% The door swings shut.
 else
  %at% 18292 %echo% The door swings shut.
 end
 %door% 18201 2 flags abc
 %door% 18292 0 flags abc
else
 wait 1
 %echo% The security panel to the left of the door flashes as the password is analyzed.
 wait 1 s
 %echo% The security panel flashes a failure message and the door remains closed.
end
~
#18207
Look at gate trigger~
2 c 100
examine~
if %arg% == gate
  if %actor.clan(Black Spider Mercs)%
    wait 1 s
    %send% %actor% To pass, insert the code: 'Fallen Angel'
  else
    wait 1 s
    %send% %actor% @nPlease @Yinsert@n the code.
    wait 3 s
    %send% %actor% FZFYIAFPIYZ
    %send% %actor% YAXZFEXTKEP
    %send% %actor% MTLIBWGGZWC
    %send% %actor% VZVLJICRSDI
    %send% %actor% GWGHERXITHB
    %send% %actor% TVCOUNAKUNE
    %send% %actor% WEFSXRAQQKS
    %send% %actor% IMRRVTGNNSC
    %send% %actor% TOQNWTJMGVX
    %send% %actor% MGEGFNTQGEZ
    %send% %actor% XOJYNOYRAIL
  end
end
~
#18208
Trench gates open~
2 c 100
insert~
if (%arg% == Fallen Angel)
  wait 2 s
  %echo% Code verified. Please proceed quickly.
  wait 1 s
  %echo% The gate rises upward.
  if (%self.vnum% == 18278)
    %at% 18279 %echo% The gate rises upward.
  else
    %at% 18278 %echo% The gate rises upward.
  end
  %door% 18279 2 flags a
  %door% 18278 0 flags a
  wait 4 s
  %echo% The gate slams down shut.
  if (%self.vnum% == 18278
    %at% 18279 %echo% The gate slams down shut.
  else
    %at% 18278 %echo% The gate slams down shut.
  end
  %door% 18279 2 flags abc
  %door% 18278 0 flags abc
else
  wait 1 s
  %echo% Code incorrect.
  wait 2 s
  %echo% Please enter the correct code.
end
~
#18209
2nd Trench Gates Open~
2 c 100
insert~
if (%arg% == Fallen Angel)
  wait 2 s
  %echo% Code verified. Please proceed quickly.
  wait 1 s
  %echo% The gate rises upward.
  if (%self.vnum% == 18284)
    %at% 18285 %echo% The gate rises upward.
  else
    %at% 18284 %echo% The gate rises upward.
  end
  %door% 18285 2 flags a
  %door% 18284 0 flags a
  wait 4 s
  %echo% The gate slams down shut.
  if (%self.vnum% == 18284)
    %at% 18285 %echo% The gate slams down shut.
  else
    %at% 18284 %echo% The gate slams down shut.
  end
  %door% 18285 2 flags abc
  %door% 18284 0 flags abc
else
  wait 1 s
  %echo% Code incorrect.
  wait 2 s
  %echo% Please enter the correct code.
end
~
#18210
3rd Trench Gate opens~
2 c 100
insert~
if (%arg% == Fallen Angel)
  wait 2 s
  %echo% Code verified. Please proceed quickly.
  wait 1 s
  %echo% The gate rises upward.
  if (%self.vnum% == 18291)
    %at% 18292 %echo% The gate rises upward.
  else
    %at% 18291 %echo% The gate rises upward.
  end
  %door% 18292 2 flags a
  %door% 18291 0 flags a
  wait 4 s
  %echo% The gate slams down shut.
  if (%self.vnum% == 18291)
    %at% 18292 %echo% The gate slams down shut.
  else
    %at% 18291 %echo% The gate slams down shut.
  end
  %door% 18292 2 flags abc
  %door% 18291 0 flags abc
else
  wait 1 s
  %echo% Code incorrect.
  wait 2 s
  %echo% Please enter the correct code.
end
~
$~
