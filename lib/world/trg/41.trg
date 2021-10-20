#4100
Palace Guard trigger~
0 q 100
~
if (%direction% == north && %actor.race% != icer)
   say Only Icers may enter the palace.
   %send% %actor.name% %self.name% prevents you from entering the palace.
   %echoaround% %actor.name% As %actor.name% tries to enter the palace %self.name% stops %actor.himher%
return 0
else
return 1
end
~
#4101
Icicle Hotel - Slot Machine Trigger~
1 c 100
pull~
* Original Trigger by Random of TBA
* Altered by Dean of DBAT
set display[1]  @D\|@D\| @CKing Cold @D\|@D\| @CKing Cold @D\|@D\| @CKing Cold @D\|@D\|
set display[2]  @D\|@D\| @CCooler    @D\|@D\| @CCooler    @D\|@D\| @CCooler    @D\|@D\|
set display[3]  @D\|@D\| @CFrieza    @D\|@D\| @CFrieza    @D\|@D\| @CFrieza    @D\|@D\|
set display[4]  @D\|@D\| @CFrieza    @D\|@D\| @CFrieza    @D\|@D\| @CFrieza    @D\|@D\|
set display[5]  @D\|@D\| @CKing Cold @D\|@D\| @CKing Cold @D\|@D\| @CCooler    @D\|@D\|
set display[6]  @D\|@D\| @CKing Cold @D\|@D\| @CKing Cold @D\|@D\| @CFrieza    @D\|@D\|
set display[7]  @D\|@D\| @CKing Cold @D\|@D\| @CCooler    @D\|@D\| @CKing Cold @D\|@D\|
set display[8]  @D\|@D\| @CKing Cold @D\|@D\| @CFrieza    @D\|@D\| @CKing Cold @D\|@D\|
set display[9]  @D\|@D\| @CCooler    @D\|@D\| @CKing Cold @D\|@D\| @CKing Cold @D\|@D\|
set display[10] @D\|@D\| @CFrieza    @D\|@D\| @CKing Cold @D\|@D\| @CKing Cold @D\|@D\|
set display[11] @D\|@D\| @CCooler    @D\|@D\| @CCooler    @D\|@D\| @CKing Cold @D\|@D\|
set display[12] @D\|@D\| @CCooler    @D\|@D\| @CCooler    @D\|@D\| @CFrieza    @D\|@D\|
set display[13] @D\|@D\| @CCooler    @D\|@D\| @CKing Cold @D\|@D\| @CCooler    @D\|@D\|
set display[14] @D\|@D\| @CCooler    @D\|@D\| @CFrieza    @D\|@D\| @CCooler    @D\|@D\|
set display[15] @D\|@D\| @CKing Cold @D\|@D\| @CCooler    @D\|@D\| @CCooler    @D\|@D\|
set display[16] @D\|@D\| @CFrieza    @D\|@D\| @CCooler    @D\|@D\| @CCooler    @D\|@D\|
set display[17] @D\|@D\| @CFrieza    @D\|@D\| @CFrieza    @D\|@D\| @CKing Cold @D\|@D\|
set display[18] @D\|@D\| @CFrieza    @D\|@D\| @CFrieza    @D\|@D\| @CCooler    @D\|@D\|
set display[19] @D\|@D\| @CFrieza    @D\|@D\| @CKing Cold @D\|@D\| @CFrieza    @D\|@D\|
set display[20] @D\|@D\| @CFrieza    @D\|@D\| @CCooler    @D\|@D\| @CKing Cold @D\|@D\|
set display[21] @D\|@D\| @CFrieza    @D\|@D\| @CCooler    @D\|@D\| @CFrieza    @D\|@D\|
set display[22] @D\|@D\| @CKing Cold @D\|@D\| @CFrieza    @D\|@D\| @CFrieza    @D\|@D\|
set display[23] @D\|@D\| @CCooler    @D\|@D\| @CFrieza    @D\|@D\| @CFrieza    @D\|@D\|
set display[24] @D\|@D\| @CKing Cold @D\|@D\| @CCooler    @D\|@D\| @CFrieza    @D\|@D\|
set display[25] @D\|@D\| @CCooler    @D\|@D\| @CKing Cold @D\|@D\| @CFrieza    @D\|@D\|
set display[26] @D\|@D\| @CFrieza    @D\|@D\| @CKing Cold @D\|@D\| @CCooler    @D\|@D\|
set display[27] @D\|@D\| @CKing Cold @D\|@D\| @CFrieza    @D\|@D\| @CCooler    @D\|@D\|
set display[28] @D\|@D\| @CCooler    @D\|@D\| @CFrieza    @D\|@D\| @CKing Cold @D\|@D\|
set display[29] @D\|@D\| @CFrieza    @D\|@D\| @CCooler    @D\|@D\| @CKing Cold @D\|@D\|
set display[30] @D\|@D\| @CKing Cold @D\|@D\| @CCooler    @D\|@D\| @CFrieza    @D\|@D\|
set display[31] @D\|@D\| @CCooler    @D\|@D\| @CKing Cold @D\|@D\| @CFrieza    @D\|@D\|
set display[32] @D\|@D\| @CFrieza    @D\|@D\| @CKing Cold @D\|@D\| @CCooler    @D\|@D\|
set display[33] @D\|@D\| @CKing Cold @D\|@D\| @CFrieza    @D\|@D\| @CCooler    @D\|@D\|
set display[34] @D\|@D\| @CCooler    @D\|@D\| @CFrieza    @D\|@D\| @CKing Cold @D\|@D\|
set display[35] @D\|@D\| @CFrieza    @D\|@D\| @CCooler    @D\|@D\| @CKing Cold @D\|@D\|
set prize[1] 100
set prize[2] 25
set prize[3] 25
set prize[4] 25
set prize[5] 25
set prize[6] 10
set prize[7] 10
set prize[8] 10
set prize[9] 10
set prize[10] 10
set prize[11] 10
set prize[12] 5
set prize[13] 5
set prize[14] 5
set prize[15] 5
set prize[16] 5
set prize[17] 5
set prize[18] 5
set prize[19] 5
set prize[20] 0
set prize[21] 5
set prize[22] 0
set prize[23] 0
set prize[24] 0
set prize[25] 0
set prize[26] 0
set prize[27] 0
set prize[28] 0
set prize[29] 0
set prize[30] 0
set prize[31] 0
set prize[32] 0
set prize[33] 0
set prize[34] 0
set prize[35] 0
set message[1] You are The Grand Prize Winner!!
set message[2] You are a Big Prize Winner!
set message[3] You are a Big Prize Winner!
set message[4] You are a Big Prize Winner!
set message[5] You are a winner!
set message[6] You are a winner!
set message[7] You are a winner!
set message[8] You are a winner!
set message[9] You are a winner!
set message[10] You are a winner!
set message[11] You are a winner!
set message[12] You broke even!
set message[13] You broke even!
set message[14] You broke even!
set message[15] You broke even!
set message[16] You broke even!
set message[17] You broke even!
set message[18] You broke even!
set message[19] You broke even!
set message[20] You LOSE!!
set message[21] You broke even!
set message[22] You LOSE!!
set message[23] You LOSE!!
set message[24] You LOSE!!
set message[25] You LOSE!!
set message[26] You LOSE!!
set message[27] You LOSE!!
set message[28] You LOSE!!
set message[29] You LOSE!!
set message[30] You LOSE!!
set message[31] You LOSE!!
set message[32] You LOSE!!
set message[33] You LOSE!!
set message[34] You LOSE!!
set message[35] You LOSE!!
*
if %arg% == lever
  if %actor.gold% < 5
    %send% %actor% You don't have enough cash.
  else
    %send% %actor% You insert 5 zenni.
    %echoaround% %actor% %actor.name% inserts some zenni.
    %send% %actor% You pull the slot machine lever.
    %echoaround% %actor% %actor.name% pulls the slot machine lever.
    nop %actor.gold(-5)%
    wait 1 sec
    %send% %actor% @R __
    %send% %actor% @R(__)
    %send% %actor% @D \|\|
    %send% %actor% @D \|\|
    %send% %actor% @D \|\|
    wait 1 sec
    %send% %actor% @R   _
    %send% %actor% @R  (_)
    %send% %actor% @D  //
    %send% %actor% @D //
    %send% %actor% @D//
    wait 1 sec
    %send% %actor% @R     _
    %send% %actor% @D####@R(_)@n
    wait 1 sec
    %send% %actor% The slots begin spinning.
    wait 1 sec
    %send% %actor% The slots start to slow down.
    wait 2 sec
    %send% %actor% The slots stop one at a time.
    eval number %random.35%
    set slots %%display[%number%]%%
    set value %%prize[%number%]%%
    set msg %%message[%number%]%%
    eval slots %slots%
    eval value %value%
    eval msg %msg%
    %send% %actor% \ @D_______________________________________
    %send% %actor% @D\|                                       @n@D\|
    %send% %actor% @D\|    @YLUCKY        @W<@w777@W>        @YLUCKY    @n@D\|
    %send% %actor% @D\| ___________  ___________  ___________ @n@D\|
    %send% %actor% @D\|@D\|           @D\|@D\|           @D\|@D\|           @D\|@D\|
    %send% %actor% %slots%
    %send% %actor% @D\|@D\|___________@D\|@D\|___________@D\|@D\|___________@D\|@D\| @R_
    %send% %actor% @D\|          @W_____                        @n@D\|@R(_)
    %send% %actor% @D\|         @W\|@D[@D]   @W\|                       @n@D\|@c//
    %send% %actor% @D\|          @W-----                        @n@D\|
    %send% %actor% @D\|_______________________________________@n@D\|@n
    %send% %actor% You recieve %value% zenni.
    %send% %actor% %msg%
    nop %actor.gold(%value%)%
    if (%value% == 100)
      %echoaround% %actor% %actor.name% is the Grand Prize Winner!!
    elseif (%value% == 25)
      %echoaround% %actor% %actor.name% is a Big Prize Winner!
    elseif (%value% == 10)
      %echoaround% %actor% %actor.name% is a Winner.
    elseif (%value% == 5)
      %echoaround% %actor% %actor.name% broke even.
    else
      %echoaround% %actor% %actor.name% is a LOSER!!
    end
  end
else
  %send% %actor% Pull what? Try lever.
end
~
#4102
Slots already pulled trigger~
1 c 100
pull~
if (%arg% == lever)
wait 1
%send% %actor% The slot machine has already been pulled.
else
wait 1
%send% %actor% Pull what? Try lever.
end
~
#4103
Quarantine Guard trig 1~
0 g 100
~
wait 1 s
say This is a restricted area. You can go no further unless you have clearance papers from the security chief.
~
#4104
Give clearance papers~
0 j 100
4101~
if %object.vnum(4101)%
wait 1 s
emote glances at the clearance papers.
wait 2 s
say Well, it checks out. Be warned though, it's dangerous in there.
wait 2 s
say And once you go through, there's no coming back through this gate.
wait 1 s
unlock gate
open gate
wait 10 s
close gate
lock gate
%purge% %object%
else
say I don't want that!
return 0
end
~
#4118
Elevator Outside floor 1~
2 c 100
push~
if ("%arg%" != "button" && "%arg%" != "open")
%send% %actor% Push what? Try button or open.
else
%send% %actor% You push the open button.
wait 1 s
%send% %actor% The elevator doors slide open.
%echoaround% %actor% The elevator doors open as %actor.name% pushes the open button.
wdoor 4118 10 flags a
wdoor 4119 11 flags a
end
~
#4119
Elevator Outside floor 2~
2 c 100
push~
if ("%arg%" != "button" && "%arg%" != "open")
%send% %actor% Push what? Try button or open.
else
%send% %actor% You push the open button.
wait 1 s
%send% %actor% The elevator doors slide open.
%echoaround% %actor% The elevator doors open as %actor.name% pushes the open button.
wdoor 4073 10 flags a
wdoor 4120 11 flags a
end
~
#4120
Ice Crown City Elevator floor 1.~
2 c 100
push~
if ("%arg%" == "five") 
  if %self.vnum% != 4123 
    %send% %actor% You have pushed the button for Royal Palace. 
    %echoaround% %actor% %actor.name% has pushed the button for the 5th floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4118 10 flags abc 
    %door% 4119 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "5th Floor - Royal Palace" 
    wteleport all 4123 
    %door% 4123 11 flags a 
    %door% 4140 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end
if ("%arg%" == "four") 
  if %self.vnum% != 4122 
    %send% %actor% You have pushed the button for Rich District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 4th floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4118 10 flags abc 
    %door% 4119 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "4th Floor - Rich District" 
    wteleport all 4122 
    %door% 4122 11 flags a 
    %door% 4017 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end
if ("%arg%" == "three") 
  if %self.vnum% != 4121 
    %send% %actor% You have pushed the button for Commercial District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 3rd floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4118 10 flags abc 
    %door% 4119 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "3rd Floor - Commercial District" 
    wteleport all 4121 
    %door% 4121 11 flags a 
    %door% 4040 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end
if ("%arg%" == "two") 
  if %self.vnum% != 4120 
    %send% %actor% You have pushed the button for Residential District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 2nd floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4118 10 flags abc 
    %door% 4119 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "2nd Floor - Residential District" 
    wteleport all 4120 
    %door% 4120 11 flags a 
    %door% 4073 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end
if ("%arg%" == "one") 
  if %self.vnum% != 4119 
    %send% %actor% You have pushed the button for Abandoned Level. 
    %echoaround% %actor% %actor.name% has pushed the button for the 1st floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4118 10 flags abc 
    %door% 4119 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "1st Floor - Abandoned Level" 
    wteleport all 4119 
    %door% 4119 11 flags a 
    %door% 4118 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end
~
#4121
Elevator floor 2~
2 c 100
push~
if ("%arg%" == "five") 
  if %self.vnum% != 4123 
    %send% %actor% You have pushed the button for Royal Palace. 
    %echoaround% %actor% %actor.name% has pushed the button for the 5th floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4073 10 flags abc 
    %door% 4120 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "5th Floor - Royal Palace" 
    wteleport all 4123 
    %door% 4123 11 flags a 
    %door% 4140 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end
if ("%arg%" == "four") 
  if %self.vnum% != 4122 
    %send% %actor% You have pushed the button for Rich District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 4th floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4073 10 flags abc 
    %door% 4120 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "4th Floor - Rich District" 
    wteleport all 4122 
    %door% 4122 11 flags a 
    %door% 4017 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "three") 
  if %self.vnum% != 4121 
    %send% %actor% You have pushed the button for Commercial District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 3rd floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4073 10 flags abc 
    %door% 4120 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "3rd Floor - Commercial District" 
    wteleport all 4121 
    %door% 4121 11 flags a 
    %door% 4040 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end
if ("%arg%" == "two") 
  if %self.vnum% != 4120 
    %send% %actor% You have pushed the button for Residential District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 2nd floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4073 10 flags abc 
    %door% 4120 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "2nd Floor - Residential District" 
    wteleport all 4120 
    %door% 4120 11 flags a 
    %door% 4073 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "one") 
  if %self.vnum% != 4119 
    %send% %actor% You have pushed the button for Abandoned Level. 
    %echoaround% %actor% %actor.name% has pushed the button for the 1st floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4073 10 flags abc 
    %door% 4120 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "1st Floor - Abandoned Level" 
    wteleport all 4119 
    %door% 4119 11 flags a 
    %door% 4118 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end
~
#4122
Elevatour outside floor 3~
2 c 100
push~
if ("%arg%" != "button" && "%arg%" != "open")
%send% %actor% Push what? Try button or open.
else
%send% %actor% You push the open button.
wait 1 s
%send% %actor% The elevator doors slide open.
%echoaround% %actor% The elevator doors open as %actor.name% pushes the open button.
wdoor 4040 10 flags a
wdoor 4121 11 flags a
end
~
#4123
Elevator outside floor 4~
2 c 100
push~
if ("%arg%" != "button" && "%arg%" != "open")
%send% %actor% Push what? Try button or open.
else
%send% %actor% You push the open button.
wait 1 s
%send% %actor% The elevator doors slide open.
%echoaround% %actor% The elevator doors open as %actor.name% pushes the open button.
wdoor 4017 10 flags a
wdoor 4122 11 flags a
end
~
#4124
Security Headquarters Elevator trigger~
2 c 100
push~
if ("%arg%" == "one")
  if %self.vnum% != 4186
    %send% %actor% You have pushed the button for the 1st floor.
    %echoaround% %actor% %actor.name% has pushed the button for the 1st floor.
    wait 1 s
    %echo% The elevator begins moving towards its destintation.
    wait 1 s
    %echo% The elevator has arrived on the 1st floor.
    wteleport all 4186
  else
    %send% %actor% You are already on that floor
  end
end
if ("%arg%" == "two")
  if %self.vnum% != 4190
    %send% %actor% You have pushed the button for the 2nd floor.
    %echoaround% %actor% %actor.name% has pushed the button for the 2nd floor.
    wait 1 s
    %echo% The elevator begins moving towards its destination.
    wait 1 s
%echo% The elevator has arrived on the 2nd floor.
    wteleport all 4190
  else
%send% %actor% You are already on that floor.
  end
end
if ("%arg%" == "three")
  if %self.vnum% != 4194
    %send% %actor% You have pushed the button for the 3rd floor.
    %echoaround% %actor% %actor.name% has pushed the button for the 3rd floor.
    wait 1 s
    %echo% The elevator begins moving towards its destination.
    wait 1 s
    %echo% The elevator has arrived on the 3rd floor.
    wteleport all 4194
  else
    %send% %actor% You are already on that floor.
  end
end
~
#4125
Elevator Outside floor 5~
2 c 100
push~
if ("%arg%" != "button" && "%arg%" != "open")
%send% %actor% Push what? Try button or open.
else
%send% %actor% You push the open button.
wait 1 s
%send% %actor% The elevator doors slide open.
%echoaround% %actor% The elevator doors open as %actor.name% pushes the open button.
wdoor 4140 10 flags a
wdoor 4123 11 flags a
end
~
#4126
Elevator Floor 3~
2 c 100
push~
if ("%arg%" == "five") 
  if %self.vnum% != 4123 
    %send% %actor% You have pushed the button for Royal Palace. 
    %echoaround% %actor% %actor.name% has pushed the button for the 5th floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4040 10 flags abc 
    %door% 4121 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "5th Floor - Royal Palace" 
    wteleport all 4123 
    %door% 4123 11 flags a 
    %door% 4140 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "four") 
  if %self.vnum% != 4122 
    %send% %actor% You have pushed the button for Rich District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 4th floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4040 10 flags abc 
    %door% 4121 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "4th Floor - Rich District" 
    wteleport all 4122 
    %door% 4122 11 flags a 
    %door% 4017 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "three") 
  if %self.vnum% != 4121 
    %send% %actor% You have pushed the button for Commercial District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 3rd floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4040 10 flags abc 
    %door% 4121 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "3rd Floor - Commercial District" 
    wteleport all 4121 
    %door% 4121 11 flags a 
    %door% 4040 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "two") 
  if %self.vnum% != 4120 
    %send% %actor% You have pushed the button for Residential District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 2nd floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4040 10 flags abc 
    %door% 4121 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "2nd Floor - Residential District" 
    wteleport all 4120 
    %door% 4120 11 flags a 
    %door% 4073 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "one") 
  if %self.vnum% != 4119 
    %send% %actor% You have pushed the button for Abandoned Level. 
    %echoaround% %actor% %actor.name% has pushed the button for the 1st floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4040 10 flags abc 
    %door% 4121 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "1st Floor - Abandoned Level" 
    wteleport all 4119 
    %door% 4119 11 flags a 
    %door% 4118 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end
~
#4127
Elevator floor 4~
2 c 100
push~
if ("%arg%" == "five") 
  if %self.vnum% != 4123 
    %send% %actor% You have pushed the button for Royal Palace. 
    %echoaround% %actor% %actor.name% has pushed the button for the 5th floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4017 10 flags abc 
    %door% 4122 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "5th Floor - Royal Palace" 
    wteleport all 4123 
    %door% 4123 11 flags a 
    %door% 4140 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "four") 
  if %self.vnum% != 4122 
    %send% %actor% You have pushed the button for Rich District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 4th floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4017 10 flags abc 
    %door% 4122 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "4th Floor - Rich District" 
    wteleport all 4122 
    %door% 4122 11 flags a 
    %door% 4017 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "three") 
  if %self.vnum% != 4121 
    %send% %actor% You have pushed the button for Commercial District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 3rd floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4017 10 flags abc 
    %door% 4122 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "3rd Floor - Commercial District" 
    wteleport all 4121 
    %door% 4121 11 flags a 
    %door% 4040 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "two") 
  if %self.vnum% != 4120 
    %send% %actor% You have pushed the button for Residential District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 2nd floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4017 10 flags abc 
    %door% 4122 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "2nd Floor - Residential District" 
    wteleport all 4120 
    %door% 4120 11 flags a 
    %door% 4073 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "one") 
  if %self.vnum% != 4119 
    %send% %actor% You have pushed the button for Abandoned Level. 
    %echoaround% %actor% %actor.name% has pushed the button for the 1st floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4017 10 flags abc 
    %door% 4122 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "1st Floor - Abandoned Level" 
    wteleport all 4119 
    %door% 4119 11 flags a 
    %door% 4118 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end
~
#4128
Opening the closet trigger *fun*~
2 c 100
open~
if "%arg%" != "closet"
  %send% %actor% Try opening the CLOSET.
else
  eval line %random.6%
  switch %line%
    case 1
      %send% %actor% You open the closet... and a weirdo named James jumps out and starts to dry hump your leg.
      %echoaround% %actor% %actor.name% opens up the closet, only to have James, the transdimensial weirdo start dry humping %actor.name%'s leg.
      wait 4 s
      %send% %actor% With a wicked grin on his face, James lets go of you and retreats to the safety of the closet.
      %echoaround% %actor% With a wicked grin on his face, James lets go of %actor.name% then retreats to the safety of the closet.
      break
    case 2
      %send% %actor% You open the closet and dirty socks fly at your face!
      %damage% %actor% 10
      %echoaround% %actor% %actor.name% opens up the closet and is hit with deadly accuracy, by dirty socks!!
      break
    case 3
      %send% %actor% You try to open the closet but it appears locked.
      %echoaround% %actor% %actor.name% tries to open the closet but it appears to be locked.
      break
    case 4
      %send% %actor% You open the closet and nothing weird happens.
      %echoaround% %actor% %actor.name% opens the closet and nothing weird happens.
      wait 4 s
      %echo% The room lighting turns red and a sadistic voice says, "I'm the boogey man and I'm coming to get you!"
      wait 1 s
      %send% %actor% You quickly shut the closet and the room lighting returns to normal.
      %echoaround% %actor% %actor.name% quickly shuts the closet and the room lighting returns to normal.
      break
    case 5
      %send% %actor% You open up the closet and are quickly sucked into a void!
      %echoaround% %actor% %actor.name% opens up the closet then is sucked into a void!!
      wteleport %actor% 0
      break
    default
      %send% %actor% You open up the closet and the sound of a little boy sobbing can be heard.
      %echoaround% %actor% %actor.name% opens the closet and the sound of a little boy sobbing can be heard.
    break
  done
end
~
#4129
Ice crown city elevator 5~
2 c 100
push~
if ("%arg%" == "five") 
  if %self.vnum% != 4123 
    %send% %actor% You have pushed the button for Royal Palace. 
    %echoaround% %actor% %actor.name% has pushed the button for the 5th floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4140 10 flags abc 
    %door% 4123 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "5th Floor - Royal Palace" 
    wteleport all 4123 
    %door% 4123 11 flags a 
    %door% 4140 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "four") 
  if %self.vnum% != 4122 
    %send% %actor% You have pushed the button for Rich District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 4th floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4140 10 flags abc 
    %door% 4123 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "4th Floor - Rich District" 
    wteleport all 4122 
    %door% 4122 11 flags a 
    %door% 4017 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "three") 
  if %self.vnum% != 4121 
    %send% %actor% You have pushed the button for Commercial District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 3rd floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4140 10 flags abc 
    %door% 4123 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "3rd Floor - Commercial District" 
    wteleport all 4121 
    %door% 4121 11 flags a 
    %door% 4040 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "two") 
  if %self.vnum% != 4120 
    %send% %actor% You have pushed the button for Residential District. 
    %echoaround% %actor% %actor.name% has pushed the button for the 2nd floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4140 10 flags abc 
    %door% 4123 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "2nd Floor - Residential District" 
    wteleport all 4120 
    %door% 4120 11 flags a 
    %door% 4073 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end 
if ("%arg%" == "one") 
  if %self.vnum% != 4119 
    %send% %actor% You have pushed the button for Abandoned Level. 
    %echoaround% %actor% %actor.name% has pushed the button for the 1st floor. 
    wait 1 s 
    %echo% The elevator begins to move slowly. 
    %door% 4140 10 flags abc 
    %door% 4123 11 flags abc 
    wait 1 s 
    %echo% The elevator moves rapidly towards its destination. 
    wait 2 s 
    %echo% The elevator comes to a stop and the doors open. "1st Floor - Abandoned Level" 
    wteleport all 4119 
    %door% 4119 11 flags a 
    %door% 4118 10 flags a 
  else 
    %send% %actor% You are already on that floor 
  end 
end
~
#4130
King Cold load~
0 n 100
~
wait 1 s
sit throne
~
#4131
King Cold Kickass~
0 k 5
~
deathball
~
$~
