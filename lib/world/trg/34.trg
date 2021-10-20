#3400
Chef Tells student about food~
0 g 100
~
if (!%actor.varexists(jintofood)%)
  wait 1
  say Oh, uh hi there. Are you another student of Jinto's? Well he has instructed me to give you guys some @Gfood@C, you need only ask.@n
else
  wait 1
  say Hello there buddy, hope your training is going well.
end
~
#3401
Acturis Newb Help~
0 g 100
~
if (%actor.level% <= 2 && %actor.class% == Jinto)
wait 1
say Hello there, %actor.name%. Master Jinto has instructed me
wait 3 sec
say To get you started you should probably head south, then west.
wait 3 sec
say You're looking for the Jaguar Dojo.
wait 3 sec
say If you have left Utatlan you've gone too far!
wait 4 sec
say Be careful %actor.name%.
end
~
#3402
Acturis Block~
0 q 100
~
if (%direction% == north && %actor.class% != Jinto)
  return 0
  wait 1
  say You are not one of Master Jinto's students. Please leave.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == north && %actor.class% == Jinto)
  wait 1
  say Welcome in friend.
  %send% %actor% Acturis says, 'Welcome in friend.'
  return 1
end
~
#3403
Jinto Greet~
0 g 100
~
if (%actor.level% <= 2)
wait 1
say Hello there %actor.name%, you are one of my newest students. If you need any help getting started speak with Acturis on your way out, he will direct you on where you can go.
 if !%actor.varexists(jintoeq)%
  wait 2 sec
  say Now %actor.name% here is the official symbol of my students.
wait 1 sec
  mload obj 3421
  give glyph %actor.name%
  set jintoeq
  remote jintoeq %actor.id%
 end
elseif (%actor.level% < 5)
wait 1
say %actor.name% you are still pretty new, make sure to do your best.
 if %actor.varexists(jintoeq)%
  rdelete jintoeq %actor.id%
end
elseif (%actor.level% < 10)
wait 1
say %actor.name% you are coming along great, keep at it.
elseif (%actor.level% < 20)
wait 1
say %actor.name% you are doing really well, you are one of my most dedicated students.
elseif (%actor.level% < 40)
wait 1
say %actor.name% you do me proud with your dedication. You are an exceptional student.
elseif (%actor.level% < 75)
wait 1
say %actor.name% you are one of my best students, make sure to be an example to the rest.
elseif (%actor.level% < 100)
wait 1
say %actor.name% you are nearly a master yourself, keep it up.
elseif (%actor.level% == 100)
wait 1
say %actor.name% what are you doing here still? Come to visit? I know you don't need any training, I have little to teach someone of your skill.
smile
else
wait 1
say %actor.name% I don't know what to say about you.
end
~
#3404
Chef gives food~
0 d 100
food~
if (!%actor.varexists(jintofood1)% && !%actor.varexists(jintofood)%)
  wait 1
  say Mkay, I'll get you something.
  wait 2 sec
  emote runs the kitchen, quickly prepairing things.
  wait 2 sec
  say Here you go.
  %load% obj 3416
  %load% obj 3416
  %load% obj 3416
  %load% obj 3416
  %load% obj 3417
  give all.carambola %actor.name%
  give all.Cacao %actor.name%
  set jintofood1
  remote jintofood1 %actor.id%
elseif (!%actor.varexists(jintofood2)% && !%actor.varexists(jintofood)%)
  wait 1
  say You want some more? Well I'm glad you like it!
  wait 2 sec
  emote chuckles to himself while gathering up some more food.
  wait 2 sec
  say Enjoy!
  %load% obj 3416
  %load% obj 3416
  %load% obj 3416
  %load% obj 3416
  %load% obj 3417
  give all.carambola %actor.name%
  give all.Cacao %actor.name%
  set jintofood2
  remote jintofood2 %actor.id%
elseif (!%actor.varexists(jintofood3)% && !%actor.varexists(jintofood)%)
  wait 1
  say Okay, I can give you some more, but no more after that!
  wait 2 sec
  emote gathers up some food.
  wait 2 sec
  say Enjoy!
  %load% obj 3416
  %load% obj 3416
  %load% obj 3416
  %load% obj 3416
  %load% obj 3417
  give all.carambola %actor.name%
  give all.Cacao %actor.name%
  rdelete jintofood1 %actor.id%
  rdelete jintofood2 %actor.id%
  set jintofood
  remote jintofood %actor.id%
else
  wait 1
  say Nope, I gave you enough. You need to go out and get your own from now on.
end
~
#3405
Chef Mute Food~
0 c 3
point~
if %arg% == food
    if (!%actor.varexists(jintofood1)% && !%actor.varexists(jintofood)%)
        wait 1
        say Mkay, I'll get you something.
        wait 2 sec
        emote runs the kitchen, quickly prepairing things.
        wait 2 sec
        say Here you go.
        %load% obj 3416
        %load% obj 3416
        %load% obj 3416
        %load% obj 3416
        %load% obj 3417
        give all.carambola %actor.name%
        give all.Cacao %actor.name%
        set jintofood1
        remote jintofood1 %actor.id%
    elseif (!%actor.varexists(jintofood2)% && !%actor.varexists(jintofood)%)
        wait 1
        say You want some more? Well I'm glad you like it!
        wait 2 sec
        emote chuckles to himself while gathering up some more food.
        wait 2 sec
        say Enjoy!
        %load% obj 3416
        %load% obj 3416
        %load% obj 3416
        %load% obj 3416
        %load% obj 3417
        give all.carambola %actor.name%
        give all.Cacao %actor.name%
        set jintofood2
        remote jintofood2 %actor.id%
    elseif (!%actor.varexists(jintofood3)% && !%actor.varexists(jintofood)%)
        wait 1
        say Okay, I can give you some more, but no more after that!
        wait 2 sec
        emote gathers up some food.
        wait 2 sec
        say Enjoy!
        %load% obj 3416
        %load% obj 3416
        %load% obj 3416
        %load% obj 3416
        %load% obj 3417
        give all.carambola %actor.name%
        give all.Cacao %actor.name%
        rdelete jintofood1 %actor.id%
        rdelete jintofood2 %actor.id%
        set jintofood
        remote jintofood %actor.id%
    else
        wait 1
        say Nope, I gave you enough. You need to go out and get your own from now on.
    end
else
    %send% %actor% Point at what?
end
~
#3417
Argus Greet~
0 g 100
~
if (!%actor.varexists(argustrain)%)
 wait 2 sec
 %echo% %self.name% looks up slowly from his scroll.
 wait 3 sec
 say Ah, hello there.
 wait 2 sec
 if (%actor.race% == Hoshijin)
  say Have you come here to train in the ancient Runic Arts?
 else
  say You're a stranger to this world aren't you?
  wait 2 sec
  say Even though it is usually frowned upon to offer lessons to non-Hoshijin I will make an exception, but only on one condition.
  wait 4 sec
  say Hmm? You do want lessons in Runic right? If you do then what I need you to do to 'earn' it is this...
  wait 1 sec
  %echo% %self.name% looks carefully from side to side.
  wait 5 sec
  say I need some... Wylseroot. I've heard that it can uh, cure certain issues concerning...sexual vigor.
  wait 6 sec
  say Now I don't want you repeating any of this to anyone. Keep that in mind... In any case I need ten Wylseroots.
  wait 4 sec
  say I believe you can find some on planet Yardrat. I've never been there myself though...
  wait 3 sec
  say Hmm? Oh yeah...
 end
 wait 2 sec
 say You do know about what the Runic Arts are, right?
 wait 3 sec
 %echo% %self.name% seems to be thinking deeply for a moment.
 wait 3 sec
 say Well no matter... 
 wait 5 sec
 say The Runic Arts are an ancient form of using the energies of the Eldritch to bestow enchantments upon those they are written to.
 wait 5 sec
 say It takes a special ink harvested from the Seraf plant. That plant has a special connection with the star.
else
 if (%actor.argustrain% == done)
  wait 1 sec
  greet %actor.name%
  say Are you wanting to train? Perhaps you would like to buy some supplies? Whichever you want friend.
 else
  wait 3 sec
  %echo% %self.name% looks up slowly from his scroll.
  wait 5 sec
  say If you want to learn Runic then you must bring me those samples of Wylseroot I asked you for.
  wait 3 sec
  eval count %actor.arguscount%
  say You still need to gather %count% Wylseroot.
 end
end
~
#3418
Argus receive~
0 j 100
~
if (%object.vnum% == 14103 && !%actor.varexists(argustrain)%)
 if (!%actor.varexists(arguscount)%)
  eval arguscount 10
  remote arguscount %actor.id%
 end
 wait 1
 %purge% %object%
 eval count %actor.arguscount% - 1
 eval arguscount %count%
 remote arguscount %actor.id%
 if (%actor.arguscount% > 0)
  say Thanks! Only %count% more needed.
 else
  say Thanks! That's all I needed. I'll now train you in Runic now if you wish.
  set argustrain done
  remote argustrain %actor.id%
 end
elseif (%actor.varexists(argustrain)%)
 return 0
 say I've already gotten enough from you.
else
 return 0
 say I don't want that!
end
~
#3419
Argus Train~
0 c 100
pr~
if (%actor.race% != Hoshijin && !%actor.varexists(argustrain)%)
 return 1
 wait 1
 say You need to bring me Wylseroot before I train you in Runic AT ALL!
else
 return 0
end
~
#3428
Topaz Teleport~
1 c 3
portal~
if (%actor.race% != Hoshijin)
 %send% %actor% You do not understand how to use this odd relic.
elseif (%actor.affect(ETHEREAL%)
%send% %actor% You cannot transcend planar boundaries with this!
elseif (%self.cost% >= 5000)
 if (%arg% == Earth)
  %send% %actor% You brush the crystal with your fingers while thinking about the planet Earth. Suddenly you are transported through time and space to Earth!
  %echoaround% %actor% There's a brief bright flash of golden light that fills the whole area!
  %teleport% %actor% 300
  %echoaround% %actor% %actor.name% appears in a bright flash of golden light!
  eval %self.cost(-1)%
 elseif (%arg% == Konack)
  %send% %actor% You brush the crystal with your fingers while thinking about the planet Konack. Suddenly you are transported through time and space to Konack!
  %echoaround% %actor% There's a brief bright flash of golden light that fills the whole area!
  %teleport% %actor% 8000
  %echoaround% %actor% %actor.name% appears in a bright flash of golden light!
  eval %self.cost(-1)%
 elseif (%arg% == Vegeta)
  %send% %actor% You brush the crystal with your fingers while thinking about the planet Vegeta. Suddenly you are transported through time and space to Vegeta!
  %echoaround% %actor% There's a brief bright flash of golden light that fills the whole area!
  %teleport% %actor% 2250
  %echoaround% %actor% %actor.name% appears in a bright flash of golden light!
  eval %self.cost(-1)%
 elseif (%arg% == Zenith)
  %send% %actor% You brush the crystal with your fingers while thinking about beautiful Zenith. Suddenly you are transported through time and space to Zenith!
  %echoaround% %actor% There's a brief bright flash of golden light that fills the whole area!
  %teleport% %actor% 3517
  %echoaround% %actor% %actor.name% appears in a bright flash of golden light!
  eval %self.cost(-1)%
 else
  %send% %actor% Syntax: portal [earth - vegeta - konack - zenith]
 end
else
 %send% %actor% You must wait for the Topaz to absorb more energy from the star!
end
~
#3429
Topaz 2~
1 t 0
~
if (%self.cost% < 5000)
 %echo% %self.name% Glows brightly for a moment before dimming back to normal.
 eval %self.cost(4)%
end
~
#3430
Topaz 3~
1 t 15
~
if (%self.cost% < 5000)
%echo% %self.name% Glows brightly for a moment before dimming back to normal.
 eval %self.cost(4)%
end
~
#3431
Topaz Check~
1 c 3
checktopaz~
if (%actor.race% == Hoshijin)
 if (%actor.varexists(selfalias)%)
   eval sa %actor.selfalias%
 else
   eval sa A Hoshijin
 end
 
 if (%self.cost% == 5004)
  %send% %actor% The Topaz is fully charged.
  %echoaround% %actor% %sa% studies the Topaz.
 elseif (%self.cost% == 5003)
  %send% %actor% The Topaz is almost full.
  %echoaround% %actor% %sa% studies the Topaz.
 elseif (%self.cost% == 5002)
  %send% %actor% The Topaz is half full.
  %echoaround% %actor% %sa% studies the Topaz.
 elseif (%self.cost% == 5001)
  %send% %actor% The Topaz is almost empty.
  %echoaround% %actor% %sa% studies the Topaz.
 else
  %send% %actor% The Topaz is empty.
  %echoaround% %actor% %sa% studies the Topaz.
 end
else
 %send% %actor% You do not understand how to use this odd relic.
end
~
$~
