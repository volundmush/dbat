#16300
Ship/House Pamphlet~
0 d 100
*~
if (%speech% == house)
  if %actor.inventory(4005)%
    wait 1 s
    say Sorry, you already have a house pamphlet.
  else
    wait 1 s
    %load% obj 4005
    give house %actor.name%
  end
elseif (%speech% == ship)
  if %actor.inventory(63)%
    wait 1 s
    say Sorry, you already have a ship pamphlet.
  else
    wait 1 s
    %load% obj 63
    give ship %actor.name%
  end
end
~
#16301
Combat Simulator End~
2 d 100
*~
if %speech% == leave
wait 1 s
%send% %actor% Ending simulation program.
wait 2
%teleport% %actor% 16311
%at% 13567 %force% %actor% look
%send% %actor% Training simulation program ended.
end
~
#16302
Combat simulator run~
2 d 100
*~
if (%speech% == begin)
  if (%actor.level% < 10)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, level one opponents.
wait 2 s
%send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the combat simulator!
    %teleport% %actor% 16358
    %force% %actor% look
  elseif (%actor.level% < 20)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, level two opponents.
wait 2 s
%send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the combat simulator!
    %teleport% %actor% 16359
    %force% %actor% look
  elseif (%actor.level% < 30)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, level three opponents.
wait 2 s
%send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the combat simulator!
    %teleport% %actor% 16360
    %force% %actor% look
  elseif (%actor.level% < 40)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, level four opponents.
wait 2 s
%send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the combat simulator!
    %teleport% %actor% 16361
    %force% %actor% look
  elseif (%actor.level% < 50)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, level five opponents.
wait 2 s
%send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the combat simulator!
    %teleport% %actor% 16362
    %force% %actor% look
  elseif (%actor.level% < 60)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, level six opponents.
wait 2 s
%send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the combat simulator!
    %teleport% %actor% 16363
    %force% %actor% look
  elseif (%actor.level% < 70)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, level seven opponents.
wait 2 s
%send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the combat simulator!
    %teleport% %actor% 16364
    %force% %actor% look
  elseif (%actor.level% < 80)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, level eight opponents.
wait 2 s
%send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the combat simulator!
    %teleport% %actor% 16365
    %force% %actor% look
  elseif (%actor.level% < 90)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, level nine opponents.
wait 2 s
%send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the combat simulator!
    %teleport% %actor% 16366
    %force% %actor% look
  elseif (%actor.level% < 100)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, level ten opponents.
wait 2 s
%send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the combat simulator!
    %teleport% %actor% 16367
    %force% %actor% look
  elseif (%actor.level% == 100)
    %send% %actor.name% Evaluating your strength, one moment please.
    wait 2 s
    %send% %actor.name% Your strength has been determined, level eleven opponents.
wait 2 s
%send% %actor.name% A blinding light flashes around you momentarily, then you find yourself in the combat simulator!
    %teleport% %actor% 16368
    %force% %actor% look
  else
    %send% %actor.name% You can't use the combat simulator!
  end
end
~
#16303
CryoSec Customs~
0 q 100
~
if (%direction% == north)
   wait 1 s
   %send% %actor% %self.name% scans you for security risks.
   wait 1 s
   %send% %actor% %self.name% waves you through, saying, "Welcome to the Space Hub."
elseif (%direction% == south)
   wait 1 s
   %send% %actor% %self.name% scans you for security risks.
   wait 1 s
   %send% %actor% %self.name% waves you through, saying, "Enjoy your flight."
else
   wait 1 s
   %send% %actor% %self.name% signals you to halt and says, "Sorry, but I can't let you go any further."
   return 0
end
~
#16304
Practice Range Teleport To~
2 c 100
press~
if (%arg% == button)
 wait 2 s
 %send% %actor% You press the button on the wall.  A blinding light flashes around you momentarily, then you find yourself in the practice range!
 %echoaround% %actor% %actor.name% presses the button on the wall.  A blinding light flashes around %actor.hisher% body momentarily, then they vanish with the light!
 %teleport% %actor% 16399
wait 1 s
 %force% %actor% look
else
 %send% %actor% Maybe try pressing the @Rbutton@n? 
end
~
#16305
Custom Skill Practice~
2 c 100
enhance~
if (%arg% == blessed)
 if (%actor.name% == Koshou)
  wait 1 s
  eval num %random.8%
   switch %num%
    case 1
    case 2
     %send% %actor% You gather your ki around you and begin to use your technique.  However, as the attack begins to charge, your ki dissipates around you!
     %echoaround% %actor% %actor.name% begins to channel their ki around them, preparing their technique. However, as the attack begins to charge, the ki dissipates harmlessly around them!
     wait 3 s
     %send% %actor% You may try to enhance your skill once more.
     break
    case 3
    case 4
    case 5
     %send% %actor% You grin, setting your sights upon the practice target before you. Gathering your ki around you, you prepare your attack successfully.  However, as you fire, your aim was off and goes careening past the target into the wall!
     %echoaround% %actor% %actor.name% grins, focusing intently upon the practice dummy in front of them. %actor.heshe% gathers their energy, successfully preparing their attack.  However, as they fire, they miss the target and watch hopelessly as their attack soars into the wall!
     wait 3 s
     %send% %actor% You may try to enhance your skill once more.
     break
    case 6
    case 7
     %send% %actor% You gather your energy around you and begin to charge your technique.  Focusing on your target, you unleash the Blessed Hammer onto the target dummy!  You've succeeded!.. but could use some more work.
     %echoaround% %actor% %actor.name% gathers energy and begins to charge %actor.hisher% technique.  %actor.heshe% focuses intently on the practice dummy, then unleashes the Blessed Hammer onto the target dummy!  %actor.hisher% attack was a success, however, %actor.heshe% look puzzled.
     %load% m 16323
     %force% %actor% practice blessed
     %purge% bham
     wait 3 s
     %send% %actor% You may try to enhance your skill once more.
     break
    case 8
     %send% %actor% You gather your energy around you and begin to charge your technique.  Focusing on your target, you unleash the Blessed Hammer onto the target dummy!  You've met with great success, and have learned a lot!
     %echoaround% %actor% %actor.name% gathers energy and begins to charge %actor.hisher% technique.  %actor.heshe% focuses intently on the practice dummy, then unleashes the Blessed Hammer onto the target dummy!  %actor.hisher% attack was great success!
     %load% m 16323
     %force% %actor% practice blessed
     %force% %actor% practice blessed
     %purge% bham
     wait 3 s
     %send% %actor% You may try to enhance your skill once more.
     break
 else
  %send% %actor% You can't practice someone else's skill!
 end
else
end
~
#16306
Practice Range Teleport Out~
2 c 100
press~
if (%arg% == button)
 wait 2 s
 %send% %actor% You press the button on the wall.  A blinding light flashes around you momentarily, then you find yourself in the Combat Training Hallway!
 %echoaround% %actor% %actor.name% presses the button on the wall.  A blinding light flashes around %actor.hisher% body momentarily, then they vanish with the light!
 %teleport% %actor% 16311
wait 1 s
 %force% %actor% look
else
 %send% %actor% Maybe try pressing the button? 
end
~
#16307
Gym Instructor greet~
0 g 25
~
wait 1 s
say Oooooh yeah baby, feel the burn!
~
#16310
Space Hub M-Deck Radio~
2 b 5
~
 eval ccm %random.1000%
  if (%ccm% >= 900)
    wait 2 s
    %echo% The crackled speaker of some radio hidden amongst the clutter suddenly cuts in, "@CR-E-B-E-L Ray-Dee-Ohhhh! Welcome back to Saiyan Slam Hip Hop Hour ..@n"
            Wait 1
           %echo% Just as suddenly, the terrible speaker cuts out again.
  elseif (%ccm% >= 750)
            wait  2 s
    %echo% The crackled speaker of some radio hidden amongst the clutter suddenly cuts in, "@CAnd that was 'Be my Overlord' by Icerian pop princess, Snowflake ..@n"
            Wait 1
           %echo% Just as suddenly, the terrible speaker cuts out again.
  elseif (%ccm% >= 500)
    wait 2 s
    %echo% The crackled speaker of some radio hidden amongst the clutter suddenly cuts in, "@CWelcome back to Galactic! News! Radio! Tonight we discuss the ongoing conflict on Aether ..@n"
            Wait 1
           %echo% Just as suddenly, the terrible speaker cuts out again.
  elseif (%ccm% >= 250)
    wait 2s
    %echo% The crackled speaker of some radio hidden amongst the clutter suddenly cuts in, "@CDoc, we're losing her! We're losing her goddamn-it!@n"
            Wait 1
           %echo% Just as suddenly, the terrible speaker cuts out again.
  else
    wait 2s
    %echo% The crackled speaker of some radio hidden amongst the clutter suddenly cuts in, and the air is filled with a loud @DST@WAT@DIC@n noise that fills the ear with an annoying buzz.
            Wait 1
           %echo% Just as suddenly, the terrible speaker cuts out again.
End
~
#16311
Sticky Brownie~
1 c 10
partake~
if ("%arg%" != "brownie")
   wait 1s
   %send% %actor% @nTry @RParktake@ning of the @RBrownie@n.
   return 0
Else
  wait 2 s
  %send% %actor% You find that you will feel very mellow over the next several minutes.
  %echoaround% %actor% %actor.name% seems to look a lot more mellow.
  wait 60 s
  %send% %actor% You feel that mellow settling in at full effect, now.
  wait 1 s
  %force% %actor% blink
  wait 1 s
  %force% %actor% yawn
  wait 30 s
  %send% %actor% The high settles in and you are fully stoned. Life is good. Life is love.
  %echoaround% %actor% %actor.name% definitely seems stoned now.
  %purge% %self%
End
~
#16312
Spice Trigger~
1 c 10
partake~
if ("%arg%" != "spice")
   wait 1s
   %send% %actor% @nTry @RParktake@ning of the @RSpice@n.
   return 0
Else
  wait 2 s
  %send% %actor% You feel a tingling sensation as the @CS@cp@Ri@cc@Ce@n flows down your throat.
  %echoaround% %actor% %actor.name% seems to take on a far-away look to their features as they imbibe a vial of @CS@cp@Ri@cc@Ce@n. 
  wait 60 s
  %send% %actor% The world seems in focus. Your mind feels ready to unlock the vast mysteries of the universe, as the @CS@cp@Ri@cc@Ce@n shows you the way.
  %echoaround% %actor% %actor.name% seems to improve their focus, a faint hint of the @CS@cp@Ri@cc@Ce@n glow shining in their eyes.
  wait 45 s
  %send% %actor% The @CS@cp@Ri@cc@Ce@n must flow. With more, you feel you could do anything!
  %echoaround% %actor% %actor.name% seems intently focused on whatever task they're undertaking as they fall prey to the effects of the @CS@cp@Ri@cc@Ce@n. 
  %purge% %self%
End
~
#16313
Metallic Dust Trigger~
1 c 10
partake~
if ("%arg%" != "dust")
   wait 1 s
   %send% %actor% @nTry @RParktake@ning of the @RDust@n.
   return 0
Else
  wait 2 s
  %send% %actor% The @Dm@Ce@Dt@Wa@Dll@Ci@Dc@n dust gives you a sudden rush as you imbibe it. Colors waver and pulse in your vision and objects seem to phase in and out of existence.
  %echoaround% %actor% %actor.name% seems energized as they imbibe or inhale some @Dm@Ce@Dt@Wa@Dll@Ci@Dc@n dust.
  wait 60 s
  %send% %actor% It's hard to determine the difference between fantasy and reality as the world continues to expand and contract around you. Now everything has a @Dm@Ce@Dt@Wa@Dll@Ci@Dc@n tint to it.
  %echoaround% %actor% %actor.name%'s skin seem to take on a faintly @Dm@Ce@Dt@Wa@Dll@Ci@Dc@n hue that soon goes away as they continue to experience the substance.
  wait 45 s
  %send% %actor% The @Dm@Ce@Dt@Wa@Dll@Ci@Dc@n slowly loosens its grip on your mind and the world returns to normal. Everything seems so bland now, lacking a certain vibrancy.
  %echoaround% %actor% %actor.name% seems to return to normal, a momentary expression of loss flitting across their face. 
  %purge% %self%
End
~
#16314
Shroom Trigger~
1 c 10
partake~
if ("%arg%" != "shrooms")
   wait 1 s
   %send% %actor% @nTry @RParktake@ning of the @RShrooms@n.
   return 0
Else
  wait 2 s
  %send% %actor% @DYou eat the @DDried@y Mushrooms@n, and the taste is horrible. It takes a moment or two, and a few @Rgu@rr@Rgl@re@Rs@D of protest for your stomach to settle.@n
  %echoaround% %actor% %actor.name% ingests some @DDried@y Mushrooms@n, and the horrible taste is written all over their face.
  wait 60 s
  %send% %actor% @DHey Man. It's good to see you again.@n
  %echoaround% %actor% %actor.name% glances around subtly, as if hearing someone talk to them.
  wait 45 s
  %send% %actor% @R HEY MAN.@D It's good to see you again. Goodbye ...@n
  %echoaround% %actor% %actor.name% glances around now with more paranoia, as if they can hear someone talking to them.
    %purge% %self%
End
~
$~
