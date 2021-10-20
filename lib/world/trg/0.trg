#0
Void Trigger~
2 ag 100
~
%echo% blah
~
#1
Void Trigger~
2 a 100
~
%echo% blah
~
#2
Load Dballs~
0 b 100
~
if (%time.month% == 5 && %time.hour% >= 2)
mecho The dballs have been loaded.
eval key2 26
set key 20 21 22 23 24 25 26
    set i 1 
    while %i% < 8 
      extract %i% %i% %key% 
      eval i %i% + 1 
    done 
    while %ii% < 7 
      eval room 0 + %random.4%
      if (%key2% == 26)
      eval key2 %key2% - 1
      %at% %random.4% %load% obj 26
      eval ii %ii% + 1
      elseif (%key2% == 25)
      eval key2 %key2% - 1
      %at% %random.4% %load% obj 25
      eval ii %ii% + 1
      elseif (%key2% == 24)
      eval key2 %key2% - 1
      %at% %random.4% %load% obj 24
      eval ii %ii% + 1
      elseif (%key2% == 23)
      eval key2 %key2% - 1
      %at% %random.4% %load% obj 23
      eval ii %ii% + 1
      elseif (%key2% == 22)
      eval key2 %key2% - 1
      %at% %random.4% %load% obj 22
      eval ii %ii% + 1
      elseif (%key2% == 21)
      eval key2 %key2% - 1
      %at% %random.4% %load% obj 21
      eval ii %ii% + 1
      elseif (%key2% == 20)
      eval key2 %key2% - 1
      %at% %random.4% %load% obj 20
      eval ii %ii% + 1
      end
    done
else
mecho The dballs have not been loaded.
end
~
#3
Test~
0 d 100
time~
mecho The current year: %time.year%, month: %time.month%, day: %time.day%, and hour: %time.hour%
~
#4
What time is it?~
0 b 50
~
mecho %self.name% looks around and sighs.
~
#5
Test trigger~
0 j 100
~
wait 1
say Object Val 3: %object.val3%
~
#7
Samael's Roflcopter~
2 d 100
rofl~
%echo% ROFL-ROFL-LOL-ROFL-ROFL
%echo%           |
%echo%  L    /----------
%echo% LOL---         | \
%echo%  L    \        |__\
%echo%        \___________\
%echo%          |      |
%echo%       --------------/
~
#8
Mud School Quit~
1 c 3
quit~
if (%actor.room.vnum% > 100 && %actor.room.vnum% < 139)
 return 1
 wait 1
 %send% %actor% Leaving the Mud School...
 wait 1
 if (%actor.has_item(61)%)
  %purge% %actor.inventory(61)%
 end
 if (%actor.has_item(108)%)
  %purge% %actor.inventory(108)%
 end
 if (%actor.has_item(4901)%)
  %purge% %actor.inventory(4901)%
 end
 wait 1
 %send% %actor% You return to the beginning of the mud school.
 %teleport% %actor% 100
 wait 1 sec
 %force% %actor% quit
else
 return 0
end
~
#9
Random Assault~
1 ab 2
~
%echo% This trigger commandlist is not complete!
~
#12
Map~
1 c 3
map~
%send% %actor% @W               @D-[@CArea Map@D]-
%send% %actor% @D-------------------------------------------@w
%send% %actor% @WC = City, I = Inside, P = Plain, F = Forest
%send% %actor% @WM = Mountain, H = Hills, S = Sky, W = Water
%send% %actor% @WU = Underwater, $ = Shop, # = Important, and
%send% %actor% @Wlastly @YX@W = You.
%send% %actor% @D-------------------------------------------@w
eval here %actor.room.vnum%
eval north -10
eval east 1
eval south 10
eval west -1
eval up 0
eval down 0
eval inside 0
eval outside 0
eval northwest -11
eval northeast -9
eval southeast 11
eval southwest 9
if %here.north%
  eval 23 %here.north(vnum)%
  eval temp %23.sector%
  eval x23 %temp.charat(1)%
  eval directionlist north east south west northwest northeast southeast southwest
  while %directionlist%
    eval firstdir %directionlist.car%
    eval newroom 23 + %%%firstdir%%%
    eval %newroom% %%23.%firstdir%(vnum)%%
    eval vnumexist %%%newroom%%%
    if %vnumexist%
      eval temp %%%newroom%.sector%%
      eval x%newroom% %temp.charat(1)%
    end
    eval directionlist %directionlist.cdr%
  done
end 
if %here.east%
  eval 34 %here.east(vnum)%
  eval temp %34.sector%
  eval x34 %temp.charat(1)%
  eval directionlist north east south west northwest northeast southeast southwest
  while %directionlist%
    eval firstdir %directionlist.car%
    eval newroom 34 + %%%firstdir%%%
    eval %newroom% %%34.%firstdir%(vnum)%%
    eval vnumexist %%%newroom%%%
    if %vnumexist%
      eval temp %%%newroom%.sector%%
      eval x%newroom% %temp.charat(1)%
    end 
    eval directionlist %directionlist.cdr%
  done 
end 
if %here.south%
  eval 43 %here.south(vnum)%
  eval temp %43.sector%
  eval x43 %temp.charat(1)%
  eval directionlist north east south west northwest northeast southeast southwest
  while %directionlist%
    eval firstdir %directionlist.car%
    eval newroom 43 + %%%firstdir%%%
    eval %newroom% %%43.%firstdir%(vnum)%%
    eval vnumexist %%%newroom%%%
    if %vnumexist%
      eval temp %%%newroom%.sector%%
      eval x%newroom% %temp.charat(1)%
    end 
    eval directionlist %directionlist.cdr%
  done 
end
if %here.west%
  eval 32 %here.west(vnum)%
  eval temp %32.sector%
  eval x32 %temp.charat(1)%
  eval directionlist north east south west northwest northeast southeast southwest
  while %directionlist%
    eval firstdir %directionlist.car%
    eval newroom 32 + %%%firstdir%%%
    eval %newroom% %%32.%firstdir%(vnum)%%
    eval vnumexist %%%newroom%%%
    if %vnumexist%
      eval temp %%%newroom%.sector%%
      eval x%newroom% %temp.charat(1)%
    end 
    eval directionlist %directionlist.cdr%
  done 
end
if %here.northwest%
  eval 22 %here.northwest(vnum)%
  eval temp %22.sector%
  eval x22 %temp.charat(1)%
  eval directionlist north east south west northwest northeast southeast southwest
  while %directionlist%
    eval firstdir %directionlist.car%
    eval newroom 22 + %%%firstdir%%%
    eval %newroom% %%22.%firstdir%(vnum)%%
    eval vnumexist %%%newroom%%%
    if %vnumexist%
      eval temp %%%newroom%.sector%%
      eval x%newroom% %temp.charat(1)%
    end
    eval directionlist %directionlist.cdr%
  done
end
if %here.northeast%
  eval 24 %here.northeast(vnum)%
  eval temp %24.sector%
  eval x24 %temp.charat(1)%
  eval directionlist north east south west northwest northeast southeast southwest
  while %directionlist%
    eval firstdir %directionlist.car%
    eval newroom 24 + %%%firstdir%%%
    eval %newroom% %%24.%firstdir%(vnum)%%
    eval vnumexist %%%newroom%%%
    if %vnumexist%
      eval temp %%%newroom%.sector%%
      eval x%newroom% %temp.charat(1)%
    end
    eval directionlist %directionlist.cdr%
  done
end 
if %here.southeast%
  eval 44 %here.southeast(vnum)%
  eval temp %44.sector%
  eval x44 %temp.charat(1)%
  eval directionlist north east south west northwest northeast southeast southwest
  while %directionlist%
    eval firstdir %directionlist.car%
    eval newroom 44 + %%%firstdir%%%
    eval %newroom% %%44.%firstdir%(vnum)%%
    eval vnumexist %%%newroom%%%
    if %vnumexist%
      eval temp %%%newroom%.sector%%
      eval x%newroom% %temp.charat(1)%
    end
    eval directionlist %directionlist.cdr%
  done
end
if %here.southwest%
  eval 42 %here.southwest(vnum)%
  eval temp %42.sector%
  eval x42 %temp.charat(1)%
  eval directionlist north east south west northwest northeast southeast southwest
  while %directionlist%
    eval firstdir %directionlist.car%
    eval newroom 42 + %%%firstdir%%%
    eval %newroom% %%42.%firstdir%(vnum)%%
    eval vnumexist %%%newroom%%%
    if %vnumexist%
      eval temp %%%newroom%.sector%%
      eval x%newroom% %temp.charat(1)%
    end
    eval directionlist %directionlist.cdr%
  done
end
if %x11% ==  
set x11 -
end
if %x12% ==  
set x12 -
end
if %x13% ==  
set x13 -
end
if %x14% ==  
set x14 -
end
if %x15% ==  
set x15 -
end
if %x21% ==  
set x21 -
end
if %x22% ==  
set x22 -
end
if %x23% ==  
set x23 -
end
if %x24% ==  
set x24 -
end
if %x25% ==  
set x25 -
end
if %x31% ==  
set x31 -
end
if %x32% ==  
set x32 -
end
if %x34% ==  
set x34 -
end
if %x35% ==  
set x35 -
end
if %x41% ==  
set x41 -
end
if %x42% ==  
set x42 -
end
if %x43% ==  
set x43 -
end
if %x44% ==  
set x44 -
end
if %x45% ==  
set x45 -
end
if %x51% ==  
set x51 -
end
if %x52% ==  
set x52 -
end
if %x53% ==  
set x53 -
end
if %x54% ==  
set x54 -
end
if %x55% ==  
set x55 -
end
eval x33 @YX@w
%send% %actor% @w                %x11% %x12% %x13% %x14% %x15%
%send% %actor% @w                %x21% %x22% %x23% %x24% %x25%
%send% %actor% @w                %x31% %x32% %x33% %x34% %x35%
%send% %actor% @w                %x41% %x42% %x43% %x44% %x45%
%send% %actor% @w                %x51% %x52% %x53% %x54% %x55%@n
~
#13
Blackjack Deal~
2 d 100
deal~
if (%actor.varexists(handvalue)%)
wait 1
%send% %actor% You are already playing! Say @Cfold@n to stop playing.
elseif (%actor.gold% < 5)
wait 1
%send% %actor% Minimum bet is 5 zenni.
elseif (%speech.cdr% > 25)
wait 1
%send% %actor% You can not have an opening bet higher than 25 zenni.
elseif (%speech.cdr% && %speech.cdr% < 5)
wait 1
%send% %actor% Minimum bet is 5 zenni.
elseif (%actor.gold% < %speech.cdr%)
wait 1
%send% %actor% You do not have enough zenni.
else
 if (%speech.cdr% > 0)
  wait 1
  %send% %actor% You bet %speech.cdr% on the hand.
  set %actor.gold(-%speech.cdr%)%
  set bet %speech.cdr%
 else
  set %actor.gold(-5)% 
  set bet 5
 end
%echoaround% %actor% The dealer deals %actor.name% a face down card.
remote bet %actor.id%
set acehandvalue 0 
set cace @DCA@n 
set sace @DSA@n 
set dace @RDA@n 
set hace @RHA@n 
rdelete card1 %actor.id% 
rdelete card1_2 %actor.id% 
rdelete card2 %actor.id% 
rdelete card2_2 %actor.id% 
rdelete card3 %actor.id% 
rdelete card4 %actor.id% 
rdelete card5 %actor.id% 
rdelete handvalue %actor.id% 
rdelete acehandvalue %actor.id% 
rdelete handvalue1 %actor.id% 
rdelete acehandvalue1 %actor.id% 
rdelete handvalue2 %actor.id% 
rdelete acehandvayylue2 %actor.id% 
set cardarray[1] @DCA@n
set cardarray[2] @DC2@n
set cardarray[3] @DC3@n
set cardarray[4] @DC4@n
set cardarray[5] @DC5@n
set cardarray[6] @DC6@n
set cardarray[7] @DC7@n
set cardarray[8] @DC8@n
set cardarray[9] @DC9@n
set cardarray[10] @DC10@n
set cardarray[11] @DCJ@n
set cardarray[12] @DCQ@n
set cardarray[13] @DCK@n
set cardarray[14] @RDA@n
set cardarray[15] @RD2@n
set cardarray[16] @RD3@n
set cardarray[17] @RD4@n
set cardarray[18] @RD5@n
set cardarray[19] @RD6@n
set cardarray[20] @RD7@n
set cardarray[21] @RD8@n
set cardarray[22] @RD9@n
set cardarray[23] @RD10@n
set cardarray[24] @RDJ@n
set cardarray[25] @RDQ@n
set cardarray[26] @RDK@n
set cardarray[27] @DSA@n
set cardarray[28] @DS2@n
set cardarray[29] @DS3@n
set cardarray[30] @DS4@n
set cardarray[31] @DS5@n
set cardarray[32] @DS6@n
set cardarray[33] @DS7@n
set cardarray[34] @DS8@n
set cardarray[35] @DS9@n
set cardarray[36] @DS10@n
set cardarray[37] @DSJ@n
set cardarray[38] @DSQ@n
set cardarray[39] @DSK@n
set cardarray[40] @RHA@n
set cardarray[41] @RH2@n
set cardarray[42] @RH3@n
set cardarray[43] @RH4@n
set cardarray[44] @RH5@n
set cardarray[45] @RH6@n
set cardarray[46] @RH7@n
set cardarray[47] @RH8@n
set cardarray[48] @RH9@n
set cardarray[49] @RH10@n
set cardarray[50] @RHJ@n
set cardarray[51] @RHQ@n
set cardarray[52] @RHK@n
eval cnum %random.52%
set card1 %%cardarray[%cnum%]%%
eval card1 %card1%
eval num1 %card1.strlen% 
if (%card1% == @DCJ@n || %card1% == @DCQ@n || %card1% == @DCK@n || %card1% == @RHJ@n || %card1% == @RHQ@n || %card1% == @RHK@n || %card1% == @RDJ@n || %card1% == @RDQ@n || %card1% == @RDK@n || %card1% == @DSJ@n || %card1% == @DSQ@n || %card1% == @DSK@n)
 eval cnum %random.52%
 set card1 %%cardarray[%cnum%]%%
 eval card1 %card1%
 eval num1 %card1.strlen%
end
if (%card1% == @DCJ@n || %card1% == @DCQ@n || %card1% == @DCK@n || %card1% == @RHJ@n || %card1% == @RHQ@n || %card1% == @RHK@n || %card1% == @RDJ@n || %card1% == @RDQ@n || %card1% == @RDK@n || %card1% == @DSJ@n || %card1% == @DSQ@n || %card1% == @DSK@n)
 set value1 10 
elseif (%card1% == @DSA@n || %card1% == @DCA@n || %card1% == @RHA@n || %card1% == @RDA@n)
 set value1 1 
elseif (%card1% /= 2)
 set value1 2
elseif (%card1% /= 3)
 set value1 3
elseif (%card1% /= 4)
 set value1 4
elseif (%card1% /= 5)
 set value1 5
elseif (%card1% /= 6)
 set value1 6
elseif (%card1% /= 7)
 set value1 7
elseif (%card1% /= 8)
 set value1 8
elseif (%card1% /= 9)
 set value1 9
elseif (%card1% /= 10)
 set value1 10
end 
set handvalue %value1% 
if %card1% == %cace% || %card1% == %sace% || %card1% == %dace% || %card1% == %hace% 
eval acehandvalue %handvalue% + 10 
else 
set acehandvalue %handvalue% 
end 
eval cnum %random.52%
set card2 %%cardarray[%cnum%]%%
eval card2 %card2%
eval num2 %card2.strlen%
if (%card2% == @DCJ@n || %card2% == @DCQ@n || %card2% == @DCK@n || %card2% == @RHJ@n || %card2% == @RHQ@n || %card2% == @RHK@n || %card2% == @RDJ@n || %card2% == @RDQ@n || %card2% == @RDK@n || %card2% == @DSJ@n || %card2% == @DSQ@n || %card2% == @DSK@n)
 eval cnum %random.52%
 set card2 %%cardarray[%cnum%]%%
 eval card2 %card2%
 eval num1 %card2.strlen%
end
if (%card2% == @DCJ@n || %card2% == @DCQ@n || %card2% == @DCK@n || %card2% == @RHJ@n || %card2% == @RHQ@n || %card2% == @RHK@n || %card2% == @RDJ@n || %card2% == @RDQ@n || %card2% == @RDK@n || %card2% == @DSJ@n || %card2% == @DSQ@n || %card2% == @DSK@n)
 set value2 10
elseif (%card2% == @DSA@n || %card2% == @DCA@n || %card2% == @RHA@n || %card2% == @RDA@n)
 set value2 1
elseif (%card2% /= 2)
 set value2 2
elseif (%card2% /= 3)
 set value2 3
elseif (%card2% /= 4)
 set value2 4
elseif (%card2% /= 5)
 set value2 5
elseif (%card2% /= 6)
 set value2 6
elseif (%card2% /= 7)
 set value2 7
elseif (%card2% /= 8)
 set value2 8
elseif (%card2% /= 9)
 set value2 9
elseif (%card2% /= 10)
 set value2 10
end
eval handvalue %handvalue% + %value2% 
if %card2% == %cace% || %card2% == %sace% || %card2% == %dace% || %card2% == %hace% && %card1% != %cace% && %card1% != %sace% && %card1% != %dace% && %card1% != %hace% 
eval acehandvalue %handvalue% + 10 
else 
eval acehandvalue %acehandvalue% + %value2% 
end 
if %acehandvalue% != %handvalue% && %acehandvalue% > 0 
 if (%acehandvalue% < 22)
 %send% %actor% You have been dealt: %card1% %card2% Hand Value: <%handvalue%> Alt Ace Hand V.: <%acehandvalue%>
 else
 %send% %actor% You have been dealt: %card1% %card2% Hand Value: <%handvalue%>
 end
elseif %acehandvalue% == %handvalue% 
%send% %actor% You have been dealt: %card1% %card2% Hand Value: <%handvalue%> 
end 
%echoaround% %actor% The dealer deals %actor.name% a %card2%.
remote card1 %actor.id% 
remote card2 %actor.id% 
remote handvalue %actor.id% 
remote acehandvalue %actor.id% 
if %handvalue% == 21 
%send% %actor% @YYou have 21!@n 
rdelete card1 %actor.id% 
rdelete card2 %actor.id% 
rdelete card3 %actor.id% 
rdelete card4 %actor.id% 
rdelete card5 %actor.id% 
rdelete handvalue %actor.id% 
rdelete acehandvalue %actor.id%
eval prize %actor.bet% * 2
rdelete bet %actor.id%
%send% %actor% You win %prize% zenni!
set %actor.gold(%prize%)%
elseif %acehandvalue% == 21 
%send% %actor% @YYou have BLACKJACK!!@n 
%echoaround% %actor% %actor.name% has a BLACKJACK!!
rdelete card1 %actor.id% 
rdelete card2 %actor.id% 
rdelete card3 %actor.id% 
rdelete card4 %actor.id% 
rdelete card5 %actor.id% 
rdelete handvalue %actor.id% 
rdelete acehandvalue %actor.id% 
eval prize %actor.bet% * 3
rdelete bet %actor.id%
%send% %actor% You win %prize% zenni!
set %actor.gold(%prize%)%
else 
%send% %actor% Would you like to "hit", "stand", "raise", "hand", or "fold"? 
end
end
~
#14
Blackjack Hit~
2 d 100
hit~
unset card3 
unset card4 
unset card5 
unset card1 
unset card2 
unset handvalue 
set acehandvalue 0 
set cace @DCA@n
set sace @DSA@n
set dace @RDA@n
set hace @RHA@n
set cardarray[1] @DCA@n
set cardarray[2] @DC2@n
set cardarray[3] @DC3@n
set cardarray[4] @DC4@n
set cardarray[5] @DC5@n
set cardarray[6] @DC6@n
set cardarray[7] @DC7@n
set cardarray[8] @DC8@n
set cardarray[9] @DC9@n
set cardarray[10] @DC10@n
set cardarray[11] @DCJ@n
set cardarray[12] @DCQ@n
set cardarray[13] @DCK@n
set cardarray[14] @RDA@n
set cardarray[15] @RD2@n
set cardarray[16] @RD3@n
set cardarray[17] @RD4@n
set cardarray[18] @RD5@n
set cardarray[19] @RD6@n
set cardarray[20] @RD7@n
set cardarray[21] @RD8@n
set cardarray[22] @RD9@n
set cardarray[23] @RD10@n
set cardarray[24] @RDJ@n
set cardarray[25] @RDQ@n
set cardarray[26] @RDK@n
set cardarray[27] @DSA@n
set cardarray[28] @DS2@n
set cardarray[29] @DS3@n
set cardarray[30] @DS4@n
set cardarray[31] @DS5@n
set cardarray[32] @DS6@n
set cardarray[33] @DS7@n
set cardarray[34] @DS8@n
set cardarray[35] @DS9@n
set cardarray[36] @DS10@n
set cardarray[37] @DSJ@n
set cardarray[38] @DSQ@n
set cardarray[39] @DSK@n
set cardarray[40] @RHA@n
set cardarray[41] @RH2@n
set cardarray[42] @RH3@n
set cardarray[43] @RH4@n
set cardarray[44] @RH5@n
set cardarray[45] @RH6@n
set cardarray[46] @RH7@n
set cardarray[47] @RH8@n
set cardarray[48] @RH9@n
set cardarray[49] @RH10@n
set cardarray[50] @RHJ@n
set cardarray[51] @RHQ@n
set cardarray[52] @RHK@n
if %actor.varexists(card1)% && %actor.varexists(card2)% && %actor.varexists(handvalue)% && %actor.varexists(acehandvalue)% && !%actor.varexists(card3) %&& !%actor.varexists(card4)% && !%actor.varexists(card5)% 
set card1 %actor.card1% 
set card2 %actor.card2% 
set handvalue %actor.handvalue% 
set acehandvalue %actor.acehandvalue% 
eval cnum %random.52%
set card3 %%cardarray[%cnum%]%%
eval card3 %card3%
eval num3 %card3.strlen%
if (%card3% == @DCJ@n || %card3% == @DCQ@n || %card3% == @DCK@n || %card3% == @RHJ@n || %card3% == @RHQ@n || %card3% == @RHK@n || %card3% == @RDJ@n || %card3% == @RDQ@n || %card3% == @RDK@n || %card3% == @DSJ@n || %card3% == @DSQ@n || %card3% == @DSK@n)
 set value3 10
elseif (%card3% == @DSA@n || %card3% == @DCA@n || %card3% == @RHA@n || %card3% == @RDA@n)
 set value3 1
elseif (%card3% /= 2)
 set value3 2
elseif (%card3% /= 3)
 set value3 3
elseif (%card3% /= 4)
 set value3 4
elseif (%card3% /= 5)
 set value3 5
elseif (%card3% /= 6)
 set value3 6
elseif (%card3% /= 7)
 set value3 7
elseif (%card3% /= 8)
 set value3 8
elseif (%card3% /= 9)
 set value3 9
elseif (%card3% /= 10)
 set value3 10
end
eval handvalue %handvalue% + %value3% 
if %card3% == %cace% || %card3% == %sace% || %card3% == %dace% || %card3% == %hace% && %card1% != %cace% && %card1% != %sace% && %card1% != %dace% && %card1% != %hace% && %card2% != %cace% && %card2% != %sace% && %card2% != %dace% && %card2% != %hace% 
eval acehandvalue %handvalue% + 10 
else 
eval acehandvalue %acehandvalue% + %value3% 
end 
if %acehandvalue% != %handvalue% && %acehandvalue% > 0 
 if (%acehandvalue% < 22)
 %send% %actor% You have been dealt: %card3% Hand Value: <%handvalue%> Alt Ace Hand V.: <%acehandvalue%>
 else
 %send% %actor% You have been dealt: %card3% Hand Value: <%handvalue%>
 end
elseif %acehandvalue% == %handvalue% 
%send% %actor% You have been dealt: %card3% Hand Value: <%handvalue%> 
end 
%echoaround% %actor% The dealer deals %actor.name% a %card3%.
remote card3 %actor.id% 
remote handvalue %actor.id% 
remote acehandvalue %actor.id% 
if %handvalue% == 21 
%send% %actor% @YYou have 21!!@n 
%send% %actor% You better stand...
elseif %acehandvalue% == 21 
%send% %actor% @YYou have 21!!@n 
%send% %actor% You better stand...
elseif %handvalue% > 21 
%send% %actor% @RYou have gone bust!!@n 
%echoaround% %actor% %actor.name% has busted.
rdelete card1 %actor.id% 
rdelete card2 %actor.id% 
rdelete card3 %actor.id% 
rdelete handvalue %actor.id% 
rdelete acehandvalue %actor.id%
eval prize %actor.bet%
rdelete bet %actor.id%
rdelete raise %actor.id%
%send% %actor% You lose %prize% zenni!
else 
%send% %actor% Would you like to "hit", "stand", "raise", "hand", or "fold"?
end 
elseif %actor.varexists(card1)% && %actor.varexists(card2)% && %actor.varexists(handvalue)% && %actor.varexists(acehandvalue)% && %actor.varexists(card3) %&& !%actor.varexists(card4)% && !%actor.varexists(card5)% 
set card1 %actor.card1% 
set card2 %actor.card2% 
set card3 %actor.card3% 
set handvalue %actor.handvalue% 
set acehandvalue %actor.acehandvalue% 
eval cnum %random.52%
set card4 %%cardarray[%cnum%]%%
eval card4 %card4%
eval num4 %card4.strlen%
if (%card4% == @DCJ@n || %card4% == @DCQ@n || %card4% == @DCK@n || %card4% == @RHJ@n || %card4% == @RHQ@n || %card4% == @RHK@n || %card4% == @RDJ@n || %card4% == @RDQ@n || %card4% == @RDK@n || %card4% == @DSJ@n || %card4% == @DSQ@n || %card4% == @DSK@n)
 set value4 10
elseif (%card4% == @DSA@n || %card4% == @DCA@n || %card4% == @RHA@n || %card4% == @RDA@n)
 set value4 1
elseif (%card4% /= 2)
 set value4 2
elseif (%card4% /= 3)
 set value4 3
elseif (%card4% /= 4)
 set value4 4
elseif (%card4% /= 5)
 set value4 5
elseif (%card4% /= 6)
 set value4 6
elseif (%card4% /= 7)
 set value4 7
elseif (%card4% /= 8)
 set value4 8
elseif (%card4% /= 9)
 set value4 9
elseif (%card4% /= 10)
 set value4 10
end
eval handvalue %handvalue% + %value4% 
if %card4% == %cace% || %card4% == %sace% || %card4% == %dace% || %card4% == %hace% && %card1% != %cace% && %card1% != %sace% && %card1% != %dace% && %card1% != %hace% && %card2% != %cace% && %card2% != %sace% && %card2% != %dace% && %card2% != %hace% && %card3% != %cace% && %card3% != %sace% && %card3% != %dace% && %card3% != %hace% 
eval acehandvalue %handvalue% + 10 
else 
eval acehandvalue %acehandvalue% + %value4% 
end 
if %acehandvalue% != %handvalue% && %acehandvalue% > 0 
 if (%acehandvalue% < 22)
 %send% %actor% You have been dealt: %card4% Hand Value: <%handvalue%> Alt Ace Hand V.: <%acehandvalue%>
 else
 %send% %actor% You have been dealt: %card4% Hand Value: <%handvalue%>
 end
elseif %acehandvalue% == %handvalue% 
%send% %actor% You have been dealt: %card4% Hand Value: <%handvalue%> 
end
%echoaround% %actor% The dealer deals %actor.name% a %card4%.
remote card4 %actor.id% 
remote handvalue %actor.id% 
remote acehandvalue %actor.id% 
if %handvalue% == 21 
%send% %actor% @YYou have 21!!@n 
%send% %actor% You better stand...
elseif %acehandvalue% == 21 
%send% %actor% @YYou have 21!!@n 
%send% %actor% You better stand...
elseif %handvalue% > 21 
%send% %actor% @RYou have gone bust!!@n 
%echoaround% %actor% %actor.name% has busted.
rdelete card1 %actor.id% 
rdelete card2 %actor.id% 
rdelete card3 %actor.id% 
rdelete card4 %actor.id% 
rdelete handvalue %actor.id% 
rdelete acehandvalue %actor.id%
eval prize %actor.bet%
rdelete bet %actor.id%
rdelete raise %actor.id%
%send% %actor% You lose %prize% zenni!
else 
%send% %actor% Would you like to "hit", "stand", "raise", "hand", or "fold"?
end 
elseif %actor.varexists(card1)% && %actor.varexists(card2)% && %actor.varexists(handvalue)% && %actor.varexists(acehandvalue)% && %actor.varexists(card3) %&& %actor.varexists(card4)% && !%actor.varexists(card5)% 
set card1 %actor.card1% 
set card2 %actor.card2% 
set card3 %actor.card3% 
set card4 %actor.card4% 
set handvalue %actor.handvalue% 
set acehandvalue %actor.acehandvalue% 
eval cnum %random.52%
set card5 %%cardarray[%cnum%]%%
eval card5 %card5%
eval num5 %card5.strlen%
if (%card5% == @DCJ@n || %card5% == @DCQ@n || %card5% == @DCK@n || %card5% == @RHJ@n || %card5% == @RHQ@n || %card5% == @RHK@n || %card5% == @RDJ@n || %card5% == @RDQ@n || %card5% == @RDK@n || %card5% == @DSJ@n || %card5% == @DSQ@n || %card5% == @DSK@n)
 set value5 10
elseif (%card5% == @DSA@n || %card5% == @DCA@n || %card5% == @RHA@n || %card5% == @RDA@n)
 set value5 1
elseif (%card5% /= 2)
 set value5 2
elseif (%card5% /= 3)
 set value5 3
elseif (%card5% /= 4)
 set value5 4
elseif (%card5% /= 5)
 set value5 5
elseif (%card5% /= 6)
 set value5 6
elseif (%card5% /= 7)
 set value5 7
elseif (%card5% /= 8)
 set value5 8
elseif (%card5% /= 9)
 set value5 9
elseif (%card5% /= 10)
 set value5 10
end
eval handvalue %handvalue% + %value5% 
if %card5% == %cace% || %card5% == %sace% || %card5% == %dace% || %card5% == %hace% && %card1% != %cace% && %card1% != %sace% && %card1% != %dace% && %card1% != %hace% && %card2% != %cace% && %card2% != %sace% && %card2% != %dace% && %card2% != %hace% && %card3% != %cace% && %card3% != %sace% && %card3% != %dace% && %card3% != %hace% && %card4% != %cace% && %card4% != %sace% && %card4% != %dace% && %card4% != %hace% 
eval acehandvalue %handvalue% + 10 
else 
eval acehandvalue %acehandvalue% + %value5% 
end 
if %acehandvalue% != %handvalue% && %acehandvalue% > 0 
 if (%acehandvalue% < 22)
 %send% %actor% You have been dealt: %card5% Hand Value: <%handvalue%> Alt Ace Hand V.: <%acehandvalue%>
 else
 %send% %actor% You have been dealt: %card5% Hand Value: <%handvalue%>
 end
elseif %acehandvalue% == %handvalue% 
%send% %actor% You have been dealt: %card5% Hand Value: <%handvalue%> 
end 
%echoaround% %actor% The dealer deals %actor.name% a %card5%.
remote card5 %actor.id% 
remote handvalue %actor.id% 
remote acehandvalue %actor.id% 
if %handvalue% == 21 
%send% %actor% @YYou have 21!!@n 
%send% %actor% You better stand...
elseif %acehandvalue% == 21 
%send% %actor% @YYou have 21!!@n 
%send% %actor% You better stand...
elseif %handvalue% > 21 
%send% %actor% @RYou have gone bust!!@n 
%echoaround% %actor% %actor.name% has busted.
rdelete card1 %actor.id% 
rdelete card2 %actor.id% 
rdelete card3 %actor.id% 
rdelete card4 %actor.id% 
rdelete card5 %actor.id% 
rdelete handvalue %actor.id% 
rdelete acehandvalue %actor.id%
eval prize %actor.bet%
rdelete bet %actor.id%
rdelete raise %actor.id%
%send% %actor% You lose %prize% zenni!
else 
%send% %actor% @CYou have a 5 Card Charlie!!@n 
%echoaround% %actor% %actor.name% has a 5 Card Charlie!!
rdelete card1 %actor.id% 
rdelete card2 %actor.id% 
rdelete card3 %actor.id% 
rdelete card4 %actor.id% 
rdelete card5 %actor.id% 
rdelete handvalue %actor.id% 
rdelete acehandvalue %actor.id% 
eval prize %actor.bet% * 3
rdelete bet %actor.id%
rdelete raise %actor.id%
%send% %actor% You win %prize% zenni!
set %actor.gold(%prize%)%
end 
else 
%send% %actor% You need to "deal" before you can hit/stand. 
end
~
#15
Blackjack Fold~
2 d 100
fold~
if (%actor.varexists(handvalue)%)
wait 1
eval bett %actor.bet%
%send% %actor% You fold, losing %bett% zenni you had betted.
rdelete bet %actor.id%
rdelete card1 %actor.id%
rdelete card1_2 %actor.id%
rdelete card2 %actor.id%
rdelete card2_2 %actor.id%
rdelete card3 %actor.id%
rdelete card4 %actor.id%
rdelete card5 %actor.id%
rdelete handvalue %actor.id%
rdelete acehandvalue %actor.id%
rdelete handvalue1 %actor.id%
rdelete acehandvalue1 %actor.id%
rdelete handvalue2 %actor.id%
rdelete acehandvayylue2 %actor.id%
rdelete raise %actor.id%
%echoaround% %actor% %actor.name% folds, losing what %actor.heshe% had bet.
else
wait 1
%send% %actor% You are not even playing, try saying 'deal'.
end
~
#16
Eagle Eye~
1 c 2
look~
eval room2 %self.room%
eval thisRoom %room2.vnum%
if %arg% == north || %arg% ==  east || %arg% == south || %arg% == west || %arg% == up || %arg% == down || %arg% == northwest || %arg% == northeast || %arg% == southeast || %arg% == southwest || %arg% == n || %arg% == s || %arg% == e || %arg% == w || %arg% == sw || %arg% == se || %arg% == ne || %arg% == nw || %arg% == ne
  if %arg% == n
   eval dir north
   eval dr 0
  elseif %arg% == s
   eval dir south
   eval dr 2
  elseif %arg% == w
   eval dir west
   eval dr 3
  elseif %arg% == e
   eval dir east
   eval dr 1
  elseif %arg% == ne
   eval dir northeast
   eval dr 7
  elseif %arg% == nw
   eval dir northwest
   eval dr 6
  elseif %arg% == sw
   eval dir southwest
   eval dr 9
  elseif %arg% == se
   eval dir southeast
   eval dr 8
  else
   eval dir %arg%
  end
  eval room1 %self.room%
  eval thatRoom %%room1.%dir%(vnum)%%
  if (%room1.%dir%(bits)% == DOOR CLOSED SECRET)
   %send% %actor% You don't notice a direction that way.
  elseif (%room1.%dir%(bits)% == DOOR CLOSED)
   %send% %actor% You can't see anything that way right now.
  elseif (%room1.%dir%(bits)% == DOOR CLOSED LOCKED)
   %send% %actor% You can't see anything that way right now.
  elseif %thatRoom%
    if %arg% == n
     %echoaround% %actor% %actor.name% looks north.
    elseif %arg% == s
     %echoaround% %actor% %actor.name% looks south.
    elseif %arg% == w
     %echoaround% %actor% %actor.name% looks west.
    elseif %arg% == e
     %echoaround% %actor% %actor.name% looks east.
    elseif %arg% == ne
     %echoaround% %actor% %actor.name% looks northeast.
    elseif %arg% == nw
     %echoaround% %actor% %actor.name% looks northwest.
    elseif %arg% == sw
     %echoaround% %actor% %actor.name% looks southwest.
    elseif %arg% == se
     %echoaround% %actor% %actor.name% looks southeast.
    else
     %echoaround% %actor% %actor.name% looks %arg%.
    end
     %teleport% %actor% %thatRoom%
     %force% %actor% look
     %teleport% %actor% %thisRoom%
  else
   %send% %actor% That direction doesn't exist.
  end
else
return 0
end
~
#17
Eagle Eye~
1 c 2
l~
eval room2 %self.room%
eval thisRoom %room2.vnum%
*
if %arg% == north || %arg% ==  east || %arg% == south || %arg% == west || %arg% == up || %arg% == down || %arg% == northwest || %arg% == northeast || %arg% == southeast || %arg% == southwest || %arg% == n || %arg% == s || %arg% == e || %arg% == w || %arg% == sw || %arg% == se || %arg% == ne || %arg% == nw || %arg% == ne
  if %arg% == n
   eval dir north
  elseif %arg% == s
   eval dir south
  elseif %arg% == w
   eval dir west
  elseif %arg% == e
   eval dir east
  elseif %arg% == ne
   eval dir northeast
  elseif %arg% == nw
   eval dir northwest
  elseif %arg% == sw
   eval dir southwest
  elseif %arg% == se
   eval dir southeast
  else
   eval dir %arg%
  end
  eval room1 %self.room%
  eval thatRoom %%room1.%dir%(vnum)%%
  if %thatRoom%
   if %arg% == n
    %echoaround% %actor% %actor.name% looks north.
   elseif %arg% == s
    %echoaround% %actor% %actor.name% looks south.
   elseif %arg% == w
    %echoaround% %actor% %actor.name% looks west.
   elseif %arg% == e
    %echoaround% %actor% %actor.name% looks east.
   elseif %arg% == ne
    %echoaround% %actor% %actor.name% looks northeast.
   elseif %arg% == nw
    %echoaround% %actor% %actor.name% looks northwest.
   elseif %arg% == sw
    %echoaround% %actor% %actor.name% looks southwest.
   elseif %arg% == se
    %echoaround% %actor% %actor.name% looks southeast.
   else
    %echoaround% %actor% %actor.name% looks %arg%.
   end
    %teleport% %actor% %thatRoom%
    %force% %actor% look
    %teleport% %actor% %thisRoom%
  else
   %send% %actor% That direction doesn't exist.
  end
else
return 0
end
~
#18
Blackjack Raise~
2 d 100
raise~
if (!%actor.varexists(raise)%)
 if (%actor.varexists(handvalue)%)
 if (%actor.handvalue% > 12 && %actor.handvalue% < 22)
 wait 1
 %send% %actor% You can only raise if you have a possible hand value of 12 or less.
 elseif (%speech.cdr% > 25)
 wait 1
 %send% %actor% You can not have a raise higher than 25 zenni.
 elseif (%speech.cdr% && %speech.cdr% < 5)
 wait 1
 %send% %actor% Minimum raise is 5 zenni.
 elseif (%actor.gold% < %speech.cdr%)
 wait 1
 %send% %actor% You do not have enough zenni.
 else
  if (%speech.cdr% > 0)
  wait 1
  %send% %actor% You raise the bet %speech.cdr% zenni.
  set %actor.gold(-%speech.cdr%)%
  eval amt %actor.bet% + %speech.cdr%
  set bet %amt%
  remote bet %actor.id%
  elseif (%actor.gold% >= 5)
  wait 1
  eval plus %actor.bet% + 5
  %send% %actor% You raise the bet to %plus% zenni.
  set %actor.gold(-5)%
  eval amt %actor.bet% + 5
  set bet %amt%
  remote bet %actor.id%
  else
  wait 1
  %send% %actor% You need at least 5 zenni to raise.
  end
 set raise
 remote raise %actor.id%
 end
 else
 wait 1
 %send% %actor% You are not even playing, try saying 'deal'.
 end
else
wait 1
%send% %actor% You already raised, you can only do that once.
end
~
#19
Blackjack Stand~
2 d 100
stand~
if (%actor.varexists(handvalue)% && !%actor.varexists(stand)%)
set stand
remote stand %actor.id%
set handvalue %actor.handvalue%
set acehandvalue %actor.acehandvalue%
if (%acehandvalue% <= 21 && %acehandvalue% > %handvalue%)
set handvalue %acehandvalue%
end
if (%random.3% != 3)
eval dcard1 1 + %random.10%
eval dcard2 1 + %random.10%
elseif (%random.4% >= 2)
eval dcard1 11
eval dcard2 1 + %random.10%
else
eval dcard1 11
eval dcard2 10
end
eval dealerh %dcard1% + %dcard2%
eval count 2
wait 1 s
%echoaround% %actor% %actor.name% decides to stand.
wait 1 s
%echo% The dealer looks at his cards.
 if (%dealerh% == 22)
  eval dealerh 12
  wait 2 s
  %echo% The dealer takes another card.
  eval dcard3 %random.11%
  eval dealerh %dealerh% + %dcard3%
  eval count %count% + 1
 elseif (%dealerh% < 17)
  wait 2 s
  %echo% The dealer takes another card.
  eval dcard3 %random.11%
  eval dealerh %dealerh% + %dcard3%
  eval count %count% + 1
 end
 if (%dealerh% < 17)
  wait 2 s
  %echo% The dealer takes another card.
  eval dcard3 %random.11%
  eval dealerh %dealerh% + %dcard3%
  eval count %count% + 1
 end
 if (%dealerh% < 17)
  wait 2 s
  %echo% The dealer takes another card.
  eval dcard3 %random.11%
  eval dealerh %dealerh% + %dcard3%
  eval count %count% + 1
 end
 if (%dealerh% > 21 && %random.2% == 2)
  eval dealerh 16 + %random.3%
 end
 if (%dealerh% >= 17 && %dealerh% <= 20)
  wait 1 s
  %echo% The dealer stands with a total of %dealerh%.
 elseif (%dealerh% == 21 && %count% < 3)
  wait 1 s
  %send% %actor% The dealer has 21! Sorry you lose %actor.bet% zenni.
  %echoaround% %actor% The dealer has 21! %actor.name% loses %actor.bet% zenni.
  eval over 1
 elseif (%dealerh% > 21)
  wait 1 s
  eval prize %actor.bet% * 2
  %echoaround% %actor% The dealer has busted! %actor.name% wins %prize% zenni!
  %send% %actor% The dealer has busted! Congratulations, you win %prize% zenni!
  set %actor.gold(%prize%)%
  eval over 1
 end
 if (!%over%)
 if (%handvalue% > %dealerh%)
  wait 1 s
  eval prize %actor.bet% * 2
  %send% %actor% The dealer has a total of %dealerh% vs your %handvalue%. Congratulations, you win %prize% zenni!
  %echoaround% %actor% The dealer has a total of %dealerh% vs %actor.name%'s %handvalue%. %actor.name% wins %prize% zenni!
  set %actor.gold(%prize%)%
 elseif (%handvalue% < %dealerh%)
  wait 1 s
  eval prize %actor.bet%
  %send% %actor% The dealer has a total of %dealerh% vs your %handvalue%. Sorry you lose %prize% zenni.
  %echoaround% %actor% The dealer has a total of %dealerh% vs %actor.name%'s %handvalue%. %actor.name% loses %prize% zenni!
 elseif (%handvalue% == %dealerh%)
  wait 1 s
  eval prize %actor.bet% / 2
  %send% %actor% The dealer has a total of %dealerh% vs your %handvalue%. Push, you lose half your money.
  %echoaround% %actor% The dealer has a total of %dealerh% vs %actor.name%'s %handvalue%. A Push, %actor.name% loses %prize% zenni!
  set %actor.gold(%prize%)%
 end
 end
rdelete stand %actor.id%
rdelete raise %actor.id%
rdelete bet %actor.id%
rdelete card1 %actor.id%
rdelete card1_2 %actor.id%
rdelete card2 %actor.id%
rdelete card2_2 %actor.id%
rdelete card3 %actor.id%
rdelete card4 %actor.id%
rdelete card5 %actor.id%
rdelete handvalue %actor.id%
rdelete acehandvalue %actor.id%
rdelete handvalue1 %actor.id%
rdelete acehandvalue1 %actor.id%
rdelete handvalue2 %actor.id%
rdelete acehandvayylue2 %actor.id%
elseif (%actor.varexists(stand)%)
wait 1
%send% %actor% You are already standing, wait for the dealer to finish.
else
wait 1
%send% %actor% You are not even playing, try saying 'deal'.
end
~
#20
Blackjack Leave~
2 q 100
fold~
if (%actor.varexists(handvalue)%)
return 0
wait 1
%send% %actor% If you want to leave, say 'fold'. Otherwise finish your hand.
end
~
#21
Blackjack Show Hand~
2 c 100
hand~
if (!%actor.varexists(handvalue)%)
wait 1
%send% %actor% You are not even playing, say 'deal' to play.
else
 if (%actor.varexists(card1)%)
  %send% %actor% Your down card is: @n         <%actor.card1%>
 end
 if (%actor.varexists(card2)%)
  %send% %actor% Your second card is: @n       <%actor.card2%>
 end
 if (%actor.varexists(card3)%)
  %send% %actor% Your third card is: @n        <%actor.card3%>
 end
 if (%actor.varexists(card4)%)
  %send% %actor% Your fourth card is: @n       <%actor.card4%>
 end
  if (%actor.handvalue% < %actor.acehandvalue% && %actor.acehandvalue% < 22)
   %send% %actor% Your Hand Value is: @n       <%actor.handvalue%>
   %send% %actor% Your Alt. Ace Hand Value is: <%actor.acehandvalue%>
  else
   %send% %actor% Your Hand Value is: @n        <%actor.handvalue%>
  end
end
~
#22
Blackjack Stand Two~
2 d 100
stand~
if (%actor.varexists(handvalue)% && !%actor.varexists(stand)%)
set stand
remote stand %actor.id%
set handvalue %actor.handvalue%
set acehandvalue %actor.acehandvalue%
if (%acehandvalue% <= 21 && %acehandvalue% > %handvalue%)
set handvalue %acehandvalue%
end
if (%random.3% != 3)
eval dcard1 1 + %random.10%
eval dcard2 1 + %random.10%
elseif (%random.4% >= 2)
eval dcard1 11
eval dcard2 1 + %random.10%
else
eval dcard1 11
eval dcard2 10
end
eval dealerh %dcard1% + %dcard2%
eval count 2
wait 1 s
%echoaround% %actor% %actor.name% decides to stand.
wait 1 s
%echo% The dealer looks at his cards.
 if (%dealerh% == 22)
  eval dealerh 12
  wait 2 s
  %echo% The dealer takes another card.
  eval dcard3 %random.11%
  eval dealerh %dealerh% + %dcard3%
  eval count %count% + 1
 elseif (%dealerh% < 17)
  wait 2 s
  %echo% The dealer takes another card.
  eval dcard3 %random.11%
  eval dealerh %dealerh% + %dcard3%
  eval count %count% + 1
 end
 if (%dealerh% < 17)
  wait 2 s
  %echo% The dealer takes another card.
  eval dcard3 %random.11%
  eval dealerh %dealerh% + %dcard3%
  eval count %count% + 1
 end
 if (%dealerh% < 17)
  wait 2 s
  %echo% The dealer takes another card.
  eval dcard3 %random.11%
  eval dealerh %dealerh% + %dcard3%
  eval count %count% + 1
 end
 if (%dealerh% > 21 && %random.2% == 2)
  eval dealerh 16 + %random.3%
 end
 if (%dealerh% >= 17 && %dealerh% <= 20)
  wait 1 s
  %echo% The dealer stands with a total of %dealerh%.
 elseif (%dealerh% == 21 && %count% < 3)
  wait 1 s
  %send% %actor% The dealer has 21! Sorry you lose %actor.bet% zenni.
  %echoaround% %actor% The dealer has 21! %actor.name% loses %actor.bet% zenni.
  eval over 1
 elseif (%dealerh% > 21)
  wait 1 s
  eval prize %actor.bet% * 2
  %echoaround% %actor% The dealer has busted! %actor.name% wins %prize% zenni!
  %send% %actor% The dealer has busted! Congratulations, you win %prize% zenni!
  set %actor.gold(%prize%)%
  eval over 1
 end
 if (!%over%)
 if (%handvalue% > %dealerh%)
  wait 1 s
  eval prize %actor.bet% * 2
  %send% %actor% The dealer has a total of %dealerh% vs your %handvalue%. Congratulations, you win %prize% zenni!
  %echoaround% %actor% The dealer has a total of %dealerh% vs %actor.name%'s %handvalue%. %actor.name% wins %prize% zenni!
  set %actor.gold(%prize%)%
 elseif (%handvalue% < %dealerh%)
  wait 1 s
  eval prize %actor.bet%
  %send% %actor% The dealer has a total of %dealerh% vs your %handvalue%. Sorry you lose %prize% zenni.
  %echoaround% %actor% The dealer has a total of %dealerh% vs %actor.name%'s %handvalue%. %actor.name% loses %prize% zenni!
 elseif (%handvalue% == %dealerh%)
  wait 1 s
  eval prize %actor.bet% / 2
  %send% %actor% The dealer has a total of %dealerh% vs your %handvalue%. Push, you lose half your money.
  %echoaround% %actor% The dealer has a total of %dealerh% vs %actor.name%'s %handvalue%. A Push, %actor.name% loses %prize% zenni!
  set %actor.gold(%prize%)%
 end
 end
rdelete stand %actor.id%
rdelete raise %actor.id%
rdelete bet %actor.id%
rdelete card1 %actor.id%
rdelete card1_2 %actor.id%
rdelete card2 %actor.id%
rdelete card2_2 %actor.id%
rdelete card3 %actor.id%
rdelete card4 %actor.id%
rdelete card5 %actor.id%
rdelete handvalue %actor.id%
rdelete acehandvalue %actor.id%
rdelete handvalue1 %actor.id%
rdelete acehandvalue1 %actor.id%
rdelete handvalue2 %actor.id%
rdelete acehandvayylue2 %actor.id%
elseif (%actor.varexists(stand)%)
wait 1
%send% %actor% You are already standing, wait for the dealer to finish.
else
wait 1
%send% %actor% You are not even playing, try saying 'deal'.
end
~
#25
Vonstuben~
1 l 100
~
if (%actor.name% == Lonewuelf || %actor.name% == lonewuelf)
 wait 1
 %echoaround% %actor% %actor.name% sheathes %self.shortdesc%.
 %send% %actor% You sheath the %self.shortdesc%.
else
 return 0
 %send% %actor% You have not been bestowed with the Vonstuben!
end
~
#26
Vonstuben Unsheath~
1 j 100
~
%echoaround% %actor% %actor.name% unsheathes %self.shortdesc%.
%send% %actor% You unsheath the %self.shortdesc%.
~
#27
Vonstuben Mark~
1 c 1
mark~
if (%actor.varexists(vonstuben)%)
 if (%actor.vonstuben% == 0)
  %send% %actor% You are unable to mark again.
  else
   %send% %actor% You concentrate and then quickly slash through the fabric of space and time. This place is now marked.
   %echoaround% %actor% The vonstuben has marked this room with its power!
   set vonroom %actor.room.vnum%
   remote vonroom %actor.id%
  end
else
 %send% %actor% You concentrate and then quickly slash through the fabric of space and time. This place is now marked.
 %echoaround% %actor% The vonstuben has marked this room with its power!
 set vonstuben 20
 remote vonstuben %actor.id%
 set vonroom %actor.room.vnum%
 remote vonroom %actor.id%
end
~
#28
Vonstuben Teleport~
1 c 1
recall~
if (!%actor.varexists(vonstuben)%)
  %send% %actor% You haven't marked anywhere to recall to!
elseif (%actor.vonstuben% == 0)
  %send% %actor% You have used all the recalls that Vonstuben is willing to grant you.
else
 %send% %actor% @BAll time stops around you as Vonstuben pulls you through the tear you had made in space and time. You have arrived!@n
 %echoaround% %actor% Suddenly it seems like something has changed, but you aren't quite sure what.
 %teleport% %actor% %actor.vonroom%
end
~
#30
Knock~
2 c 100
knock~
wait 1
%send% %actor% You knock on the wall.
%echoaround% %actor% %actor.name% knocks on the wall
~
#31
Rename Object~
1 c 3
nickname~
if (!%arg%)
 %send% %actor% What do you want to nickname it?
elseif (%self.shortdesc% /= nicknamed)
 %send% %actor% It has already been nicknamed.
else
 set limit blahblahblahblahblah
 if (%arg.strlen% > %limit.strlen%)
  %send% %actor% You can't nickname it anything longer than 20 characters.
 else
  %send% %actor% You nickname %self.shortdesc% as %arg%.
  nop %self.shortdesc(%arg%)%
  nop %self.name(%arg%)%
 end
end
~
#32
Selfalias~
1 c 3
selfalias~
if (%cmd% == selfalias && %arg%)
 %send% %actor% Setting your personal alias to %arg%!
 set selfalias %arg%
 remote selfalias %actor.id%
elseif (%cmd% == selfalias && !%arg%)
 %send% %actor% Set your alias to what?
end
~
#33
Checkalias~
1 c 3
checkalias~
if (%actor.varexists(selfalias)%)
  %send% %actor% Your alias is set to: %actor.selfalias%.
else
  %send% %actor% You have no alias set, so you will show as %actor.race%
end
~
#40
Storage Plus Greet~
0 g 100
~
if (%actor.varexists(storing)%)
 mgoto 21
 get all
 while (%self.inventory%)
  set tar %self.inventory%
  if (%tar.cost% == %actor.id%)
   set obj %tar%
   drop %obj.name%
  else
   drop %tar.name%
  end
 done
 mgoto %actor.room.vnum%
 if (%obj%)
  eval num %obj.cost_per_day%
  wait 1
  say Greetings, %actor.name%, we are still holding onto your box.
  wait 2 s
  say You will have to @grenew@C your storage rental in %num% days. 
 else
  wait 1
  say Your storage box was removed due to lack of payment.
  rdelete storing %actor.id%
  wait 2 s
  say Remember to renew your storage rentals before they expire.
 end
else
 wait 1
 say Hello. Please read the sign before conducting business with Storage Plus for the first time.
end
~
#41
Storage Plus Store~
0 c 100
store~
if (%actor.varexists(storing)%)
 wait 1
 tell %actor.name% %actor.name%, you are already storing something with us, limit of one storage per customer.
elseif (%actor.gold% < 1500)
 wait 1
 tell %actor.name% The initial fee to store something here is 1500 zenni, you are short that.
elseif (!%arg%)
 wait 1
 tell %actor.name% What object are you wanting to store?
else
 wait 1
 set story
 remote story %actor.id%
 %force% %actor% give %arg% clerk
 rdelete story %actor.id%
 if (!%self.inventory%)
   wait 1
   tell %actor.name% You do not have an object called %arg%.
 elseif (%self.inventory.weight% > 9000)
   wait 1
   give all %actor.name%
   tell %actor.name% Your object can not exceed 9,000 weight.
 else
  %actor.gold(-1500)%
  wait 1
  %load% o 74
  set box %self.inventory(74)%
  %box.cost(%actor.id%)%
  %box.cost_per_day(160)%
  put all box
  mgoto 21
  drop box
  mgoto %actor.room.vnum%
  wait 1
  tell %actor.name% Your item has been stored and will need picked up or renewed before 160 (MUD DAYS) pass.
  set storing
  remote storing %actor.id%
 end
end
~
#42
Storage Plus - Renew~
0 c 100
renew~
if (!%actor.varexists(storing)%)
 wait 1
 tell %actor.name% You are not even storing anything.
else
 mgoto 21
 get all
 while (%self.inventory%)
  set tar %self.inventory%
  if (%tar.cost% == %actor.id%)
   set obj %tar%
   drop %obj.name%
  else
   drop %tar.name%
  end
 done
 mgoto %actor.room.vnum%
 if (%obj%)
  if (%actor.gold% < 1500)
   wait 1
   tell %actor.name% You do not have enough zenni, you need 1500 zenni to renew.
  else
   wait 1
   %actor.gold(-1500)%
   tell %actor.name% Your storage box rental has been renewed for another 160 (MUD DAYS).
   eval num %obj.cost_per_day%
   %obj.cost_per_day(-%num%)%
   %obj.cost_per_day(160)%
  end
 else
  wait 1
  tell %actor.name% Your storage box was removed due to lack of renewal or pickup.
  rdelete storing %actor.id%
 end
end
~
#43
Storage Plus - Pickup~
0 c 100
pickup~
if (!%actor.varexists(storing)%)
 wait 1
 tell %actor.name% You are not even storing anything.
else
 mgoto 21
 get all
 while (%self.inventory%)
  set tar %self.inventory%
  if (%tar.cost% == %actor.id%)
   set obj %tar%
   north
   drop %obj.name%
   south
  else
   drop %tar.name%
  end
 done
 mgoto %actor.room.vnum%
 if (%obj%)
  mgoto 22
  get all
  mgoto %actor.room.vnum%
  wait 1
  give box %actor.name%
  if (!%self.inventory%)
   %force% %actor% get all all.box
   %purge% %obj%
  else
   get all all.box
   %purge% %obj%
   drop all
  end
  rdelete storing %actor.id%
  wait 1
  tell %actor.name% Thanks for your business!
 else
  wait 1
  tell %actor.name% Your storage box was removed due to lack of renewal or pickup.
  rdelete storing %actor.id%
 end
end
~
#44
Storage Plus - Check~
0 c 100
check~
if (!%actor.varexists(storing)%)
 wait 1
 tell %actor.name% You are not even storing anything.
else
 mgoto 21
 get all
 while (%self.inventory%)
  set tar %self.inventory%
  if (%tar.cost% == %actor.id%)
   set obj %tar%
   drop %obj.name%
  else
   drop %tar.name%
  end
 done
 mgoto %actor.room.vnum%
 if (%obj%)
  eval num %obj.cost_per_day%
  wait 1
  tell %actor.name% Your storage will need picked up or renewed in %num% (MUD DAYS).
 else
  wait 1
  tell %actor.name% Your storage box was removed due to lack of renewal or pickup.
  rdelete storing %actor.id%
 end
end
~
#45
Storage Plus - Purge~
0 t 22
~
%echo% Storage Purge Check
 get all
 while (%self.inventory%)
  set obj %self.inventory%
  if (%obj.cost_per_day% == 0)
   %purge% %obj%
  else
   %obj.cost_per_day(-1)%
   drop %obj.name%
  end
 done
detach 45 %self.id%
attach 47 %self.id%
~
#46
Storage Plus Don't Give~
0 j 100
~
if (!%actor.varexists(story)%)
 return 0
 wait 1
 tell %actor.name% I do not hold onto items that are not being stored or picked up.
end
~
#47
Storage Plus Attach~
0 t 20
~
attach 45 %self.id%
detach 47 %self.id%
~
#48
Generatre Assassin~
1 t 16
~
if (%self.carried_by% && %random.200% >= 195) {
 set victim %self.carried_by%
 if (%victim.level% <= 10)
  return 0
 else if (%victim.varexists(random1)% && %random.4% == 4)
  rdelete random1 %victim.id%
 elseif (!%victim.varexists(random1)% && %victim.pos% != sitting && %victim.pos% != resting && %victim.pos% != sleeping)
  set random1targ
  remote random1targ %victim.id%
  if (%victim.level% <= 20)
   %load% m 800
  elseif (%victim.level% <= 40)
   %load% m 801
  elseif (%victim.level% <= 60)
   %load% m 802
  elseif (%victim.level% <= 80)
   %load% m 803
  elseif (%victim.level% < 100)
   %load% m 804
  elseif (%victim.level% == 100)
   %load% m 805
  end
  set random1
  remote random1 %victim.id%
 end
end
~
#49
Assassin Load~
0 n 100
~
set room_var %self.room%
set trg %room_var.people%
while (%trg%)
 set next_trg %trg.next_in_room%
 if (%trg.varexists(random1targ)%)
  wait 1 s
  %load% o 454
  %load% o 454
  %load% o 454
  %load% o 454
  %load% o 454
  if (%random.4% == 4)
   %load% o 455
  end
  emote pulls a knife from its sheath and aims the blade at *%trg.name%!
  eval num %random.3% + 1
  wait %num% s
  throw ass %trg.name%
  rdelete random1targ %trg.id%
 end
 set trg %next_trg%
done
~
#50
Generatre Assassin~
1 t 0
~
if (%self.carried_by% && %random.200% >= 195) {
 set victim %self.carried_by%
 if (%victim.level% <= 10)
  return 0
 else if (%victim.varexists(random1)% && %random.4% == 4)
  rdelete random1 %victim.id%
 elseif (!%victim.varexists(random1)% && %victim.pos% != sitting && %victim.pos% != resting && %victim.pos% != sleeping)
  set random1targ
  remote random1targ %victim.id%
  if (%victim.level% <= 20)
   %load% m 800
  elseif (%victim.level% <= 40)
   %load% m 801
  elseif (%victim.level% <= 60)
   %load% m 802
  elseif (%victim.level% <= 80)
   %load% m 803
  elseif (%victim.level% < 100)
   %load% m 804
  elseif (%victim.level% == 100)
   %load% m 805
  end
  set random1
  remote random1 %victim.id%
 end
end
~
#51
Reese Greet~
0 h 100
~
wait 1
say Ah what a fool are you to enter my presence uninvited.
wait 2 s
say I shall make an example out of you for your impudence.
~
#52
Reese Fight~
0 ab 100
~
wait 1
if (%self.hitp% < %self.maxhitp%)
 eval gain %self.hitp% + (%self.maxhitp% / 50)
 %echo% %self.name% grins as his wounds and injuries heal some what.
 set %self.hitp(%gain%)%
end
~
#53
Boots Cost~
1 j 100
~
wait 1
%send% %actor% Every hour you wear these boots will cost you 500 zenni.
set halfboots
remote halfboots %actor.id%
if (%time.hour% < 23)
 eval change %time.hour% + 1
 nop %self.cost_per_day(%change%)%
else
 eval change %self.cost_per_day% - (self.cost_per_day% * 2)
 nop %self.cost_per_day(%change%)%
end
~
#54
Boots Trigger 2~
1 b 100
~
if (%self.cost_per_day% == %time.hour% && %self.worn_by%)
 if (%actor.varexists(halfboots)%)
  if (%actor.gold% > 500)
   wait 1
   if (%time.hour% < 23)
    nop %self.cost_per_day(1)%
   else
    %eval% change %self.cost_per_day% - (%self.cost_per_day% * 2)
    nop %self.cost_per_day(%change%)%
   end
   %send% %actor% Your boots absorb some of your money in a glow of green energy.
   nop %actor.gold(-500)%
  else
   wait 1
   %send% %actor% You are too poor to satisfy the needs of the boots!
   %force% %actor% remove gurglheim
   rdelete halfboots %actor.id%
  end
 end
elseif (%self.cost_per_day% + 2 < %time.hour%)
  eval diff %time.hour% - %self.cost_per_day%
  nop %self.cost_per_day(%diff%)%
elseif (%self.cost_per_day% > %time.hour%)
  eval diff %self.cost_per_day% - %time.hour%
  nop %self.cost_per_day(-%diff%)%
end
~
#55
Boots Trigger 3~
1 l 100
~
if (%actor.varexists(halfboots)%)
 rdelete halfboots %actor.id%
eval change %self.cost_per_day% - (%self.cost_per_day% * 2)
nop %self.cost_per_day(%change%)%
end
~
#59
Flush Space Room~
2 b 100
~
set target_char %self.people%
while %target_char%
 set tmp_target %target_char.next_in_room%
 if (%target_char.varexists(intransport)%)
  if (%target_char.intransport% == earth)
   %teleport% %target_char% 408
  elseif (%target_char.intransport% == frigid)
   %teleport% %target_char% 4250
  elseif (%target_char.intransport% == vegeta)
   %teleport% %target_char% 2323
  elseif (%target_char.intransport% == aether)
   %teleport% %target_char% 12002
  elseif (%target_char.intransport% == namek)
   %teleport% %target_char% 11626
  elseif (%target_char.intransport% == konack)
   %teleport% %target_char% 8194
  end
 end
 set target_char %tmp_target%
done
~
#60
peek~
1 c 2
peek~
if !%arg.cdr%
  if %arg% >= 0 && %arg% < 666
    set i 0
    set origroom %actor.room.vnum%
    while %i% < 100
      if %i.strlen% == 1
        set room %arg%0%i%
      else
        set room %arg%%i%
      end
      if %room.vnum% == %room%
        %teleport% %actor% %room%
        %force% %actor% look
      end
      eval i %i% + 1
    done
    %teleport% %actor% %origroom%
  else
    %send% %actor% Please type 'peek <zone #>' to use this script.
  end
else
  set cdr %arg.cdr%
  if !%cdr.cdr%
    if %arg.car% >= 0 && %arg.car% < 65535 && %arg.cdr% > 0 && %arg.cdr% <= 65535 && %arg.cdr% > %arg.car%
      eval diff %arg.cdr% - %arg.car%
      if %diff% > 100
        %send% %actor% That range is too large, please keep the range too 100 vnums or fewer.
        halt
      end
      set i %arg.car%
      set origroom %actor.room.vnum%
      while %i% <= %arg.cdr%
        if %i.vnum% == %i%
          %teleport% %actor% %i%
          %force% %actor% look
        end
        eval i %i% + 1
      done
      %teleport% %actor% %origroom%
    else
      %send% %actor That is an invalid range.
    end
  else
    %send% %actor That is an invalid range.
  end
end
~
#61
Random Bonus!~
1 ab 1
~
if (%self.carried_by%)
 set targ %self.carried_by%
 if (%targ%)
  if (%targ.pos% == resting && %random.40% >= 37)
   eval num %targ.level% * 1000
   set %targ.exp(%num%)%
   %send% %self.carried_by% @YYou reflect upon your experiences of your recent adventures... @D[@GExp@D: @C+%num%@D]@n
  elseif (%targ.gold% + %targ.bank% <= 25000 && %random.30% == 20)
   eval num %random.250% + %random.250%
   set %targ.bank(%num%)%
   %send% %self.carried_by% @YA mysterious anonymous deposit has been made to your bank account! @D[@GZenni@D: @C+%num%@D]@n
  elseif (%targ.prac% <= 200 && %random.50% >= 45)
   eval num %random.100%
   set %targ.prac(%num%)%
   %send% %self.carried_by% @YWhile daydreaming about a past battle you suddenly have an epiphany! @D[@GPS@D: @C+%num%@D]@n
  elseif (%targ.move% <= 100 && %random.8% >= 8)
   eval num %targ.level% * 100
   if (%targ.level% >= 90)
    eval num %targ.level% * 5000
   elseif (%targ.level% >= 70)
    eval num %targ.level% * 2500
   elseif (%targ.level% >= 50)
    eval num %targ.level% * 1500
   elseif (%targ.level% >= 30)
    eval num %targ.level% * 500
   end
   set %targ.move(%num%)%
   %send% %self.carried_by% @YYou manage to recover some of your stamina through will alone! @D[@G+%num%@D]@n
  end
 end
end
~
#62
Random 02: EDF Squad~
1 c 2
blah~
set targ %self.carried_by%
if (%targ%)
 if (!%targ.varexists(random2)% && %random.200% >= 1)
  set roomv %targ.room%
  if (%roomv.roomflag(EARTH)%)
   set random2
   remote random2 %targ.id%
   if (%targ.level% >= 60)
    %load% m 2053
    %load% m 2053
    %load% m 2053
    %load% m 2053
    %load% m 2057
   elseif (%targ.level% >= 40) {
    %load% m 2052
    %load% m 2052
    %load% m 2052
    %load% m 2052
    %load% m 2056
   elseif (%targ.level% >= 20) {
    %load% m 2051
    %load% m 2051
    %load% m 2051
    %load% m 2051
    %load% m 2055
   else
    %load% m 2050
    %load% m 2050
    %load% m 2050
    %load% m 2050
    %load% m 2050
   end
  end
 else
  if (%random.100% >= 98)
   rdelete random2 %targ.id%
  end
 end
end
~
#63
Random 02: EDI Squad Leader~
0 n 100
~
eval was_in %self.room.vnum%
%goto% 49
if (%self.level% >= 50)
 %load% o 558
 wear vest
 wait 1 s
else
 %load% o 557
 wear vest
 wait 1 s
end
%goto% %was_in%
set room_var %self.room%
set targ %room_var.people%
while (%targ%)
 set tmp_trg %targ.next_in_room%
 if (%targ.varexists(random2)%)
  wait 1
  %echo% A small squad of EDI soldiers arrives from nearby.
  wait 3 s
  emote looks at *%targ.name% and frowns.
  wait 3 s
  if (%targ.race% != human)
   if (%targ.maxhitp% >= 100000000)
    say HALT! Unacceptable power readings have been detected! You are unwelcome on this planet. You have till the count of 5 to leave the area!
    wait 5 s
    attack %targ.name%
   elseif (%targ.maxhitp% >= 10000000)
    say HALT! Your power is reading exceptionally high. You are a risk to the good people of Earth. You had better leave soon.
   elseif (%targ.maxhitp% > 20000)
    say You... Whatever your name is... you better not be up to no good. We EDI troopers are not going to take alien violence lightly.
   else
    say Hey offworlder. You better behave.
   end
  else
   say Greetings citizen. Have you noticed any dangerous alien activity recently? 
  end
 end
 set targ %tmp_trg%
done
~
#64
EDI Lower Equip~
0 n 100
~
eval was_in %self.room.vnum%
%goto% 49
if (%self.level% >= 50)
 %load% o 558
 wear vest
else
 %load% o 557
 wear vest
end
%goto% %was_in%
~
#67
Food/Drink Dispensor~
1 c 4
dispense~
if (%actor.gold% < 10) {
 wait 1
 %send% %actor% You push random buttons on the dispensor, however you didn't put in any money.
 %echoaround% %actor% %actor.name% pushes random buttons on the dispensor, but nothing comes out.
 %send% %actor% You need at least 10 zenni.
else
 if (%arg% == food)
nop %actor.gold(-10)%
  wait 1
  %send% %actor% You put 10 zenni in the machine and push the button.
  %echoaround% %actor% %actor.name% puts 10 zenni in the machine and pushes the button.
  wait 2 s
  %echo% %self.shortdesc% whirs and a clunking noise can be heard.
  wait 1 s
  %echo% %self.shortdesc% falls quiet as a ration lands in its dispensor bay.
  wait 1
  %load% obj 68
  %force% %actor% get ration
 elseif (%arg% == drink)
nop %actor.gold(-10)%
  wait 1
  %send% %actor% You put 10 zenni in the machine and push the button.
  %echoaround% %actor% %actor.name% puts 10 zenni in the machine and pushes the button.
  wait 2 s
  %echo% %self.shortdesc% whirs and a clunking noise can be heard.
  wait 1 s
  %echo% %self.shortdesc% falls quiet as a drink lands in its dispensor bay.
  wait 1
  %load% obj 69
  %force% %actor% get bottle
 else
  wait 1
  %send% %actor% You didn't make a selection. Food or drink?
 end
end
~
#68
Generatre Assassin~
1 t 20
~
if (%self.carried_by% && %random.200% >= 195) {
 set victim %self.carried_by%
 if (%victim.level% <= 10)
  return 0
 else if (%victim.varexists(random1)% && %random.4% == 4)
  rdelete random1 %victim.id%
 elseif (!%victim.varexists(random1)% && %victim.pos% != sitting && %victim.pos% != resting && %victim.pos% != sleeping)
  set random1targ
  remote random1targ %victim.id%
  if (%victim.level% <= 20)
   %load% m 800
  elseif (%victim.level% <= 40)
   %load% m 801
  elseif (%victim.level% <= 60)
   %load% m 802
  elseif (%victim.level% <= 80)
   %load% m 803
  elseif (%victim.level% < 100)
   %load% m 804
  elseif (%victim.level% == 100)
   %load% m 805
  end
  set random1
  remote random1 %victim.id%
 end
end
~
#72
Odin Teleport~
0 ab 5
~
%echo% Odin places two fingers on his forehead and vanishes!
eval odinport %random.9%
switch %odinport%
case 1
%teleport% %self% 50
break
case 2
%teleport% %self% 51
break
case 3
%teleport% %self% 52
break
case 4
%teleport% %self% 53
break
case 5
%teleport% %self% 54
break
case 6
%teleport% %self% 55
break
case 7
%teleport% %self% 56
break
case 8
%teleport% %self% 57
break
default
%teleport% %self% 53
break
done
~
#73
Odin Fight~
0 k 35
~
eval tactics %random.4%
switch %tactics%
case 1
wait 1 sec
kienzan
break
case 2
wait 1 sec
death ball
break
case 3
wait 1 sec
kousengan
break
default
%echo% Odin focuses his ki and heals some of his combat damage!
nop %self.hitp(50000000000)%
 break
done
~
#80
Test Echoaround~
0 g 100
~
%send% %actor% %self.name% @Wsays, '@CYou look tasty, %actor.name%!@W'@n
%echoaround% %actor% %self.name% @Wsays, '@CYou look tasty, %actor.name%!@W'@n
~
#97
Random 100~
1 c 3
random~
eval num %random.100%
wait 1
%echo% @mOOC@D: @WSomeone has requested a random number from the @CInvisible Cloud of Infinite Numbers@W (but only 1 through 100)! @D[@G%num%@D]@n
wait 1
~
#98
Test Speech~
2 d 100
"Wee doo wee"~
%echo% This trigger commandlist is not complete!
~
#99
Roll Dice~
1 c 3
roll~
if (%arg% == standard)
 eval num1 %random.6%
 eval num2 %random.6%
 wait 1
 eval num3 %num2% + %num1%
  switch (%num3%)
   case 2
    %send% %actor% You roll the dice and they come up as snake eyes!
    %echoaround% %actor% %actor.name% rolls the dice and they come up as snake eyes!
    break
   case 7
    %send% %actor% You roll the dice and they come up as lucky seven!
    %echoaround% %actor% %actor.name% rolls the dice and they come up as lucky seven!
    break
   case 12
    %send% %actor% You roll the dice and they come up as box cars, number 12!
    %echoaround% %actor% %actor.name% rolls the dice and they come up as box cars, number 12!
    break
   default
    %send% %actor% You roll the dice and they come up as %num3%!
    %echoaround% %actor% %actor.name% rolls the dice and they come up as %num3%!
    break
   done
elseif (%arg% == d12)
 eval num1 %random.12%
 %send% %actor% You roll a d12 and it comes up as %num1%!
 %echoaround% %actor% %actor.name% rolls a d12 and they come up as %num1%
elseif (%arg% == d20)
 eval num1 %random.20%
 %send% %actor% You roll a d20 and it comes up as %num1%!
 %echoaround% %actor% %actor.name% rolls a d20 and they come up as %num1%
elseif (%arg% == d100)
 eval num1 %random.100%
 %send% %actor% you roll a d100 and it comes up as %num1%!
 %echoaround% %actor% %actor.name% rolls a d100 and they come up as %num1%
else
 %send% %actor% Proper commands: standard, d12, d20, d100.
end
~
$~
