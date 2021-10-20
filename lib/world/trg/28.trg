#2800
Tree Roots~
1 g 100
~
if (!%self.carried_by%)
 wait 1
 %echo% Loose dirt falls from the roots of %self.shortdesc%.
end
~
#2801
Guardian Greet~
0 g 100
~
if (%actor.varexists(pride)%)
 wait 1 s
 %echo% %self.name%'s eyes glow.
 wait 2 s
 say Welcome %actor.name%, you have already proven yourself worthy to enter.
 wait 5 s
 say That is if you still possess your key.
else
 wait 1 s
 %send% %actor% %self.name%'s eyes glow as he comes to his full height and he looks at you.
 %echoaround% %actor% %self.name%'s eyes glow as he comes to his full height and he looks at %actor.name%.
 wait 4 sec
 say Do you dare the dangers of Pride Tower!?
 wait 3 sec
 say There are six floors to this tower, each more dangerous than the last.
 wait 4 sec
 say Defeat me to claim your key if you wish to challenge the tower!
end
~
#2802
Guardian Die~
0 f 100
~
if (!%actor.varexists(pride)%)
set pride
remote pride %actor.id%
end
~
#2803
Guardian Asshole~
0 q 100
~
if (%direction% == east && !%actor.varexists(pride)%)
 return 0
 wait 1
 say You have not proven yourself worthy!
 wait 1
 punch %actor.name%
else if (%direction% == east)
 wait 1
 close gate
 lock gate
end
~
$~
