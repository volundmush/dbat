#1500
HBTC Enter~
2 g 100
~
if (%direction% == west)
if (%actor.level% < 25)
return 0
wait 1
%send% %actor% You will get no use out of this until at least level 25.
elseif !%actor.varexists(hbtconce)%
 wait 1
 %send% %actor% You feel this place opperates on a different time frame from the rest of the universe.
 wait 3 sec
 %send% %actor% You can only enter this place twice, so be sure to do all you can before you leave.
 wait 4 sec
 %send% %actor% This is your first visit to the Hyperbolic Time Chamber.
 set hbtconce
 remote hbtconce %actor.id%
elseif %actor.varexists(hbtconce)% && !%actor.varexists(hbtctwice)%
 wait 1
 %send% %actor% This is your second and last visit to the Hyperbolic Time Chamber.
 set hbtctwice
 remote hbtctwice %actor.id%
else
 %force% %actor% west
 wait 1
 %send% %actor% You have already been to the Hyperbolic Time Chamber twice, you can enter it no more.
end
end
~
#1501
Portal leading to Hell~
2 c 100
enter~
if (%cmd% == enter && %arg% == portal)
  if (%actor.race% == Demon)
    wait 1 s
    %send% %actor% You enter the portal leading to @RH@De@rl@Rl@n and are quickly transported there!
    %echoaround% %actor% %actor.name% enters the portal and vanishes!
    %force% %actor% drag
    %teleport% %actor% 6484
    %at% %actor% %force% %actor% look
  else
    return 0
    %send% %actor% @DOnly @RDemons@D can enter this portal!@n
  end
else
   return 0 
  %send% %actor% @RTry typing enter portal@n.
end
~
#1502
Past Stuck~
2 c 2
escapepast~
%send% %actor% Sending you back to the Lookout.
%teleport% %actor% 1561
rdelete pendcomp %actor.id%
~
#1510
Super Buu Greet~
0 g 100
~
if (%actor.align% > 51)
 wait 1
 say You aren't entertaining enough for me...
 wait 2 s
 say Get out of here before I eat you...
else
 if (%actor.varexists(superbuu)%)
  wait 1
  say Mmmm that chocolate was good... maybe you bring more?
  wait 2 s
  say What you want me to teach you?
 else if (%actor.has_item(1500)%)
  wait 1
  say YOU have CHOCOLATE! Give it to me!
  wait 3 s
  say If you give it to me now you can live... Maybe I even teach you something fun..
 else
  wait 1
  say Hmph. You might be good entertainment...
  wait 2 s
  say Maybe I rip your body to pieces before I make you candy...
  wait 4 s
  say Unless you have something really good for me.
  wait 3 s
  say Then maybe I spare you... maybe.
  wait 3 s
  say Something large...grand...megasized.. and Chocolatey.
 end
end
~
#1511
Super Buu Receive~
0 j 100
~
if %actor.varexists(ElwinTrain)%
  wait 1
  say You train with Ghost!  Buu no like!  NO LIKE!  YOU GO AWAY NOW!
  %purge% %object%
elseif %actor.varexists(kktrain)%
  wait 1
  say You smell like stupid kaio.  Buu no train you.  You train with goody two shoe snobs that no have fun!
  %purge% %object%
elseif (%actor.varexists(superbuu)%)
  wait 1
  say You already gave me one but ok.
  wait 2 s
  %echo% %self.name% munches on %object.shortdesc%.
  %purge% %object%
  wait 2 s
  say That was best I have ever had... 
elseif (%actor.align% > 49)
  return 0
  wait 1
  say I don't want nothing from you... goody goody..
elseif (%actor.level% < 30 && %object.vnum% == 1500)
  return 0
  wait 1
  %send% %actor% You feel like you need to be at least level 30 to complete this quest.
elseif (%object.vnum% == 1500)
  wait 1
  say Wow... this looks good...
  wait 2 s
  %echo% %self.name% munches on %object.shortdesc%.
  %purge% %object%
  wait 2 s
  say That was best I have ever had...
  wait 3 s
  say Ok you can train with me..
  set superbuu
  remote superbuu %actor.id%
elseif (%object.type% == FOOD)
  wait 1
  say This isn't what I want...
  wait 2 s
  %echo% %self.name% munches on %object.shortdesc%.
  %purge% %object%
  wait 2 s
  say It was good enough to let you live...
else
  wait 1
  say This isn't what I want...
  wait 2 s
  %echo% %self.name% munches on %object.shortdesc%.
  %purge% %object%
  wait 2 s
  say Blech! This taste awful...
  wait 2 s
  say You better hurry and give me the chocolate...
end
~
#1512
Super Buu Command~
0 c 100
pr~
if (%actor.varexists(kktrain)%)
return 1
wait 1
say You can't train with me.. You train with stupid kaio.
elseif (%actor.varexists(ElwinTrain)%)
return 1
say You can't train with me... you train with stupid ghost.
elseif (!%actor.varexists(superbuu)%)
 return 1
 wait 1
 say You can't train with me... give me chocolate!
else
return 0
end
~
#1513
RPP Test~
2 c 100
depart~
%send% %actor% @RYou open the door to the normal world and depart from the Hypbolic Time Chamber.@n
wait 1s
%teleport% %actor% 1573
wait 1
%echoaround% %actor% @RThe door on the east wall opens and someone exits the room...@n
%force% %actor% l
~
$~
