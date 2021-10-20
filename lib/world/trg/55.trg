#5500
Undefined~
0 c 100
talk~
%echo% This commandlist is incomplete
~
#5501
Conversation Trial prt 2~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% says, 'What would you like to talk about?'
    %send% %actor% @y------------------------------------@n
    %send% %actor% @ROption@Y 1)@n Blades
    %send% %actor% @ROption@Y 2)@n Kuriza
    %send% %actor% @ROption@Y 3)@n Latest Rumours
    %send% %actor% @ROption@Y 4)@n About me
    %send% %actor% @y------------------------------------@n
  elseif %arg% /= %self.alias% && %arg% /= Blades
    %send% %actor% %self.name% says, 'Ahh the Blades. They were once the elite guard that served the Royal Family of Frigid from the time Frigid was first unified up until a few centuries ago. The blades still maintain the ancient Royal Palace, Cloud Ruler Temple high up on Wyrm Spine Mountains. Rumor is they have a new master.'
  elseif %arg% /= %self.alias% && %arg% /= Kuriza
    %send% %actor% %self.name% says, 'Kuriza is the son of Frieza. He is the new charge under the protection of the ancient order of the Blades and seeks to usurp the throne. He is not one to be trifled with.'
  elseif %arg% /= %self.alias% && %arg% /= Latest rumors
    %send% %actor% %self.name% says, 'Have you heard? Kuriza is planning to usurp the throne. He might be looking for mercenaries.'
  elseif %arg% /= %self.alias% && %arg% /= About me
    %send% %actor% %self.name% says, 'I am %self.name% of the Blades.'
  end
end
~
#5502
Open Temple Door~
2 c 100
knock~
if %arg% == gate
  %door% 5500 north flags a
  %echo% The gate creaks open slowly.
else
%send% %actor% Try knocking on the gate.
end
~
#5503
Jumping off the battlements~
2 c 100
jump~
if %cmd% == jump
 %send% %actor% You jump off the battlments.
  %echoaround% %actor% %actor.name% jumps off the battlments.
wait 1 s
  %teleport% %actor% 5500
  %force% %actor% look
%send% %actor% You land on the ground with a thud.
else
  return 0
end
~
#5504
Knocker 2~
2 c 100
knock~
if %arg% == gate
wait 1
 %door% 5501 2 flags a
 %echo% The gate slides open.
else
 wait 1
 %send% %actor% Try it on the gate...
end
~
$~
