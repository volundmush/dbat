#500
Park Officer~
0 g 100
~
if (%actor.is_pc%)
wait 1 sec
say Hey hold on a sec'
wait 2 sec
say Have you seen any suspicious behavior around the park lately?
wait 4 sec
say Well that thief has made off with a Blue Gem that is quite important to the mayor of the city.
wait 4 sec
say If you happen to find it, there is a reward for returning it. Bring it to me to collect.
wait 8 sec
say I wonder where that damn thief is hiding....
end
~
#501
Thief Trigger~
0 g 100
~
wait 1 sec
say Hey! What are you doing back here!?
wait 3 sec
say This is my territory, you get the hell out of here if you know what is good for you.
~
#502
THief Trigger 2~
0 b 15
~
wait 1 sec
say What are YOU doing here still!?
wait 3 sec
say You better get out of here while my patience lasts...
wait 1 sec
%echo% Veskas counts a stack of zenni sitting in his lap.
~
#503
Thief Trigger 3~
0 k 15
~
wait 1 sec
say You bastard! You can't take me down!
wait 1 sec
%echo% Veskas concentrates and glows green for a moment.
wait 1 sec
%echo% Veskas looks a little healthier!
%self.hitp(20)%
~
#504
Gous Trigger~
0 g 100
~
wait 1 sec
%echo% Gous looks up slowly.
wait 2 sec
say Well now...what brings someone like you to my wonderful abode?
wait 4 sec
say I mean, I know it isn't much but stay as long as you like.
wait 2 sec
say I rarely get visitors down here.
~
#505
Gous Trigger 2~
0 b 15
~
wait 2 sec
%echo% Gous throws a kick into the air.
wait 1 sec
%echo% Gous spins to avoid an imaginary blow.
wait 2 sec
%echo% Gous throws an uppercut at an imaginary opponent.
~
#506
Park Officer Receive~
0 j 100
~
if (%object.vnum% != 500)
 return 0
 wait 1
 say This isn't something I want.
else
 wait 1
 %purge% %object%
 say Ah so you managed to find the stolen item huh?
 wait 2 s
 say Well here is the reward. A hefty 250 zenni.
 wait 1
%actor.gold(250)%
 wait 1
 %send% %actor% You also receive 500 exp!
 set %actor.exp(500)%
 wait 2 s
 say Well I guess I will be going then. I have other things to do.
 %purge% %self%
end
~
#556
Car move~
1 ab 35
~
eval dnum %random.4%
 switch %dnum%
  case 1
   if (%self.room.north%)
    if (%self.room.north(vnum)% == 217 || %self.room.north(vnum)% == 218|| %self.room.north(vnum)% == 219)
     %echo% %self.shortdesc% speeds along the highway.
    else    
     wait 1
     %echo% %self.shortdesc% speeds along the highway to the north.
     ogoto %self.room.north(vnum)%
     %echo% %self.shortdesc% speeds in from the south.
    end
   else
    %echo% %self.shortdesc% speeds along the highway.
   end
   break
    
  case 2
   if (%self.room.east%)
    if (%self.room.east(vnum)% == 224|| %self.room.east(vnum)% == 228 || %self.room.east(vnum)% == 232 || %self.room.east(vnum)% == 236 || %self.room.east(vnum)% == 244)
     %echo% %self.shortdesc% speeds along the highway.
    else
     wait 1
     %echo% %self.shortdesc% speeds along the highway to the east.
     ogoto %self.room.east(vnum)%
     %echo% %self.shortdesc% speeds in from the west.
    end 
   else
    %echo% %self.shortdesc% speeds along the highway.
   end
   break
   
  case 3
   if (%self.room.west%)
    if (%self.room.west(vnum)% == 382 || %self.room.west(vnum)% == 226 || %self.room.west(vnum)% == 230 || %self.room.west(vnum)% == 234%)
     %echo% %self.shortdesc% speeds along the highway.
    else
     wait 1
     %echo% %self.shortdesc% speeds along the highway to the west.
     ogoto %self.room.west(vnum)%
     %echo% %self.shortdesc% speeds in from the east.
    end
   else
    %echo% %self.shortdesc% speeds along the highway.
   end
  break
  
  case 4
   if (%self.room.south%)
    if (%self.room.south(vnum)% == 225 || %self.room.south(vnum)% == 226 || %self.room.south(vnum)% == 253 || %self.room.south(vnum)% == 244)
     %echo% %self.shortdesc% speeds along the highway.
    else
     wait 1 
     %echo% %self.shortdesc% speeds along the highway to the south.
     ogoto %self.room.south(vnum)%
     %echo% %self.shortdesc% speeds in from the north.
    end
   else
    %echo% %self.shortdesc% speeds along the highway.
   end
   break
  done
~
$~
