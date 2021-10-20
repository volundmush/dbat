#2900
Portal Enter Trigger~
1 c 4
enter~
if (!%actor.varexists(pride)% && %arg% == portal)
 wait 1
 %send% %actor% You shouldn't be in this tower.
 %send% %actor% Due to this you have been warped to its entrance.
 %teleport% %actor% 2899
 wait 1 s
 %force% %actor% look
elseif (%arg% == portal)
 switch (%actor.room.vnum%)
  case 2941
   if (%actor.pride% != bravery && %actor.pride% != strength && %actor.pride% != wisdom && %actor.pride% != cunning && %actor.pride% != honor && %actor.pride% != victory && %actor.pride% != pride)
    wait 1
    %send% %actor% You haven't solved the puzzle of this floor yet.
   else
    wait 1
    %send% %actor% Lightning swirls through the portal as it teleports you.
    wait 1 s
    %teleport% %actor% 2989
    %force% %actor% look
   end
  break
  case 2989
   wait 1
   %send% %actor% Lightning swirls through the portal as it teleports you.
   wait 1 s
   %teleport% %actor% 2941
   %force% %actor% look
  break
  case 2948
   if (%actor.pride% != strength && %actor.pride% != wisdom && %actor.pride% != cunning && %actor.pride% != honor && %actor.pride% != victory && %actor.pride% != pride)
    wait 1
    %send% %actor% You haven't solved the puzzle of this floor yet.
   else
    wait 1
    %send% %actor% Lightning swirls through the portal as it teleports you.
    wait 1 s
    %teleport% %actor% 2996
    %force% %actor% look
   end
  break
  case 2996
   wait 1
   %send% %actor% Lightning swirls through the portal as it teleports you.
   wait 1 s
   %teleport% %actor% 2948
   %force% %actor% look
  break
  case 3037
   if (%actor.pride% != wisdom && %actor.pride% != cunning && %actor.pride% != honor && %actor.pride% != victory && %actor.pride% != pride)
    wait 1
    %send% %actor% You haven't solved the puzzle of this floor yet.
   else
    wait 1
    %send% %actor% Lightning swirls through the portal as it teleports you.
    wait 1 s
    %teleport% %actor% 3085
    %force% %actor% look
   end
  break
  case 3085
   wait 1
   %send% %actor% Lightning swirls through the portal as it teleports you.
   wait 1 s
   %teleport% %actor% 3037
   %force% %actor% look
  break
  case 3044
   if (%actor.pride% != cunning && %actor.pride% != honor && %actor.pride% != victory && %actor.pride% != pride)
    wait 1
    %send% %actor% You haven't solved the puzzle of this floor yet.
   else
    wait 1
    %send% %actor% Lightning swirls through the portal as it teleports you.
    wait 1 s
    %teleport% %actor% 3092
    %force% %actor% look
   end
  break
  case 3092
   wait 1
   %send% %actor% Lightning swirls through the portal as it teleports you.
   wait 1 s
   %teleport% %actor% 3044
   %force% %actor% look
  break
  case 3133
   if (%actor.pride% != honor && %actor.pride% != victory && %actor.pride% != pride)
    wait 1
    %send% %actor% You haven't solved the puzzle of this floor yet.
   else
    wait 1
    %send% %actor% Lightning swirls through the portal as it teleports you.
    wait 1 s
    %teleport% %actor% 3181
    %force% %actor% look
   end
  break
  case 3181
   wait 1
   %send% %actor% Lightning swirls through the portal as it teleports you.
   wait 1 s
   %teleport% %actor% 3133
   %force% %actor% look
  break
  default
   wait 1
   %send% %actor% This portal is not where it should be, please report this error.
  break
 done
else
 return 0
end
~
#2901
Riddle Invoke~
1 c 2
invoke~
 wait 1
 %send% %actor% You hold up %self.shortdesc% and a beam of white light shoots out from it.
 %echoaround% %actor% %actor.name% holds up %self.shortdesc% and a beam of white light shoots out from it.
 wait 3 s
 %send% %actor% A voice in your mind speaks the following riddle.
 wait 3 s
 switch (%actor.room.vnum%)
  case 2940
   if (%self.vnum% == 2901)
    %send% %actor% @w"Loved by those who are rescued, I am remembered even while in the grave."@n
   elseif (%self.vnum% == 2902)
    %send% %actor% @w"Orders bravely I follow, on the battle field is where I win my fame."@n
   elseif (%self.vnum% == 2903)
    %send% %actor% @w"For battle I head, following my own stead, my skills for war are bred."@n
   else
    %send% %actor% You need to put away orbs not meant for this floor.
   end
  break
  case 2979
   if (%self.vnum% == 2904)
    %send% %actor% @w"I lift, I pull, I push. Not of mind or bone am I."@n
   elseif (%self.vnum% == 2905)
    %send% %actor% @w"I increase your struggle, increase your load, but without me you wouldn't keep your feet home."@n
   elseif (%self.vnum% == 2906)
    %send% %actor% @w"I am to you what a cup is to water."@n
   else
    %send% %actor% You need to put away orbs not meant for this floor.
   end
  break
  case 3036
   if (%self.vnum% == 2907)
    %send% %actor% @w"I am something you make even though I don't exist myself. You shouldn't make me in haste either."@n
   elseif (%self.vnum% == 2908)
    %send% %actor% @w"I come after a question."@n
   elseif (%self.vnum% == 2909)
    %send% %actor% @w"I am a test, a game, or a jest. Put me together or take me apart. Depends on the me."@n
   else
    %send% %actor% You need to put away orbs not meant for this floor.
   end
  break
  case 3075
   if (%self.vnum% == 2910)
    %send% %actor% @w"A tool of magicians and con-artists."@n
   elseif (%self.vnum% == 2911)
    %send% %actor% @w"A group of ideas for accomplishing a task is what I am."@n
   elseif (%self.vnum% == 2912)
    %send% %actor% @w"I think but am merely the tool for thinking."@n
   else
    %send% %actor% You need to put away orbs not meant for this floor.
   end
  break
  case 3132
   if (%self.vnum% == 2913)
    %send% %actor% @w"By me the honorable live their life and by breaking me they leave their road."@n
   elseif (%self.vnum% == 2914)
    %send% %actor% @w"I am an opponent worthy of your fight. We face off not merely for survival."@n
   elseif (%self.vnum% == 2915)
    %send% %actor% @w"If your honor is ever lost, I am the feeling your spirit will feel spoken."@n
   else
    %send% %actor% You need to put away orbs not meant for this floor.
   end
  break
  case 3171
   if (%self.vnum% == 2916)
    %send% %actor% @w"I stand above all others who fight the fight, having won victory. What am I?"@n
   elseif (%self.vnum% == 2917)
    %send% %actor% @w"I am opposite of what you see when you look about thee. You may yet have to suffer me."@n
   elseif (%self.vnum% == 2918)
    %send% %actor% @w"You cheat death with each victory, but when you lose me that cycle ends. This is a certainty"@n
   else
    %send% %actor% You need to put away orbs not meant for this floor.
   end
  break
  default
   %send% %actor% Nothing happens.
  break
done
%echo% The light from %self.shortdesc% disappears.
wait 4 s
%send% %actor% Now @gguess@n your answer.
~
#2902
Riddle Say~
1 c 2
guess~
if (!%actor.varexists(pride)%)
 wait 1
 %send% %actor% You shouldn't have this, you shouldn't be in this tower.
 wait 2 s
 %send% %actor% You need to earn the right to traverse this tower at its entrance.
 wait 1
 %echo% %self.shortdesc% disintegrates and blows away.
 %purge% %self%
elseif (!%arg%)
 wait 1
 %send% %actor% Syntax: guess (text)
else
 switch (%actor.room.vnum%)
  case 2940
   if (%arg% /= hero && %self.vnum% == 2901)
    if (%actor.pride% == bravery || %actor.pride% == strength || %actor.pride% == wisdom || %actor.pride% == cunning || %actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride bravery
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   elseif (%arg% /= soldier && %self.vnum% == 2902)
    if (%actor.pride% == bravery || %actor.pride% == strength || %actor.pride% == wisdom || %actor.pride% == cunning || %actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride bravery
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   elseif (%arg% /= warrior && %self.vnum% == 2903)
    if (%actor.pride% == bravery || %actor.pride% == strength || %actor.pride% == wisdom || %actor.pride% == cunning || %actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride bravery
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   else
    wait 1
    %send% %actor% You feel that answer is incorrect.
   end
  break
  case 2979
   if (%arg% /= muscle && %self.vnum% == 2904)
    if (%actor.pride% == strength || %actor.pride% == wisdom || %actor.pride% == cunning || %actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride strength
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   elseif (%arg% /= gravity && %self.vnum% == 2905)
    if (%actor.pride% == strength || %actor.pride% == wisdom || %actor.pride% == cunning || %actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride strength
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   elseif (%arg% /= body && %self.vnum% == 2906)
    if (%actor.pride% == strength || %actor.pride% == wisdom || %actor.pride% == cunning || %actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride strength
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   else
    wait 1
    %send% %actor% You feel that answer is incorrect.
   end
  break
  case 3036
   if (%arg% /= choice && %self.vnum% == 2907)
    if (%actor.pride% == wisdom || %actor.pride% == cunning || %actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride wisdom
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   elseif (%arg% /= answer && %self.vnum% == 2908)
    if (%actor.pride% == wisdom || %actor.pride% == cunning || %actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride wisdom
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   elseif (%arg% /= puzzle && %self.vnum% == 2909)
    if (%actor.pride% == wisdom || %actor.pride% == cunning || %actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride wisdom
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   else
    wait 1
    %send% %actor% You feel that answer is incorrect.
   end
  break
  case 3075
   if (%arg% /= trick && %self.vnum% == 2910)
    if (%actor.pride% == cunning || %actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride cunning
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   elseif (%arg% /= plan && %self.vnum% == 2911)
    if (%actor.pride% == cunning || %actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride cunning
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   elseif (%arg% /= mind && %self.vnum% == 2912)
    if (%actor.pride% == cunning || %actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride cunning
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   else
    wait 1
    %send% %actor% You feel that answer is incorrect.
   end
  break
  case 3132
   if (%arg% /= code && %self.vnum% == 2913)
    if (%actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride honor
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   elseif (%arg% /= rival && %self.vnum% == 2914)
    if (%actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride honor
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   elseif (%arg% /= broken && %self.vnum% == 2915)
    if (%actor.pride% == honor || %actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now advance through the portal anytime you wish.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride honor
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   else
    wait 1
    %send% %actor% You feel that answer is incorrect.
   end
  break
  case 3171
   if (%arg% /= champion && %self.vnum% == 2916)
    if (%actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now enter the Sanctum of Pride.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride victory
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   elseif (%arg% /= defeat && %self.vnum% == 2917)
    if (%actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now enter the Sanctum of Pride.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride victory
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   elseif (%arg% /= life && %self.vnum% == 2918)
    if (%actor.pride% == victory)
     wait 1
     %send% %actor% You already passed this test.
     wait 1
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    else
     wait 1
     %send% %actor% You speak the answer in your heart.
     %echoaround% %actor% %actor.name% closes %actor.hisher% eyes and whispers something.
     wait 2 s
     %send% %actor% You have answered correctly, you may now enter the Sanctum of Pride.
     %echoaround% %actor% %actor.name% opens %actor.hisher% eyes and smiles.
     wait 1 s
     set pride victory
     remote pride %actor.id%
     %echo% %self.shortdesc% disintegrates and blows away.
     %purge% %self%
    end
   else
    wait 1
    %send% %actor% You feel that answer is incorrect.
   end
  break
  default
   wait 1
   %send% %actor% The orb remains cold here.
  break
 done
end
~
#2903
Guardian Death Trigger~
0 f 100
~
eval num %random.3%
switch (%self.vnum%)
 case 2906
  if (%num% == 1)
   %load% obj 2901
  elseif (%num% == 2)
   %load% obj 2902
  else
   %load% obj 2903
  end
 break
 case 2907
  if (%num% == 1)
   %load% obj 2904
  elseif (%num% == 2)
   %load% obj 2905
  else
   %load% obj 2906
  end
 break
 case 2908
  if (%num% == 1)
   %load% obj 2907
  elseif (%num% == 2)
   %load% obj 2908
  else
   %load% obj 2909
  end
 break
 case 2909
  if (%num% == 1)
   %load% obj 2910
  elseif (%num% == 2)
   %load% obj 2911
  else
   %load% obj 2912
  end
 break
 case 2910
  if (%num% == 1)
   %load% obj 2913
  elseif (%num% == 2)
   %load% obj 2914
  else
   %load% obj 2915
  end
 break
 case 2911
  if (%num% == 1)
   %load% obj 2916
  elseif (%num% == 2)
   %load% obj 2917
  else
   %load% obj 2918
  end
 break
done
~
#2904
Guardian Greet~
0 g 100
~
wait 1 s
%echo% A loud grinding noise comes from %self.name% as it begins moving.
wait 3 s
switch (%self.vnum%)
 case 2906
  say Overcome me to prove thy bravery!
 break
 case 2907
  say Overcome me to prove thy strength!
 break
 case 2908
  say Overcome me to prove thy wisdom!
 break
 case 2909
  say Overcome me to prove thy cunning!
 break
 case 2910
  say Overcome me to prove thy honor!
 break
 case 2911
  say Overcome me to attain thy victory!
 break
done
~
#2905
Keeper Room Trigger~
2 g 100
~
if (!%actor.varexists(pride)%)
 return 0
 wait 1
 %send% %actor% You are not supposed to be in this tower. You need to face the tower guardian...
 wait 1 s
 %teleport% %actor% 2899
 %force% %actor% look
elseif (%actor.pride% != victory)
 return 0
 wait 1
 %send% %actor% You haven't solved the puzzle for this floor yet.
 wait 2 s
 %send% %actor% Until you have you are not worthy to face the Keeper.
end
~
#2906
Keeper Greet Trigger~
0 g 100
~
if (!%actor.varexists(opride)%)
wait 1
say So you have managed to conquer the tower have you?
wait 2 s
say Congratulations. I now dub thee a Warrior of Pride.
wait 4 s
say The tower is your's to fully explore any time you wish.
wait 4 s
say Here is a token proving your pride as a warrior.
%load% obj 2919
give orb %actor.name%
set opride
remote opride %actor.id%
wait 3 s
say If you wish you can face me in combat.
wait 4 s
say You don't want that sort of humiliation though, now do you?
else
wait 1
say Ready to face me in combat?
wait 3 s
say Or are you scared?
wait 1
grin
end
~
#2907
Keeper Fight Trigger~
0 k 5
~
galik %actor.name%
~
#2908
Book of Pride Drop~
1 h 100
~
return 0
wait 1
%send% %actor% The book vaporizes as it touches the ground...
%echoaround% %actor% %actor.name% drops %self.shortdesc% and it vaporizes as it touches the ground.
%purge% %self%
~
#2909
Book of Pride Timer~
1 f 100
~
wait 1
%send% %self.carried_by% %self.shortdesc% disappears from existence.
%send% %self.carried_by% Only saiyans of high-blood can keep me...
%purge% %self%
~
#2910
Keeper Room Trigger~
2 g 100
~
if (!%actor.varexists(pride)%)
 return 0
 wait 1
 %send% %actor% You are not supposed to be in this tower. You need to face the tower guardian...
 wait 1 s
 %teleport% %actor% 2899
 %force% %actor% look
elseif (%actor.pride% != victory)
 return 0
 wait 1
 %send% %actor% You haven't solved the puzzle for this floor yet.
wait 1s
 %send% %actor% Until you have you are not worthy to face the Keeper.
end
~
$~
