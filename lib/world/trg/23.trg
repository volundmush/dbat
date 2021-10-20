#2300
human male mcstuff~
0 abcdefghir 100
Hello, may I take your order?~
wait 1 sec
Emote looks at the menu board and thinks for a second or two..
wait 1 sec
say @whmm@n
wait 1 sec
say @wI think i will have a cheese burger and soft drink please..@n
buy 2
buy 3
mjunk all
~
#2301
wield/wear trigger, whee~
1 j 100
~
%echo% %actor.name% does such and such.
~
#2302
Sarason's Spear~
1 j 100
~
if (%actor.name% == Sarason || %actor.name% == sarason)
else
 return 0
%send% %actor% @WYou have not been bestowed with the @BM@bo@mrt@ba@Bl @BRe@bm@min@bd@Ber@W!@n
end
~
#2303
No-osay~
2 c 100
osay~
if (%actor.name% == X)
return 0
~
#2305
Position tester~
2 c 100
asdf~
if (%actor.pos% == hide)
%echo% %actor.name% is a dirty hider!
else
%echo% %actor.name% is not a hider!
end
~
#2312
No removing Dragon Scar~
1 l 100
~
wait 1
%force% %actor% wear scar
wait 1
%force% %actor% emote tries to remove the scar from %actor.hisher% chest, but only manages to dig %actor.hisher% fingers into %actor.hisher% flesh.
~
#2318
Bleach Drink~
1 c 3
bleach~
   %send% %actor% You raise the jug of bleach above your head and pour the firebreather out onto your eyes, scalding them! It burns!  IT BURNS!
   %echoaround% %actor% %actor.name% raises the jug of eye bleach over their head and pours the liquid fire out all over their face! 
wait 2 s
   %send% %actor% The horrid memory you just had to endure is slowly burnt out of your mind!
   %echoaround% %actor% %actor.name% grabs their eyes and flails around for a second or two as the bleach works its magic.
wait 2 s
   %send% %actor% You lower the jug and wipe your face, glad to be clean.
   %echoaround% %actor% %actor.name% lowers the jug and wipes the bleach off of their face.
~
#2322
Telepathy Helmet~
1 c 3
farsight~
eval skillholder %actor.skill(telepathy)%
nop %actor.skillset(telepathy 100)%
%force% %actor% telepathy far %arg%
nop %actor.skillset(telepathy %skillholder%)%
~
#2323
Ring Heal~
1 c 3
dragonheal~
eval skillholder %actor.skill(heal)%
nop %actor.skillset(heal 100)%
%force% %actor% heal self
nop %actor.skillset(heal %skillholder%)%
%force% %actor% emote twists the %self.name% around on %actor.hisher% finger slowly, lost some sort of emotional anguish.  A single tear rolls down %actor.hisher% face.
~
#2324
Koala Trigger~
1 c 100
press~
if (%arg% == button)
  %send% %actor% You press the button.
  %echoaround% %actor% %actor.name% presses the button on the @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n.
  wait 1s
  eval nuz %random.1000%
  if (%nuz% >= 900)
    wait 1s
    %send% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, '@GGday mate, hozzit goin?@n'
    %echoaround% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, '@GGday mate, hozzit goin?@n'
  elseif (%nuz% >= 800)
wait 2s
    %send% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, '@YWhy did the manager hire the marsupial? Because he was koala-fied. Hah!@n'
    %echoaround% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, '@YWhy did the manager hire the marsupial? Because he was koala-fied. Hah!@n'
  elseif (%nuz% >= 700)
    wait 2s
    %send% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, 'Crickey!'
    %echoaround% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, Crickey!'
  elseif (%nuz% >= 600)
    wait 2s
    %send% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, @G'Put another shri- I mean, prawn. Put another prawn on the barbie!@n'
    %echoaround% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, @GPut another shri- I mean, prawn. Put another prawn on the barbie!@n'
elseif (%nuz% >= 500)
    wait 2s
    %send% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, '@YThats not a knife.. this is a knife.@n'
    %echoaround% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, @YThats not a knife.. this is a knife.@n'
elseif (%nuz% >= 400)
    wait 2s
    %send% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, '@GGive me a home among the gum trees, with lots of plum trees, a dog or two and a barbecue, flowers down the side, and veggies by the fence.@n'
    %echoaround% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, @GGive me a home among the gum trees, with lots of plum trees, a dog or two and a barbecue, flowers down the side, and veggies by the fence.@n'
elseif (%nuz% >= 300)
    wait 2s
    %send% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, '@YWhat do you call a boomerang that doesnt come back? A stick!@n'
    %echoaround% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, @YWhat do you call a boomerang that doesnt come back? A stick!@n'
elseif (%nuz% >= 200)
    wait 2s
    %send% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, '@GOnce a jolly swagman camped by a billabong. Under the shade of a coolibah tree,
and he sang as he watched and waited till his billy boiled: "Who'll come a-waltzing Matilda, with me?"@n'
    %echoaround% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, @GOnce a jolly swagman camped by a billabong. Under the shade of a coolibah tree,
and he sang as he watched and waited till his billy boiled: "Who'll come a-waltzing Matilda, with me?"@n'
elseif (%nuz% >= 100)
    wait 2s
    %send% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, '@YI could go for a vegemite sandwich, a moist lamington and a cold pint.@n'
    %echoaround% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, @YI could go for a vegemite sandwich, a moist lamington and a cold pint.@n'
  else
    wait 2s
    %send% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, '@GWaltzing Matilda, waltzing Matilda- who the bloody hell is Matilda?@n'
    %echoaround% %actor% The @YK@Go@Ya@Gl@Ya @GP@Ylu@Gs@Yhi@Ge@n says, '@GWaltzing Matilda, waltzing Matilda- who the bloody hell is Matilda?@n'
  end
else
wait 1 s
%send% %actor% Try pressing the button!
end
~
#2325
Level Caps~
1 c 3
g~
set levelcap 110
set mycmd %cmd.mudcommand%
if %mycmd% == gain
  if %actor.level% >= %levelcap%
    %send% %actor% You are able to gain another level, however, the level cap at this time is currently at %actor.level%.
  else
    return 0
  end
else
  return 0
end
~
#2326
age~
1 c 3
bananas~
* No Script
~
#2327
Level Cap Backup~
1 c 3
gai~
if (%actor.level% == 20)
  if (%cmd% == gai || %cmd% == gain)
    %send% %actor% You are able to gain another level, however, the level cap at this time is currently at %actor.level%.
  end
else
  return 0
end
~
#2369
age trigger~
1 c 100
asdf~
eval oldage %actor.age%
eval newage %oldage% + 1
 set %actor.age% %newage%
~
#2397
fucking king kai~
1 c 3
asdf~
    set kktrain done
    remote kktrain %actor.id%
~
#2398
Cigarette Smoke~
1 c 3
smoke~
 wait 2 s
 %force% %actor% emote lifts the cigarette to their mouth and pulls a lighter from their pocket.
wait 10 s
 %force% %actor% emote lights the cigarette and takes a long draw from it, the end turning cherry red as they inhale.
wait 120 s
 %send% %actor% You feel the stress begin to wash out of you.
 %echoaround% %actor% %actor.name% seems a bit more relaxed.
wait 120 s
 %echo% Thin wisps of smoke rise off of the cigarette.
wait 120 s
 %force% %actor% emote finishes off the cigarette and stubs it out.  They then flick the butt of the smoke away.
 %purge% %self%
else
 %send% %actor% Try to smoke the cigarette!
end
~
#2399
coin flipz~
1 c 3
flip~
if (%arg% == coin )
 %send% %actor% @WYou place the coin on your knuckle and your thumb, then flick it up into the air, sending it spinning high in the air...@n 
 %echoaround% %actor% @C%actor.name%@W carefully sets the coin upon %actor.hisher% knuckle and them, then flicks it into the air, sending the zenni spinning high in the air...@n
 wait 4 s
eval num %random.3%
if %num% == 1
  %send% %actor% @WThe zenni comes down and you snatch it out of mid air, quickly slapping it down on the back of your hand. @CIt's... @Yheads@C!!@n 
  %echoaround% %actor% @WThe zenni spins in the air several times, and at the last possible second @C%actor.name%@W snatches the coin out of mid air! @C%actor.name%@W slaps it on the back of %actor.hisher% hand!  @CIt's... @Yheads@C!!@n
elseif %num% == 2
  %send% %actor% @WThe zenni comes down and you snatch it out of mid air, quickly slapping it down on the back of your hand. @CIt's... @Rtails@C!!@n 
  %echoaround% %actor% @WThe zenni spins in the air several times, and at the last possible second @C%actor.name%@W snatches the coin out of mid air! @C%actor.name%@W slaps it on the back of %actor.hisher% hand!  @CIt's... @Rtails@C!!@n
elseif %num% == 3
%send% %actor% @WThe zenni comes down and you snatch it out of mid air, quickly slapping it down on the back of your hand. However, you fumble and drop the coin, watching as it lands.. on it's side?! @GIndeterminate!!@n
%echoaround% %actor% The zenni spins in the air several times, and at the last possible moment, @C%actor.name%@W attempts to snatch the coin out of mid air! However, @C%actor.name%@W fumbles and drops the coin, watching as it lands... on it's side?! @GIndeterminate!!@n
  end
 else
 %send% %actor% @CTry to @Wflip@C the @Wcoin@C!@n
end
~
$~
