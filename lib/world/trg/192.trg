#19200
Rawr~
0 g 100
~
wait 1
mecho The Tarrasque roars loudly as you enter it's domain.
~
#19201
Vampiric Draining~
1 c 4
*~
if %cmd% == attack
%echo% You twist the handle of the dagger.
else
return 0
end
~
#19202
Jumping off Hatsugan's Battlement~
2 c 100
jump~
if %cmd% == jump
 %send% %actor% You jump off the battlement.
  %echoaround% %actor% %actor.name% jumps off the battlements.
 wait 2 s
  %teleport% %actor% 19215
  %force% %actor% look
%send% %actor% You land on your ass as you hit the ground.
 %force% %actor% sit
else
 return 0
end
~
#19203
Command Test~
2 c 100
commandtest~
%echo% The trigger tests successfully!
~
#19204
Key-Per's Greeting~
0 g 100
~
if (%actor.is_pc%)
wait 3 s
say Hello there %actor.name%. If you want a key, you are going to have to bring me someting from each of the four holding cells. If you need some basic supplies, I can sell them to you.
wait 10 s
say What are you still doing here!?! Get me those items!
~
#19205
Key-Per Recieve~
0 j 100
*~
if (%object.vnum% == 19214)
%echo% %self.name% takes %object.shortdesc%.
 %purge% %object%
set blueitem
 remote blueitem %actor.id%
say Thanks, I needed one of those. Now bring me the others!
elseif (%object.vnum == 19215)
%echo% %self.name% takes %object.shortdesc%.
 %purge% %object%
set reditem
  remote reditem %actor.id%
say Thanks, I needed one of those. Now bring me the others!
elseif (%object.vnum% == 19216)
%echo% %self.name% takes %object.shortdesc%.
 %purge% %object%
set blackitem
 remote blackitem %actor.id%
say Thanks, I needed one of those. Now bring me the others!
elseif (%object.vnum% == 19217)
%echo% %self.name% takes %object.shortdesc%.
 %purge% %object%
set purpleitem
 remote purpleitem %actor.id%
say Thanks, I needed one of those. Now bring me the others!
else return 0
end
if (%actor.varexists(blueitem)%)
 while (%actor.varexists(reditem)%)
 while (%actor.varexist (blackitem)%)
 while (%actor.varexists(purpleitem)%)
give key %actor%
load o 19213
else return 0
end
~
#19206
Key-Per Blue~
0 j 100
~
if (%object.vnum% == 19214)
wait 2
%echo% %self.name% takes %object.shortdesc%
%purge% %object%
wait 2
say Okay, heres the key, like I promised.
give key %actor.name%
load o 19213
else return 0
end
~
#19207
Key-Per Red~
0 j 100
~
if (%actor.varexists(reditem)%)
say You already gave me one of those!
return 0
elseif (%object.vnum% == 19215)
%echo% %self.name% takes %object.shortdesc%.
 %purge% %object%
set reditem
remote %reditem% %actor.id%
else return 0
end
~
#19208
Key-Per Black~
0 j 100
~
if (%actor.varexists(blackitem)%)
say You already gave me one of those!
return 0
elseif (%object.vnum% == 19216)
%echo% %self.name% takes %object.shortdesc%.
%purge% %object%
set blackitem
remote %blackitem% %actor.id%
else return 0
end
~
#19209
Key-Per Purple~
0 j 100
4~
if (%actor.varexists(purpleitem)%)
say You already gave me one of those!
return 0
elseif (%object.vnum% == 19216)
%echo% %self.name% takes %object.shortdesc%.
 %purge% %object%
set purpleitem
remote %purpleitem% %actor.id%
else return 0
end
~
#19210
Singing Sword~
1 b 100
~
%echo% This trigger commandlist is not complete!
~
#19233
buycandy Test~
0 c 3
buycandy~
if (%actor.race% != Majin)
    wait 1 s
    say I don't care if you want it, you're not of Majin kind.
else
    wait 1 s
    say Want some candy, do ya?
    wait 1 s
    say Hand over 1,500 zenni then.
    wait 1 s
    if (%actor.gold% <= 1500)
        say Get outta here, you don't have the zenni for some.
    else
    %send% %actor% You hand over some zenni.
    %echoaround% %actor% %actor.name% hands over some zenni.
    %actor.gold(-1500)%
    wait 1 s
    say Alright, here you go.%
 eval candy %random.3%
 switch %candy%
  case 1
   %load% obj 93
   give jawbreaker %actor.name%
   break
  case 2
   %load% obj 94
   give chocolate %actor.name%
   break
  default
   %load% obj 95
   give chocolate %actor.name%
  done
 end
end
~
$~
