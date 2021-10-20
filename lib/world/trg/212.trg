#62500
Inner Airlock Leave~
1 c 4
ship~
if (%arg% == leave)
    %send% %actor% You press a button to open the air lock and warning lights blare as the pressure equalizes.
    %echoaround% %actor% %actor.name% presses a button to open the air lock and warning lights blare as the pressure equalizes.
    wait 2 s
    %echo% You feel the gravity lessen.
    wait 2 s
    %send% %actor% The door slowly slides open and you float out of the ship.
    %echoaround% %actor% The door slowly slides open and %actor.name% floats out of the ship.
    wait 1 s
    %teleport% %actor% %self.cost%
    %at% %actor% %echoaround% %actor% %actor.name% floats out of the ship.
    %force% %actor% look
else
    %send% %actor% Try @Rleaving@n?
end
~
#62501
Black Omen Teleporter - Air to VH~
2 c 100
press~
if ("%arg%" != "controls" && "%arg%" != "halls")
   wait 1s
   %send% %actor% Try @Rpressing@n one of the controls!. 
     else 
      wait 1 s
      %send% %actor% The floor begins to glow brightly filling your vision.
      wait 1 s
      %send% %actor% @nThe light surrounds you and you feel your body being ripped apart at the molecular level!
      wait 1s
      %send% %actor% @nSuddenly you feel yourself materialising in another place, once again yourself.
      wait 1s
      %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
      %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
      %teleport% %actor% 62502
      %at% %actor% %force% %actor% look
end
~
#62502
Black Omen Teleporter - VH to Air~
2 c 100
press~
if ("%arg%" != "controls" && "%arg%" != "airlock")
   wait 1s
   %send% %actor% Try @Rpressing@n one of the controls!. 
     else 
      wait 1 s
      %send% %actor% The floor begins to glow brightly filling your vision.
      wait 1 s
      %send% %actor% @nThe light surrounds you and you feel your body being ripped apart at the molecular level!
      wait 1s
      %send% %actor% @nSuddenly you feel yourself materialising in another place, once again yourself.
      wait 1s
      %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
      %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
      %teleport% %actor% 62501
      %at% %actor% %force% %actor% look
end 
~
#62503
Outter Airlock Teleport~
1 ab 5
~
eval rrt %random.900%
eval rsn %random.5%
 
switch %rsn%
case 1
  eval prt 20
  break
case 2
  eval prt 21
  break
case 3
  eval prt 24
  break
case 4
  eval prt 30
  break
case 5
  eval prt 29
  break
done
 
if (%prt% == 20)
    if (%rrt% <= 99)
       %echo% A bright light begins to cover the ship.
       wait 2 s
       %echo% The light suddenly crashes inwards, with no trace of the ship remaining.
       eval rrt 200%random.99%
       wait 1
       ogoto 62500
       wait 1
       set tele %self.room.contents(62500)%
       eval %tele.cost(-%tele.cost%)%
       eval %tele.cost(%rrt%)%
       wait 1
       ogoto %tele.cost%
       %echo% The Black Omen appears with a sudden flash of light!
    else
       %echo% A bright light begins to cover the ship.
       wait 2 s
       %echo% The light suddenly crashes inwards, with no trace of the ship remaining.
       eval rrt 20%rrt%
       wait 1
       ogoto 62500
       wait 1
       set tele %self.room.contents(62500)%
       eval %tele.cost(-%tele.cost%)%
       eval %tele.cost(%rrt%)%
       wait 1
       ogoto %tele.cost%
       %echo% The Black Omen appears with a sudden flash of light!
    end
elseif (%prt% == 21)
    if (%rrt% <= 99)
       %echo% A bright light begins to cover the ship.
       wait 2 s
       %echo% The light suddenly crashes inwards, with no trace of the ship remaining.
       eval rrt 210%random.99%
       wait 1
       ogoto 62500
       wait 1
       set tele %self.room.contents(62500)%
       eval %tele.cost(-%tele.cost%)%
       eval %tele.cost(%rrt%)%
       wait 1
       ogoto %tele.cost%
       %echo% The Black Omen appears with a sudden flash of light!
    else
       %echo% A bright light begins to cover the ship.
       wait 2 s
       %echo% The light suddenly crashes inwards, with no trace of the ship remaining.
       eval rrt 21%rrt%
       wait 1
       ogoto 62500
       wait 1
       set tele %self.room.contents(62500)%
       eval %tele.cost(-%tele.cost%)%
       eval %tele.cost(%rrt%)%
       wait 1
       ogoto %tele.cost%
       %echo% The Black Omen appears with a sudden flash of light!
    end
elseif (%prt% == 24)
    if (%rrt% <= 99)
       %echo% A bright light begins to cover the ship.
       wait 2 s
       %echo% The light suddenly crashes inwards, with no trace of the ship remaining.
       eval rrt 240%random.99%
       wait 1
       ogoto 62500
       wait 1
       set tele %self.room.contents(62500)%
       eval %tele.cost(-%tele.cost%)%
       eval %tele.cost(%rrt%)%
       wait 1
       ogoto %tele.cost%
       %echo% The Black Omen appears with a sudden flash of light!
    else
       %echo% A bright light begins to cover the ship.
       wait 2 s
       %echo% The light suddenly crashes inwards, with no trace of the ship remaining.
       eval rrt 21%rrt%
       wait 1
       ogoto 62500
       wait 1
       set tele %self.room.contents(62500)%
       eval %tele.cost(-%tele.cost%)%
       eval %tele.cost(%rrt%)%
       wait 1
       ogoto %tele.cost%
       %echo% The Black Omen appears with a sudden flash of light!
    end
elseif (%prt% == 30)
    if (%rrt% <= 99)
       %echo% A bright light begins to cover the ship.
       wait 2 s
       %echo% The light suddenly crashes inwards, with no trace of the ship remaining.
       eval rrt 300%random.99%
       wait 1
       ogoto 62500
       wait 1
       set tele %self.room.contents(62500)%
       eval %tele.cost(-%tele.cost%)%
       eval %tele.cost(%rrt%)%
       wait 1
       ogoto %tele.cost%
       %echo% The Black Omen appears with a sudden flash of light!
    else
       %echo% A bright light begins to cover the ship.
       wait 2 s
       %echo% The light suddenly crashes inwards, with no trace of the ship remaining.
       eval rrt 21%rrt%
       wait 1
       ogoto 62500
       wait 1
       set tele %self.room.contents(62500)%
       eval %tele.cost(-%tele.cost%)%
       eval %tele.cost(%rrt%)%
       wait 1
       ogoto %tele.cost%
       %echo% The Black Omen appears with a sudden flash of light!
    end
elseif (%prt% == 29)
    if (%rrt% <= 99)
       %echo% A bright light begins to cover the ship.
       wait 2 s
       %echo% The light suddenly crashes inwards, with no trace of the ship remaining.
       eval rrt 290%random.99%
       wait 1
       ogoto 62500
       wait 1
       set tele %self.room.contents(62500)%
       eval %tele.cost(-%tele.cost%)%
       eval %tele.cost(%rrt%)%
       wait 1
       ogoto %tele.cost%
       %echo% The Black Omen appears with a sudden flash of light!
    else
       %echo% A bright light begins to cover the ship.
       wait 2 s
       %echo% The light suddenly crashes inwards, with no trace of the ship remaining.
       eval rrt 29%rrt%
       wait 1
       ogoto 62500
       wait 1
       set tele %self.room.contents(62500)%
       eval %tele.cost(-%tele.cost%)%
       eval %tele.cost(%rrt%)%
       wait 1
       ogoto %tele.cost%
       %echo% The Black Omen appears with a sudden flash of light!
    end
end
~
#62504
Outer Airlock Entry~
1 c 4
ship~
if (%arg% == enter)
 %send% %actor% You press a button by the airlock and it slowly slides open, letting you in.
 %echoaround% %actor% %actor.name% presses a button by the airlock and it slowly slides open, letting them in.
 wait 1 s
 %teleport% %actor% 62500
 %force% %actor% look
else
 %send% %actor% Try @Gentering@n?
end
~
#62505
Malmsteen Death~
0 f 100
~
Eval num %random.1000% 
    if (%num% >= 920) 
    %load% obj 62699
    %load% obj 62555
    rdelete onmalmq %actor.id%
  Else
    if (%num% >=700)
    %load% obj 62510
    rdelete onmalmq %actor.id%
  Else
    rdelete onmalmq %actor.id%
   End
  End
End
~
#62506
Horus Greeting~
0 g 100
~
If (%actor.is_pc%)
  If (!%actor.varexists(onhq)%)
    wait 1s
    %echoaround% horus The kingly saiyan folds his arms across his powerful chest and stares at %actor.name% imperiously.
    wait 2s
    %echoaround% horus With a flick of his tail, the Saiyan spirit stares %actor.name% down with a more appraising glance, looking unimpressed.
    wait 2s
    say @RAll crowns will eventually bow before the Great Fire. If you think you are foolish enough to stop this, accept my @Ychallenge@R and I will give you final mercy.@n
  Else
    wait 2s
    %echoaround% horus The kingly saiyans raises an eyebrow and stares at you imperiously.
    wait 1s
    say @RYou are foolish enough to return? Very well ...@n
    wait 5s
mtransform 62696
wait 10s
    mkill %actor%
  End
End
~
#62507
Malmsteen Greeting~
0 h 100
~
If (%actor.is_pc%)
  If (!%actor.varexists(onmalmq)%)
    wait 1s
    %echoaround% malmsteen The small truffle turns his head from the impressive view and then turns to face you.
    wait 2s
    %echoaround% malmsteen He laces his fingers together, clasping his hands behind his back at the base of his spine. 
    wait 2s
    say @mAhh, you's look capable yes! Me's been stuck on this ship so long. Accept my @Rchallenge@m and free me's from this place!@n
  Else
    wait 2s
    say @mYou's gots to finish the job! Come back here!@n
    wait 5s
mtransform 62605
wait 10s
    mkill %actor%
  End
End
~
#62508
Malmsteen Quest Accept~
0 cd 100
challenge~
If ("%arg%" != "malm" && "%arg%" != "malmsteen")
    return 1
    wait 1s
    %send% %actor% @DTry offering a @RChallenge@D to @GM@ga@Gl@gm@Gs@gt@Ge@ge@Gn@D.@n
Else
  if (!%actor.varexists(onmalmq)%)
    set onmalmq
    remote onmalmq %actor.id%
wait 1s
say @mI knew I could count on you's!@n
wait 5s
    mtransform 62605
wait 10s
    mkill %actor%
Else
  if (%actor.varexists(onmalmq)%)
    say @mYou's already agreed to helps me's, now fight!@n
    wait 5s
    mtransform 62695
    wait 10s
    mkill %actor%
   End
  End
End
~
#62511
Black Omen Teleporter - VH to LQ~
2 c 100
press~
if ("%arg%" != "controls" && "%arg%" != "res")
   wait 1s
   %send% %actor% Try @Rpressing@n one of the controls!. 
     else 
      wait 1 s
      %send% %actor% The floor begins to glow brightly filling your vision.
      wait 1 s
      %send% %actor% @nThe light surrounds you and you feel your body being ripped apart at the molecular level!
      wait 1s
      %send% %actor% @nSuddenly you feel yourself materialising in another place, once again yourself.
      wait 1s
      %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
      %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
      %teleport% %actor% 62512
      %at% %actor% %force% %actor% look
end 
~
#62512
@nBlack Omen Teleporter - LQ to VH/LB~
2 c 100
press~
if (%cmd% == press && %arg% == halls)
  wait 1 s
  %send% %actor% The floor begins to glow brightly filling your vision.
  wait 1 s
  %send% %actor% The light surrounds you and you feel your body being ripped apart at the molecular level!
  wait 1s
  %send% %actor% Suddenly you feel yourself materialising in another place, once again yourself.
  wait 1s
  %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
  %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
  %teleport% %actor% 62511
  %at% %actor% %force% %actor% look
    else
     if (%cmd% == press && %arg% == lab)
     wait 1 s
     %send% %actor% The floor begins to glow brightly filling your vision.
     wait 1 s
     %send% %actor% The light surrounds you and you feel your body being ripped apart at the molecular level!
     wait 1s
     %send% %actor% Suddenly you feel yourself materialising in another place, once again yourself.
     wait 1s
     %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
     %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
     %teleport% %actor% 62525
     %at% %actor% %force% %actor% look
  else
   if (%cmd% == press && %arg% == controls)
   wait 1s
   %send% %actor% But which one?!
  else
   wait 1s
   %send% %actor% @nTry @Rpressing@n one of the controls!.
  end
 else
  end
 else
end
~
#62525
BOT - Lab to Res~
2 c 100
press~
if ("%arg%" != "controls" && "%arg%" != "res")
   wait 1s
   %send% %actor% Try @Rpressing@n one of the controls!. 
     else 
      wait 1 s
      %send% %actor% The floor begins to glow brightly filling your vision.
      wait 1 s
      %send% %actor% @nThe light surrounds you and you feel your body being ripped apart at the molecular level!
      wait 1s
      %send% %actor% @nSuddenly you feel yourself materialising in another place, once again yourself.
      wait 1s
      %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
      %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
      %teleport% %actor% 62512
      %at% %actor% %force% %actor% look
end 
~
#62526
Isis Check~
2 g 100
~
if (%actor.varexists(onisq)%) 
rdelete onisq %actor.id% 
end
~
#62532
BOT - Lab to CMD/NO~
2 c 100
press~
if (%cmd% == press && %arg% == command)
  wait 1 s
  %send% %actor% The floor begins to glow brightly filling your vision.
  wait 1 s
  %send% %actor% The light surrounds you and you feel your body being ripped apart at the molecular level!
  wait 1s
  %send% %actor% Suddenly you feel yourself materialising in another place, once again yourself.
  wait 1s
  %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
  %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
  %teleport% %actor% 62604
  %at% %actor% %force% %actor% look
    else
     if (%cmd% == press && %arg% == no)
     wait 1 s
     %send% %actor% The floor begins to glow brightly filling your vision.
     wait 1 s
     %send% %actor% The light surrounds you and you feel your body being ripped apart at the molecular level!
     wait 1s
     %send% %actor% Suddenly you feel yourself materialising in another place, once again yourself.
     wait 1s
     %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
     %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
     %teleport% %actor% 62646
     %at% %actor% %force% %actor% look
  else
   if (%cmd% == press && %arg% == controls)
   wait 1s
   %send% %actor% But which one?!
  else
   wait 1s
   %send% %actor% @nTry @Rpressing@n one of the controls!.
  end
 else
  end
 else
end
~
#62537
BOT - LAB to ARMY/GYM~
2 c 100
press~
if (%cmd% == press && %arg% == Army)
  wait 1 s
  %send% %actor% The floor begins to glow brightly filling your vision.
  wait 1 s
  %send% %actor% The light surrounds you and you feel your body being ripped apart at the molecular level!
  wait 1s
  %send% %actor% Suddenly you feel yourself materialising in another place, once again yourself.
  wait 1s
  %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
  %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
  %teleport% %actor% 62570
  %at% %actor% %force% %actor% look
    else
     if (%cmd% == press && %arg% == gym)
     wait 1 s
     %send% %actor% The floor begins to glow brightly filling your vision.
     wait 1 s
     %send% %actor% The light surrounds you and you feel your body being ripped apart at the molecular level!
     wait 1s
     %send% %actor% Suddenly you feel yourself materialising in another place, once again yourself.
     wait 1s
     %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
     %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
     %teleport% %actor% 62586
     %at% %actor% %force% %actor% look
  else
   if (%cmd% == press && %arg% == controls)
   wait 1s
   %send% %actor% But which one?!
  else
   wait 1s
   %send% %actor% @nTry @Rpressing@n one of the controls!.
  end
 else
  end
 else
end
~
#62547
Keypad Hint 2~
2 c 100
enter~
If %arg% == 051015
  if %self.inside(bits)% != DOOR)
    wait 1
    %echoaround% %actor% %actor.name% punches in a code on the door panel.
    %send% %actor% you punch in the code on the panel.
    wait 1 sec
    %echo% Suddenly a section of the western wall opens and reveals a hidden doorway!
    %load% mob 62560
    %teleport% inviso 62560
    %echoaround% inviso  Suddenly a section of the western wall opens and reveals a hidden doorway!
    wdoor 62547 3 flags a
    wdoor 62560 1 flags a
    wait 5 sec
    %echo% With an almost silent hiss of pressure, the door shuts as it conceals itself once more into the wall.
    %echoaround% inviso With an almost silent hiss of pressure, the door shuts as it conceals itself once more into the wall.
    %teleport% inviso %self.vnum%
    %purge% inviso
    wdoor 62547 3 flags abcd
    wdoor 62560 1 flags abcd
  Else
    wait 1 sec
    %send% %actor% The access panel is already open, you had best hurry through it.
  End
Else
  wait 1 sec
  %echoaround% %actor% %actor.name% punches in a code on the door panel.
  %send% %actor% you punch in the code on the panel.
  wait 1 sec
  %echo% The panel buzzes loudly, signaling an incorrect code.
End
~
#62554
BO - Lab Keypad 1~
2 c 100
enter~
if %arg% == 314159
  if %self.inside(bits)% != DOOR)
    wait 1s
    %echoaround% %actor% %actor.name% punches in a code on the door panel.
    %send% %actor% you punch in the code on the panel.
    wait 1s
    %echo% Suddenly a section of the western wall opens and reveals a hidden doorway!
    %load% mob 62560
    %teleport% inviso 62558
    %echoaround% inviso  Suddenly a section of the western wall opens and reveals a hidden doorway!
    wdoor 62558 3 flags a
    wdoor 62554 1 flags a
    wait 5s
    %echo% With an almost silent hiss of pressure, the door shuts as it conceals itself once more into the wall.
    %echoaround% inviso With an almost silent hiss of pressure, the door shuts as it conceals itself once more into the wall.
    %teleport% inviso %self.vnum%
    %purge% inviso
    wdoor 62558 3 flags abcd
    wdoor 62554 1 flags abcd
  Else
    wait 1s
    %send% %actor% The access panel is already open, you had best hurry through it.
  End
Else
  wait 1s
  %echoaround% %actor% %actor.name% punches in a code on the door panel.
  %send% %actor% you punch in the code on the panel.
  wait 1s
  %echo% The panel buzzes loudly, signaling an incorrect code ..
End
~
#62558
BO - Lab Keypad 2~
2 c 100
enter~
if %arg% == 314159
  if %self.inside(bits)% != DOOR)
    wait 1s
    %echoaround% %actor% %actor.name% punches in a code on the door panel.
    %send% %actor% you punch in the code on the panel.
    wait 1s
    %echo% Suddenly a section of the western wall opens and reveals a hidden doorway!
    %load% mob 62560
    %teleport% inviso 62554
    %echoaround% inviso  Suddenly a section of the western wall opens and reveals a hidden doorway!
    wdoor 62558 3 flags a
    wdoor 62554 1 flags a
    wait 5s
    %echo% With an almost silent hiss of pressure, the door shuts as it conceals itself once more into the wall.
    %echoaround% inviso With an almost silent hiss of pressure, the door shuts as it conceals itself once more into the wall.
    %teleport% inviso %self.vnum%
    %purge% inviso
    wdoor 62558 3 flags abcd
    wdoor 62554 1 flags abcd
  Else
    wait 1s
    %send% %actor% The access panel is already open, you had best hurry through it.
  End
Else
  wait 1s
  %echoaround% %actor% %actor.name% punches in a code on the door panel.
  %send% %actor% you punch in the code on the panel.
  wait 1s
  %echo% The panel buzzes loudly, signaling an incorrect code ..
End
~
#62559
BOT - LAB to OB~
2 c 100
press~
if ("%arg%" != "controls" && "%arg%" != "ob")
   wait 1s
   %send% %actor% Try @Rpressing@n one of the controls!. 
     else 
      wait 1 s
      %send% %actor% The floor begins to glow brightly filling your vision.
      wait 1 s
      %send% %actor% @nThe light surrounds you and you feel your body being ripped apart at the molecular level!
      wait 1s
      %send% %actor% @nSuddenly you feel yourself materialising in another place, once again yourself.
      wait 1s
      %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
      %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
      %teleport% %actor% 62559
      %at% %actor% %force% %actor% look
end 
~
#62560
BOT - Lab to Engine~
2 c 100
press~
if ("%arg%" != "controls" && "%arg%" != "jump")
   wait 1s
   %send% %actor% Try @Rpressing@n one of the controls!. 
     else 
      wait 1 s
      %send% %actor% The floor begins to glow brightly filling your vision.
      wait 1 s
      %send% %actor% @nThe light surrounds you and you feel your body being ripped apart at the molecular level!
      wait 1s
      %send% %actor% @nSuddenly you feel yourself materialising in another place, once again yourself.
      wait 1s
      %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
      %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
      %teleport% %actor% 62561
      %at% %actor% %force% %actor% look
end
~
#62561
Black Omen Teleport - Engine to Lab~
2 c 100
press~
if (%cmd% == press && %arg% == escape)
    wait 1 s
    %send% %actor% The floor begins to glow brightly filling your vision.
    wait 1 s
    %send% %actor% The light surrounds you and you feel your body being ripped apart at the molecular level!
    wait 1s
    %send% %actor% Suddenly you feel yourself materialising in another place, once again yourself.
    wait 1s
    %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
    %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
    %teleport% %actor% 62501
    %at% %actor% %force% %actor% look
      Else
          If (%cmd% == press && %arg% == lab)
          wait 1 s
          %send% %actor% The floor begins to glow brightly filling your vision.
          wait 1 s
          %send% %actor% The light surrounds you and you feel your body being ripped apart at the molecular level!
          wait 1s
          %send% %actor% Suddenly you feel yourself materialising in another place, once again yourself.
          wait 1s
          %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
          %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
          %teleport% %actor% 62525
          %at% %actor% %force% %actor% look
      Else
        If (%cmd% == press && %arg% == controls)
        wait 1s
        %send% %actor% But which one?!
    Else
      wait 1s
      %send% %actor% Try pressing one of the controls!.
      End
  Else
    End
Else
End
~
#62570
BOT - ARMY to LAB~
2 c 100
press~
if ("%arg%" != "controls" && "%arg%" != "lab")
   wait 1s
   %send% %actor% Try @Rpressing@n one of the controls!. 
     else 
      wait 1 s
      %send% %actor% The floor begins to glow brightly filling your vision.
      wait 1 s
      %send% %actor% @nThe light surrounds you and you feel your body being ripped apart at the molecular level!
      wait 1s
      %send% %actor% @nSuddenly you feel yourself materialising in another place, once again yourself.
      wait 1s
      %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
      %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
      %teleport% %actor% 62537
      %at% %actor% %force% %actor% look
end 
~
#62571
BOT - OB to LAB~
2 c 100
press~
If ("%arg%" != "controls" && "%arg%" != "Lab")
    wait 1s
    %send% %actor% Try pressing one of the @Rcontrols!@n. 
  Else 
      wait 1 s
      %send% %actor% The floor begins to glow brightly filling your vision.
      wait 1 s
      %send% %actor% The light surrounds you and you feel your body being ripped apart at the molecular level!
      wait 1s
      %send% %actor% Suddenly you feel yourself materialising in another place, once again yourself.
      wait 1s
      %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
      %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
      %teleport% %actor% 62525
      %at% %actor% %force% %actor% look
End
~
#62572
hidden computer bank~
2 c 100
enter~
if %arg% == 051015
  if %self.inside(bits)% != DOOR)
    wait 1
    %echoaround% %actor% %actor.name% punches in a code on the door panel.
    %send% %actor% you punch in the code on the panel.
    wait 1 sec
    %echo% Suddenly a section of the western wall opens and reveals a hidden doorway!
    %load% mob 62560
    %teleport% inviso 62547
    %echoaround% inviso  Suddenly a section of the western wall opens and reveals a hidden doorway!
    wdoor 62547 3 flags a
    wdoor 62560 1 flags a
    wait 5 sec
    %echo% With an almost silent hiss of pressure, the door shuts as it conceals itself once more into the wall.
    %echoaround% inviso With an almost silent hiss of pressure, the door shuts as it conceals itself once more into the wall.
    %teleport% inviso %self.vnum%
    %purge% inviso
    wdoor 62547 3 flags abcd
    wdoor 62560 1 flags abcd
  Else
    wait 1 sec
    %send% %actor% The access panel is already open, you had best hurry through it.
  End
Else
  wait 1 sec
  %echoaround% %actor% %actor.name% punches in a code on the door panel.
  %send% %actor% you punch in the code on the panel.
  wait 1 sec
  %echo% The panel buzzes loudly, signaling an incorrect code ..
End
~
#62573
Figure Drop D~
0 f 100
~
eval num %random.1000%
 if (%num% >= 750) 
 %load% obj 62602
end
~
#62574
Figure Drop J~
0 f 100
~
eval num %random.1000%
   if (%num% >= 750)
%load% obj 62600
end
~
#62575
Figure Drop O~
0 f 100
~
eval num %random.1000%
if (%num% >= 750)
%load% obj 62601
end
~
#62576
Figure Drop R~
0 f 100
~
eval num %random.1000%
if (%num% >= 750)
%load% obj 62603
end
~
#62577
Figure Drop S~
0 f 100
~
eval num %random.1000%
if (%num% >= 750)
%load% obj 62604
end
~
#62578
Figure Drop E~
0 f 100
~
eval num %random.1000%
if (%num% >= 750)
%load% obj 62605
end
~
#62579
Egg Drop~
0 f 100
~
Eval num %random.1000%
   If (%num% >= 500)
   %load% obj 62610
End
~
#62580
Candy Claw Drop~
0 f 100
~
eval num %random.1000%
if (%num% >= 600)
%load% obj 62504
end
~
#62581
Knife Drop~
0 f 100
~
eval num %random.1000%
if (%num% => 800)
%load% obj 62593
end
~
#62586
BOT - GYM to LAB~
2 c 100
press~
if ("%arg%" != "controls" && "%arg%" != "Lab")
   wait 1s
   %send% %actor% Try @Rpressing@n one of the controls!. 
     else 
      wait 1 s
      %send% %actor% The floor begins to glow brightly filling your vision.
      wait 1 s
      %send% %actor% @nThe light surrounds you and you feel your body being ripped apart at the molecular level!
      wait 1s
      %send% %actor% @nSuddenly you feel yourself materialising in another place, once again yourself.
      wait 1s
      %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
      %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
      %teleport% %actor% 62537
      %at% %actor% %force% %actor% look
end
~
#62595
Ra Greet~
0 h 100
~
If (%actor.is_pc%)
  If (!%actor.varexists(onraq)%)
    wait 1s
    %echoaround% ra The large @yh@Wa@Yw@nk-headed mutant cocks his head to the side, catching %actor.name% with a piercing gaze that stares into their soul.
    wait 2s
    %echoaround% ra The mutant @Rscreeches@n loudly, though somehow ears begin to understand ... 
    wait 2s
    say @WHave you come to @Ychallenge@W me, mortal? I will absorb you into the All that is @yR@Ya@W if you try.@n
  Else
    wait 2s
    say @WYou've returned after escaping with your life? Fool!@n
    wait 5s
mtransform 62695
wait 10s
    mkill %actor%
  End
End
~
#62596
Ra Death~
0 f 100
~
Eval num %random.1000% 
    if (%num% >= 975) 
    %load% obj 62595
    %load% obj 62512
    rdelete onraq %actor.id%
  Else
    if (%num% >=700)
    %load% obj 62512
    rdelete onraq %actor.id%
  Else
    rdelete onraq %actor.id%
   End
  End
End
~
#62597
Ra Quest Accept~
0 cd 100
challenge~
If ("%arg%" != "Ra" && "%arg%" != "Rah")
    return 1
    wait 1s
    %send% %actor% @DTry offering a @RChallenge@D to @YR@Wa@D.@n
Else
  if (!%actor.varexists(onraq)%)
    set onraq
    remote onraq %actor.id%
wait 1s
say @WVery well, fool! Take your last breath!@n
wait 5s
    mtransform 62695
wait 10s
    mkill %actor%
Else
  if (%actor.varexists(onraq)%)
    wait 1s
    say @WYou return fool? Die!@n
    wait 5s
    mtransform 62695
    wait 10s
    mkill %actor%
   End
  End
End
~
#62598
Osiris Greeting~
0 g 100
~
If (%actor.is_pc%)
  If (!%actor.varexists(onosq)%)
    wait 1s
    %echoaround% osiris The tall, slender Namekian smiles cruelly.
    wait 2s
    %echoaround% osiris The @Ggreen-skinned@n alien gestures toward %actor.name% with imperious arrogance.
    wait 2s
    say @GThe Great Fire will consume all. If you are foolish enough to think you can stop this, then accept my @Rchallenge@G and face your final fate!@n
  Else
    wait 2s
    say @GYou again? I will finish this.@n
    wait 5s
mtransform 62698
wait 10s
    mkill %actor%
  End
End
~
#62599
Osiris Quest Accept~
0 cd 100
challenge~
If ("%arg%" != "Os" && "%arg%" != "Osiris")
    return 1
    wait 1s
    %send% %actor% @DTry offering a @RChallenge@D to @gOsiris@D.@n
Else
  if (!%actor.varexists(onosq)%)
    set onosq
    remote onosq %actor.id%
wait 1s
say @GSo be it. your soul too can be bound to this hellish prison.@n
wait 5s
    mtransform 62698
wait 10s
    mkill %actor%
Else
  if (%actor.varexists(onosq)%)
    say @GSeeing you're backside running away once was enough, fool!@n
    wait 5s
    mtransform 62698
    wait 10s
    mkill %actor%
   End
  End
End
~
#62604
BOT - CMD to LAB~
2 c 100
press~
if ("%arg%" != "controls" && "%arg%" != "lab")
   wait 1s
   %send% %actor% Try @Rpressing@n one of the controls!. 
     else 
      wait 1 s
      %send% %actor% The floor begins to glow brightly filling your vision.
      wait 1 s
      %send% %actor% @nThe light surrounds you and you feel your body being ripped apart at the molecular level!
      wait 1s
      %send% %actor% @nSuddenly you feel yourself materialising in another place, once again yourself.
      wait 1s
      %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
      %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
      %teleport% %actor% 62532
      %at% %actor% %force% %actor% look
end 
~
#62605
Malmsteen Check~
2 g 100
~
if (%actor.varexists(onmalmq)%)
rdelete onmalmq %actor.id%
end
~
#62606
Buster Overload~
1 j 100
~
wait until 6:00
 eval num %random.1000% 
  if (%num% >= 995)
   %send% %actor% @nAn urgent beeping begin emitting from the barrel of this device!
    wait 3s 
     %send% %actor% @nEnergy begins to build rapidly within the barrel of the @WX@R-@YB@yu@Ws@Yt@Re@Wr!
       wait 2s
        %send% %actor% @nThe energy is approaching dangerous levels!
         wait 1s
           eval gem %random.1000%
            if (%gem% >= 971)
             %send% %actor% @nAs the energy buildup reaches critical mass, the special barrel suddenly releases the pressure through a hidden set of vents! A moment later, a small shimmering diamond seems to materialize from the excess energy!@n
               %echoaround% %actor% @nA rapid beeping begins to emit from %actor.name% and a dangerous amount of energy can be sensed building up around them! As the energy reaches critical, it suddenly disperses!
                   %load% obj 61514
             else
       if (%actor.is_pc%)
        * Get Variables
         eval X %random.20%
         eval A %random.6%
         eval B %random.6%
         eval C %random.6%
         eval H %actor.maxhitp%
         eval Y %A% + %B% + %C%
         eval E %H% / %Y%
         eval D %E%
         **************
          if (%X% > 5)
           wait 2 s
            %send% %actor% The energy reaches critical mass and the vents kick in too late! Your body absorbs most of the resulting blast!
               %echoaround% %actor% %actor.name% is surrounded by a largel blast of energy!
                 %damage% %actor% %D%
                  %send% %actor% @W[@RDamage@W: @r%D%@W]@n
              else
          wait 2 s
           %send% %actor% The energy reaches critical mass .. but the vents kick in at the last moment! Your body absorbs only a minor blast!
             %damage% %actor% %D% / 3
               %send% %actor% @W[@RDamage@W: @r%D% / 3@W]@n
                 %echoaround% %actor% %actor.name% is surrounded by a minor blast of energy!
end
end
************** 
   wait until 12:00
eval num %random.1000% 
  if (%num% >= 995)
   %send% %actor% @nAn urgent beeping begin emitting from the barrel of this device!
    wait 3s 
     %send% %actor% @nEnergy begins to build rapidly within the barrel of the @WX@R-@YB@yu@Ws@Yt@Re@Wr!
       wait 2s
        %send% %actor% @nThe energy is approaching dangerous levels!
         wait 1s
           eval gem %random.1000%
            if (%gem% >= 971)
             %send% %actor% @nAs the energy buildup reaches critical mass, the special barrel suddenly releases the pressure through a hidden set of vents! A moment later, a small shimmering diamond seems to materialize from the excess energy!@n
               %echoaround% %actor% @nA rapid beeping begins to emit from %actor.name% and a dangerous amount of energy can be sensed building up around them! As the energy reaches critical, it suddenly disperses!
                   %load% obj 61514
             else
       if (%actor.is_pc%)
        * Get Variables
         eval X %random.20%
         eval A %random.6%
         eval B %random.6%
         eval C %random.6%
         eval H %actor.maxhitp%
         eval Y %A% + %B% + %C%
         eval E %H% / %Y%
         eval D %E%
         **************
          if (%X% > 5)
           wait 2 s
            %send% %actor% The energy reaches critical mass and the vents kick in too late! Your body absorbs most of the resulting blast!
               %echoaround% %actor% %actor.name% is surrounded by a largel blast of energy!
                 %damage% %actor% %D%
                  %send% %actor% @W[@RDamage@W: @r%D%@W]@n
              else
          wait 2 s
           %send% %actor% The energy reaches critical mass .. but the vents kick in at the last moment! Your body absorbs only a minor blast!
             %damage% %actor% %D% / 3
               %send% %actor% @W[@RDamage@W: @r%D% / 3@W]@n
                 %echoaround% %actor% %actor.name% is surrounded by a minor blast of energy!
end
end
************** 
 wait until 16:00
eval num %random.1000% 
  if (%num% >= 995)
   %send% %actor% @nAn urgent beeping begin emitting from the barrel of this device!
    wait 3s 
     %send% %actor% @nEnergy begins to build rapidly within the barrel of the @WX@R-@YB@yu@Ws@Yt@Re@Wr!
       wait 2s
        %send% %actor% @nThe energy is approaching dangerous levels!
         wait 1s
           eval gem %random.1000%
            if (%gem% >= 971)
             %send% %actor% @nAs the energy buildup reaches critical mass, the special barrel suddenly releases the pressure through a hidden set of vents! A moment later, a small shimmering diamond seems to materialize from the excess energy!@n
               %echoaround% %actor% @nA rapid beeping begins to emit from %actor.name% and a dangerous amount of energy can be sensed building up around them! As the energy reaches critical, it suddenly disperses!
                   %load% obj 61514
             else
       if (%actor.is_pc%)
        * Get Variables
         eval X %random.20%
         eval A %random.6%
         eval B %random.6%
         eval C %random.6%
         eval H %actor.maxhitp%
         eval Y %A% + %B% + %C%
         eval E %H% / %Y%
         eval D %E%
         **************
          if (%X% > 5)
           wait 2 s
            %send% %actor% The energy reaches critical mass and the vents kick in too late! Your body absorbs most of the resulting blast!
               %echoaround% %actor% %actor.name% is surrounded by a largel blast of energy!
                 %damage% %actor% %D%
                  %send% %actor% @W[@RDamage@W: @r%D%@W]@n
              else
          wait 2 s
           %send% %actor% The energy reaches critical mass .. but the vents kick in at the last moment! Your body absorbs only a minor blast!
             %damage% %actor% %D% / 3
               %send% %actor% @W[@RDamage@W: @r%D% / 3@W]@n
                 %echoaround% %actor% %actor.name% is surrounded by a minor blast of energy!
end
end
************** 
  wait until 20:00
eval num %random.1000% 
  if (%num% >= 995)
   %send% %actor% @nAn urgent beeping begin emitting from the barrel of this device!
    wait 3s 
     %send% %actor% @nEnergy begins to build rapidly within the barrel of the @WX@R-@YB@yu@Ws@Yt@Re@Wr!
       wait 2s
        %send% %actor% @nThe energy is approaching dangerous levels!
         wait 1s
           eval gem %random.1000%
            if (%gem% >= 971)
             %send% %actor% @nAs the energy buildup reaches critical mass, the special barrel suddenly releases the pressure through a hidden set of vents! A moment later, a small shimmering diamond seems to materialize from the excess energy!@n
               %echoaround% %actor% @nA rapid beeping begins to emit from %actor.name% and a dangerous amount of energy can be sensed building up around them! As the energy reaches critical, it suddenly disperses!
                   %load% obj 61514
             else
       if (%actor.is_pc%)
        * Get Variables
         eval X %random.20%
         eval A %random.6%
         eval B %random.6%
         eval C %random.6%
         eval H %actor.maxhitp%
         eval Y %A% + %B% + %C%
         eval E %H% / %Y%
         eval D %E%
         **************
          if (%X% > 5)
           wait 2 s
            %send% %actor% The energy reaches critical mass and the vents kick in too late! Your body absorbs most of the resulting blast!
               %echoaround% %actor% %actor.name% is surrounded by a largel blast of energy!
                 %damage% %actor% %D%
                  %send% %actor% @W[@RDamage@W: @r%D%@W]@n
              else
          wait 2 s
           %send% %actor% The energy reaches critical mass .. but the vents kick in at the last moment! Your body absorbs only a minor blast!
             %damage% %actor% %D% / 3
               %send% %actor% @W[@RDamage@W: @r%D% / 3@W]@n
                 %echoaround% %actor% %actor.name% is surrounded by a minor blast of energy!
end
end
************** 
  wait until 1:00
eval num %random.1000% 
  if (%num% >= 995)
   %send% %actor% @nAn urgent beeping begin emitting from the barrel of this device!
    wait 3s 
     %send% %actor% @nEnergy begins to build rapidly within the barrel of the @WX@R-@YB@yu@Ws@Yt@Re@Wr!
       wait 2s
        %send% %actor% @nThe energy is approaching dangerous levels!
         wait 1s
           eval gem %random.1000%
            if (%gem% >= 971)
             %send% %actor% @nAs the energy buildup reaches critical mass, the special barrel suddenly releases the pressure through a hidden set of vents! A moment later, a small shimmering diamond seems to materialize from the excess energy!@n
               %echoaround% %actor% @nA rapid beeping begins to emit from %actor.name% and a dangerous amount of energy can be sensed building up around them! As the energy reaches critical, it suddenly disperses!
                   %load% obj 61514
             else
       if (%actor.is_pc%)
        * Get Variables
         eval X %random.20%
         eval A %random.6%
         eval B %random.6%
         eval C %random.6%
         eval H %actor.maxhitp%
         eval Y %A% + %B% + %C%
         eval E %H% / %Y%
         eval D %E%
         **************
          if (%X% > 5)
           wait 2 s
            %send% %actor% The energy reaches critical mass and the vents kick in too late! Your body absorbs most of the resulting blast!
               %echoaround% %actor% %actor.name% is surrounded by a largel blast of energy!
                 %damage% %actor% %D%
                  %send% %actor% @W[@RDamage@W: @r%D%@W]@n
              else
          wait 2 s
           %send% %actor% The energy reaches critical mass .. but the vents kick in at the last moment! Your body absorbs only a minor blast!
             %damage% %actor% %D% / 3
               %send% %actor% @W[@RDamage@W: @r%D% / 3@W]@n
                 %echoaround% %actor% %actor.name% is surrounded by a minor blast of energy!
end
end
************** 
~
#62607
Locutus Death~
0 f 100
~
Eval num %random.1000% 
    if (%num% >= 920) 
    %load% obj 62650
    %load% obj 62699
    rdelete onlq %actor.id%
  Else
    if (%num% >=700)
    %load% obj 62699
    rdelete onlq %actor.id%
  Else
    rdelete onlq %actor.id%
   End
  End
End
~
#62608
Locutus Greeting~
0 h 100
~
If (%actor.is_pc%)
  If (!%actor.varexists(onlq)%)
    wait 1s
    %echoaround% locutus The strange looking @Dc@Wy@Db@Wo@Dr@Wg@n turns its lazer eye onto %actor.name%, scanning for threats.
    wait 2s
    %echoaround% locutus The @Dc@Wy@Db@Wo@Dr@Wg@n looks at %actor.name% menacingly ...
    wait 2s
    say @YThe Great Fire will consume all, resistance is futile. @RChallenge@Y me if you dare.@n
  Else
    wait 2s
    say @YResistance is futile, Prepare for assimilation.@n
    wait 5s
mtransform 62611
wait 10s
    mkill %actor%
  End
End
~
#62609
Locutus Quest Accept~
0 cd 100
challenge~
If ("%arg%" != "Loc" && "%arg%" != "Locutus")
    return 1
    wait 1s
    %send% %actor% @DTry offering a @RChallenge@D to L@Wo@Dc@Wu@Dt@Wu@Ds.@n
Else
  if (!%actor.varexists(onlq)%)
    set onlq
    remote onlq %actor.id%
wait 1s
say @YResistance is futile, prepare to be assimilated ...@n
wait 5s
    mtransform 62611
wait 10s
    mkill %actor%
Else
  if (%actor.varexists(onlq)%)
    say @YResistance is futile, prepare to be assimilated ...@n
    wait 5s
    mtransform 62611
    wait 10s
    mkill %actor%
   End
  End
End
~
#62610
Omen Computer Menu~
1 c 100
select~
If (%cmd% == select && %arg% == project)
    Wait 1s
Return 0
%force% %actor% look menuproject
  Else
    If (%cmd% == select && %arg% == siphon)
        Wait 1s
Return 0
%force% %actor% look menusiphon
  Else
    If (%cmd% == select && %arg% == kaiser)
        Wait 1s
Return 0
%force% %actor% look menukaiser
  Else
    %Send% %Actor% @RSelect@D a different directory.@n
End
End
End
~
#62611
Booster purge~
0 f 100
~
%purge% xzena
~
#62632
Newspaper thwap~
1 c 3
thwap~
if (%arg%)
   %send% %actor% You thwap %arg% on the head!
   %echoaround% %actor% %actor.name% thwaps %arg% on the head!
else
   %send% %actor% Thwap who?
end
~
#62633
No Escape~
2 c 100
inst~
if (%cmd% == inst || %cmd% == insta || %cmd% == instan || %cmd% == instant)
return 1
wait 1s
%send% %actor% You can't seem to sense anything from within here!
end
~
#62645
Gizmo Drop~
0 f 100
~
eval num %random.1000%
if (%num% >= 650)
%load% obj 62607
end
~
#62646
BOT - Depths to Lab~
2 c 100
press~
if ("%arg%" != "controls" && "%arg%" != "lab")
   wait 1s
   %send% %actor% Try @Rpressing@n one of the controls!. 
     else 
      wait 1 s
      %send% %actor% The floor begins to glow brightly filling your vision.
      wait 1 s
      %send% %actor% @nThe light surrounds you and you feel your body being ripped apart at the molecular level!
      wait 1s
      %send% %actor% @nSuddenly you feel yourself materialising in another place, once again yourself.
      wait 1s
      %echoaround% %actor% The floor of the entire room begins to glow brightly filling your vision!
      %echoaround% %actor% %actor.name% is srrounded by bright light and energy as the pad powers up and teleports them somewhere else!
      %teleport% %actor% 62532
      %at% %actor% %force% %actor% look
end
~
#62647
G-Key Drop~
0 f 100
~
eval num %random.1000%
if (%num% >= 575)
%load% obj 62615
end
~
#62648
Plasma Jet Trigger~
1 ab 25
~
if (%self.timer% <= 0)
 if (%self.room.vnum% != 62603)
  wait 10 s
  %purge% self
 End
Else
 eval num %random.10%
 switch %num%
  case 1
   wait 10 s
   %echo% The jet burns brightly, before dimming.
   break
  case 2
   wait 10 s
   %echo% A mote of plasma flickers off of the jet, sizzling into nothing.
   break
  case 3
  case 4
  case 5
   wait 10 s
   %echo% The jet gives off a slight hiss.
   break
  default
  wait 10 s
  %echo% The jet sputters, before steadying out.
  break
 done
end
~
#62649
Shock Collar~
1 ab 100
~
set regenpeeps %self.room.people%
eval shock1 3
eval shock2 8
eval shock3 13
eval shock4 18
while (%regenpeeps%)
  if (%regenpeeps.varexists(shocklevel)% && %regenpeeps.is_pc%)
    eval shocklevel %regenpeeps.shocklevel%
    if (%regenpeeps.shocklevel% >= 1)
      eval regenchance %random.100%
      if (%regenchance% > 40)
        eval shocklevel %shocklevel% - 1
        if (%shocklevel% < 0)
          eval shocklevel 0
        end
        remote shocklevel %regenpeeps.id%
        %send% %regenpeeps.name% @RYou feel some of the electricity in your body dissipate.@n
      else
        if (%shocklevel% > %shock4%)
          %send% %regenpeeps.name% @RSo much residual electricy courses through you, you flail about on the ground!@n
          %echoaround% %regenpeeps.name% @R%regenpeeps.name% flials about, limbs spasming beyond %regenpeeps.hisher% control!@n
          %force% %regenpeeps.name% rest
        elseif (%shocklevel% > %shock3%)
          %send% %regenpeeps.name% @RSo much residual electricy courses through you, you can only struggle to your knees!@n
          %echoaround% %regenpeeps.name% @R%regenpeeps.name% flails about, barely able to get to %regenpeeps.hisher% knees.@n
          %force% %regenpeeps.name% sit
        elseif (%shocklevel% > %shock2%)
          %send% %regenpeeps.name% @RElectricity continues to course painfully through you, but you are barely able to manage control.@n
          %echoaround% %regenpeeps.name% @R%regenpeeps.name% twitches and spasms, but is able to maintain control.@n
        elseif (%regenlevel% > %shock1%)
          %send% %regenpeeps.name% @RYou twitch and spasm annoyingly as an aftereffect of being shocked.@n
          %echoaround% %regenpeeps.name% @R%regenpeeps.name% twitches and spasms, but is able to control %shockpeeps.hisher%self.@n
        else
          %send% %regenpeeps.name% @RYou twitch slightly as an aftereffect of being shocked.@n
          %echoaround% %regenpeeps.name% @R%regenpeeps.name% twitches slightly.@n
        end
      end
    end
  end
  eval nextpeep %regenpeeps.next_in_room%
  eval regenpeeps %nextpeep%
done
~
#62650
Shock Controller~
1 c 3
shock~
eval collarvnum 62693
eval shock1 3
eval shock2 8
eval shock3 13
eval shock4 18
%force% %actor% emote presses a button on a shock collar remote control.
if (%cmd% == Shock)
  eval myfreq %arg%
  if (%myfreq% < 1 || %myfreq% > 15)
    %send% %actor% @RThe Shock Controller only works if you give a frequency between @Y1@R and @Y15@R.@n
  else
    eval shockpeeps %self.room.people%
    while (%shockpeeps%)
      eval hascollar %shockpeeps.eq(3)%
      eval hascollar2 %shockpeeps.eq(4)%
      if (%hascollar.vnum% == %collarvnum% || %hascollar2.vnum% == %collarvnum%)
        if (!%shockpeeps.varexists(shocklevel)%)
          eval shocklevel 0
          remote shocklevel %shockpeeps.id%
          %echoaround% %shockpeeps.name% @R%shockpeeps.name% convulses as a shock collar delivers a jolt to them!@n
          %send% %shockpeeps.name% @RYou convulse as a shock collar delivers a jolt to you!@n
          wait 1
        else
          eval shocklevel %shockpeeps.shocklevel%
          eval shocklevel %shocklevel% + 2
          remote shocklevel %shockpeeps.id%
          if (%shocklevel% > %shock4%)
            %send% %shockpeeps.name% @RYou lose control of your body entirely from the cumulative affects of being shocked!@n
            %echoaround% %shockpeeps.name% @R%shockpeeps.name% falls to the ground, limbs spasming beyond %shockpeeps.hisher% control!@n
            %force% %shockpeeps.name% rest
          elseif (%shocklevel% > %shock3%)
            %send% %shockpeeps.name% @RThe shock delivered to you through the shock collar forces you to your knees!@n
            %echoaround% %shockpeeps.name% @R%shockpeeps.name% falls to the ground, limbs spasming beyond %shockpeeps.hisher% control!@n
            %force% %shockpeeps.name% sit
          elseif (%shocklevel% > %shock2%)
            %send% %shockpeeps.name% @RThe collar shocks you, causing your muscles to contract involuntarily!  This is starting to be really uncomfortable.@n
            %echoaround% %shockpeeps.name% @R%shockpeeps.name% spasms, barely able to control %shockpeeps.himher%self through the shock delivered by the shock collar!@n
            %force% %shockpeeps.name% sit
          elseif (%shocklevel% > %shock1%)
            %send% %shockpeeps.name% @RThe collar shocks you, causing your muscles to contract slightly!@n
            %echoaround% %shockpeeps.name% @R%shockpeeps.name% spasms slightly as the shock collar delivers a mild jolt.@n
            %force% %shockpeeps.name% sit
          else
            %send% %shockpeeps.name% @RThe collar shocks you, causing your muscles to contract slightly!@n
            %echoaround% %shockpeeps.name% @R%shockpeeps.name% spasms slightly as the shock collar delivers a mild jolt.@n
          end
        end
      end
      eval nextpeep %shockpeeps.next_in_room%
      eval shockpeeps %nextpeep%
    done
  end
else
return 0
end
~
#62651
Collar Removal Prevention~
1 l 100
~
set shocklevel 0
if (%actor.varexists(shocklevel)%)
  eval shocklevel %actor.shocklevel%
  if (%shocklevel% > 0)
    wait 1
    %force% %actor.name% wear collar
    %force% %actor% emote tries to take the collar off, but is shocked by it and can't!
  end
end
~
#62652
Remove Pride Var~
2 c 3
Booyah~
rdelete %pride% actor.id
~
#62677
Lavos Quest Accept~
0 cd 100
challenge~
If ("%arg%" != "lav" && "%arg%" != "lavos")
    return 1
    wait 1s
    %send% %actor% @DTry offering a @RChallenge@D to @RL@Ya@Rv@Yo@Rs@D.@n
Else
  if (!%actor.varexists(onraq)%)
    set onlavq
    remote onlavq %actor.id%
wait 1s
say @RVery well. Your soul ... is mine.@n
wait 5s
    mtransform 62699
wait 10s
    mkill %actor%
Else
  if (%actor.varexists(onlavq)%)
    wait 1s
    say @RSuch a fool ...@n
    wait 5s
    mtransform 62699
    wait 10s
    mkill %actor%
   End
  End
End
~
#62688
Lavos Death~
0 f 100
~
Eval num %random.1000% 
    if (%num% >= 920) 
    %load% obj 62599
    %load% obj 62595
    rdelete onlavq %actor.id%
  Else
    if (%num% >=700)
    %load% obj 62599
    rdelete onlavq %actor.id%
  Else
    rdelete onlavq %actor.id%
   End
  End
End
~
#62690
Lavos Greeting~
0 g 100
~
If (%actor.is_pc%)
  If (!%actor.varexists(onlavq)%)
    wait 1s
    %echoaround% lavos The elemental of @Rl@Yi@Rv@Yi@Rn@Yg@n fire lifts its head slowly to stare at %actor.name% with soulless eyes.
    wait 2s
    %echoaround% lavos His mouth opens and an immense heat washes over the room, nearly overwhelming in its intensity.
    wait 1s
    say @RI am L@Ya@Rv@Yo@Rs, the Great Fire. @WChallenge@R me fool, and I will consume you as I will consume all in this universe.@n
  Else
    wait 2s
    say @RYou were lucky once, fool. You will not be so again!@n
    wait 5s
mtransform 62699
wait 10s
    mkill %actor%
  End
End
~
#62691
Omen Computer Boot~
1 c 100
boot~
If ("%arg%" != "comp" && "%arg%" != "computer")
    return 1
    wait 1s
    %send% %actor% @DTry to @GBoot@D the @GComputer@n.
  Else
      %force% %actor% look menuone
End
~
#62692
Lavos Mammon Drain~
0 b 5
~
emote lets out a low rumble as the Mammon Machine leeches energy from it.
~
#62693
Random Energy Pulse~
2 ab 20
~
%echo% @DA particularly intense pulse of dull, @rred@D energy suddenly surges through the conduits in the walls, fading slowly.@n
~
#62694
Isis Death~
0 f 100
~
Eval num %random.1000% 
    if (%num% >= 920) 
    %load% obj 62594
    %load% obj 62597
    rdelete onisq %actor.id%
  Else
    if (%num% >=700)
    %load% obj 62594
    rdelete onisq %actor.id%
  Else
    rdelete onisq %actor.id%
   End
  End
End
~
#62695
Isis Accept~
0 cd 100
challenge~
If ("%arg%" != "Is" && "%arg%" != "Isis")
    return 1
    wait 1s
    %send% %actor% @DTry offering a @RChallenge@D to @WI@ns@Wi@ns@D.@n
Else
  if (!%actor.varexists(onisq)%)
    set onisq
    remote onisq %actor.id%
wait 1s
say @CVery well. I will give you the Mother's last embrace.@n
wait 5s
    mtransform 62697
wait 10s
    mkill %actor%
Else
  if (%actor.varexists(onisq)%)
    say @CI have already offered you mercy, please escape this place!@n
    wait 5s
    mtransform 62697
    wait 10s
    mkill %actor%
   End
  End
End
~
#62696
Isis Greeting~
0 g 100
~
If (%actor.is_pc%)
  If (!%actor.varexists(onisq)%)
    wait 1s
    %echoaround% isis The beautiful Kai smiles at %actor.name% sadly.
    wait 2s
    say @CHere ... those controls back there will take you back to the exit of this horrible prison.@n 
    wait 2s
    say @CYou should escape while you can, but if you think you're brave enough to defeat the Great Fire then accept my @Rchallenge@n so that I may know it is true.@n
  Else
    wait 1s
%echoaround% isis The beautiful Kai smiles at %actor.name% sadly.
    wait 2s
    say @CAre you ready to show me your worth now?@n
    wait 5s
mtransform 62697
wait 10s
    mkill %actor%
  End
End
~
#62697
Horus Death~
0 f 100
~
Eval num %random.1000% 
    if (%num% >= 920) 
    %load% obj 62596
    %load% obj 62692
    rdelete onhq %actor.id%
  Else
    if (%num% >=700)
    %load% obj 62692
    rdelete onhq %actor.id%
  Else
    rdelete onhq %actor.id%
   End
  End
End
~
#62698
Osiris Death~
0 f 100
~
Eval num %random.1000% 
    if (%num% >= 920) 
    %load% obj 62599
    %load% obj 62692
    rdelete onosq %actor.id%
  Else
    if (%num% >=700)
    %load% obj 62606
    rdelete onosq %actor.id%
  Else
    rdelete onosq %actor.id%
   End
  End
End
~
#62699
Horus Quest Accept~
0 cd 100
challenge~
If ("%arg%" != "Ho" && "%arg%" != "Horus")
    return 1
    wait 1s
    %send% %actor% @DTry offering a @RChallenge@D to @RH@ro@Rr@ru@Rs@D.@n
Else
  if (!%actor.varexists(onhq)%)
    set onhq
    remote onhq %actor.id%
wait 1s
say @RYour choice, mortal. Prepare to die in this horrible place.@n
wait 5s
    mtransform 62696
wait 10s
    mkill %actor%
Else
  if (%actor.varexists(onhq)%)
    say @RYou've already consigned yourself to death. Face it with pride!@n
    wait 5s
    mtransform 62696
    wait 10s
    mkill %actor%
   End
  End
End
~
$~
