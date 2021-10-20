#16100
Kurzak Greet~
0 g 100
~
if (%actor.level% <= 2)
wait 1
say Hello there %actor.name%, looks like you are one of our newest recruits. If you need any help, Hitok can provide you with assistance.
 if !%actor.varexists(kurzakeq)%
  wait 2 sec
  say Now %actor.name% here is the official symbol of my gladiators.
wait 1 sec
  mload obj 16100
  give symbol %actor.name%
  set kurzakeq
  remote kurzakeq %actor.id%
 end
elseif (%actor.level% < 5)
wait 1
say %actor.name% you are still pretty new, be sure to watch your back. Never know who is spying for King Moai.
 if %actor.varexists(kurzakeq)%
  rdelete kurzakeq %actor.id%
end
elseif (%actor.level% < 10)
wait 1
say %actor.name% you have made some good progress, well done.
elseif (%actor.level% < 20)
wait 1
say %actor.name% you are doing really well, you have the makings of a true champion.
elseif (%actor.level% < 40)
wait 1
say %actor.name% you do me proud with the progress you have made. Few warriors achieve this level of competency.
elseif (%actor.level% < 75)
wait 1
say %actor.name% you are one of the finest warriors I have ever trained. But don't lose focus.
elseif (%actor.level% < 100)
wait 1
say %actor.name% you will have soon learnt everything I could possibly teach you.
elseif (%actor.level% == 100)
wait 1
say %actor.name% what are you doing here still? Come to visit? I know you don't need any training, I have little to teach someone of your skill.
else
wait 1
say %actor.name% I don't know what to say about you.
end
~
#16101
Hitok newb help~
0 g 100
~
if (%actor.level% <= 2 && %actor.class% == Kurzak)
wait 1
say Hello there, %actor.name%. Welcome to the pits.
wait 3 sec
say Head into prison block a and cut your teeth there. The guards tend to throw the weak ones in our direction.
wait 3 sec
say And pay attention to what I'm about to say. If you want out of this hell hole, you are going to have to do some things
wait 3 sec
say Specifically beating Granir and Oct in single combat. The guards will recognize you as a man of potential and let you out.
wait 4 sec
say Be careful %actor.name%.
end
~
#16102
Hitok guard~
0 q 100
~
if (%direction% == east && %actor.class% != Kurzak)
 return 0
 wait 1
 say You may not enter, you are not permitted to see Kurzak.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == east && %actor.class% == Kurzak)
wait 1
 say Welcome, friend.
%send% %actor% Hitok says, 'Welcome, friend.'
return 1
end
~
#16103
Roti Food Greet~
0 g 100
~
if (!%actor.varexists(kurzakfood)%)
    wait 1
    say Hello. Are you another... 'Prodigy' taken in by Kurzak? Well no matter, I have a supply of @Gfood@C I've been hiding from the guards. I'd be willing to part with some if you ask, in return for a favor at some point.@n
else
   wait 1
   emote nods his head at %actor.name%, and resumes his cleaning.
end
~
#16104
Granir defeated~
0 f 100
~
if !%actor.varexists(granirdefeated)%
set granirdefeated
remote granirdefeated %actor.id%
end
~
#16105
oct defeated~
0 f 100
~
if !%actor.varexists(octdefeated)%
set octdefeated
remote octdefeated %actor.id%
end
~
#16106
First Guard~
0 q 100
~
if (%direction% == west && !%actor.varexists(granirdefeated)%)
return 0
wait 1 s
say Beat Granir, you worm and I might let you pass.
elseif (%direction% == west && %actor.varexists(granirdefeated)%)
return 1
end
~
#16107
Second guard~
0 q 100
~
if (%direction% == north && !%actor.varexists(octdefeated)%)
return 0
wait 1 s
say Show me you're worth more than trash by beating Oct and I'll let you pass.
elseif (%direction% == north && %actor.varexists(octdefeated)%)
return 1
end
~
#16108
Roti Food Give~
0 d 100
food~
if (!%actor.varexists(kurzakfood1)% && !%actor.varexists(kurzakfood)%)
    wait 1
    say Very well.
    wait 2 sec
    emote pulls out a loose bit from the wall and grabs some food.
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1614
    give all.cake %actor.name%
    give all.cup %actor.name%
    set kurzakfood1
    remote kurzakfood1 %actor.id%
elseif (!%actor.varexists(kurzakfood2)% && !%actor.varexists(kurzakfood)%)
    wait 1
    say You're going to eat all the food I have hidden...
    wait 2 sec
    emote pulls out a loose bit from the wall and grabs some food.
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1614
    give all.cake %actor.name%
    give all.cup %actor.name%
    set kurzakfood2
    remote kurzakfood2 %actor.id%
elseif (!%actor.varexists(kurzakfood3)% && !%actor.varexists(kurzakfood)%)
    wait 1
    say Well, you're taking all my food are you? You better do this favor of mine when I ask...
    wait 2 sec
emote pulls out a loose bit from the wall and grabs some food.
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1614
    give all.cake %actor.name%
    give all.cup %actor.name%
    rdelete kurzakfood1 %actor.id%
    rdelete kurzakfood2 %actor.id%
    set kurzakfood
    remote kurzakfood %actor.id%
else
    wait 1 sec
    say No, you took all the food I had.
end
~
$~
