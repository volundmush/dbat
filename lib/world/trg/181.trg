#18150
Guard trigger~
0 g 100
~
if (%actor.clan(Darkthorne Syndicate)%)
 wait 1
 %echo% %self.name% beeps an acknowledgement.
else
 wait 1
 %echo% %self.name% spots an intruder!
 wait 1
 punch %actor.name%
end
~
$~
