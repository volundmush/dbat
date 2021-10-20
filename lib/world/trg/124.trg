#12400
Z Sword Get~
1 g 100
~
if (!%actor.varexists(zsword)%)
 wait 1
 %send% %actor% You pull free the z-sword and feel its mighty weight in your hands!
 %echoaround% %actor% %actor.name% pulls the z-sword from its resting place!
 set zsword
 remote zsword %actor.id%
elseif (%actor.room.vnum% == 12488)
 return 0
 wait 1
 %send% %actor% You already have the z-sword, this one might be fake... better leave it.
end
~
#12401
Z Sword Drop~
1 h 100
~
if (%actor.varexists(zsword)% && %actor.room.vnum% == 12488)
 wait 1
 %send% %actor% You stab the sword into its resting place!
 %echoaround% %actor% %actor.name% stabs the sword into its reseting place!
 rdelete zsword %actor.id%
end
~
#12402
Mistress Ortusia Load~
1 c 3
place~
if %arg% != pearl
  wait 1 s
  %send% %actor% Try placing the pearl.
else
  if %actor.room.vnum% != 12479
    wait 1 s
    %send% %actor% There is no where to place this pearl.
  else
    wait 1 s
    %send% %actor% You place the pearl into the indent on the sarcophagus.
    %echoaround% %actor% %actor.name% places a pearl into the indent on the sarcophagus.
    wait 2 sec
    %send% %actor% You push the pearl gently and a clicking sound can be heard.
    %echoaround% %actor% %actor.name% pushes the pearl gently and a clicking sound can be heard.
    wait 4 sec
    %echo% The clicking of mechanisms inside the sarcophagus can be heard coming from inside.
    wait 3 sec
    %echo% Creaking can be heard as the seal holding the lid together breaks and begins to open.
    wait 3 sec
    %echo% The sarcophagus opens fully, revealing an unaged Mistress Ortusia!
    wait 3 sec
    %echo% Ortusia begins singing softly, her words invading your thoughts.
    wait 3 sec
    %echo% Ortusia sings, "Join the Makai or die" as she steps out of the sarcophagus.
    wait 2 s
    %load% mob 12406
    wait 2 sec
    %send% %actor% The %self.shortdesc% disappears in a flash of bright light!
    %echoaround% %actor% The %self.shortdesc% disappears in a flash of bright light!
    %purge% self
  end 
end
~
#12403
Mistress Ortusia load tongue~
0 n 100
~
%load% obj 12401
~
#12404
Shin Teleport~
0 ab 5
~
%echo% Shin places two fingers on his forehead and vanishes!
eval shinport %random.9%
switch %shinport%
case 1
%teleport% %self% 1544
break
case 2
%teleport% %self% 3293
break
case 3
%teleport% %self% 5578
break
case 4
%teleport% %self% 9216
break
case 5
%teleport% %self% 11901
break
case 6
%teleport% %self% 12498
break
case 7
%teleport% %self% 14333
break
case 8
%teleport% %self% 17198
break
default
%teleport% %self% 13552
break
done
~
#12405
Shin Fight~
0 k 35
~
eval tactics %random.4%
switch %tactics%
  case 1
    wait 1 sec
    kousengan
  break
  case 2
    wait 1 sec
    kiblast
  break
  case 3
    wait 1 sec
    barrage
    break
  default
    %echo% Shin focuses his ki and heals some of his combat damage!
    nop %self.hitp(10000000)%
  break
done
~
#12406
Kai Phase~
2 c 100
phase~
 if (%actor.race% == Kai)
  Wait 1 s
  %send% %actor% @WYou sit upon the rock with your legs crossed.  You close your eyes and focus your energy intently, finding yourself filled with a deep calm.@n
  %echoaround% %actor% @C%actor.name% @Wclimbs upon the rock and crosses their legs. They close their eyes and grow quiet and still, very focused.  Their breathing slows.@n
  Wait 3 s
  %send% %actor% @WA bright flash of light surrounds you, and when you open your eyes you find yourself in the fields of the Grand Palace of the Afterlife!@n
  %echoaround% %actor% @WA bright flash of light surrounds @C%actor.name%@W and once the light fades, its apparent that theyre gone!@n
  %teleport% %actor% 6813
  %force% %actor% look
  %echoaround% %actor% @C%actor.name%@W winks into existence upon the smooth gray rock.@n
 Else
  Wait 1 s
  %send% %actor% @WYou sit upon the rock with your legs crossed. You close your eyes and focus your energy intently, filling yourself with a deep calm.  Nothing else happens.@n
 %echoaround% %actor% @C%actor.name% @Wclimbs upon the rock and crosses their legs. They close their eyes and grow quiet and still.  Nothing else happens.@n
 end
~
#12407
Kai Phase from AL~
2 c 100
phase~
If (%actor.race% == Kai)
 If (!%actor.dead%)
  If (%arg% == Earth)
    %send% %actor% @WYou sit upon the rock with your legs crossed.  You close your eyes and focus your energy intently, finding yourself filled with a deep calm.@n
    %echoaround% %actor% @C%actor.name% @Wclimbs upon the rock and crosses their legs. They close their eyes and grow quiet and still, very focused.  Their breathing slows.@n
    Wait 3 s
    %send% %actor% @WA bright flash of light surrounds you, and when you open your eyes you find yourself by the river in the Northern Plains!@n
    %echoaround% %actor% @WA bright flash of light surrounds @C%actor.name%@W and once the light fades, its apparent that theyre gone!@n
    %teleport% %actor% 1462
    %force% %actor% look
    %echoaround% %actor% @C%actor.name%@W winks into existence upon the smooth gray rock.@n
   Elseif (%arg% == Aether)
    %send% %actor% @WYou sit upon the rock with your legs crossed.  You close your eyes and focus your energy intently, finding yourself filled with a deep calm.@n
    %echoaround% %actor% @C%actor.name% @Wclimbs upon the rock and crosses their legs. They close their eyes and grow quiet and still, very focused.  Their breathing slows.@n
    Wait 3 s
    %send% %actor% @WA bright flash of light surrounds you, and when you open your eyes you find yourself inside the center of the Silent Glade!@n
    %echoaround% %actor% @WA bright flash of light surrounds @C%actor.name%@W and once the light fades, its apparent that theyre gone!@n
    %teleport% %actor% 12488
    %force% %actor% look
    %echoaround% %actor% @C%actor.name%@W winks into existence upon the smooth gray rock.@n
   Elseif (%arg% == Namek)
    %send% %actor% @WYou sit upon the rock with your legs crossed.  You close your eyes and focus your ene-rgy intently, finding yourself filled with a deep calm.@n
    %echoaround% %actor% @C%actor.name% @Wclimbs upon the rock and crosses their legs. They close their eyes and grow quiet and still, very focused.  Their breathing slows.@n
    Wait 3 s
    %send% %actor% @WA bright flash of light surrounds you, and when you open your eyes you find yourself by Senzu Village on Namek!@n
    %echoaround% %actor% @WA bright flash of light surrounds @C%actor.name%@W and once the light fades, its apparent that theyre gone!@n
    %teleport% %actor% 10702
    %force% %actor% look
    %echoaround% %actor% @C%actor.name%@W winks into existence upon the smooth gray rock.@n
   Elseif (%arg% == Konack)
    %send% %actor% @WYou sit upon the rock with your legs crossed.  You close your eyes and focus your energy intently, finding yourself filled with a deep calm.@n
    %echoaround% %actor% @C%actor.name% @Wclimbs upon the rock and crosses their legs. They close their eyes and grow quiet and still, very focused.  Their breathing slows.@n
    Wait 3 s
    %send% %actor% @WA bright flash of light surrounds you, and when you open your eyes you find yourself inside of Meridius Memorial!@n
    %echoaround% %actor% @WA bright flash of light surrounds @C%actor.name%@W and once the light fades, its apparent that theyre gone!@n
    %teleport% %actor% 8820
    %force% %actor% look
    %echoaround% %actor% @C%actor.name%@W winks into existence upon the smooth gray rock.@n
   else 
    Return 1
    %send% %actor% @CTry using @Wphase <destination>@C. Valid destinations are Earth, Aether, Namek, and Konack.@n
   End
  Elseif (%actor.dead%)
   Return 1
  %send% %actor% @WYou sit upon the rock with your legs crossed. You close your eyes and focus your energy intently, filling yourself with a deep calm.  Nothing else happens.@n
  %echoaround% %actor% @C%actor.name% @Wclimbs upon the rock and crosses their legs. They close their eyes and grow quiet and still.  Nothing else happens.@n   
  %send% %actor% You can't phase out of the afterlife if you're dead...
  End
 Else
  Return 1
  %send% %actor% @WYou sit upon the rock with your legs crossed. You close your eyes and focus your energy intently, filling yourself with a deep calm.  Nothing else happens.@n
  %echoaround% %actor% @C%actor.name% @Wclimbs upon the rock and crosses their legs. They close their eyes and grow quiet and still.  Nothing else happens.@n
 end
~
#12408
Available for use, no longer needed~
2 c 100
phase~
If (%actor.race% == Kai)
 If (!%actor.dead%)
  If (%arg% == earth || Earth)
    %teleport% %actor% 1462
    %force% %actor% look
   elseif (%arg% == aether || Aether)
    %teleport% %actor% 12488
    %force% %actor% look
   elseif (%arg% == namek || Namek)
    %teleport% %actor% 10702
    %force% %actor% look
   elseif (%arg% == konack || Konack)
    %teleport% %actor% 8820
    %force% %actor% look
   else 
    return 1
    %send% %actor% @CTry using @Wphase <destination>@C. Valid destinations are Earth, Aether, Namek, and Konack.@n
   end
  elseif (%actor.dead%)
   return 1
  %send% %actor% You cant phase out of the afterlife if youre dead
  end
 else
  return 1
  %send% %actor% Youre no kai.
 end
~
$~
