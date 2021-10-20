#8800
Time Travel Stone~
2 c 100
remember~
if (%self.vnum% == 8838)
  if %actor.level% >= 70
    wait 1 s
    %send% %actor% You start to feel dizzy.
    wait 2 s
    %send% %actor% A flash of bright white light envelops you.
    wait 2 s
    %teleport% %actor% 8849
    %send% %actor% You find yourself admist a great battle!
  else
    wait 1 s
    %send% %actor% You feel a dizzy sensation but nothing else happens.
    wait 2 s
    %send% %actor% Come back when you are stronger.
  end
elseif (%self.vnum% == 8849)
  wait 1 s
  %send% %actor% You start to feel dizzy.
  wait 2 s
  %send% %actor% A flash of bright white light envelops you.
  wait 2 s
  %teleport% %actor% 8838
  %send% %actor% You find yourself back in the Memorial Field!
end
~
#8801
Quit out trigger~
2 c 100
quit~
wait 1 s
%send% %actor% You can't quit out here! Return to the present.
~
$~
