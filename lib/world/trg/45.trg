#4500
Glugs Volcano- Boulder block part 1~
0 c 100
slam~
if ("%arg%" != "boulder")
%send% %actor% Maybe slam the boulder.
elseif (%actor.str% < 14)
%send% %actor% Perhaps you are not strong enough to break this boulder.
else
%send% %actor% You slam the boulder with amazing force causing it to shatter and a path leading down has been revealed.
%echoaround% %actor% A large boulder has been shattered with a thunderous blow from %actor.name%, revealing a path leading downward.
%purge% self
end
~
#4501
Glugs Volcano - Boulder block part 2~
0 q 100
~
if (%direction% == down && %actor.race% != orc)
%send% %actor% A %self.name% prevents you from going any further. Maybe try to slam it?
%echoaround% %actor% As %actor.name% tries to leave down, %actor.heshe% walks right into a boulder.
%self.name% stops %actor.himher%
return 0
end
~
$~
