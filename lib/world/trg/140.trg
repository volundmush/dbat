#14000
Vigor Orb~
1 c 3
meld~
if (!%arg%)
 wait 1
 %send% %actor% Meld it with who?
elseif (%arg% == %actor.name% || %arg% == self)
  wait 1
  %send% %actor% You take the %self.shortdesc% and meld it into your skin.
  %echoaround% %actor% %actor.name% takes the %self.shortdesc% and melds it into %actor.hisher% skin.
  wait 1 s
  eval num %random.3%
  switch (%num%)
   case 1
    %send% %actor% You didn't manage to sync with the orb and it is wasted.
    %purge% %self%
    break
   case 2
    eval numb %actor.maxmove% / 10
     if (%actor.move% + %numb% > %actor.maxmove%)
      eval diff %actor.maxmove% - %actor.move%
      set %actor.move(%diff%)%
      %send% %actor% You sync with the orb and completely refill your stamina!
      %purge% %self%
     else
      set %actor.move(%numb%)%
      %send% %actor% You sync with the orb and restore %numb% stamina!
      %purge% %self%
     end
    break
   case 3
    eval numb %actor.maxmove% / 5
     if (%actor.move% + %numb% > %actor.maxmove%)
      eval diff %actor.maxmove% - %actor.move%
      set %actor.move(%diff%)%
      %send% %actor% You sync with the orb and completely refill your stamina!
      %purge% %self%
     else
      set %actor.move(%numb%)%
      %send% %actor% You sync with the orb and restore %numb% stamina!
      %purge% %self%
     end
    break
   default
    eval numb %actor.maxmove% / 10
     if (%actor.move% + %numb% > %actor.maxmove%)
      eval diff %actor.maxmove% - %actor.move%
      set %actor.move(%diff%)%
      %send% %actor% You sync with the orb and completely refill your stamina!
      %purge% %self%
     else
      set %actor.move(%numb%)%
      %send% %actor% You sync with the orb and restore %numb% stamina!
      %purge% %self%
     end
    break
  done
else
 wait 1
 set target_char %actor.room.people%
 eval found 0
 wait 1
  while %target_char%
     set tmp_target %target_char.next_in_room%
     if (%target_char.alias% /= %arg% && !%target_char.is_pc%)
       %send% %actor% Why would you waste it on THAT?
       eval found 1
       set target_char %tmp_target%
     elseif (%target_char.name% /= %arg%)
       wait 1
       %send% %target_char% %actor.name% takes the %self.shortdesc% and melds it into your skin.
       %echoaround% %target_char% %target_char.name% has %self.shortdesc% melded into %target_char.hisher% skin.
       wait 1 s
       eval num %random.3%
       switch (%num%)
        case 1
         %send% %target_char% You didn't manage to sync with the orb and it is wasted.
         %purge% %self%
         break
        case 2
         eval numb %target_char.maxmove% / 10
         if (%target_char.move% + %numb% > %target_char.maxmove%)
          eval diff %target_char.maxmove% - %target_char.move%
          set %target_char.move(%diff%)%
          %send% %target_char% You sync with the orb and completely refill your stamina!
          %purge% %self%
         else
          set %target_char.move(%numb%)%
          %send% %target_char% You sync with the orb and restore %numb% stamina!
          %purge% %self%
         end
        break
        case 3
         eval numb %target_char.maxmove% / 5
         if (%target_char.move% + %numb% > %target_char.maxmove%)
          eval diff %target_char.maxmove% - %target_char.move%
          set %target_char.move(%diff%)%
          %send% %target_char% You sync with the orb and completely refill your stamina!
          %purge% %self%
         else
          set %target_char.move(%numb%)%
          %send% %target_char% You sync with the orb and restore %numb% stamina!
          %purge% %self%
         end
        break
        default
         eval numb %target_char.maxmove% / 10
         if (%target_char.move% + %numb% > %target_char.maxmove%)
          eval diff %target_char.maxmove% - %target_char.move%
          set %target_char.move(%diff%)%
          %send% %target_char% You sync with the orb and completely refill your stamina!
          %purge% %self%
         else
          set %target_char.move(%numb%)%
          %send% %target_char% You sync with the orb and restore %numb% stamina!
          %purge% %self%
         end
        break
       done
     else
      set target_char %tmp_target%
     end
  done
 wait 1
 if (%found% == 0)
  %send% %actor% That person isn't here.
 end
end
~
#14001
Desmon Greet~
0 g 100
~
if (%actor.varexists(dsmith)% && !%self.varexists(smithing)% && !%self.varexists(smithed)%)
 wait 1 s
 say Ah %actor.name% I owe you a refund, I lost the ore you gave me...
 wait 3 s
 give 15000 zenni %actor.name%
 wait 3 s
 say My deepest apologies.
 rdelete dsmith %actor.id%
elseif (%actor.varexists(dsmith)% && %self.varexists(smithing)%)
 if (%actor.id% == %self.smithing%)
  wait 1 s
  say I am still working on your piece %actor.name% I will tell you when it is finished.
  wait 3 s
  say Feel free to go about your business some where else until I notify you.
 else
  wait 1 s
  say I am working on someone else's piece right now, %actor.name%.
  wait 2 s
  say Come back later and I will talk to you about your piece.
 end
elseif (%actor.varexists(dsmith)% && %self.varexists(smithed)%)
 if (%actor.id% == %self.smithed%)
  wait 1 s
  say Ah, %actor.name%, your piece is finished.
  wait 2 s
  %echo% %self.name% pulls a bracer out of a soot covered chest.
  wait 1 s
  %load% obj 14017
  give bracer %actor.name%
  rdelete dsmith %actor.id%
  rdelete smithed %self.id%
  wait 2 s
  say Thanks for your business.
 else
  wait 1 s
  say Ah %actor.name% I owe you a refund, I lost the ore you gave me...
  wait 3 s
  give 15000 zenni %actor.name%
  wait 3 s
  say My deepest apologies.
  rdelete dsmith %actor.id%
 end
elseif (%self.varexists(smithing)%)
 wait 1 s
 %echo% %self.name% hammers a piece of metal.
 wait 2 s
 %send% %actor% %self.name% looks up at you.
 %echoaround% %actor% %self.name% looks up at %actor.name%.
 wait 2 s
 say I am busy right now, so I won't be taking any orders. Check back later.
 wait 2 s
 %echo% %self.name% resumes his work on a bracer.
elseif (!%self.varexists(smithed)%)
 wait 1 s
 %send% %actor% %self.name% stops what he is doing and looks at you.
 %echoaround% %actor% %self.name% stops what he is doing and looks at %actor.name%.
 wait 2 s
 say Ah if you are looking to have something smithed I only work in Fangol ore.
 wait 3 s
 say It is a rare type of metal only found on this planet.
 wait 3 s
 say Give me some of it and then pay me 15,000 zenni.
 wait 4 s
 say I will then begin work on the piece.
 wait 3 s
 say What piece? Who knows, I like to be creative.
 wait 3 s
 say If that isn't a good enough answer, find another smith.
end
~
#14002
Desmon Receive~
0 j 100
~
if (%object.vnum% != 14016)
 wait 1
 say That isn't the ore I work with.
 wait 2 s
 say I only work with Fangol ore.
elseif (!%actor.varexists(paid)%)
 wait 1
 say Alright now give me the money.
 set ore
 remote ore %actor.id%
 wait 1 s
 say 15,000 zenni.
else
 wait 1
 say Alright, I will get started on your piece.
 set dsmith
 remote dsmith %actor.id%
 wait 2 s
 say You can go do something else, it will take about 2 hours.
 wait 4 s
 say I will notify you when the piece is done.
 if (%time.hour% <= 21)
  eval tme %time.hour% + 2
 elseif (%time.hour% == 22)
  eval tme 0
 elseif (%time.hour% == 23)
  eval tme 1
 end
 set smithc %tme%
 remote smithc %self.id%
 set smithing %actor.id%
 remote smithing %self.id%
 set workfor %actor.name%
 remote workfor %self.id%
end
~
#14003
Desmon Bribe~
0 m 1
~
if (%amount% < 15000)
 return 0
 wait 1
 say That isn't enough, don't try and cheat me.
elseif (!%actor.varexists(ore)%)
 wait 1
 say Alright now give me the ore.
 set paid
 remote paid %actor.id%
else
 wait 1
 say Alright, I will get started on your piece.
 set dsmith
 remote dsmith %actor.id%
 wait 2 s
 say You can go do something else, it will take about 2 hours.
 wait 4 s
 say I will notify you when the piece is done.
 if (%time.hour% <= 21)
  eval tme %time.hour% + 2
 elseif (%time.hour% == 22)
  eval tme 0
 elseif (%time.hour% == 23)
  eval tme 1
 end
 set smithc %tme%
 remote smithc %self.id%
 set smithing %actor.id%
 remote smithing %self.id%
 set workfor %actor.name%
 remote workfor %self.id%
end
~
#14004
Desmon Timer~
0 ab 100
~
if (%self.varexists(smithing)%)
 if (%self.smithc% == %time.hour%)
  rdelete smithc %self.id%
  tell %self.workfor% I have finished my work, come and pay me to receive the product.
  set smithed %self.smithing%
  rdelete smithing %self.id%
  redelete workfor %self.id%
  remote smithed %self.id%
 end
end
~
#14005
Augment circuits~
1 c 3
augment~
 %echoaround% %actor% %actor.name% augments their circuits.
 eval gain %actor.maxmana% / 500
 if (%actor.maxmana% + %gain% < %actor.maxmana%)
  %send% %actor% ERROR!
 else
  set %actor.maxmana(%gain%)%
  %send% %actor% You augment your circuits @D[@G+%gain%@D]@n
 end
~
#14042
Dimensional Prism~
1 c 3
invoke~
if (%actor.room.vnum% < 19800 || %actor.room.vnum% > 19899)
 if (%self.weight% <= 1)
  nop %self.weight(0)%
  nop %self.weight(%actor.room.vnum%)%
  eval rm 19799
  eval finished 0
  while (%finished% != 1)
   eval rm %rm% + 1
   if (%rm% < 19900)
    %teleport% %actor% %rm%
    if (!%actor.next_in_room%)
     eval finished 1
    end
    %teleport% %actor% %self.weight%
   else
    eval finished 1
    eval rm 19899
   end
  done
  wait 1
  %send% %actor% As you invoke the powers of the prism, arcs of energy shoot off of it and pool into a vortex above your head! The vortex immediately sucks you into it and teleports you to a pocket dimension!
  %echoaround% %actor% As %actor.name% invokes the powers of a prism in %actor.hisher% hand, arcs of energy shoot off of it and pool into a vortex above %actor.hisher% head! The vortex immediately sucks %actor.himher% into it!
  %teleport% %actor% %rm%
 else
  %send% %actor% The prism melts away and then evaporates!
  %purge% self
 end
else
 if (%self.weight% <= 1)
  wait 1
  %send% %actor% As you invoke the powers of the prism, arcs of energy shoot off of it and pool into a vortex above your head! The vortex immediately sucks you into it and teleports you back to a random location!
  eval num %random.5%
  switch (%num%)
   case 1
     %teleport% %actor% 300
    break
   case 2
     %teleport% %actor% 4250
    break
   case 3
     %teleport% %actor% 1700
    break
   case 4
     %teleport% %actor% 2200
    break
   case 5
    %teleport% %actor% 11300
    break
  done
  %send% %actor% The prism melts away and then evaporates!
  %purge% self
 else
  %send% %actor% As you invoke the powers of the prism, arcs of energy shoot off of it and pool into a vortex above your head! The vortex immediately sucks you into it and teleports you back to where you had been prior to first using the prism!
  %teleport% %actor% %self.weight%
  eval wgt %self.weight%
  nop %self.weight(0)%
  nop %self.weight(1)%
  %send% %actor% The prism melts away and then evaporates!
  %purge% self
 end
end
~
#14043
Kachin Capsule~
1 c 3
click~
if (%self.name% /= %arg%)
%send% %actor% You click %self.name% then throw it on the ground.
%echoaround% %actor% %actor.name% clicks %self.name% then throws it on the ground.
wait 2 s
  %echo% A cloud of smoke surrounds the area.
%load% obj 87
 wait 2 s
%echo% The smoke clears, revealing the capsule's contents and a very large crater in the ground.
%purge% self
else
return 0
end
~
$~
