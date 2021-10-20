#12600
Mob Block during certain hours @R(LEAVE)@n~
0 q 100
~
if (%direction% == north)
  if (%time.hour% >= 16)
    return 0
    wait 1 s 
    %send% %actor% @w%self.name% shakes her head at you. 'Come back tomorrow.'
    %echoaround% %actor% @w%self.name% shakes her head at %actor.name%. '@CCome back tomorrow.@w'@n
  elseif (%time.hour% <= 8)
    return 0
    wait 1 s
    %send% %actor% @w%self.name% shakes her head at you. '@CCome back later. It's still too early@w'@n
    %echoaround% %actor% @w%self.name% shakes her head at %actor.name%. '@CCome back later. It's still too early@w'@n
  end
end
~
#12602
King Furry's Sit trigger @R(LOAD)@n~
0 n 100
~
wait 1 s
sit chair
~
#12606
Table Guy Reports Load~
0 n 100
~
%load% obj 12600
unlock 12604
open 12604
take all.report 12604
junk all.report
%load% obj 12609
%load% obj 12610
put all.report 12604
close 12604
lock 12604
junk key
%purge% self
~
#12614
Moving Camera @R(ENTER)@n~
2 g 100
~
wait 1 s
%send% %actor% @YA @Dcamera@Y beeps and slowly moves to watch your actions!@n
%echoaround% %actor% @YA @Dcamera@Y beeps and slowly moves to watch @c%actor.name%@Y's actions!@n
~
#12628
@CS@cu@wp@Wr@De@cm@Ce @CC@co@wu@Wn@Dc@ci@Cl @YCold/Water@n~
2 g 50
~
  eval nuz %random.1000%
  if (%nuz% >= 950)
    wait 1s
    %send% %actor% @cAs you breathe, you see your breath in the form of @Cvapor@c.@n
    %echoaround% %actor% @CYou see @c%actor.name%s@C breath in the form of vapor.@n
 elseif (%nuz% >= 850)
  wait 2s
    %send% %actor% @cYou hear the sound of water splashing and crashing in other portions of the tower.@n
    %echoaround% %actor% @cThe sounds of water crashing and moving can be heard from other portions of the tower.@n
    else
End
~
#12629
@CS@ce@Ba @cS@Ce@Br@Wp@Ce@Bn@ct@w trigger~
0 g 100
~
%send% %actor% @CVatten@w, the @DT@ww@We@bl@Bft@bh@W extends his arms up into the air and back down to the ground.
wait 2 s
%send% %actor% @CThe water from the ceiling is pulled down and starts circling @CVatten@w, the @DT@ww@We@bl@Bft@bh@n.
wait 2 s
%send% %actor% @CVatten@w, the @DT@ww@We@bl@Bft@bh@W clasps his hands together and the @Cwater@W explodes violently into a watery @CS@ce@Ba @cS@Ce@Br@Wp@Ce@Bn@ct@W!@n
%load% obj 12629
nop drop sea
wait 2 s
%send% %actor% @CVatten@w, the @DT@ww@We@bl@Bft@bh@W twists and turns and finally extends his arms into the direction of the @BI@cc@Be @DD@Wummy@W!@n
wait 1 s
%send% %actor% @wA @CS@ce@Ba @cS@Ce@Br@Wp@Ce@Bn@ct@W roars loudly and then rushes the @BI@cc@Be @DD@Wummy@W. Completely engulfing the thing!@n
%purge% Sea
%purge% Self
~
#12642
Aquis Tower Guard~
0 q 100
~
if (%direction% == up && %actor.race% != Kanassan)
     Return 0
     %echo% @nThe Hydromancer steps forward to deny you passage. @COnly Kanassan are permitted to walk the upper halls!@n
elseif (%actor.drag%)
     return 0
     %send% %actor% @RYou may not drag anyone inside.@n
elseif (%direction% == up && %actor.race% = Kanassan)
     wait 2s
     %echo% @nThe guard gives %actor.name% a solemn bow and steps side to let them pass.@n
return 1
end
~
#12643
Moai Palace Guard~
0 gq 100
~
if (%actor.race% != Arlian && !%actor.inventory(16000)%)
      Wait 1s
      Return 0
      %echo% @nThe powerful looking Arlian buzzes angrily as faceted eyes scan %actor.name% for something. "@GYou dont have a pass! Intruder!@n" The sound of footsteps can be heard approaching the scene!
      punch %actor.name%
elseif (%actor.race% != Arlian && %actor.inventory(16000)%)
      say @GCarry on outsider.@n
end
~
#12644
Moai Greet~
0 g 100
~
 if (!%actor.varexists(onmoaiq)% && !%actor.varexists(moaicomp)%)
      wait 4 s
      %echoaround% %self% With an angry buzzing and an indignant posture, %self.name% turns to look at %actor.name%. "@GAn Intruder? In my Palace?! Gua-@n"
      wait 4 s
      %echoaround% %self% %self.name% pauses a moment before regarding %actor.name% more sharply. "@GPerhaps you can be of some use to me. You will do something for me, outsider! To assuage my mighty wrath!@n"
      wait 4 s
      %echoaround% %self% The Arlian King shuffle backs in forth in pompous fashion, his carapace wings clicking and clacking as he moves with agitation. "@GYou will do it! @RDefer@G to the authority of King Moai!@n"
      wait 4 s
      %echoaround% %self% %self.name% hisses in anger and for a moment he swells up to an impressive stature, the insectoid nature of his race making him look quite imposing indeed.
      wait 4 s
      %echoaround% %self% After a moment or two more, the Arlian ruler settles back down with an irritated buzzing to his final words. "@GJust begone before I change my mind.@n"
elseif (%actor.varexists(moaicomp)%)
            wait 4 s
            %echoaround% %self% %self.name% buzzes about greedily and he barely pays attention to %actor.name%@n.
            wait 4 s
            %echoaround% %self% @n"@GYou again? Buzz off ...@n"
       else
         wait 3 s
         %echoaround% %self% %self.name% turns to look at %actor.name% with impatience.
         wait 4 s
         %echoaround% %self% @n"@GWhere is my proof! Huh?! Obey King Moai!@n"
         wait 4 s
         %echoaround% %self% %self.name% turns away from %actor.name% after a moment or two longer, ignoring them completely.
  End
~
#12645
Moai Accept~
0 c 100
defer~
If (%actor.varexists(onmoaiq)%)
  return 0
  wait 3 s
   say @GI thought I told you to go and kill that damn Yetti! What are you doing here?! Obey King Moai!@n
    Elseif (%actor.varexists(moaicomp)%)
      return 0
      wait 3 s
      say @GBegone, outsider! You've already done my dirty work!@n
Else
  wait 3 s
  %echoaround% %self% %self.name% buzzes about even more pompously and angrily. "@GThere is a stupid, slothful creature on this planet. A @RYetti@G. I want you to kill him and bring me proof. Something I know is his.@n"
  wait 3 s
  say @GNow get out of my sight, and don't let any of the guards see you!@n
  Set onmoaiq
  remote onmoaiq %actor.id%
End
~
#12646
Moai Receive~
0 j 100
~
If (!%actor.varexists(onmoaiq)% && !%actor.varexists(moaicomp)%)
      return 0
      wait 3 s
      say @GHow dare you defy my royal authority? Get out!@n
         Elseif (%actor.varexists(moaicomp)%)
            return 0
            wait 3 s
            say @GYou already did my dirty work! Buzz off!@n
    else
       If (%object.vnum% != 16609)
           return 0
           wait 3 s
           say @GThis isn't good enough! I want some real proof!@n
           wait 2 s
           %echoaround% %self% %self.name% buzzes angrily in indignation and turns his back on %actor.name%
Else
       wait 3 s
       %echoaround% %self% %self.name% takes the orb in clawed hands as he stares at it greedily.
       wait 3 s
       say @GYes .. yes. You've done the crown of Arlia a great service ...   uh ... @n
       wait 2 s
       %purge% %object.name%
       %echoaround% %self% The Arlian spends several moments just muttering in gluttony to himself and pacing back and forth with the scratchy sounds of insect feet.
       wait 3 s
       %echoaround% %self% Finally, %self.name% turns to look at %actor.name% with a weasley-eyed expression. 
       wait 3 s
       rdelete onmoaiq %actor.id%
       wait 1 s
       %echoaround% %self% %self.name% turns and walks into the passage tucked behind the area where his throne should be. He emerges a moment later with something in his clawed hands, no longer carrying the orb. "@GTake this as a token of my benevolence. Now buzz off!@n"
         eval num %random.1000%
          if (%num% >= 990)
                wait 3s
                   %echoaround% %self% %self.name% tosses what look like Arlian carapace shells fashioned into some sort of armor at %actor.name%.
                   wait 2 s
               set moaicomp
               remote moaicomp %actor.id%
               %load% obj 12631
               give carapace %actor.name%
               wait 3 s
               %echoaround% %self% The Arlian monarch turns away from %actor.name% and back to the self-important surveillance of his slaves.
        Else
                wait 3 s
                   %echoaround% %self% %self.name% tosses what look like Arlian carapace shells fashioned into some sort of armor at %actor.name%.
                   wait 2 s
               set moaicomp
               remote moaicomp %actor.id%
               %load% obj 12631
               give carapace %actor.name%
               wait 3 s
               %echoaround% %self% The Arlian monarch turns away from %actor.name% and back to the self-important surveillance of his slaves.
         End
      End
  End
~
#12699
Underwater Ambience~
2 g 100
~
  eval nuz %random.1000%
  if (%nuz% >= 950)
    wait 1s
    %send% %actor% @DAs you swim through here, you leave a trail of little @Cbu@Wbb@Cle@Ws@D as air escapes your mouth.@n
    %echoaround% %actor% @W%actor.name%@c leaves a trail of small @Cbu@Wbb@Cle@Ws@c behind them as they swim by.@n
 elseif (%nuz% >= 850)
  wait 2s
    %send% %actor% @DA small @Rpiranha@D suddenly darts by, in chase of a smaller fish!@n 
    %echoaround% %actor% @DA small @Rpiranha@D suddenly darts past %actor.name% in chase of a smaller fish!@n
    else
end
~
$~
