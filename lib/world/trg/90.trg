#9000
Insert sword and load reaper elemental~
1 c 3
insert~
if %arg% != blade
  wait 1 s
  %send% %actor% Try inserting the blade.
else
  if %actor.room.vnum% != 9074
    wait 1 s
    %send% %actor% You are not in the right location to insert the blade.
  else
    wait 1 s
    %send% %actor% You insert the Shadow Blade into the slit in the wall.
    %echoaround% %actor% %actor.name% thrusts a shadow blade into a slit in the wall.
    wait 4 sec
    %echo% Suddenly the air becomes still and cold.
    wait 3 sec
    %echo% Shadows creep down the walls to the floor, pooling together at a central location.
    wait 3 sec
    %echo% As the shadows begin to merge, a form rises upward and continues growing.
    wait 3 sec
    %echo% The shadows stop merging as a Reaper Elemental appears.
    wait 3 sec
    %echo% An explosion of cold air erupts down the hallway exuding from the Reaper Elemental as it readies to fight.
    wait 2 s
    %load% mob 9010
    wait 2 sec
    %send% %actor% The %self.shortdesc% crumbles to dust!
    %echoaround% %actor% The %self.shortdesc% crumbles to dust!
    %purge% self
  end 
end
~
#9001
Reaper Load Trigger~
0 n 100
~
%load% obj 9003
~
#9002
Reaper death trigger~
0 f 100
~
%echo% The Reaper Elemental lets out a deafening roar as it explodes into shadowy light.
~
$~
