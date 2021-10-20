#6100
King Kai Greet~
0 g 100
~
if (%actor.varexists(superbuu)%)
  wait 1 s
  say I sense you have trained with someone extremely evil.
  wait 2 s
  say I will not train you, leave.
elseif (%actor.varexists(ElwinTrain)%)
  wait 1 s
  say I sense an incredibly dark aura around you.  I don't trust my training with someone like you.
elseif (%actor.varexists(kktrain)%)
 if (%actor.kktrain% == done)
  wait 1 s
  say Ah hello %actor.name%.
  wait 2 s
  say Ready for some more training?
 elseif (%actor.kktrain% == bubbles)
  wait 1 s
  say You haven't caught Bubbles yet have you?
  wait 2 s
  say I am not training you until you complete the two tests.
  wait 4 s
  say Catching Bubbles is first. So get to it.
 else
  wait 1 s
  say You still have to hit Gregory with the hammer.
  if (!%self.room.contents(7005)% && !%actor.inventory(7005)%)
   wait 1 s
   %load% obj 7005
   %echo% %self.name% holds out his hand and a hammer appears in it.
   drop hammer
  end
 end
elseif (%actor.align% < -49)
 wait 1 s
 say What are you doing here? I won't train someone with your ki. It doesn't feel right.
elseif (%actor.level% < 30)
 wait 1 s
 say If you are here for training I don't think you are quite ready yet...
 wait 3 s
 %send% %actor% You feel you need to be level 30 for King Kai to accept you.
else
 wait 1 s
 say Ah hello.
 wait 2 s
 say Are you here to receive some of my training?
 wait 3 s
 say I don't just train anyone you know, first you got to pass two tests.
 wait 4 s
 say The first test is you have to catch Bubbles the monkey.
 wait 3 s
 say He should be wandering around here somewhere.
 set kktrain bubbles
 remote kktrain %actor.id%
end
~
#6101
King Kai Command~
0 c 100
pr~
if (%actor.varexists(superbuu)%)
 return 1
 wait 1 s
 say I do not train those who associate with evil techniques.
elseif (%actor.varexists(ElwinTrain)%)
 return 1
 wait 1 s
 say I do not train those with such dark auras.
elseif (!%actor.varexists(kktrain)%)
 return 1
 wait 1 s
 say I don't train anyone who hasn't passed my tests.
else
 if (%actor.kktrain% == done)
  return 0
 elseif (%actor.kktrain% == bubbles)
  return 1
  wait 1
  say %actor.name% You still have to catch Bubbles!
  wait 2 s
  say No training until you pass my tests.
 else
  return 1
  wait 1
  say %actor.name% You still have to hit Gregory!
  wait 2 s
  say No training until you pass that test.
 end
end
~
#6102
Bubbles Catch~
0 c 100
catch~
if (%actor.varexists(kktrain)%)
 if (%actor.kktrain% != bubbles)
  wait 1
  %send% %actor% You already caught Bubbles before.
 elseif (%actor.move% < %actor.maxmove% / 20)
  wait 1
  %send% %actor% You are too tired to chase after Bubbles at the moment..
 else
  wait 1 s
  %send% %actor% You run up and try to catch Bubbles.
  %echoaround% %actor% %actor.name% runs up and tries to catch Bubbles.
  wait 2 s
  eval num %random.12%
  switch (%num%)
   case 1
    eval minus %actor.maxmove% / 20
    eval equa -%minus%
    set %actor.move(%equa%)%
    %send% %actor% You manage to catch Bubbles!
    %echoaround% %actor% %actor.name% manages to catch Bubbles!
    set kktrain gregory
    remote kktrain %actor.id%
    wait 2 s
    %teleport% %actor% 6115
    %force% %actor% look
    wait 1 s
    %at% %actor.room.vnum% %force% king say Well done %actor.name%
    wait 2 s
    %at% %actor.room.vnum% %force% king say Now the next test is you need to @ghit@C Gregory with a hammer.@n
    wait 3 s
    %at% %actor.room.vnum% %force% king say This hammer to be exact.
    wait 1
%at% %actor.room.vnum% %force% king mload obj 7005
%at% %actor.room.vnum% %force% king drop hammer
   break
   case 2
    eval minus %actor.maxmove% / 20
    eval equa -%minus%
    set %actor.move(%equa%)%
    %send% %actor% You chase Bubbles but slip in a wet part of the grass!
    %echoaround% %actor% %actor.name% chases Bubbles but slips in a wet part of the grass!
    wait 1 s
    %send% %actor% Try again...
   break
   default
    eval minus %actor.maxmove% / 20
    eval equa -%minus%
    set %actor.move(%equa%)%
    %send% %actor% You chase Bubbles but he leaps between your legs and causes you to fall over!
    %echoaround% %actor% %actor.name% chases Bubbles but he leaps between %actor.hisher% legs and causes %actor.himher% to fall over!
    wait 1 s
    %send% %actor% Try again...
   break
  done
 end  
else
 wait 1
 %send% %actor% You need to speak with King Kai first.
end
~
#6103
Gregory Hit~
1 c 3
hit~
set target_char %actor.room.people%
while (%target_char%)
 set tmp_target %target_char.next_in_room%
 if (!%target_char.is_pc% && %target_char.vnum% == 6102)
  set greg %target_char%
 end
 set target_char %tmp_target%
done
if (%actor.varexists(kktrain)%)
 if (%actor.kktrain% == done)
  wait 1
  %send% %actor% You have already completed this test.
  wait 1
  %send% %actor% The hammer disappears.
  %purge% %self%
 elseif (%actor.kktrain% == bubbles)
  wait 1
  %send% %actor% You still need to @gcatch@n Bubbles.
 elseif (!%greg%)
  wait 1
  %send% %actor% Gregory isn't even in this room.
 elseif (%actor.move% < %actor.maxmove% / 20)
  wait 1
  %send% %actor% You are too tired to try to hit Gregory at the moment.
 else
  eval num %random.12%
  wait 1
  %send% %actor% You heft the hammer and with a running start swing it at Gregory's head!
  %echoaround% %actor% %actor.name% hefts a large hammer and with a running start swings it at Gregory's head!
  wait 2 s
  switch (%num%)
   case 1
    eval minus %actor.maxmove% / 20
    eval equa -%minus%
    set %actor.move(%equa%)%
    %send% %actor% You manage to hit Gregory squarely on the head.
    %echoaround% %actor% %actor.name% manages to hit Gregory!
    wait 1 s
    %send% %actor% You have passed King Kai's tests. You may now train with him.
    set kktrain done
    remote kktrain %actor.id%
    %purge% %self%
   break
   case 2
    eval minus %actor.maxmove% / 20
    eval equa -%minus%
    set %actor.move(%equa%)%
    %send% %actor% You miss badly and Gregory taunts you for it!
    %echoaround% %actor% %actor.name% misses badly and Gregory taunts %actor.himher% for it!
   break
   default
    eval minus %actor.maxmove% / 20
    eval equa -%minus%
    set %actor.move(%equa%)%
%send% %actor% You swing but Gregory speeds past too fast for you to hit him!
    %echoaround% %actor% %actor.name%'s hammer swings but Gregory speeds past to fast for %actor.himher% to land the hit!
   break
  done
 end
else
 wait 1
 %send% %actor% You need to speak with King Kai first.
end
~
#6104
King Kai Command~
0 c 100
pr~
if (%actor.varexists(superbuu)%)
 return 1
elseif (%actor.varexists(ElwinTrain)%)
 return 1
elseif (!%actor.varexists(kktrain)%)
 return 1
else
 if (%actor.kktrain% == done)
  return 0
 else
  return 1
 end
end
~
#6140
snake servant asks~
0 g 100
~
if %actor.is_pc%
say @CHi %actor.name%, would you like something to eat?@n
end
~
#6141
servant yes~
0 d 100
yes~
if (!%actor.varexists(snakefood)%)
 say Very Well %actor.name%.
    wait 1 s
 mload obj 6165
 give rice %actor.name%
 wait 1 sec
 smile %actor.name%
 say Hope you enjoy your food
 set snakefood 1
 remote snakefood %actor.id%
else
 if (%actor.snakefood% >= 10)
  wait 1
  say You have had your share already. Don't be greedy.
  wait 2 s
  say Tee hee.
 else
  say Very Well %actor.name%.
     wait 1 s
  mload obj 6165
  give rice %actor.name%
  wait 1 sec
  smile %actor.name%
  say Hope you enjoy your food
  eval num %actor.snakefood% + 1
  set snakefood %num%
  remote snakefood %actor.id%
 end
end
~
#6142
Princess Snake Greet~
0 g 100
~
if (!%actor.varexists(yrevive)%)
 if (%actor.sex% == MALE)
  wait 1 s
  say Why hello there handsome.
  wait 3 s
  say What brings you to my castle?
  wait 3 s
  say It isn't little ole me is it?
  wait 1 s
  %send% %actor% %self.name% winks at you.
  %echoaround% %actor% %self.name% winks at %actor.name%.
 elseif (%actor.sex% == FEMALE)
  wait 1 s
  say Hmph what brings you here?
  wait 3 s
  say I have no interest in women and I don't need any friends.
  wait 4 s
  say State your business or leave.
 else
  wait 1 s
  say What exactly are you?
  wait 3 s
  say No matter. What is it you want?
 end
else
 wait 1 s
 say We already did business.
 wait 2 s
 say Leave before I change my mind.
end
~
#6143
Princess Snake Speech~
0 d 100
trade~
if (!%actor.varexists(yrevive)%)
 wait 1 s
 say Trade? The only thing I have to trade is this golden teacup.
 wait 3 s
 say Well I suppose I could trade it. I really don't like it.
 wait 4 s
 say I just have held onto it to piss off that old kai.
 wait 4 s
 say There is a rare herb that grows on King Kai's planet.
 wait 4 s
 say It is one of my favorite ingredients for food.
 wait 4 s
 say You will need a shovel to @gdig@C it up though, I think Goz down in Hell has one.@n
 wait 4 s
 say Give me the herb and I will give you the teacup.
else
 wait 1 s
 say We already traded.
 wait 2 s
 say You should get out of here before I change my mind about eating you.
end
~
#6144
Princess Snake Receive~
0 j 100
~
if (!%actor.varexists(yrevive)%)
 if (%object.vnum% == 7003)
  wait 1
  %purge% %object%
  say Ah perfect.
  wait 2 s
  say Alright here you go.
  %load% obj 7002
  give teacup %actor.name%
  wait 3 s
  say On top of that instead of eating you like I planned I will let you go.
  wait 4 s
  say If you leave soon...
 else
  return 0
  wait 1
  say I don't want that...
 end
else
 return 0
 wait 1
 say I already traded with you.
 wait 2 s
 say You better leave before I change my mind...
end
~
#6164
snake servant greet~
0 g 100
~
%echo% This trigger commandlist is not complete!
~
$~
