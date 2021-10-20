#1700
Sixteen Greet~
0 g 100
~
if (%actor.level% <= 2)
wait 1
say Hello there %actor.name%, as someone so new to this I would suggest you speak with Ten, he will instruct you on where you can go to get started.
 if !%actor.varexists(andeq)%
  wait 2 sec
  say Now %actor.name% here you may want to wear this.
wait 1 sec
  mload obj 1700
  give logo %actor.name%
  set andeq
  remote andeq %actor.id%
 end
elseif (%actor.level% < 5)
wait 1
say %actor.name% you are still pretty new, learn all you can.
 if %actor.varexists(andeq)%
  rdelete andeq %actor.id%
end
elseif (%actor.level% < 10)
wait 1
say %actor.name% you are growing at a sufficient rate.
elseif (%actor.level% < 20)
wait 1
say %actor.name% you are displaying decent progress.
elseif (%actor.level% < 40)
wait 1
say %actor.name% you are displaying significant progress.
elseif (%actor.level% < 75)
wait 1
say %actor.name% you are a model of continual progress.
elseif (%actor.level% < 100)
wait 1
say %actor.name% you are nearly beyond my measure to evaluate.
elseif (%actor.level% == 100)
wait 1
say %actor.name% you have progressed so far my circuits can no longer categorize you, why are you here?
smile
else
wait 1
say %actor.name% I don't know what to say about you.
end
~
#1701
Sixteen Guild Guard~
0 q 100
~
if (%direction% == inside && %actor.class% != android 16)
 return 0
 wait 1
 say You may not enter, permission denied.
* elseif (%actor.carry%)
*  return 0
*  wait 1
* say You may not carry anyone inside.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == inside && %actor.class% == android 16)
wait 1
 say Welcome in.
%send% %actor% @CAndroid Ten @wsays, '@CWelcome in.@w'@n
return 1
end
~
#1702
Ten Newb Quest~
0 g 100
~
if (%actor.class% == Android 16)
    if (%actor.level% <= 2)
        wait 1
        %echo% Beeping sounds come from someone standing next to the door, '@CFacial features recorded. Welcome student of Sixteen. My designation is Ten, and Sixteen has instructed me to tell you where you may be able to train@n'
        wait 2 s
        emote points at a nearby sign, &1These signs will tell you how to get to a park in Nexus City, it is labeled as @GRosewater Park@C.@n&2
        wait 2 s
        say There are multiple signs that tell you how to get there and back.
    
    elseif (%actor.level% >= 5 && %actor.level% <= 9 && !%actor.varexists(TenQ1Complete)% && !%actor.varexists(onTenQ1) && !%actor.varexists(TenQComplete)%)
        wait 1
        emote looks over %actor.name%, &1Data found, relaying mission information. You are to go to @GHeaven's Gate Dojo@C in Nexus City. It is on @GEagle Street@C, you passed this street on the way to the Park.@n&2
        wait 2 s
        say A @GWhite Belt@C in there recently found a strange device, and it's messing with our sensors. You are to go there, get it, and return it to me in anyway possible.@n
        set onTenQ1
        remote onTenQ1 %actor.id%
        
    elseif (%actor.level% >= 10 && %actor.level% <= 14 && !%actor.varexists(TenQ2Complete)% && !%actor.varexists(onTenQ2)% && !%actor.varexists(TenQComplete)%)
        wait 1
        emote looks over %actor.name%, &1Data found, relaying mission information. You are to go to the @GNorthern Plains@C and get a Bobcat pelt.@n&2
        wait 2 s
        say To get there, go north instead of south in the Nexus Plains to get to Cherry Blossom Mountain. Climb it and then cross the Sandy Desert. All this lies to the north.
        wait 1 s
        say An alternative route you can use is to work on your @Gfocus@C programming and try to increase the effectiveness of it by @G30@C percent, and work on your flying. You could @Gfly@C into @Gspace@C and get around quicker that way.@n
        set onTenQ2
        remote onTenQ2 %actor.id%
        
    elseif (%actor.level% >= 15 && %actor.level% <= 19 && !%actor.varexists(TenQ3Complete)% && !%actor.varexists(onTenQ3)% && !%actor.varexists(TenQComplete)%)
        wait 1
        emote looks over %actor.name%, &1Data found, relaying mission information. You are to go to @GCherry Blossom Mountain@C to the north and get some scales from the Tyrannosaurus Rex living there.@n&2
        wait 2 s
        say If you are curious about why I need these items, it is none of your concern.
        set onTenQ3
        remote onTenQ3 %actor.id%
        
    elseif (%actor.level% >= 20 && !%actor.varexists(TenQ4Complete)% && !%actor.varexists(onTenQ4)% && !%actor.varexists(TenQComplete)%)
        wait 1
        emote looks over %actor.name%, &1Data found, relaying mission information. You are to go to @GWest City@C. There you shall find a place called @GSilver Mine@C.@n&2
        wait 2 s
        say In this mine, there are creatures called @GSilver Worms@C. They sometimes eat valuable gems that form in the walls. You are to get a @GDiamond@C for me.@n
        wait 2 s
        emote looks over %actor.name%, &1I'm not going to tell you how to get there. You are to use your own processors for that. The name should give you all the information you need.&2
        set onTenQ4
        remote onTenQ4 %actor.id%     
        
    else
        if (!%actor.varexists(onTenQ1)% && !%actor.varexists(onTenQ2)% && !%actor.varexists(onTenQ3)% && !%actor.varexists(onTenQ4)%)
            wait 1
            say Data found. You may enter, %actor.name%.
        end
    end 
else
    wait 1
    say I have no data of you. You may not enter.
end
~
#1703
Gero's Lab Passcode~
2 c 100
enter~
if %arg% == 57624
if %self.inside(bits)% != DOOR)
wait 1
%echoaround% %actor% %actor.name% punches in a code on the door panel.
%send% %actor% you punch in the code on the panel.
wait 1 sec
%echo% The door beeps loudly for a second before sliding open. A counter above the panel starts ticking off seconds starting with ten.
%load% mob 1702
%teleport% govmir 7701
%echoaround% govmir The door beeps loudly for a second before sliding open. A counter above the panel starts ticking off seconds starting with ten.
wdoor 1715 10 flags a
wdoor 7701 11 flags a
wait 10 sec
%echo% The door beeps loudly and then closes with a whoosh. Locking with a loud grinding sound.
%echoaround% govmir The door beeps loudly and then closes with a whoosh. Locking with a loud grinding sound.
%teleport% govmir %self.vnum%
%purge% govmir
wdoor 1715 10 flags abc
wdoor 7701 11 flags abc
else
wait 1 sec
%send% %actor% The door is already open, you had best hurry through it.
end
else
wait 1 sec
%echoaround% %actor% %actor.name% punches in a code on the door panel.
%send% %actor% you punch in the code on the panel.
wait 1 sec
%echo% The door doesn't respond.
end
~
#1704
Gero's Lab Passcode 2~
2 c 100
enter~
if %arg% == 57624
 if %self.inside(bits)% != DOOR)
  wait 1
  %echoaround% %actor% %actor.name% punches in a code on the door panel.
  %send% %actor% you punch in the code on the panel.
  wait 1 sec
  %load% mob 1702
  %echo% The door beeps loudly for a second before sliding open. A counter above the panel starts ticking off seconds starting with ten.
  %teleport% govmir 1715
  %echoaround% govmir The door beeps loudly for a second before sliding open. A counter above the panel starts ticking off seconds starting with ten.
  wdoor 1715 10 flags a
  wdoor 1716 11 flags a
  wait 10 sec
  %echo% The door beeps loudly and then closes with a whoosh. Locking with a loud grinding sound.
  %echoaround% govmir The door beeps loudly and then closes with a whoosh. Locking with a loud grinding sound.
  %teleport% govmir %self.vnum%
  %purge% govmir
  wdoor 1715 10 flags abc
  wdoor 1716 11 flags abc
 else
  wait 1 sec
  %send% %actor% The door is already open, you had best hurry through it.
 end
else
 wait 1 sec
 %echoaround% %actor% %actor.name% punches in a code on the door panel.
 %send% %actor% you punch in the code on the panel.
 wait 1 sec
 %echo% The door doesn't respond.
end
~
#1705
Sensei Guard Redundancy~
0 q 100
~
return 0
~
#1706
Ten Noob Quest Completion~
0 j 100
~
if (%actor.varexists(onTenQ1)% && %object.vnum% == 687)
  %echoaround% %actor% %actor.name% gives Android Ten a strange looking device.
  %purge% %object%
  wait 1
  emote looks over the object, &1Confirmed. Mission completed.&2
  wait 2 s
  emote crushes the device, &1Here is your reward for completing the mission. Return later for a new mission.&2
  wait 1 s
  emote hands over a small sack of zenni and then starts picking through the parts of the crushed device.
  nop %actor.gold(500)%
  rdelete onTenQ1 %actor.id%
  set TenQ1Complete
  remote TenQ1Complete %actor.id%
  
elseif (%actor.varexists(onTenQ2)% && %object.vnum% == 1411)
  %echoaround% %actor% %actor.name% gives Android Ten a Bobcat Pelt.
  %purge% %object%
  wait 1
  emote looks over the pelt, &1Confirmed. Mission completed. I thank you in your assistance with this task.&2
  wait 1 s
  say Here is your reward. Some money and tickets to board a spaceship in the spaceport. These ships will take you to a Space station, where you can get on other ships to other planets.
  nop %actor.gold(500)%
  %load% obj 3999
  %load% obj 3999
  %load% obj 3999
  %load% obj 3999
  give all.ticket %actor.name%
  wait 1
  say The spaceport is located near the center of Nexus on @GShale Boulevard@C. I may have another task for you when you get stronger. Return later.@n
  rdelete onTenQ2 %actor.id%
  set TenQ2Complete
  remote TenQ2Complete %actor.id%
  
elseif (%actor.varexists(onTenQ3)% && %object.vnum% == 1205)
  %echoaround% %actor% %actor.name% gives Android 10 some deep green scales.
  %purge% %object%
  wait 1
  emote takes the scales, &1Confirmed. Mission completed.&2
  wait 2 s
  emote hands over a large bag, &1For your continued work, I have included extra Zenni. Return later for another mission.&2
  nop %actor.gold(2000)%
  rdelete onTenQ3 %actor.id%
  set TenQ3Complete
  remote TenQ3Complete %actor.id%
  
elseif (%actor.varexists(onTenQ4)% && %object.vnum% == 2013)
  %echoaround% %actor% %actor.name% gives Android 10 a Diamond.
  %purge% %object%
  wait 1
  emote looks over the diamond, &1Confirmed. Mission complete. All tasks completed.&2
  wait 3 s
  emote grabs some green armor with Sixteen's symbol on the front, &1This is your reward, to show that you are a true student of Sixteen.&2
  %load% obj 1769
  give armor %actor.name%
  rdelete onTenQ4 %actor.id%
  rdelete TenQ1Complete %actor.id%
  rdelete TenQ2Complete %actor.id%
  rdelete TenQ3Complete %actor.id%
  set TenQComplete
  remote TenQComplete %actor.id%
  
else
  return 0
  wait 1
  %echoaround% %actor% %actor.name% attempts to give Android Ten something.
  say I have no data on needing this.
end
~
#1710
Earth Soldier Quarantine Block~
0 q 100
~
if (%direction% == west && %actor.level% != 101)
  return 0
  wait 1 s
  say West City has been quarantined until further notice.
  wait 2 s
  say I can only let authorized individuals through, sorry.
elseif (%direction% == west && %actor.level% == 101
  wait 1 s
  emote nods.
  return 1
end
~
#1711
Ten Quest checkers~
0 g 100
~
if (%actor.varexists(onTenQ1)%)
    wait 1
    say Did you finish your mission? Or have you lost your way?
    wait 2 s
    say Remember, go to @GHeaven's Gate Dojo@C in Nexus City, which is on @GEagle Street@C. You may have passed this street on your way to the Park. We need that device that a White Belt there picked up.@n
    
elseif (%actor.varexists(onTenQ2)%)
    wait 1
    say Did you finish your mission? Or have you lost your way?
    wait 2 s
    emote points north, &1Remember, go north of @GNexus@C to @GCherry Blossom Mountain@C and follow the path through it, into the @GDesert@C while continuing north and then you are there. And don't forget the Bobcat Pelt.@n&2
elseif (%actor.varexists(onTenQ3)%)
    wait 1
    say Did you finish your mission? Or have you lost your way?
    wait 2 s
    emote points north, &1Just head north and climb @GCherry Blossom Mountain@C. Remember to bring back the scales.@n&2
elseif (%actor.varexists(onTenQ4)%)
    wait 1
    say Did you finish your mission? Or have you lost your way?
    wait 2 s
    say Remember, the Silver Mine is in West City. Bring back the diamond.
end
~
$~
