#12300
Kai Captain informs~
0 g 100
~
if %actor.is_pc%
  wait 1 s
  emote grumbles.
  wait 2 s
  say What do you what? Can't you see I am relaxing here?!
  wait 3 s
  say Oh well. If you're looking to get past the barricade to the south...
  wait 3 s
  say You'd have to head over to the eastern section and speak with the Commander.
  wait 3 s
  say Now get lost!
end
~
#12301
Commander Astral talk trigger~
0 c 100
talk~
if %cmd% ==  talk
  if %arg% == %self.alias%
    %send% %actor% %self.name% says, 'What would you like to talk about?'
    %send% %actor% ------------------------------------
    %send% %actor% @ROption @Y1)@n Kaiju Forest
    %send% %actor% @ROption @Y2)@n Mission
    %send% %actor% @ROption @Y3)@n Password
    %send% %actor% @ROption @Y4)@n Advice
    %send% %actor% @ROption @Y5)@n About me
    %send% %actor% ------------------------------------
  elseif %arg% /= %self.alias% && %arg% /= Kaiju Forest
    %send% %actor% %self.name% says, 'This is where us Kais are born. It's currently a warzone though. Our Dark Brethen hold the south while we struggle to hold the eastern and western sections of the Forest.'
  elseif %arg% /= %self.alias% && %arg% /= Mission
    %send% %actor% %self.name% says, 'When we lost the Southern Section of the forest, the Makai gained control of a special Kaiju Tree. If you happen to find any golden fruit, bring it to me for a reward.'
  elseif %arg% /= %self.alias% && %arg% /= Password
    %send% %actor% %self.name% says, 'You actually want to go into the Southern Section? You are mad. Oh well, just say ascend to the guard at the Barricade and you will be let through.'
  elseif %arg% /= %self.alias% && %arg% /= Advice
    %send% %actor% %self.name% says, 'Turn around and head back where you came from. This is a war zone and we don't need people playing hero here.'
  elseif %arg% /= %self.alias% && %arg% /= About me
    %send% %actor% %self.name% says, 'I am %self.name%, in charge of the Kaiju Garrison.
  end
end
~
#12302
Kai Barricade Guard~
0 d 100
ascend~
wait 2 s
say Alright. Be careful in there!
unlock barricade
open barricade
wait 10 s
close barricade
lock barricade
~
#12303
Commander Astral receive~
0 j 100
~
if %object.vnum% == 12301
%purge% %object%
  wait 2 s
  say Excellent! I should probably tell you that only Kai born from these rare golden fruit go on to become Supreme Kais.
  wait 2 s
  say Let us hope you got to it before the Makai could corrupt it.
  wait 2 s
  %send% %actor% %self.name% hands you a pouch of zenni.
  %echoaround% %actor% %self.name% hands %actor.name% a pouch of zenni.
  nop %actor.gold(2000)%
else
  return 0
  wait 1 s
  say I don't want that that!
end
~
#12304
Kai Barricade Guard~
0 d 100
ascend~
wait 2 s
say Alright.
unlock barricade
open barricade
wait 10 s
close barricade
lock barricade
~
#12305
Makai Symbol drop~
0 f 100
~
if (%actor.varexists(onSenQ4)%)
    %load% obj 12302
end
~
$~
