#17900
Santa's Loot Table~
0 f 10
~
eval drop %random.9%
switch %drop%
  case 1
    %load% obj 17900
    break
  case 2
    %load% obj 17901
    break
  case 3
    %load% obj 17902
    break
  case 4
    %load% obj 17903
    break
  case 5
    %load% obj 17904
    break
  case 6
    %load% obj 17905
    break
  case 7
    %load% obj 17906
    break
  case 8
   %load% obj 17918
   break
 default
   %load% obj 17919
   break
done
~
#17901
Elf Miner Loot Table~
0 f 15
~
eval drop %random.9%
switch %drop%
  case 1
    %load% obj 17907
    break
  case 2
    %load% obj 17908
    break
  case 3
    %load% obj 17909
    break
  case 4
    %load% obj 17910
    break
  case 5
    %load% obj 17911
    break
  case 6
    %load% obj 17913
    break
  case 7
    %load% obj 17915
    break
  case 8
   %load% obj 17914
   break
 default
   %load% obj 17922
   break
done
~
#17902
Elf Worker Loot Table~
0 f 20
~
eval drop %random.7%
switch %drop%
  case 1
    %load% obj 17907
    break
  case 2
    %load% obj 17908
    break
  case 3
    %load% obj 17909
    break
  case 4
    %load% obj 17910
    break
  case 5
    %load% obj 17911
    break
  case 6
    %load% obj 17913
    break
 default
   %load% obj 17922
   break
done
~
#17903
Reindeer Loot Table~
0 f 10
~
eval drop %random.3%
switch %drop%
  case 1
    %load% obj 17916
    break
  case 2
    %load% obj 17913
    break
 default
   %load% obj 17914
   break
done
~
#17904
Snow Wolf & Bear Loot Table~
0 f 10
~
eval drop %random.3%
switch %drop%
  case 1
    %load% obj 17922
    break
  case 2
    %load% obj 17917
    break
 default
   %load% obj 17915
   break
done
~
#17905
Alan teleports player to Northran~
0 d 100
*~
if %speech% == help
  wait 2 s
  say Great!
  wait 3 s
  say I have escaped from a place called Northran.
  wait 3 s
  say The master of the Northran Star is Santa Claws but he has been away for a while delivering presents across the galaxy.
  wait 4 s
  say The evil Jack Frost has taken control of the Northran Star and is using it for his own nefarious schemes!
  wait 4 s
  say My friend Susie will be waiting on the other side to tell you what to do.
  wait 2 s
  %send% %actor% Alan utters a chant and a wave of energy envelops you.
  %echoaround% %actor% utters a chant causing a wave of energy envelop %actor.name%.
  wait 2 s
  %send% %actor% You are whisked away in the blinding light, before arriving at the Northran Star!
  %echoaround% %actor.name% disappears in a flash of bright energy.
  %teleport% %actor.name% 17901
  %at% 17901 %force% %actor.name% south
else
  wait 2 s
say You need to say help, if you wish to help me.
end
~
#17906
Susie Greets~
0 g 100
~
if (!%actor.varexists(fedsusie)%)
 wait 2 s
 %send% %actor% Susie says, 'Great! Alan found someone to help us!'
 wait 2 s
 %send% %actor% Susie says, 'This is the Northran Star and it's master is Santa Claws. But since he is out delivering  presents..'
 wait 2 s
 %send% %actor% Susie says, 'Our arch nemesis Jack Frost invaded and took over Santa's workshop. He's been forcing us Elves to work in the mines.'
 wait 2 s
 %send% %actor% Susie says, 'You're not going to be able to get to him without my help.. and I'm ever so hungry.'
 wait 2 s
 %send% %actor% Susie says, 'Jack Frost has paid the wolves and bears in the nearby area with choc chip cookies.'
 wait 2 s
 %send% %actor% Susie says, 'Bring me back four and I'll tell you what to do next.'
 wait 2 s
 %send% %actor% Susie says, 'Oh and if you want to leave Northran just say leave and I'll return you to Alan.'
elseif (%actor.varexists(meltedfrost)%)
 if (%actor.meltedfrost% != 2)
  wait 2 s
  %send% %actor% Susie says, 'You managed to defeat Jack Frost!?'
  wait 2 s
  %send% %actor% Susie says, 'Well I promised you a reward so here it is.'
  wait 1
  %load% obj 17933
  drop tree
  %force% %actor% get tree
  set meltedfrost 2
  remote meltedfrost %actor.id%
 else
  wait 2 s
  %send% %actor% Susie says, 'If you want to leave Northran just say leave and I'll return you to Alan.'
 end
else
 wait 2 s
 %send% %actor% Susie says, 'If you want to leave Northran just say leave and I'll return you to Alan.'
end
~
#17907
Susie Teleports~
0 d 100
*~
if %speech% == leave
  wait 2 s
  say That's a shame.
  wait 2 s
  emote sighs
  wait 2 s
  say I'll send you back to where Alan is then I suppose.
  wait 2 s
  %send% %actor% Susie utters a chant and a wave of energy envelops you.
  %echoaround% %actor% utters a chant causing a wave of energy envelop %actor.name%.
  wait 2 s
  %send% %actor% You are whisked away in the blinding light, before arriving in Nexus City!
  %echoaround% %actor.name% disappears in a flash of bright energy.
  %teleport% %actor.name% 300
  %at% 300 %force% %actor.name% look
else
  wait 2 s
  say You need to say leave, if you wish to leave..
end
~
#17908
Alan greet~
0 g 100
~
wait 2 s
say Hey! You look like a strong warrior... I need help.
wait 3 s
say If you @Gagree@C, say "help" and I will tell you about the situation.
~
#17909
Wishbone Trig~
0 n 100
~
Load obj 17927
~
#17910
Santa Claws teleport trigger~
0 ab 5
~
shout Ho, Ho, Ho and a merry christmas to all!
%echo% Santa Claws vanishes in mid air!
eval santaport %random.7%
switch %santaport%
case 1
%teleport% %self% 1544
shout Ho, Ho, Ho!
break
case 2
%teleport% %self% 3293
shout Ho, Ho, Ho!
break
case 3
%teleport% %self% 5578
shout Ho, Ho, Ho!
break
case 4
%teleport% %self% 9216
shout Ho, Ho, Ho!
break
case 5
%teleport% %self% 11901
shout Ho, Ho, Ho!
break
case 6
%teleport% %self% 12293
shout Ho, Ho, Ho!
break
default
%teleport% %self% 13552
shout Ho, Ho, Ho!
break
done
~
#17911
Santa Fight~
0 k 10
~
eval sanattack %random.4%
switch %sanattack%
case 1
kienzan
break
case 2
deathball
break
case 3
kousengan
break
default
solar
break
done
~
#17912
Present trigger~
1 c 3
unwrap~
If (%arg% == present)
%send% %actor% You unwrap the present!
eval gift %random.4%
switch %gift%
case 1
%send% %actor% You receive a replica of Santa's Sack!
%load% obj 17900
%force% %actor% get sack
%purge% %self%
break
case 2
%send% %actor% You receive an RPP token!
%load% obj 17935
%force% %actor% get token
%purge% %self%
break
case 3
%send% %actor% You receive a Christmas Tree stud!
%load% obj 17936
%force% %actor% get stud
%purge% %self%
break
default
%send% %actor% You receive an Artifact Token!
%load% obj 17898
%force% %actor% get token
%purge% %self%
break
done
else
%send% %actor% You need to type @Yunwrap present@n
end
~
#17913
Susie Stuffs Her Face~
0 j 100
~
if (%object.vnum% == 17918)
 if (!%actor.varexists(fedsusie)%)
  wait 1
  %purge% %object%
  say Thank you for the delicious cookie!
  wait 2 sec
  say I'm afraid this won't be enough to fill me up though, could I have another?
  set fedsusie 1
  remote fedsusie %actor.id%
 else
  if (%actor.fedsusie% < 3)
   wait 1
   %purge% %object%
   say Thank you for this cookie too!
   wait 2 sec
   say I'm afraid this still won't be enough to fill me up though, could I have another?
   eval num %actor.fedsusie% + 1
   set fedsusie %num%
   remote fedsusie %actor.id%
  elseif (%actor.fedsusie% == 3)
   wait 1
   %purge% %object%
   say Thank you for all the cookies!
   wait 2 sec
   say I can't wait to eat these cookies, but I think I will need something to wash them down with.
   wait 3 sec
   say Can you bring me some Eggnog? I hear that one of Jack Frost's servants has some.
   set fedsusie 4
   remote fedsusie %actor.id%
  else
   return 0
   wait 1
   say I already have all the cookies I want from you silly.
  end
 end
elseif (%object.vnum% == 17917)
 if (!%actor.varexists(fedsusie)%)
  return 0
  wait 1
  say I want some chocolate chip cookies to eat first.
 elseif (%actor.fedsusie% < 4)
  return 0
  wait 1
  say You haven't given me enough cookies to eat yet.
 else
  if (!%actor.varexists(eggnog)%)
   wait 1
   %purge% %object%
   say Oh thank you! Now I can eat my cookies and wash them down with this delicious eggnog.
   wait 2 sec
   %echo% %self.name% munches on the cookies and sips her eggnog. Eventually she finishes both off.
   wait 2 sec
   say Ahh. That was delicious. Well as thanks here is a key to the Northran Mine.
   wait 1
   %load% obj 17921
   drop key
   %force% %actor% get key
   set eggnog
   remote eggnog %actor.id%
   wait 1 sec
   say Oh. If you bring me some coal and a candy cane I can make you a Workshop key as well.
  else
   return 0
   wait 1
   say I've already had cookies and eggnog from you!
  end
 end
elseif (%object.vnum% == 17915)
 if (!%actor.varexists(susiecoal)% && !%actor.varexists(susiecane)%)
  wait 1
  %purge% %object%
  say Thank you for this coal. If you bring me a candy cane now I will make you a key.
  set susiecoal
  remote susiecoal %actor.id%
 elseif (!%actor.varexists(susiecoal)%)
  wait 1
  %purge% %object%
  say Thank you for this coal and the candy cane you gave me earlier.
  wait 2 sec
  say Here I will make you a key for the workshop out of them.
  wait 2 sec
  %echo% After a few moments Susie completes her work.
  wait 1
  %load% obj 17920
  drop key
  %force% %actor% get key
  set susiecoal
  remote susiecoal %actor.id%
  wait 2 sec
  say By the way. It seems Jack Frost has taken over Santa's workshop in his absence.
  wait 2 sec
  say If you defeat him and return to me you shall receive a reward.
 elseif (!%actor.varexists(susiecane)%)
  return 0
  wait 1
  say You already gave me coal. Bring me a candy cane now and I will make you a key.
 else
  return 0
  wait 1
  say I already made you a workshop key. I'm not making you another one.
 end
elseif (%object.vnum% == 17916)
 if (!%actor.varexists(susiecoal)% && !%actor.varexists(susiecane)%)
  wait 1
  %purge% %object%
  say Thank you for this candy cane. If you bring me some coal now I will make you a key.
  set susiecane
  remote susiecane %actor.id%
 elseif (!%actor.varexists(susiecane)%)
  wait 1
  %purge% %object%
  say Thank you for this candy cane and the coal you gave me earlier.
  wait 2 sec
  say Here I will make you a key for the workshop out of them.
  wait 2 sec
  %echo% After a few moments Susie completes her work.
  wait 1
  %load% obj 17920
  drop key
  %force% %actor% get key
  set susiecane
  remote susiecane %actor.id%
  wait 2 sec
  say By the way. It seems Jack Frost has taken over Santa's workshop in his absence.
  wait 2 sec
  say If you defeat him and return to me you shall receive a reward.
 elseif (!%actor.varexists(susiecoal)%)
  return 0
  wait 1
  say You already gave me a candy cane. Bring me some coal now and I will make you a key.
 else
  return 0
  wait 1
  say I already made you a workshop key. I'm not making you another one.
 end
else
 return 0
 %send% %actor% I do not want that.
end
~
#17914
Melted Frost~
0 f 100
~
if (!%actor.varexists(meltedfrost)%)
 set meltedfrost 1
 remote meltedfrost %actor.id%
end
~
#17950
Cadbury greets~
0 g 100
~
if %actor.is_pc%
wait 2 sec
say Hi there, %actor.name%. I was robbed by the powerful Rocky Road gang.
wait 2 sec
say Maybe if you're strong enough, you can bring them to justice for me? They should still be around Earth somewhere.
end
~
#17951
Rocky Fight~
0 k 35
~
eval tactics %random.4%
switch %tactics%
  case 1
    wait 1 sec
    deathball
    break
  case 2
    wait 1 sec
    kousengan
    break
  case 3
    wait 1 sec
    big bang
   break
default
    %echo% Rocky focuses his ki and heals some of his combat damage!
    nop %self.hitp(10000000000)%
  break
done
~
#17952
Rocky Greet~
0 g 100
~
wait 2 sec
say Weakling, I have stolen from the Cadbury Bunny what nobody else could, his Mystical Golden Egg is MINE!
wait 2 sec
say Try and take it if you can.
~
#17995
Alan Teleport Mutes~
0 c 100
agree~
  wait 2 s
say Great!
wait 3 s
say I have escaped from a place called Northran.
wait 3 s
say The master of the Northran Star is Santa Claws but he has been away for a while delivering presents across the galaxy.
wait 4 s
say The evil Jack Frost has taken control of the Northran Star and is using it for his own nefarious schemes!
wait 4 s
say My friend Susie will be waiting on the other side to tell you what to do.
wait 2 s
%send% %actor% Alan utters a chant and a wave of energy envelops you.
%echoaround% %actor% utters a chant causing a wave of energy envelop %actor.name%.
wait 2 s
%send% %actor% You are whisked away in the blinding light, before arriving at the Northran Star!
%echoaround% %actor.name% disappears in a flash of bright energy.
%teleport% %actor.name% 17901
%at% 17901 %force% %actor.name% south
end
~
#17997
Susie Teleport Mutes~
0 c 100
leave~
wait 2 s
say That's a shame.
wait 2 s
emote sighs
wait 2 s
say I'll send you back to where Alan is then I suppose.
wait 2 s
%send% %actor% Susie utters a chant and a wave of energy envelops you.
%echoaround% %actor% utters a chant causing a wave of energy envelop %actor.name%.
wait 2 s
%send% %actor% You are whisked away in the blinding light, before arriving in Nexus City!
%echoaround% %actor.name% disappears in a flash of bright energy.
%teleport% %actor.name% 300
%at% 300 %force% %actor.name% look
~
$~
