#13500
Training Simulation Start: Humans~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 11)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Human opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Nexus Fields!
    %teleport% %actor% 13570
    %force% %actor% look
  elseif (%actor.level% >= 11 && %actor.level% < 20)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Human opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Nexus Fields!
    %teleport% %actor% 13585
    %force% %actor% look
  elseif (%actor.level% >= 20)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Human opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Nexus Fields!
    %teleport% %actor% 13600
    %force% %actor% look
  end
end
~
#13501
Human Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13553
%at% 13553 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13502
Training Simulation Start: Namekians~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 4)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=4 && %actor.level% < 14)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Namekian opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself on Planet Namek!
    %teleport% %actor% 13613
    %force% %actor% look
  elseif (%actor.level% >= 14 && %actor.level% < 24)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Namekian opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself on Planet Namek!
    %teleport% %actor% 13628
    %force% %actor% look
  elseif (%actor.level% >= 24)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Namekian opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself on Planet Namek!
    %teleport% %actor% 13643
    %force% %actor% look
  end
end
~
#13503
Namek Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13554
%at% 13554 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13504
Training Simulation Start: Mutants~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 7)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=7 && %actor.level% < 15)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Mutant opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the Slave Market!
    %teleport% %actor% 13658
    %force% %actor% look
  elseif (%actor.level% >= 15 && %actor.level% < 25)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Mutant opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the Slave Market!
    %teleport% %actor% 13673
    %force% %actor% look
  elseif (%actor.level% >= 25)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Mutant opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the Slave Market!
    %teleport% %actor% 13688
    %force% %actor% look
  end
end
~
#13505
Mutant Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13556
%at% 13556 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13506
Training Simulation Start: Kanassans~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 10)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=10 && %actor.level% < 18)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Kanassan opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself on planet Kanassa!
    %teleport% %actor% 13713
    %force% %actor% look
  elseif (%actor.level% >= 18 && %actor.level% < 28)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Kanassan opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself on planet Kanassa!
    %teleport% %actor% 13728
    %force% %actor% look
  elseif (%actor.level% >= 28)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Kanassan opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself on planet Kanassa!
    %teleport% %actor% 13747
    %force% %actor% look
  end
end
~
#13507
Kanassan Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13555
%at% 13555 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13508
Training Simulation Start: Kaios~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 12)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=12 && %actor.level% < 21)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Kaio opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the Silent Glade!
    %teleport% %actor% 13748
    %force% %actor% look
  elseif (%actor.level% >= 21 && %actor.level% < 31)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Kaio opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the Silent Glade!
    %teleport% %actor% 13763
    %force% %actor% look
  elseif (%actor.level% >= 31)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Kaio opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the Silent Glade!
    %teleport% %actor% 13778
    %force% %actor% look
  end
end
~
#13509
Kaio Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13558
%at% 13558 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13510
Training Simulation Start: Demons~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 15)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=15 && %actor.level% < 23)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Demon opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Hell!
    %teleport% %actor% 13803
    %force% %actor% look
  elseif (%actor.level% >= 23 && %actor.level% < 33)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Demon opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Hell!
    %teleport% %actor% 13818
    %force% %actor% look
  elseif (%actor.level% >= 33)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Demon opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Hell!
    %teleport% %actor% 13833
    %force% %actor% look
  end
end
~
#13511
Demon Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13557
%at% 13557 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13512
Training Simulation Start: Halfbreeds~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 18)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=18 && %actor.level% < 28)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Halfbreed opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in a sandy desert!
    %teleport% %actor% 13838
    %force% %actor% look
  elseif (%actor.level% >= 28 && %actor.level% < 38)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Halfbreed opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in a sandy desert!
    %teleport% %actor% 13853
    %force% %actor% look
  elseif (%actor.level% >= 38)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Halfbreed opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in a sandy desert!
    %teleport% %actor% 13868
    %force% %actor% look
  end
end
~
#13513
Halfbreed Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13560
%at% 13560 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13514
Training Simulation Start: Icers~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 21)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=21 && %actor.level% < 31)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Icerian opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself at the Topica Snowfields!
    %teleport% %actor% 13883
    %force% %actor% look
  elseif (%actor.level% >= 31 && %actor.level% < 41)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Icerian opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself at the Topica Snowfields!
    %teleport% %actor% 13898
    %force% %actor% look
  elseif (%actor.level% >= 41)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Icerian opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself at the Topica Snowfields!
    %teleport% %actor% 13913
    %force% %actor% look
  end
end
~
#13515
Icer Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13559
%at% 13559 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13516
Training Simulation Start: Androids~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 27)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=27 && %actor.level% < 37)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Android opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Doctor Gero's Lab!
    %teleport% %actor% 13928
    %force% %actor% look
  elseif (%actor.level% >= 37 && %actor.level% < 47)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Android opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Doctor Gero's Lab!
    %teleport% %actor% 13943
    %force% %actor% look
  elseif (%actor.level% >= 47)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Android opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Doctor Gero's Lab!
    %teleport% %actor% 13958
    %force% %actor% look
  end
end
~
#13517
Android Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13561
%at% 13561 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13518
Training Simulation Start: Bio Androids~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 30)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=30 && %actor.level% < 40)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Bio-Android opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Doctor Gero's Lab!
    %teleport% %actor% 13973
    %force% %actor% look
  elseif (%actor.level% >= 40 && %actor.level% < 50)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Bio-Android opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Doctor Gero's Lab!
    %teleport% %actor% 13988
    %force% %actor% look
  elseif (%actor.level% >= 50)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Bio-Android opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Doctor Gero's Lab!
    %teleport% %actor% 14503
    %force% %actor% look
  end
end
~
#13519
BioAndroid Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13562
%at% 13562 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13520
Training Simulation Start: Majin~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 34)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=34 && %actor.level% < 44)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Majin opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Candy Land!
    %teleport% %actor% 14518
    %force% %actor% look
  elseif (%actor.level% >= 44 && %actor.level% < 54)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Majin opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Candy Land!
    %teleport% %actor% 14533
    %force% %actor% look
  elseif (%actor.level% >= 54)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Majin opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in Candy Land!
    %teleport% %actor% 14548
    %force% %actor% look
  end
end
~
#13521
Majin Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13564
%at% 13564 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13522
Training Simulation Start: Saiyans~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 38)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=38 && %actor.level% < 48)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Saiyan opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the Ancestral Mountains!
    %teleport% %actor% 14563
    %force% %actor% look
  elseif (%actor.level% >= 48 && %actor.level% < 58)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Saiyan opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the Ancestral Mountains
    %teleport% %actor% 14578
    %force% %actor% look
  elseif (%actor.level% >= 58)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Saiyan opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the Ancestral Mountains
    %teleport% %actor% 14593
    %force% %actor% look
  end
end
~
#13523
Saiyan Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13563
%at% 13563 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13524
Training Simulation Start: Konatsu~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 42)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=42 && %actor.level% < 52)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Konatsu opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the Elzthuan Forest!
    %teleport% %actor% 14608
    %force% %actor% look
  elseif (%actor.level% >= 52 && %actor.level% < 62)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Konatsu opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the Elzthuan Forest!
    %teleport% %actor% 14623
    %force% %actor% look
  elseif (%actor.level% >= 62)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Konatsu opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the Elzthuan Forest!
    %teleport% %actor% 14638
    %force% %actor% look
  end
end  
~
#13525
Konatsu Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13565
%at% 13565 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13526
Training Simulation Start: Yardratians~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 46)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=46 && %actor.level% < 56)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Yardratian opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself on planet Yardrat!
    %teleport% %actor% 14653
    %force% %actor% look
  elseif (%actor.level% >= 56 && %actor.level% < 66)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Yardratian opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself on planet Yardrat!
    %teleport% %actor% 14668
    %force% %actor% look
  elseif (%actor.level% >= 66)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Yardratian opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself on planet Yardrat!
    %teleport% %actor% 14683
    %force% %actor% look
  end
end
~
#13527
Yardratian Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13566
%at% 13566 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13528
Training Simulation Start: Arlians~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 50)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, sorry you are not strong enough to use this Simulation program.
  elseif (%actor.level% >=50 && %actor.level% < 60)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level one Yardratian opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself on planet Yardrat!
    %teleport% %actor% 14698
    %force% %actor% look
  elseif (%actor.level% >= 60 && %actor.level% < 70)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level two Yardratian opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself on planet Yardrat!
    %teleport% %actor% 14713
    %force% %actor% look
  elseif (%actor.level% >= 70)
    %send% %actor.name% Evalutating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined. Level three Yardratian opponents.
    wait 2 s
    %send% %actor.name% A blinding light flashes around you momentarily, then you find yourself on planet Yardrat!
    %teleport% %actor% 14728
    %force% %actor% look
  end
end
~
#13529
Arlian Simulation: Leave~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 13567
%at% 13567 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#13530
Neo Nirvana Elevator Controls~
2 c 100
push~
if (%arg% == six)
  if %self.vnum% != 13543
    %send% %actor% You push the button for the maintenance deck.
    %echoaround% %actor% %actor.name% pushes the button for the maintenance deck.
    wait 2 s
    %echo% The doors close and the elevator begins to move slowly.
    wdoor 13544 3 flags abc
    wdoor 13543 1 flags abc
    wait 2 s
    %echo% The elevator moves rapidly towards it's destination.
    wait 2 s
    %echo% The elevator comes to a sudden stop. "Sixth Floor - Maintenance Deck"
    wdoor 13543 1 flags a
    wdoor 13544 3 flags a
    wteleport all 13543
  else
    %send% %actor% You are already on that floor.
  end
end
if (%arg% == five)
  if %self.vnum% != 13533
    %send% %actor% You push the button for the residential deck.
    %echoaround% %actor% %actor.name% pushes the button for the residential deck.
    wait 2 s
    %echo% The doors close and the elevator begins to move slowly.
    wdoor 13534 3 flags abc
    wdoor 13533 1 flags abc
    wait 2 s
    %echo% The elevator moves rapidly towards it's destination.
    wait 2 s
    %echo% The elevator comes to a sudden stop. "Fifth Floor - Residential Deck"
    wdoor 13533 1 flags a
    wdoor 13534 3 flags a
    wteleport all 13533
  else
    %send% %actor% You are already on that floor.
  end
end
if (%arg% == four)
  if %self.vnum% != 13519
    %send% %actor% You push the button for the combat training deck.
    %echoaround% %actor% %actor.name% pushes the button for the combat training deck.
    wait 2 s
    %echo% The doors close and the elevator begins to move slowly.
    wdoor 13520 3 flags abc
    wdoor 13519 1 flags abc
    wait 2 s
    %echo% The elevator moves rapidly towards it's destination.
    wait 2 s
    %echo% The elevator comes to a sudden stop. "Fourth Floor - Combat Training Deck"
    wdoor 13519 1 flags a
    wdoor 13520 3 flags a
    wteleport all 13519
  else
    %send% %actor% You are already on that floor.
  end
end
if (%arg% == three)
  if %self.vnum% != 13530
    %send% %actor% You push the button for the science deck.
    %echoaround% %actor% %actor.name% pushes the button for the science deck.
    wait 2 s
    %echo% The doors close and the elevator begins to move slowly.
    wdoor 13531 3 flags abc
    wdoor 13530 1 flags abc
    wait 2 s
    %echo% The elevator moves rapidly towards it's destination.
    wait 2 s
    %echo% The elevator comes to a sudden stop. "Third Floor - Science Deck"
    wdoor 13530 1 flags a
    wdoor 13531 3 flags a
    wteleport all 13530
  else
    %send% %actor% You are already on that floor.
  end
end
if (%arg% == two)
  if %self.vnum% != 13508
    %send% %actor% You push the button for the commercial deck.
    %echoaround% %actor% %actor.name% pushes the button for the commercial deck.
    wait 2 s
    %echo% The doors close and the elevator begins to move slowly.
    wdoor 13509 3 flags abc
    wdoor 13508 1 flags abc
    wait 2 s
    %echo% The elevator moves rapidly towards it's destination.
    wait 2 s
    %echo% The elevator comes to a sudden stop. "Second Floor - Commercial Deck"
    wdoor 13508 1 flags a
    wdoor 13509 3 flags a
    wteleport all 13508
  else
    %send% %actor% You are already on that floor.
  end
end
if (%arg% == one)
  if %self.vnum% != 13507
    %send% %actor% You push the button for the command deck.
    %echoaround% %actor% %actor.name% pushes the button for the command deck.
    wait 2 s
    %echo% The doors close and the elevator begins to move slowly.
    wdoor 13506 3 flags abc
    wdoor 13507 1 flags abc
    wait 2 s
    %echo% The elevator moves rapidly towards it's destination.
    wait 2 s
    %echo% The elevator comes to a sudden stop. "First Floor - Command Deck"
    wdoor 13507 1 flags a
    wdoor 13506 3 flags a
    wteleport all 13507
  else
    %send% %actor% You are already on that floor.
  end
end
~
#13531
Open Neo elevator~
2 c 100
push~
if (%arg% != open)
  %send% %actor% @nTry @Wpush@ning the @Wopen@n button.
else
  if %self.vnum% == 13544
    %send% %actor% You push the open button.
    wait 2 s
    %echo% The elevator doors slide open.
    wdoor 13544 3 flags a
    wdoor 13543 1 flags a
  elseif %self.vnum% == 13534
    %send% %actor% You push the open button.
    wait 2 s
    %echo% The elevator doors slide open.
    wdoor 13534 3 flags a
    wdoor 13533 1 flags a
  elseif %self.vnum% == 13531
    %send% %actor% You push the open button.
    wait 2 s
    %echo% The elevator doors slide open.
    wdoor 13531 3 flags a
    wdoor 13530 1 flags a
  elseif %self.vnum% == 13520
    %send% %actor% You push the open button.
    wait 2 s
    %echo% The elevator doors slide open.
    wdoor 13520 3 flags a
    wdoor 13519 1 flags a
  elseif %self.vnum% == 13509
    %send% %actor% You push the open button.
    wait 2 s
    %echo% The elevator doors slide open.
    wdoor 13509 3 flags a
    wdoor 13508 1 flags a
  elseif %self.vnum% == 13506
    %send% %actor% You push the open button.
    wait 2 s
    %echo% The elevator doors slide open.
    wdoor 13506 3 flags a
    wdoor 13507 1 flags a
  end
end
~
#13552
Unlocker Minion~
0 n 100
~
wait 1
%load% obj 13569
unlock barrier
mgoto 38919
close ship
mgoto 49
~
#13553
Minion 2 load~
0 n 100
~
wait 1
%load% o 13570
wait 1
%purge% %self%
~
#13570
Neo Flies!~
1 ab 10
~
eval move %random.4%
 switch (%move%)
  case 1
   if (%self.room.north%)
    wait 1
    %echo% %self.name% flies north.
    wait 1
    ogoto %self.room.north(vnum)%
   end
  break
  case 2
   if (%self.room.east%)
    wait 1
    %echo% %self.name% flies east.
    wait 1
    ogoto %self.room.east(vnum)%
   end
  break
  case 3
   if (%self.room.south%)
    wait 1
    %echo% %self.name% flies south.
    wait 1
    ogoto %self.room.south(vnum)%
   end
  break
  case 4
   if (%self.room.west%)
    wait 1
    %echo% %self.name% flies west.
    wait 1
    ogoto %self.room.west(vnum)%
   end
  break
 done
~
#13580
Asur Block~
0 q 100
~
if (%direction% == west)
  if (%actor.race% != truffle %% !%actor.varexists(neoguest)%)
    say Sorry about only Truffles or honored guests are allowed past this point. Say guest and I will tell you what you might do.
    %send% %actor.name% %self.name% prevents you from going any further.
    %echoaround% %actor.name% As %actor.name% tries to go west,%self.name% stops %actor.himher%.
    return 0
  elseif (%actor.race% == truffle)
    %send% %actor.name% %self.name% says, 'Good day to you brother.'
  elseif (%actor.varexists(neoguest)%)
    %send% %actor.name% %self.name% says, 'Good day to you honored guest.'
  end
  return 1
end 
~
$~
