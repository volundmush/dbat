#5000
Shiverius transform 1~
0 l 30
~
if (%self.vnum% == 5001)
  wait 1
  say You honestly think you can beat me?
  wait 2 s
  say I am Shiverius, feel my icy wrath!
  wait 2 s
  %echo% %self.name% blinds the area with a bright light. Once it subsides, a colossal, white dragon can be seen, hovering above the ground, beating it's wings causing a small snow storm to shoot up.
  mtransform -5002
  detach 5000 %self.id%
  detach 5001 %self.id%
  attach 5002 %self.id%
  attach 5003 %self.id%
end
~
#5001
Shiverius form 1 fight~
0 k 9
~
deathball
~
#5002
Shiverius transform 2~
0 l 30
~
if (%self.vnum% == 5002)
  wait 1
  emote roars loudly.
  wait 2 s
  say You will now feel the wratch of my ultimate form!
  wait 2 s
  %echo% %self.name% blinds the area with a bright light. Once it subsides, a colossal, white dragon of Shiverius is nowhere to be seen. Instead a sleak, but faceless woman remains.
  mtransform -5003
  detach 5002 %self.id%
  detach 5003 %self.id%
  attach 5004 %self.id%
end
~
#5003
Shiverius Dragon Form fight~
0 k 5
~
emote blasts you with her icy breath, knocking you to your feet!
%damage% %actor% 30000000
%force% %actor% sit
~
#5004
Shiverius Final Form fight~
0 k 20
~
eval fight %random.4%
switch %fight%
  case 1
    deathbeam
    break
  case 2
    deathball
    break
  case 3
    %load% mob 5004
    %force% orb assist Shiverius
    break
  default
    %echo% A wave of healing energy washes over %self.name%.
    %damage% %self% -300000000
    break
done
~
#5005
Fireplace Trigger Frigid~
1 ab 25
~
if (%self.timer% <= 0)
 if (%self.room.vnum% != 5050)
  wait 10 s
  %echo% The campfire dies out leaving behind nothing but a pile of ash.
  if (!%self.room.contents(1613)%)
   %load% o 1613
  end
  %purge% self
 end
else
 eval num %random.10%
 switch %num%
  case 1
   wait 10 s
   %echo% A stick in the fire pops.
   break
  case 2
   wait 10 s
   %echo% The fire flashes brightly before growing dimmer again.
   break
  case 3
  case 4
  case 5
   wait 10 s
   %echo% Sparks fly up from the fire into the air.
   break
  default
  wait 10 s
  %echo% The fire gives off thick smoke for a moment.
  break
 done
end
~
$~
