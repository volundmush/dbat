#16401
Contraband~
1 c 3
smoke~
set eatit %cmd%
  if (%eatit% == smoke && %arg% /= contraband)
    %force% %actor% emote looks around to either side.
     wait 7s
     %force% %actor% emote quickly rolls a smoke, sealing it shut.
     wait 5s
     %force% %actor% emote lights the smoke with a tiny bit of ki, and inhales deeply.
     wait 4s
     %force% %actor% emote exhales slowly, feeling very mellow.
     wait 10s
     %force% %actor% emote finishes the smoke, and tosses the empty bag.
      %purge% contraband
end
~
#16402
Roof Jump OSH~
2 c 100
leap~
wait 1s
    %send% %actor% @nYou glance over the edge of the roof for a moment, judging the distance.
   wait 2s
   %send% %actor% Taking a few steps back from the edge, you run full speed and make a great leap off the roof!
    wait 2s
%echoaround% %actor% %actor.name% takes a few steps back from the edge before running full speed and making a mighty leap off of the roof!
    %teleport% %actor% 15907
    wait 1s
    %send% %actor% You land on the athletic field with a resounding thud!
    %echoaround% %actor% %actor.name% lands on the grass with a loud thud!
    %force% %actor% look
~
#16403
Shara Greet~
0 g 100
~
if (%actor.is_pc%)
 if (!%actor.varexists(sharqcomp)% && !%actor.varexists(onsharq)%)
   wait 2s
   emote cries softly, looking up in startlement as you walk in.
    wait 3s
%echoaround% shara The girl sniffles a little as she wipes her nose and glares at you with anger.
   wait 2s
   say @CWhat do you want? Go away! You better not tell anyone I was crying!@n
    wait 3s
      %echoaround% shara Taking a moment to collect herself, the girl stops scowling and looks at you.
        wait 3s
         say @CYou look like you could help. Theres a bully in this school and hes gone too far! If you can, please @Roffer@C your help.@n
     elseif (%actor.varexists(onsharq)%)
        wait 2s
         say @CDid you pummel him yet? Hes only going to hurt more people you know!@n
    else
     wait 2s
        say @CThanks for your help  but you still  better not tell anyone I was crying or Ill pummel you!@n
end
~
#16404
Shara Accept~
0 c 100
offer~
if (%actor.varexists(sharqcomp)%)
  wait 3s
  say @CThank you again for dealing with that bully. Serves him right!@n
     elseif (%actor.varexists(onsharq)%)
        wait 3s
        say @CWhat are you waiting for? Go pummel that creep!@n
   else
    wait 3s
    say @CGreat! Go pummel that creep and bring me back his stupid bat as proof!@n
    set onsharq
    remote onsharq %actor.id%
end
~
#16405
Shara Complete~
0 j 100
~
if (!%actor.varexists(onsharq)%)
  return 0
   wait 3s
    say @CI don't want this, dummy!@n
      elseif (%object.vnum% != 16412)
         return 0
          wait 3s
           say @CThis isn't what I wanted, dummy!@n
  elseif (%actor.varexists(sharqcomp)%)
    return 0
    wait 3s
      say @CI dont want this anymore, dummy!@n
  else
      wait 3s
       %purge% %object.name%
        say @CWow, you really creamed him huh? Wish I could have been there to see it!@n
         wait 3s
          %echoaround% shara The girl blushes slightly, and then quickly presses a ring into your palm before turning away.
          wait 2s
           %load% obj 16421
           give shara %actor.name%
             rdelete onsharq %actor.id%
               set sharqcomp
                remote sharqcomp %actor.id%
          end
~
#16422
Director Sit~
0 n 100
~
wait 1 s
sit chair
~
#16499
Test Height increase~
2 cd 100
baja~
nop %actor.weight(100)%
~
$~
