#3600
Alukahan Guardian block~
0 q 100
~
if (%direction% == south)
   %send% %actor.name% %self.name% prevents you from moving to the south.
   %echoaround% %actor.name% As %actor.name% tries to move to the south but %self.name% stops %actor.himher%
return 0
else
return 1
end
~
#3601
Sihnon Portal~
2 c 100
enter~
if %arg% == portal
 if %actor.level% < 100
  %send% %actor% You feel like it would be a bad idea to enter this portal.
  wait 1 s
  %send% %actor% OOC - You need to be a higher level to enter this portal.
 else
  wait 1 s
  %send% %actor% You walk toward the portal and get sucked inside it!
  %echoaround% %actor% %actor.name% walks toward the portal and is sucked inside it!
  wait 1 s
  %teleport% %actor% 3700
  %force% %actor% look
 end
end
~
$~
