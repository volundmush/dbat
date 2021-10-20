#6200
recall to yemma~
2 g 100
~
if %actor.is_pc%
wait 15 sec
  %teleport% %actor% 6015
end
~
#6201
Goz Speech~
0 g 100
~
if (%actor.align% >= 0)
 wait 1 s
 say Hey, would you perhaps wish to buy a shovel?
wait 3 s
say or how about some bandages?
 wait 2 s
 say I need to save up some money and I have plenty of both to get rid of.
 wait 3 s
 say Check my @glist@C to look at what I offer..@n
else
 wait 1 s
 say Enjoying your stay in hell?
end
~
#6202
Shovel Trigger~
1 c 3
dig~
if (%actor.room.vnum% >= 6100 && %actor.room.vnum% <= 6135)
 if (%actor.move% > %actor.maxmove% / 30)
  wait 1
  %send% %actor% You dig into the dirt with the shovel.
  %echoaround% %actor% %actor.name% digs into the dirt with the shovel.
  wait 2 s
  %send% %actor% You continue digging.
  %echoaround% %actor% %actor.name% continues to dig.
  wait 2 s
  eval num %random.12%
  switch (%num%)
   case 1
    %send% %actor% You dig up a Vilch Herb!
    %echoaround% %actor% %actor.name% digs up a Vilch Herb.
    wait 1
    %load% obj 7003
    %force% %actor% get herb
    wait 1 s
    %send% %actor% You ruined the shovel though... Oh well.
    %echoaround% %actor% %actor.name% ruined %actor.hisher% shovel though.
    eval minus %actor.maxmove% / 30
    eval equa -%minus%
    set %actor.move(%equa%)% 
    %purge% %self%
   break
   case 2
    %send% %actor% You don't find anything..
    %echoaround% %actor% %actor.name% doesn't find anything useful.
    eval minus %actor.maxmove% / 30
    eval equa -%minus%
    set %actor.move(%equa%)%
   break
   default
    %send% %actor% All you find are useless rocks.
    %echoaround% %actor% %actor.name% digs up some rocks but seems uninterested in them.
    eval minus %actor.maxmove% / 30
    eval equa -%minus%
    set %actor.move(%equa%)%
   break
  done
 else
  wait 1
  %send% %actor% You are too tired to dig..
 end
else
 wait 1
 %send% %actor% You can't dig here with this.
end
~
#6203
Goz Speech~
0 g 100
~
wait 1
say If you need to level up, I can help with that. Though I do not know any skills.
~
#6204
Mez~
0 g 100
~
wait 1
say Hey, I can help you @ggain@C a higher level.@n
~
#6230
Test~
1 ab 100
~
 
~
$~
