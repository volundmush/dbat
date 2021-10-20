#8600
Dres Captain Warning~
0 g 100
~
wait 2 s
say I wouldn't head west past the farm if I were you.. that area is abandoned and dangerous.
wait 2 s
say No one heads out to St Alucia anymore.
wait 3 s
say Rumours say that were-konatsu stalk there now.
wait 3 s
say If that's true and you bring me fur as evidence, I will pay you.
~
#8601
Milking station pump~
1 c 100
pump~
if %arg% == milk
  if !%actor.varexists(pumping_milk)%
    set pumping_milk
    remote pumping_milk %actor.id%
    wait 1
    %send% %actor% You press a button the Yaxe Milking Station, causing an alert to go off.
    %echoaround% %actor% %actor.name% presses a button on the Yaxe Milking station, causing an alert to go off.
    wait 5 s
    %echo% A Yaxe is carefully herded into the Milking Station by one of the Colvian Farm's workers.
    wait 3 s
    %send% %actor% You press another button and the Milking Station starts humming loudly as the milking process begins.
    %echoaround% %actor% %actor.name% presses another button on the Milking Station and it begins humming loudly as the milking process begins.
    wait 5 s
    %echo% A loud "MOOOOOOOOO" can be heard coming from inside the Milking station.
    wait 5 s
    %echo% A loud humming sound errupts from inside the machine as the milking process picks up.
    wait 5 s
    %echo% "MAAAAAAAA" can be heard coming from inside the machine just over the humming noise.
    wait 5 s
    %echo% The humming noise starts to lower in volume.
    wait 5 s
    eval milkbottle %random.4%
    switch %milkbottle%
      case 1
        %echo% The humming of the Milking Station dies down as a bottle of milk slides down the conveyor belt.
        wait 2 s
        %send% %actor% You grab the bottle of milk and examine it.
        %echoaround% %actor% %actor.name% grabs the bottle of milk off the conveyor belt and begins examining it.
        wait 2 s
        %send% %actor% The milk is perfect! A keeper!
        %echoaround% %actor% %actor.name% seems pleased about the milk!
        %load% obj 8602
        %force% %actor% get bottle
        rdelete pumping_milk %actor.id%
        wait 2 s
        %echo% The Yaxe inside the Milking station comes outside, then is herded back toward the paddock.
      break
      case 2
        %echo% The humming of the Milking Station dies down as a bottle of milk slides down the conveyor belt.
        wait 2 s
        %send% %actor% You grab the bottle of milk and examine it.
        %echoaround% %actor% %actor.name% grabs the bottle of milk off the conveyor belt to examine it.
        wait 2 s
        %send% %actor% The milk is off! You throw it away.
        %echoaround% %actor% %actor.name% throws the bottle of milk %actor.heshe% was holding, away.
        rdelete pumping_milk %actor.id%
        wait 2 s
        %echo% The Yaxe inside the Milking station comes outside, then is herded back toward the paddock.
      break
      case 3
        %echo% The humming of the Milking Station dies down as a bottle of milk slides down the conveyor belt.
        wait 2 s
        %send% %actor% You grab the bottle of milk and examine it.
        %echoaround% %actor% %actor.name% grabs the bottle of milk off the conveyor belt and begins examining it.
        wait 2 s
        %send% %actor% The milk is of pristine quality. A real keeper!
        %echoaround% %actor% %actor.name% seems very pleased about the milk!
        %load% obj 8603
        %force% %actor% get bottle
        rdelete pumping_milk %actor.id%
        wait 2 s
        %echo% The Yaxe inside the Milking station comes outside, then is herded back toward the paddock.
      break
      default
        %echo% The humming of the Milking station dies down as a bottle of milk slides down the conveyor belt.
        wait 2 s
        %send% %actor% You go to pick the bottle of milk up and examine it, but fumble it, sending it flying to the ground, smashing it!
        %echoaround% %actor% %actor.name% goes go to pick the bottle of milk up and examine it but %actor.heshe% fumbles it, sending it flying to the ground, smashing it!
        rdelete pumping_milk %actor.id%
      break
    done
  else
    %send% %actor% You are already pumping milk!!
  end
else
  %send% %actor% Try pumping milk!
end
~
#8602
Leave trigger for Milking station~
2 q 100
~
if %actor.varexists(pumping_milk)%
%send% %actor% You can't leave while using the milking station!
return 0
end
~
#8603
Farm Guard captain reward~
0 j 100
~
if %object.vnum% == 8700
  junk fur
  wait 1 s
  say A piece of were-konatsu fur? Alright then..
  wait 2 s
  emote hands you a bag of zenni.
  nop %actor.gold(1000)%
else
  say I don't want that!
  drop %object.name%
end
~
#8604
Colvian milk reward~
0 j 100
~
if %object.vnum% == 8602
  %purge% %object%
  wait 1 s
  say Excellent work, keep the milk coming!
  wait 2 s
  emote hands you a bag of zenni.
  nop %actor.gold(100)%
elseif %object.vnum% == 8603
  %purge% %object%
  wait 1 s
  say Excellent work, the quality of this milk is great!
  wait 2 s
  emote hands you a bag of zenni.
  nop %actor.gold(200)%
else
  say I don't want that!
  drop %object.name%
end
~
#8605
Colvian milk greet~
0 g 100
~
wait 1 s
say Hi there %actor.name%, want to earn some zenni?
wait 3 s
say Just head on over to the barn and find the milking station.
wait 2 s
say If you @Ypump milk@C for me then hand me the bottles, I will reward you.@n
~
#8606
Remove Milk~
2 c 100
MilkMe~
If (!%arg% == please)
 %send% %actor% Try saying Please.
Else
 rdelete pumping_milk %actor.id%
 %send% %actor% You said Please!
End
~
#8607
new trigger~
1 c 100
event~
if %arg% == rppreward
  wait 1
  set pplcount 0
  set playercount 0
  set mypeople %self.room.people%
  while %mypeople%
    set randrpp 1
    if %mypeople.is_pc%
      %echo% Hello, %mypeople.name%.  You get (%randrpp%) RPP for participating in this event!
      %force% %actor.name% %mypeople.rpp(%randrpp%)%
      eval playercount %playercount% + 1
    end
    set mypeople %mypeople.next_in_room%
    eval pplcount %pplcount% + 1
  done
  %echo% There are %pplcount% people in this room, %playercount% of which are player characters.
elseif %arg% == assistant
  %load% mob 9198
  %echo% An assistant appears to help %actor.name%!
else
  wait 1
  %send% %actor% =======================[ Current Commands ]=======================
  %send% %actor% [RPPreward]  Grants 1 RPP to everyone in this room.
  %send% %actor% [Assistant]  Summons an assistant to help with events.
  %send% %actor%  
  %send% %actor% ==================================================================
  %send% %actor% Usage:  Event <command> [optional] [optional]
end
~
$~
