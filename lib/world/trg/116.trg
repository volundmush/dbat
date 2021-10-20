#11600
Nail guard block~
0 q 100
~
if (%direction% == north && %actor.class% != Nail)
  return 0
  wait 1
  say You may not enter, you are not one of Nail's students.
* elseif (%actor.carry%)
*  return 0
*  wait 1
*  say You may not carry anyone inside.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == north && %actor.class% == Nail)
  wait 1
  say Peace be with you, friend.
  %send% %actor% Namekian Guard says, 'Peace be with you, friend.'
  return 1
end
~
#11601
Guru Food Greet~
0 g 100
~
if (!%actor.varexists(nailfood)%)
  wait 1
  say You must be a new student of Nail's... Yes, I can sense much potential in you. If you need of it, I can give you some @Gfood@C, you need only to ask.@n
else
  wait 1
  emote yells out, &1Naaaaail, the bird is back. Kick its ass.&2
end
~
#11602
Nail's greet~
0 g 100
~
if (%actor.level% <= 2)
  wait 1
  say You are following me now, %actor.name%. You're far too weak to be of any use... yet. There's a guard outside of the house, listen to him, he will tell you where you can go to become a warrior.
  if !%actor.varexists(naileq)%
    wait 2 sec
    say Now %actor.name% here is the official symbol of my students, wear it with pride even though you may not deserve it yet.
    wait 1 sec
    mload obj 11612
    give symbol %actor.name%
    set naileq
    remote naileq %actor.id%
    wait 2 s
    say If you need any food, you should head upstairs and see Guru, he'll help you out.
  end
elseif (%actor.level% < 5)
  wait 1
  say %actor.name%, I see you are not as weak now, but you are not nearly strong enough. You better stay dedicated to your training.
  if %actor.varexists(naileq)%
    rdelete naileq %actor.id%
  end
elseif (%actor.level% < 10)
  wait 1
  say Your skills still need training %actor.name%, though you have promise.
  elseif (%actor.level% < 20)
  wait 1
  say %actor.name% I have a small bit of pride for you, but there's still along way to go before I'm impressed.
  elseif (%actor.level% < 40)
  wait 1
  say %actor.name% Hmmph, you are quite strong now %actor.name%. I'm proud.
  elseif (%actor.level% < 75)
  wait 1
  say %actor.name% you are one of my strongest students, never cease your training.
  elseif (%actor.level% < 100)
  wait 1
  say %actor.name% you are nearly beyond my abilities to teach. I hate to admit it...
  elseif (%actor.level% == 100)
  wait 1
say %actor.name% what are you doing here still? You have surpassed the need to the train with me. But be viligent, there is always someone out there stronger than you.
  %send% %actor% Nail crosses his arms and nods his head solemnly at you.
  %echoaround% %actor% Nail crosses his arms and nods his head solemly at %actor.name%.
  else
  wait 1
  say %actor.name% I don't know what to say about you except LEAVE!
end
~
#11603
Nail guard greet~
0 g 100
~
if (%actor.level% <= 2 && %actor.class% == Nail)
wait 1 s
say Hello there, %actor.name%. I can see you've already spoken with Nail..
wait 3 s
say To get started, you should probably fly down this outcrop.  Then continue flying due south until you reach a village called Senzu Village.
wait 3 s
say There is a large dome there, known as the Namekian Battle Dome. It is a good place to train for someone your size.
wait 3
say Be careful %actor.name%.
end
~
#11604
Guru Food Give~
0 d 100
food~
if (!%actor.varexists(nailfood1)% && !%actor.varexists(nailfood)%)
  wait 1
  say Very well.
  wait 2 sec
  emote holds out his hand and creates some food.
  wait 2 sec
  say Take it. There is something special for you too.
  %load% obj 11611
  %load% obj 11611
  %load% obj 11611
  %load% obj 11611
  %load% obj 1614
  %load% obj 11607
  give lantern %actor.name%
  give all.tofu %actor.name%
  give all.cup %actor.name%
  set nailfood1
  remote nailfood1 %actor.id%
elseif (!%actor.varexists(nailfood2)% && !%actor.varexists(nailfood)%)
  wait 1 sec
  say You need some more? Very well.
  wait 2 sec
  emote holds out his hand and creates some food.
  wait 2 sec
  say Please, take it.
  %load% obj 11611
  %load% obj 11611
  %load% obj 11611
  %load% obj 11611
  %load% obj 1614
  give all.tofu %actor.name%
  give all.cup %actor.name%
  set nailfood2
  remote nailfood2 %actor.id%
elseif (!%actor.varexists(nailfood3)% && !%actor.varexists(nailfood)%)
  wait 1 sec
  say Very well, Super Kami Guru shall grant this request one final time!
  wait 2 sec
  emote holds out his hand and creates some food.
  wait 1 sec
  say Take it, and begone!
  %load% obj 11611
  %load% obj 11611
  %load% obj 11611
  %load% obj 11611
  %load% obj 1614
  give all.tofu %actor.name%
  give all.cup %actor.name%
  rdelete nailfood1 %actor.id%
  rdelete nailfood2 %actor.id%
  set nailfood
  remote nailfood %actor.id%
else
  wait 1 sec
  say I'm tired. I will not give you any more.
end
~
#11605
Guru Mute Food~
0 c 3
point~
if %arg% == food
    if (!%actor.varexists(nailfood1)% && !%actor.varexists(nailfood)%)
        wait 1
        say Very well.
        wait 2 sec
        emote holds out his hand and creates some food.
        wait 2 sec
        say Take it. There is something special for you too.
        %load% obj 11611
        %load% obj 11611
        %load% obj 11611
        %load% obj 11611
        %load% obj 1614
        %load% obj 11607
        give lantern %actor.name%
        give all.tofu %actor.name%
        give all.cup %actor.name%
        set nailfood1
        remote nailfood1 %actor.id%
    elseif (!%actor.varexists(nailfood2)% && !%actor.varexists(nailfood)%)
        wait 1 sec
        say You need some more? Very well.
        wait 2 sec
        emote holds out his hand and creates some food.
        wait 2 sec
        say Please, take it.
        %load% obj 11611
        %load% obj 11611
        %load% obj 11611
        %load% obj 11611
        %load% obj 1614
        give all.tofu %actor.name%
        give all.cup %actor.name%
        set nailfood2
        remote nailfood2 %actor.id%
    elseif (!%actor.varexists(nailfood3)% && !%actor.varexists(nailfood)%)
        wait 1 sec
        say Very well, Super Kami Guru shall grant this request one final time!
        wait 2 sec
        emote holds out his hand and creates some food.
        wait 1 sec
        say Take it, and begone!
        %load% obj 11611
        %load% obj 11611
        %load% obj 11611
        %load% obj 11611
        %load% obj 1614
        give all.tofu %actor.name%
        give all.cup %actor.name%
        rdelete nailfood1 %actor.id%
        rdelete nailfood2 %actor.id%
        set nailfood
        remote nailfood %actor.id%
    else
        wait 1 sec
        say I'm tired. I will not give you any more.
    end
else
    %send% %actor% Point at what?
end
~
#11608
Nail Guard Block 2~
0 q 100
~
if (%direction% == southeast && %actor.class% != Nail)
  return 0
  wait 1
  say You may not enter, you are not one of Nail's students.
* elseif (%actor.carry%)
*  return 0
*  wait 1
*  say You may not carry anyone inside.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == southeast && %actor.class% == Nail)
  wait 1
  say Peace be with you, friend.
  %send% %actor% Namekian Guard says, 'Peace be with you, friend.'
  return 1
end
~
#11609
WTO Peon Respawn~
0 f 100
~
%load% mob 11601
return 0
~
#11610
WTO Grunt respawn~
0 f 100
~
%load% mob 11602
return 0
~
#11611
WTO Cadet Respawn~
0 f 100
~
%load% mob 11603
Return 0
~
#11612
WTO Officer Respawn~
0 f 100
~
%load% mob 11604
return 0
~
$~
