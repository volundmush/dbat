#8230
Minosha Greet~
0 g 100
~
if (%actor.level% <= 2 && %actor.class% == Tapion)
  wait 1
  say Hello there, %actor.name%. Welcome to the Music Box. I know you have already met my brother Tapion.
  wait 3 sec
  say To get you started you should probably head over to Ferios park.
  wait 3 sec
  say From here, go west onto Taloc Street, then follow it to the Northwest. Ferios Park is on either side of the street.
  wait 3 sec
  say Tapion's newest students often train there.
  wait 4 sec
  say Be careful %actor.name%.
end
~
#8231
Tapion Greet~
0 g 100
~
if (%actor.level% <= 2)
wait 1
say Hello there %actor.name%, you are one of my newest students. If you need any help, on the way out, my brother Minosha will assist you.
 if !%actor.varexists(tapioneq)%
  wait 2 sec
  say Now %actor.name% here is the official symbol of my students.
wait 1 sec
  mload obj 8230
  give symbol %actor.name%
  set tapioneq
  remote tapioneq %actor.id%
 end
elseif (%actor.level% < 5)
wait 1
say %actor.name% you are still pretty new, make sure to do your best. Konack may have to depend on you one day.
 if %actor.varexists(tapioneq)%
  rdelete tapioneq %actor.id%
end
elseif (%actor.level% < 10)
wait 1
say %actor.name% you're doing alright but don't falter now in your training.
elseif (%actor.level% < 20)
wait 1
say %actor.name% you are doing well, you are suprisingly dedicated.
elseif (%actor.level% < 40)
wait 1
say %actor.name% you do me proud with your dedication. You are close to becoming an exceptional warrior.
elseif (%actor.level% < 75)
wait 1
say %actor.name% you are one of the best warriors I have taught, I hope you use your skills well.
elseif (%actor.level% < 100)
wait 1
say %actor.name%, soon Konack's defense may rest on your shoulders if you continue to train like you have been!
elseif (%actor.level% == 100)
wait 1
say %actor.name% what are you doing here still? Come to visit? I know you don't need any training, you have surpassed my teachings.
smile
else
wait 1
say %actor.name% I don't know what to say about you.
end
~
#8232
Servant food greet~
0 g 100
~
if (!%actor.varexists(tapionfood)%)
    wait 1
    say Hello there. Are you training with Master Tapion? He has instructed me to give you some @Gfood@C upon request.@n
else
   wait 1
   say Hello there, I hope your training goes well.
end
~
#8233
Minosha Guard~
0 q 100
~
if (%direction% == inside && %actor.class% != Tapion)
 return 0
 wait 1
 say You may not enter, you are not one of Tapion's pupils.
* elseif (%actor.carry%)
* return 0
* wait 1
* say You may not carry anyone inside.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == inside && %actor.class% == Tapion)
wait 1
 say Tapion is expecting you. Remember, honor the blade.
%send% %actor% Minosha says, 'Tapion is expecting you. Remember, honor the blade.'
return 1
end
~
#8234
Servant Food Give~
0 d 100
food~
if (!%actor.varexists(tapionfood1)% && !%actor.varexists(tapionfood)%)
  wait 1
  say Right away.
  wait 2 sec
  emote runs the kitchen, quickly grabbing a few things.
  wait 2 sec
  say Here you go, these are the finest Rice Cakes and Oolong Tea on Konack.
  %load% obj 1101
  %load% obj 1101
  %load% obj 1101
  %load% obj 1101
  %load% obj 8146
  give all.cake %actor.name%
  give all.oolong %actor.name%
  set tapionfood1
  remote tapionfood1 %actor.id%
elseif (!%actor.varexists(tapionfood2)% && !%actor.varexists(tapionfood)%)
  wait 1
  say Right away.
  wait 2 sec
  emote runs around the kitchen, quickly grabbing a few things.
  wait 2 sec
  say I hope you enjoy the cakes and tea. I just made the tea fresh.
  %load% obj 1101
  %load% obj 1101
  %load% obj 1101
  %load% obj 1101
  %load% obj 8146
  give all.cake %actor.name%
  give all.oolong %actor.name%
  set tapionfood2
  remote tapionfood2 %actor.id%
elseif (!%actor.varexists(tapionfood3)% && !%actor.varexists(tapionfood)%)
  wait 1
  say Right away. And just so you know, this is the last time I can give you this.
  wait 2 sec
  emote runs around the kitchen, quickly grabbing a few things.
  wait 2 sec
  say I hope you understand.
  %load% obj 1101
  %load% obj 1101
  %load% obj 1101
  %load% obj 1101
  %load% obj 8146
  give all.cake %actor.name%
  give all.oolong %actor.name%
  rdelete tapionfood1 %actor.id%
  rdelete tapionfood2 %actor.id%
  set tapionfood
  remote tapionfood %actor.id%
else
  wait 1
  say I'm sorry, but I can't give you more.
end
~
#8235
Servant Mute Food~
0 c 3
point~
if %arg% == food
    if (!%actor.varexists(tapionfood1)% && !%actor.varexists(tapionfood)%)
        wait 1
        say Right away.
        wait 2 sec
        emote runs the kitchen, quickly grabbing a few things.
        wait 2 sec
        say Here you go, these are the finest Rice Cakes and Oolong Tea on Konack.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 8146
        give all.cake %actor.name%
        give all.oolong %actor.name%
        set tapionfood1
        remote tapionfood1 %actor.id%
    elseif (!%actor.varexists(tapionfood2)% && !%actor.varexists(tapionfood)%)
        wait 1
        say Right away.
        wait 2 sec
        emote runs around the kitchen, quickly grabbing a few things.
        wait 2 sec
        say I hope you enjoy the cakes and tea. I just made the tea fresh.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 8146
        give all.cake %actor.name%
        give all.oolong %actor.name%
        set tapionfood2
        remote tapionfood2 %actor.id%
    elseif (!%actor.varexists(tapionfood3)% && !%actor.varexists(tapionfood)%)
        wait 1
        say Right away. And just so you know, this is the last time I can give you this.
        wait 2 sec
        emote runs around the kitchen, quickly grabbing a few things.
        wait 2 sec
        say I hope you understand.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 8146
        give all.cake %actor.name%
        give all.oolong %actor.name%
        rdelete tapionfood1 %actor.id%
        rdelete tapionfood2 %actor.id%
        set tapionfood
        remote tapionfood %actor.id%
    else
        wait 1
        say I'm sorry, but I can't give you more.
    end
else
    %send% %actor% Point at what?
end
~
$~
