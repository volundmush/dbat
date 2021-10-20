#18300
Malice Throne trigger~
1 c 100
sit~
if %arg% == command
if (%actor.name% == Juliet)
%force% %actor% sit throne
%echoaround% %actor% %actor.name% sits down on the throne!
wait 2 s
%send% %actor% Your mind is flooded with data as control of the Haven Spire is transfered to you!
%echoaround% %actor% The purple lines on the throne of malice glow brightly!
return 0
else
%send% %actor% Nothing happens!
end
else
%send% %actor% Try sitting on the throne!
end
~
#18301
RoM elevator~
2 c 100
push~
if ("%arg%" == "five") 
  if %self.vnum% != 18306 
    %send% %actor% You have pushed the button for Kessler Industries Offices.
    %echoaround% %actor% %actor.name% has pushed the button for the 5th floor.
if (%actor.clan(Reavers of Malice)% && %actor.inventory(18301)%)s
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "5th Floor - Inner Sanctum" 
    wteleport all 18306
else
%send% %actor% You lack the clearance for this floor.
end
  else 
    %send% %actor% You are already on that floor 
  end 
end
if ("%arg%" == "four") 
  if %self.vnum% != 18305
    %send% %actor% This floor is currently under construction. Access is restricted. 
  else 
    %send% %actor% You are already on that floor 
  end 
end
if ("%arg%" == "three") 
  if %self.vnum% != 18304
    %send% %actor% You have pushed the button for Research and Development. 
    %echoaround% %actor% %actor.name% has pushed the button for the 3rd floor.
if (%actor.clan(Reavers of Malice)%) 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "3rd Floor - Research & Development" 
    wteleport all 18304
else
%send% %actor% You lack the clearance for this floor.
end
  else 
    %send% %actor% You are already on that floor 
  end 
end
if ("%arg%" == "two") 
  if %self.vnum% != 18303
    %send% %actor% You have pushed the button for Kessler Industries stores. 
    %echoaround% %actor% %actor.name% has pushed the button for the 2nd floor.
if (%actor.clan(Reavers of Malice)%) 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "2nd Floor - KI Stores" 
    wteleport all 18303
else
%send% %actor% You lack the clearance for this floor.
end
  else 
    %send% %actor% You are already on that floor 
  end 
end
if ("%arg%" == "one") 
  if %self.vnum% != 18302 
    %send% %actor% You have pushed the button for Hangar Bay. 
    %echoaround% %actor% %actor.name% has pushed the button for the 1st floor. 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "1st Floor - Hangar Bay" 
    wteleport all 18302
  else 
    %send% %actor% You are already on that floor 
  end 
end
~
#18302
Guard Trigger~
0 g 100
~
if (%actor.clan(Reavers of Malice)%)
 wait 1
 %echo% %self.name% nods %self.hisher% head in greeting.
else
 wait 1
 %echo% %self.name% spots an intruder!
 wait 1
 punch %actor.name%
end
~
#18303
Haven Spire Alarm~
2 c 100
push~
if (%arg% == alarm)
if (!%actor.clan(Reavers of Malice)%)
%send% %actor% You lack the security clearance to do so!
else
%send% %actor% You push the alarm button!
%echoaround% %actor% %actor.name% pushes the alarm button!
wait 1 s
%zoneecho% 18300 @R"Security Alert. Security Alert."@n
wait 2 s
%zoneecho% 18300 @R"All non-Kessler Industries life-forms are advised to evacuate Haven Spire immediately or be terminated."@n
wait 5 s
%zoneecho% 18300 @R"Security Alert. All intruders will be terminated."@n
attach 18302 %mob.18300%
attach 18304 %self.id%
detach 18303 %self.id%
end
elseif (%arg% == disarm)
if (!%actor.clan(Reavers of Malice)%)
%send% %actor% You lack the security clearance to do!
else
%send% %actor% The Haven Spire is not currently locked down. Please push the alarm.
end
else
%send% %actor% Try pushing alarm or disarm
end
~
#18304
Haven Spire disarm~
2 c 100
push~
if (%arg% == disarm)
if (!%actor.clan(Reavers of Malice)%)
%send% %actor% You lack the security clearance to do so!
else
%send% %actor% You push the disarm button!
%echoaround% %actor% %actor.name% pushes the disarm button!
wait 1 s
%zoneecho% 18300 @R"Haven Spire lockdown has been removed."@n
detach 18302 %mob.id(18300)%
attach 18303 %self.id%
detach 18304 %self.id%
end
elseif (%arg% == alarm)
if (!%actor.clan(Reavers of Malice)%)
%send% %actor% You lack the security clearance to do!
else
%send% %actor% The Haven Spire is already currently locked down. Please push the disarm to remove it.
end
else
%send% %actor% Try pushing alarm or disarm
end
~
$~
