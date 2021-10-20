#18600
Saiyan mob points~
0 c 100
squash~
%send% %actor% @RYou squash it for a bonus point!@n
%echoaround% %actor% @R%actor.name% does something for a bonus point!  WHAT WAS IT?!@n
if (!%actor.varexists(halloweeny)%)
  set halloweeny 1
  remote halloweeny %actor.id%
else
  set halloweeny 0
  eval halloweeny %actor.halloweeny% + 1
  remote halloweeny %actor.id%
end
%purge% self
wait 2s
~
#18601
halloweeny item randoms~
1 j 100
~
set halloweenychance %random.10%
wait 1
  switch %halloweenychance%
    case 1
      wait 5s
      %send% %actor% A hockey mask pops into existence over your face and a machete grows out of your hand.  You walk slowly, but somehow manage to keep up with whoever you chase!
      %echoaround% %actor% A hockey mask pops into existance over %actor.name%'s face, and a machete grows out of %actor.hisher% hand.  No matter how slow %actor.heshe% walks, %actor.heshe% can't seem to be outrun! 
      break
    case 2
      wait 5s
      %send% %actor% the Halloweeny twists up and slaps you in the face!
      %echoaround% %actor% The Halloweeny in %actor.name%'s curves up and slaps %actor.himher% in the face!
      break
    case 3
      wait 5s
      %echo% Sedalis appears out of nowhere and steals part of your soul!
      break
    case 4
      wait 5s
      %send% %actor% Your clothing seems to change its fit on you as your gender is changed!
      %echoaround% %actor.name% looks very weird in %actor.hisher% clothing.   Hm... %actor.hisher% gender has changed!
      wait 4 s
      %send% %actor% After what seems like forever, your body reverts to its previous gender.
      %echoaround% %actor% %actor.name% breathes a sigh of relief as their body returns to normal.
      break
    case 5
      wait 5s
      %send% %actor% You are locked for eternity in a closet with Zarbon.  SHIT!
      %echoaround% %actor% %actor.name% screams suddenly, thrusting %actor.hisher% hands out.  @C'No, not Zarbon!!'@n
      break
    default
      %send% %actor% As you wield the Halloweeny, you feel a cold, dark shadow surround you.
      %echoaround% %actor% A cold, dark shadow surrounds %actor.name% as they wield the %self.name%.
      break
end
~
#18602
Lister Greet~
0 g 100
~
wait 2s
emote smiles, spreading his arms wide.  @G'Welcome to Lister's!  This is the finest resort in the galaxy!'@n  Lister lowers his arms to the podium.  @G'Do you have a @Creservation@G?'@n
~
#18603
Lister Random~
0 b 5
~
say I farted!
~
#18604
Waitress 1~
0 d 100
COMMERE~
n
~
#18605
Waitress Controller~
2 c 100
garcon~
*FOOD PRICES!
set wineprice 500
set foodprice 1250
set danceprice 1750
set lapdanceprice 4000
set tipprice 500
set testfind %findmob.18669(18605)%
if (%actor.varexists(listertab)%)
  set listertabnow %actor.listertab%
else
  set listertabnow 0
end
if (%actor.varexists(listertip)%)
  set listertip %actor.listertip%
else
  set listertip 0
end
if (%actor.varexists(listerfavor)%)
  set listerfavor1 %actor.listerfavor%
  set listerfavor %actor.listerfavor%
else
  set listerfavor1 0
  set listerfavor 0
end
if (%actor.varexists(listerupset)%)
  set listerupset1 %actor.listerupset%
  set listerupset %actor.listerupset%
else
  set listerupset1 0
  set listerupset 0
end
eval listermood %listerfavor1% - %listerupset1%
if (%actor.room.vnum% == 18668)
  set actress waitress1
  set directionto north
  set directionfrom south
  if (%findmob.18648(18605)%)
    set actstate default
  elseif (%findmob.18668(18605)%)
    set actstate here
  else
    set actstate busy
  end
elseif (%actor.room.vnum% == 18669)
  set actress waitress1
  set directionto south
  set directionfrom north
  if (%findmob.18648(18605)%)
    set actstate default
  elseif (%findmob.18669(18605)%)
    set actstate here
  else
    set actstate busy
  end
elseif (%actor.room.vnum% == 18667)
  set actress waitress2
  set directionto north
  set directionfrom south
  if (%findmob.18650(18606)%)
    set actstate default
  elseif (%findmob.18667(18606)%)
    set actstate here
  else
    set actstate busy
  end
elseif (%actor.room.vnum% == 18666)
  set actress waitress2
  set directionto south
  set directionfrom north
  if (%findmob.18650(18606)%)
    set actstate default
  elseif (%findmob.18666(18606)%)
    set actstate here
  else
    set actstate busy
  end
end
if (%arg% /= call server )
  if (%actstate% == default)
    wait 1s
    %echoaround% %actor% %actor.name% raises %actor.hisher% hand and gets the attention of the server.
    %send% %actor% You raise your hand to get the attention of the server.
    wait 4s
    %at% %actress% %force% %actress% %directionto% 
    wait 2s
    %force% %actress% emote walks in, her hair bouncing behind her.  She smiles at *%actor.name% and pulls a pen out of a pocket on her uniform.  @M'How ya'll doin?  Everything good so far?'@n  He pauses but a second and then winks.  @M'I'm sure glad ya'll decided to drop in today!  What can I get for ya?'@n 
  elseif (%actstate% == here)
    wait 1s
    %echoaround% %actor% %actor.name% raises %actor.hisher% hand and gets the attention of the server.
    %send% %actor% You raise your hand to get the attention of the server.
    wait 4s
    %force% %actress% emote looks a little confused, but reaches out to give *%actor.name% a high five.
  else
    wait 5s
    %send% %actor% @RIt appears the waitress is busy at the moment.@n
    %echoaround% %actor% @RIt appears the waitress is busy at the moment.@n
  end
elseif (%arg% /= order drink )
  wait 1s
  if (%actstate% != here)
    %send% %actor% @RUm... perhaps you should @YCall Server@R first.@n
  else
    eval listertab %listertabnow% + %wineprice%
    remote listertab %actor.id%
    wait 2s
    %force% %actress% emote jots something down on her notepad.  Looking back up, she smiles at *%actor.name%.  @M'Got it.  I'll bring along a bottle of our finest wine for ya.  Give me just a sec, darlin'.'@n  She winks as she turns and heads off to get the wine.
    %force% %actress% %directionfrom%
    wait 60s
    %at% %actress% %force% %actress% %directionto%
    wait 2s
    %force% %actress% emote returns with a lonely bottle of wine on a large tray.  She takes the wine off the fancy tray and sets it down on the table.  @M'There ya go, Sweetie.  Is there anything else I can get ya, or do you need a minute to think about it?'@n
*    if (%actor.age% < 21)
*     %load% obj 69
*    else
      %load% obj 18605
*    end
  end
elseif (%arg% /= order food )
  wait 1s
  if (%actstate% != here)
    %send% %actor% @RUm... perhaps you should @YCall Server@R first.@n
  else
    eval listertab %listertabnow% + %foodprice%
    remote listertab %actor.id%
    wait 2s
    %force% %actress% emote jots something down on her notepad.  Looking back up, she smiles at *%actor.name%.  @M'What an excellent choice, sugar.  Give me just a sec, darlin'.'@n  She winks as she turns and heads off to get the food.
    %force% %actress% %directionfrom%
    wait 60s
    %at% %actress% %force% %actress% %directionto%
    wait 2s
    %force% %actress% emote returns with a delicious smelling plate of food on a large tray.  She takes the plate off the fancy tray and sets it down on the table.  @M'There ya go, Sweetie.  Is there anything else I can get ya, or do you need a minute to think about it?'@n
    %load% obj 18606
  end
elseif (%arg% /= order dance )
  wait 1s
  if (%actstate% == busy || %actstate% == default)
    %send% %actor% @RUm... perhaps you should @YCall Server@R first.@n
  else
    eval listertab %listertabnow% + %danceprice%
    remote listertab %actor.id%
    wait 2s
    %force% %actress% emote smiles at *%actor.name%.  @M'You want to see me dance, honey?'@n  With a smile, she climbs up onto the table and begins clapping her hands in a rhythm.@n
    wait 4s
    %force% %actress% emote begins tapping her feet on the table, rapping out a staccato beat that drives the movements of her body.  Her hips sway from side to side in time with the tapping, and her hands slither like snakes above her head.@n
    wait 4s
    %force% %actress% emote stomps on the table hard three times, ending her dance.  She hops down and giggles daintily into her hand.  @M'Oh that was fun.  I hope you enjoyed it, babe.  Is there anything else I can get for you or do for you?'@n  She winks, but the innocence of the look on her face deflates any possible perverted meaning that may have been suggested by a normal person.@n
  end
elseif (%arg% /= dismiss )
  wait 1s
  if (%actstate% == busy || %actstate% == default)
    %send% %actor% @RUm... perhaps you should @YCall Server@R first.@n
  else
    wait 2s
    %force% %actress% emote smiles and smiles flirtatiously at *%actor.name%.  @M'Okie dokie, dear.  If you need me, just call me again.'@n  She turns gracefully and leaves the room once again.@n
    wait 1s
    %force% %actress% %directionfrom%
  end
elseif (%arg% /= check tab )
  wait 1s
  if (%actstate% != here)
    %send% %actor% @RUm... perhaps you should @YCall Server@R first.@n
  else
    if (%listertabnow% > 0)
      %force% %actress% emote looks at her notepad and does a few calculations.  @M'Well, sugar, it looks like your tab currently comes to %listertabnow%.'@n
    else
      %force% %actress% emote giggles into her hand daintily and shakes her head.  @M'You have to order somethin' before I charge you, dear.'@n
    end
  end
elseif (%arg% /= pay )
  wait 1s
  if (%actstate% != here)
    %send% %actor% @RUm... perhaps you should @YCall Server@R first.@n
  else
    if (%listertabnow% > 0)
      if (%actor.gold% >= %listertabnow%)
        nop %actor.gold(-%listertabnow%)
        %force% %actress% emote accepts some money from %actor.name%.  @M'Thanks, honey.  I'll take this %listertabnow% to the register and pay for you.'@n
        set listertab 0
        remote listertab %actor.id%
        %force% %actress% %directionfrom%
        eval listerfavornow %listertip% / %tipprice%
        %echo% @RTest1: %listerfavornow%@n
        if (%listerfavornow% > 0)
          eval listerfavor %listerfavor1% + 1
        else
          eval listerupset %listerupset1% - 3
        end
        %echo% @RTest2: %listerfavor% and %listerfavornow%
        remote listerfavor %actor.id%
        remote listerupset %actor.id%
        set listertip 0
        remote listertip %actor.id%
      else
        %force% %actress% emote looks at her notepad and shakes her head.  @M'Doesn't seem like you have enough money, sweetheart.'@n  An annoyed look passes over her face.  @M'I hope that doesn't mean you're gonna stiff me on my tip.  Ugh.'@n        
      end
    else
      %force% %actress% emote looks confused as she looks at her notepad.  @M'Um... babe, you haven't ordered anything.  I can't charge you if you haven't ordered anything, darlin'.'@n
    end
  end
elseif (%arg% /= tip )
  wait 1s
  if (%actstate% != here)
    %send% %actor% @RUm... perhaps you should @YCall Server@R first.@n
  else
    if (%actor.gold% >= %tipprice%)
      nop %actor.gold(-%tipprice%)
      %force% %actress% emote accepts some money from %actor.name% with a wink.  @M'I do so love getting tips.  Thank you, sugar.'@n
      eval listertip %listertip% + %tipprice%
      remote listertip %actor.id%
    else
      %force% %actress% emote looks at her notepad and shakes her head.  @M'Air doesn't count as a tip, sweetie.'@n  An annoyed look passes over her face.  @M'I hope that doesn't mean you're gonna stiff me on my tip.  Ugh.'@n
    end
  end
else
  %send% %actor% @R--------- @YCommand List @R---------@n
  %send% %actor% @R- @Y1 - Call Server              @R-@n
  %send% %actor% @R- @Y1 - Order Food:  @y%foodprice%        @R-@n
  %send% %actor% @R- @Y1 - Order Drink: @y%wineprice%         @R-@n
  %send% %actor% @R- @Y1 - Order Dance: @y%danceprice%        @R-@n
  %send% %actor% @R- @Y1 - Dismiss                  @R-@n
  %send% %actor% @R- @Y1 - Check Tab                @R-@n
  %send% %actor% @R- @Y1 - Tip:         @y%tipprice%         @R-@n
  %send% %actor% @R- @Y1 - Pay                      @R-@n
  %send% %actor% @R--------- @YCommand List @R---------@n
  %send% %actor% @R@n
  %send% %actor% @YSyntax:@R garcon <command>@n
end
~
#18606
new trigger~
2 c 100
fixmyshit~
set listerfavor 0
remote listerfavor %actor.id%
~
#18699
new trigger~
0 c 100
bu~
set beforegold %actor.gold%
set profitroom 18692
%teleport% %actor% %profitroom%
wait 1
%force% %actor% get all
wait 1
%teleport% %actor% %self.room.vnum%
set aftergold %actor.gold%
if (%aftergold% > %beforegold%)
  eval golddifference %aftergold% - %beforegold%
  %echo% You collect a profit of %golddifference% zenni from your shop.
else
  %echo% No profits atm.
end
%echo% %self.bank% <--- Let's see if this throws an error.
~
$~
