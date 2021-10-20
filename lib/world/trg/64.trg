#6400
Dabura's Palace Guard~
0 q 100
~
if (%direction% == northeast && %actor.class% != Dabura)
 return 0
 wait 1
 say You may not enter, you are not one of Dabura's pupils.
* elseif (%actor.carry%)
* return 0
* wait 1
* say You may not carry anyone inside.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == northeast && %actor.class% == Dabura)
wait 1
 say Don't keep the Demon King Waiting..
%send% %actor% Palace Guard says, 'Don't keep the Demon King waiting...'
return 1
end
~
#6401
Dabura Palace guard greet~
0 g 100
~
if (%actor.level% <= 2 && %actor.class% == Dabura)
  wait 1
  say Hello there, %actor.name%. Welcome to Hell.
  wait 3 sec
  say The best place for you is the torture rack up on Lust.
  wait 3 sec
  say From here, just follow the streets winding up the cliff face until you come to the level of Lust. In the middle is the torture rack.
  wait 3 sec
  say Dabura's newest students often get the taste for their first kill there.
  wait 4 sec
  say Watch your step %actor.name%.
end
~
#6402
Dabura's Palace Servant Food Greet~
0 g 100
~
if (!%actor.varexists(daburafood)%)
    wait 1
    say Are you a new apprentice of the Master? The Master has instructed me to give out some @Gfood@C to the new apprentices. Just ask, and this one will get it for you.@n
else
   wait 1
   say This one hopes your training is going well, young Master.
end
~
#6403
Dabura greet~
0 g 100
~
if (%actor.level% <= 2)
wait 1
say Hello there %actor.name%, you are one of my newest apprentices. As you leave, the Palace Guard will point you to a training ground
 if !%actor.varexists(daburaeq)%
  wait 2 sec
  say Now %actor.name% here is the official mark of my apprentices.
wait 1 sec
  mload obj 6400
  give mark %actor.name%
  set daburaeq
  remote daburaeq %actor.id%
 end
elseif (%actor.level% < 5)
wait 1
say %actor.name% you are still fresh meat. Get stronger fast, Hell has a tendacy to swallow weaklings up.
 if %actor.varexists(daburaeq)%
  rdelete daburaeq %actor.id%
end
elseif (%actor.level% < 10)
wait 1
say %actor.name% you're getting stronger. But if you don't keep up your training, I won't hestitate to throw you in the Pit.
elseif (%actor.level% < 20)
wait 1
say Hmmph, %actor.name% you are doing well, don't falter now.
elseif (%actor.level% < 40)
wait 1
say %actor.name% you are quite dedicated to getting stronger. You may be one of the best apprentices I have had yet...
elseif (%actor.level% < 75)
wait 1
say %actor.name% you are fast becoming one of the best warriors in hell.
elseif (%actor.level% < 100)
wait 1
say %actor.name%, you've gotten quite strong. Almost too strong.. don't go getting any ideas now, I'm the king of Hell!
elseif (%actor.level% == 100)
wait 1
say %actor.name% what are you doing here still? I've nothing left to teach you. Go forth and spread destruction.
smile
else
wait 1
say %actor.name% I don't know what to say about you.
end
~
#6404
Portal to Earth trigger~
2 c 100
portal~
if (%actor.race% == demon)
  if (!%actor.dead%)
   if (%arg% == earth || %arg% == Earth)
    wait 1 s
    %send% %actor% The portal shifts and Earth can be seen on the other side. You quickly step through and are transported there!
    %force% %actor% drag
    %echoaround% %actor% The portal shifts and Earth can be seen on the other side. %actor.name% quickly steps through and is transported there!
    %teleport% %actor% 1596
    %echoaround% %actor% %actor.name% steps out of the portal.
    %at% %actor% %force% %actor% look
   elseif (%arg% == frigid || %arg% == Frigid)
    if (%actor.level% >= 15)
     wait 1 s
     %send% %actor% The portal shifts and Frigid can be seen on the other side. You quickly step through and are transported there!
     %force% %actor% drag
     %echoaround% %actor% The portal shifts and Frigid can be seen on the other side. %actor.name% quickly steps through and is transported there!
     %teleport% %actor% 5050
     %echoaround% %actor% %actor.name% steps out of the portal.
     %at% %actor% %force% %actor% look
    else
     return 1
     %send% %actor% You need to be at least level 15 or higher to use the portal to Frigid!
    end
   elseif (%arg% == aether || %arg% == Aether)
    if (%actor.level% >= 15)
     wait 1 s
     %send% %actor% The portal shifts and Aether can be seen on the other side. You quickly step through and are transported there!
     %force% %actor% drag
     %echoaround% %actor% The portal shifts and Aether can be seen on the other side. %actor.name% quickly steps through and is transported there!
     %teleport% %actor% 12424
     %echoaround% %actor% %actor.name% steps out of the portal.
     %at% %actor% %force% %actor% look
    else
     return 1
     %send% %actor% You need to be at least level 15 or higher to use the portal to Aether!
    end
   else
    return 1
    %send% %actor% Try using portal <destination>. Valid destinations are Earth, Frigid, and Aether.
   end
   elseif (%actor.dead%)
    return 1
    %send% %actor% You can't cross through the portal as you are dead...
   end
  else
    return 1
    %send% %actor% Only students of Dabura can enter this portal!
  end
~
#6405
Don't allow them sw~
2 q 100
~
if (%direction% == southwest && !%actor.affect(ETHEREAL)%)
return 0
%send% %actor% You are not dead and can't venture further. Use the portal...
end
~
#6406
Only Certains People May Pass!~
2 g 100
~
if (%actor.align% > 49 && %actor.race% != demon)
 return 0
 wait 1
 %send% %actor% You are too good to step into that wretched place!
elseif (%actor.drag%)
 return 0
 wait 1
 %send% %actor% You cannot drag anyone beyond this point.
end
~
#6407
Soul Slave Food Give~
0 d 100
food~
if (!%actor.varexists(daburafood1)% && !%actor.varexists(daburafood)%)
    wait 1 sec
    say This one will get food.
    wait 3 sec
    emote quickly scampers off into a side room and returns moments later with some food.
    wait 3 sec
    say This one hopes he served you well.
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1905
    give all.cake %actor.name%
    give all.cup %actor.name%
    set daburafood1
    remote daburafood1 %actor.id%
elseif (!%actor.varexists(daburafood2)% && !%actor.varexists(daburafood)%)
    wait 1 sec
    say This one shall get you some more food.
    wait 2 sec
    emote quickly scampers off into a side room and returns moments later with some food.
    wait 2 sec
    say This one hopes he is serving you well, young Master.
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1905
    give all.cake %actor.name%
    give all.cup %actor.name%
    set daburafood2
    remote daburafood2 %actor.id%
elseif (!%actor.varexists(daburafood3)% && !%actor.varexists(daburafood)%)
    wait 1 sec
    say This one shall get you some more food, but the Master has told this one to give no more after this one.
    wait 2 sec
    emote quickly scampers off into a side room and returns moments later with some food.
    wait 1 sec
    say Please forgive this one.
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1101
    %load% obj 1905
    give all.cake %actor.name%
    give all.cup %actor.name%
    rdelete daburafood1 %actor.id%
    rdelete daburafood2 %actor.id%
    set daburafood
    remote daburafood %actor.id%
else
    wait 1 sec
    say This one is sincerly sorry, young Master. But Master Dabura doesn't allow me to give out more.
end
~
#6408
Soul Slave Mute Food~
0 c 3
point~
if %arg% == food
    if (!%actor.varexists(daburafood1)% && !%actor.varexists(daburafood)%)
        wait 1 sec
        say This one will get food.
        wait 3 sec
        emote quickly scampers off into a side room and returns moments later with some food.
        wait 3 sec
        say This one hopes he served you well.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1905
        give all.cake %actor.name%
        give all.cup %actor.name%
        set daburafood1
        remote daburafood1 %actor.id%
    elseif (!%actor.varexists(daburafood2)% && !%actor.varexists(daburafood)%)
        wait 1 sec
        say This one shall get you some more food.
        wait 2 sec
        emote quickly scampers off into a side room and returns moments later with some food.
        wait 2 sec
        say This one hopes he is serving you well, young Master.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1905
        give all.cake %actor.name%
        give all.cup %actor.name%
        set daburafood2
        remote daburafood2 %actor.id%
    elseif (!%actor.varexists(daburafood3)% && !%actor.varexists(daburafood)%)
        wait 1 sec
        say This one shall get you some more food, but the Master has told this one to give no more after this one.
        wait 2 sec
        emote quickly scampers off into a side room and returns moments later with some food.
        wait 1 sec
        say Please forgive this one.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1905
        give all.cake %actor.name%
        give all.cup %actor.name%
        rdelete daburafood1 %actor.id%
        rdelete daburafood2 %actor.id%
        set daburafood
        remote daburafood %actor.id%
    else
        wait 1 sec
        say This one is sincerly sorry, young Master. But Master Dabura doesn't allow me to give out more.
    end
else
    %send% %actor% Point at what?
end
~
#6410
Asur - Lust Greeting~
0 g 100
~
@C**************************************************@n
@C*@W Asur quest for Lusty lust things@n
@C*@W @n
@C**************************************************@n
if (%actor.varexists(HellLust)%)
  set HellLust %actor.HellLust%
else
  set HellLust Intro
end
remote HellLust %actor.id%
if (%HellLust% == Intro)
  wait 1s
  emote licks her lips as she looks up.  @Y'@RAhhhh.  What a sweet, sweet soul.  Have you come to pay for your sins of @rLust@R?@Y'@n
end
~
#6411
Asur - Yes Speech~
0 d 100
yes~
@C**************************************************@n
@C*@W Asur quest for Lusty lust things@n
@C*@W @n
@C**************************************************@n
if (%actor.varexists(HellLust)%)
  set HellLust %actor.HellLust%
else
  set HellLust default
end
if (%HellLust% == nothing)
  wait 3s
  emote bats her eyelashes seductively.
elseif (%HellLust% == Intro)
  wait 1s
  emote chuckles, her chest heaving and bouncing more than is strictly necessary for the action.  @Y'@ROh my... are you sure about that?@Y'@n
  set HellLust IntroConfirm
  remote HellLust %actor.id%
elseif (%HellLust% == IntroConfirm)
  set HellLust Punishing
  remote HellLust %actor.id%
  wait 1s
  smote With a seductive grin,   slides her hand along your shoulder and guides you down onto a torture rack.  @Y'@RNow, honey... you just relax while Momma takes good care of you...@Y'@n
  wait 3s
  emote cuffs *%actor.name% to a torture table and takes a whip from a holster on the side of the table.
  wait 2s
  emote smiles as she cracks the whip against her hand.  @Y'@RDon't worry... this will only hurt... a lot.@Y'@n
  wait 4s
  %echo% @RCRACK!@n
  wait 2s
  %echo% @RCRACK!@n
  wait 1s
  %echo% @RCRACK!@n
  wait 1s
  emote smiles as she brings her whip down on *%actor.name% hard, leaving nasty cuts with each swing.
  wait 3s
  emote uncuffs *%actor.name%.  @Y'@RThere we are.  Now, come on back if you acquire more @rsin@R.@Y'@n
  rdelete HellLust %actor.id%
else
end
~
#6412
Asur - "NO" Answer~
0 d 100
No~
@C**************************************************@n
@C*@W Asur quest for Lusty lust things@n
@C*@W @n
@C**************************************************@n
if (%actor.varexists(HellLust)%)
  set HellLust %actor.HellLust%
else
  set HellLust default
end
if (%HellLust% == nothing)
  wait 3s
  emote bats her eyelashes seductively.
elseif (%HellLust% == Intro)
  wait 1s
  emote sighs and runs her fingers enticingly over a torture rack.  @Y'@RThat's too bad... you look like a screamer.  I @rlike@R screamers.@Y'@n
elseif (%HellLust% == IntroConfirm)
  wait 1s
  emote rolls her eyes.  @Y'@RYou are terrible.  I should spank you for lying to me... but I don't spank for free.@Y'@n
  wait 3s
  emote winks at *%actor.name%.
  set HellLust Intro
  remote HellLust %actor.id%
else
end
~
$~
