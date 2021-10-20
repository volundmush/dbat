#5300
Hunting preserve entry trigger~
0 m 100
1000~
wait 1 s
say Ok, I'll use the teleporter to bring you into the bio-dome. Happy hunting!
wait 2 s
emote pulls a lever on the wall.
wait 1 s
%send% %actor% A flash of energy appears around you before you are whisked away!
%echoaround% %actor% A flash of energy appears around %actor.name% before they are whisked away!
%teleport% %actor.name% 5303
wait 2 s
%at% 5303 %force% %actor% look
~
#5301
Entrance guard greet~
0 g 100
~
say Hello there, %actor.name%. Entry fee is 1000 zenni, to get in, just hand me the money.
~
#5302
Hunting preserve leave trigger~
0 d 100
leave~
say Ok then, one way trip out of the Hunting Preserve coming up!
wait 2 s
emote taps a button on his tablet device.
wait 1s
%send% %actor% A bright flash appears around you before suddenly whisking you away
%echoaround% %actor% A bright flash appears around %actor.name% before suddenly whisking them away.
wait 1 s
%teleport% %actor% 5301
wait 2 s
%at% 5301 %force% %actor% look
~
#5303
Hunting rifle delete~
1 b 100
~
if %self.room.vnum% < 5302 || %self.room.vnum% > 5499
  wait 1 s
  %purge% %self%
end
~
#5304
Bait Box~
1 c 100
place~
if (%cmd% == place)
  if (%self.carried_by%)
    %send% %actor% The bait box needs to be on the ground.
  else
    if (%arg% == rabbit)
      if (%actor.inventory(5300)%)
        set bait %actor.inventory(5300)%
        %purge% %bait%
        eval num %random.5%
        eval i 1
        while %i% <= %num%
          set bottom 5317
          set range 40
          set room %%random.%range%%%
          eval room %bottom% + %range%
          %at% %room% %load% mob 5300
          eval i %i% + 1
        done
          switch %num%
            case 1
              %send% %actor% You feel like the bait is working poorly.
              break
            case 2
              %send% %actor% You feel like the bait is working alright.
              break
            case 5
              %send% %actor% You feel like the bait is working great!
              break
            default
              %send% %actor% You feel like the bait is working fine.
              break
          done
        else
          %send% %actor% You do not even have any rabbit bait!
        end
    elseif (%arg% == moose)
      if (%actor.inventory(5301)%)
        set bait %actor.inventory(5301)%
        %purge% %bait%
        eval num %random.5%
        eval i 1
        while %i% <= %num%
          set bottom 5357
          set range 40
          set room %%random.%range%%%
          eval room %bottom% + %range%
          %at% %room% %load% mob 5301
          eval i %i% + 1
        done
          switch %num%
            case 1
              %send% %actor% You feel like the bait is working poorly.
              break
            case 2
              %send% %actor% You feel like the bait is working alright.
              break
            case 5
              %send% %actor% You feel like the bait is working great!
              break
            default
              %send% %actor% You feel like the bait is working fine.
              break
          done
        else
          %send% %actor% You do not even have any moose bait!
        end
    elseif (%arg% == bear)
      if (%actor.inventory(5302)%)
        set bait %actor.inventory(5302)%
        %purge% %bait%
        eval num %random.5%
        eval i 1
        while %i% <= %num%
          set bottom 5345
          set range 30
          set room %%random.%range%%%
          eval room %bottom% + %range%
          %at% %room% %load% mob 5302
          eval i %i% + 1
        done
          switch %num%
            case 1
              %send% %actor% You feel like the bait is working poorly.
              break
            case 2
              %send% %actor% You feel like the bait is working alright.
              break
            case 5
              %send% %actor% You feel like the bait is working great!
              break
            default
              %send% %actor% You feel like the bait is working fine.
              break
          done
        else
          %send% %actor% You do not even have any bear bait!
        end
    elseif (%arg% == crocodile)
      if (%actor.inventory(5303)%)
        set bait %actor.inventory(5303)%
        %purge% %bait%
        eval num %random.5%
        eval i 1
        while %i% <= %num%
          set bottom 5418
          set range 30
          set room %%random.%range%%%
          eval room %bottom% + %range%
          %at% %room% %load% mob 5303
          eval i %i% + 1
        done
          switch %num%
            case 1
              %send% %actor% You feel like the bait is working poorly.
              break
            case 2
              %send% %actor% You feel like the bait is working alright.
              break
            case 5
              %send% %actor% You feel like the bait is working great!
              break
            default
              %send% %actor% You feel like the bait is working fine.
              break
          done
        else
          %send% %actor% You do not even have any crocodile bait!
        end
      else
        %send% %actor% You want to place what bait?
      end
   end
end
~
#5305
Rabbit flee~
0 ab 10
~
wait 30 s
emote flees into the underbrush.
%purge% %self%
~
#5306
Hunting Lodge Greet~
0 g 100
~
wait 2 s
say If you want to leave the Hunting Preserve, just say leave and I'll teleport you out.
~
#5309
N/A~
1 c 3
shoot~
N/A
~
#5392
Hunting Preserve Leave Trigger (Mute)~
0 c 100
leave~
say Ok then, one way trip out of the Hunting Preserve coming up! 
wait 2 s 
emote taps a button on his tablet device. 
wait 1s 
%send% %actor% A bright flash appears around you before suddenly whisking you away 
%echoaround% %actor% A bright flash appears around %actor.name% before suddenly whisking them away. 
wait 1 s 
%teleport% %actor% 5301 
wait 2 s 
%at% 5301 %force% %actor% look
~
$~
