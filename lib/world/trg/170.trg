#17000
Unlocker Minion~
0 n 100
~
wait 1
%load% obj 17002
unlock airlock
mgoto 30756
close ship
mgoto 49
~
#17001
Elevator Open Holds~
2 c 100
push~
if (%self.south(DOOR)%)
 wait 1
 %send% %actor% Why? The doors are already open.
elseif (!%actor.has_item(17003)%)
 wait 1
 %send% %actor% You push the button, but nothing happens.
 %echoaround% %actor% %actor.name% pushes the button, but nothing happens.
 wait 1 s
 %send% %actor% You need a keycard in conjunction with the button for the doors to open.
else
 wait 1
 %send% %actor% You swipe the keycard and then push the button.
 %echoaround% %actor% %actor.name% swipes a keycard and then pushes the elevator button.
 wait 1 s
 %echo% The elevator doors slide open.
%at% 17048 %echo% The elevator doors slide open.
 %door% 17051 2 flags a
 %door% 17048 0 flags a
 wait 4 s
 %echo% The elevator doors slide closed.
 %at% 17048 %echo% The elevator doors slide closed.
 %door% 17051 2 flags abc
 %door% 17048 0 flags abc
end
~
#17002
Elevator Open Mid Deck~
2 c 100
push~
if (%self.north(DOOR)%)
 wait 1
 %send% %actor% Why? The doors are already open.
elseif (!%actor.has_item(17003)%)
 wait 1
 %send% %actor% You push the button, but nothing happens.
 %echoaround% %actor% %actor.name% pushes the button, but nothing happens.
 wait 1 s
 %send% %actor% You need a keycard in conjunction with the button for the doors to open.
else
 wait 1
 %send% %actor% You swipe the keycard and then push the button.
 %echoaround% %actor% %actor.name% swipes a keycard and then pushes the elevator button.
 wait 1 s
 %echo% The elevator doors slide open.
%at% 17048 %echo% The elevator doors slide open.
 %door% 17001 0 flags a
 %door% 17021 2 flags a
 wait 4 s
 %echo% The elevator doors slide closed.
 %at% 17048 %echo% The elevator doors slide closed.
 %door% 17001 0 flags abc
 %door% 17021 2 flags abc
end
~
#17003
Elevator Open Main Deck~
2 c 100
push~
if (%self.south(DOOR)%)
 wait 1
 %send% %actor% Why? The doors are already open.
elseif (!%actor.has_item(17003)%)
 wait 1
 %send% %actor% You push the button, but nothing happens.
 %echoaround% %actor% %actor.name% pushes the button, but nothing happens.
 wait 1 s
 %send% %actor% You need a keycard in conjunction with the button for the doors to open.
else
 wait 1
 %send% %actor% You swipe the keycard and then push the button.
 %echoaround% %actor% %actor.name% swipes a keycard and then pushes the elevator button.
 wait 1 s
 %echo% The elevator doors slide open.
%at% 17033 %echo% The elevator doors slide open.
 %door% 17034 2 flags a
 %door% 17033 0 flags a
 wait 4 s
 %echo% The elevator doors slide closed.
 %at% 17033 %echo% The elevator doors slide closed.
 %door% 17034 2 flags abc
 %door% 17033 0 flags abc
end
~
#17004
Elevator Travel~
2 c 100
push~
if (!%arg%)
 wait 1
 %send% %actor% Push which button?
elseif (%self.vnum% == 17021)
 if (%arg% == one)
  if (%self.south(DOOR)%)
   wait 1
   %send% %actor% You push the button labled one.
   %echoaround% %actor% %actor.name% pushes the button labled one.
   wait 1
   %echo% The elevator doors close and the elevator begins to move.
   %door% 17021 2 flags abc
   %door% 17001 0 flags abc
   wait 1 s
   %echo% The elevator doors open as it arrives on floor one.
   %teleport% all 17048
   %door% 17048 0 flags a
   %door% 17051 2 flags a
   %at% 17051 %echo% The elevator doors open as a dinging noise is heard.
  else
   %door% 17021 2 flags abc
   %door% 17001 0 flags abc
   wait 1
   %send% %actor% You push the button labled one.
   %echoaround% %actor% %actor.name% pushes the button labled one.
   wait 1 s
   %echo% The elevator doors open as it arrives on floor one.
   %teleport% all 17048
   %door% 17048 0 flags a
   %door% 17051 2 flags a
   %at% 17051 %echo% The elevator doors open as a dinging noise is heard.
  end  
 elseif (%arg% == two)
  if (%self.south(DOOR)%)
   wait 1
   %send% %actor% You push the button labled two, but nothing happens.
   %echoaround% %actor% %actor.name% pushes the button labled two, but nothing happens.
   wait 1
   %send% %actor% The elevator doors are already open and you are already on that floor.
  else
   wait 1
   %send% %actor% You push the button labled two.
   %echoaround% %actor% %actor.name% pushes the button labled two.
   wait 1
   %echo% The elevator doors open.
   %door% 17021 2 flags a
   %door% 17001 0 flags a
   %at% 17001 %echo% The elevator doors open as a dinging noise is heard.
  end
 elseif (%arg% == three)
  if (%self.south(DOOR)%)
   wait 1
   %send% %actor% You push the button labled three.
   %echoaround% %actor% %actor.name% pushes the button labled three.
   wait 1
   %echo% The elevator doors close and the elevator begins to move.
   %door% 17021 2 flags abc
   %door% 17001 0 flags abc
   wait 1 s
   %echo% The elevator doors open as it arrives on floor three.
   %teleport% all 17033
   %door% 17033 0 flags a
   %door% 17034 2 flags a
   %at% 17034 %echo% The elevator doors open as a dinging noise is heard.
  else
   %door% 17021 2 flags abc
   %door% 17001 0 flags abc
   wait 1
   %send% %actor% You push the button labled three.
   %echoaround% %actor% %actor.name% pushes the button labled three.
   wait 1 s
   %echo% The elevator doors open as it arrives on floor three.
   %teleport% all 17033
   %door% 17033 0 flags a
   %door% 17034 2 flags a
   %at% 17034 %echo% The elevator doors open as a dinging noise is heard.
  end
 else
  %send% %actor% Which button do you want to push? Try looking at the panel.
 end
elseif (%self.vnum% == 17048)
 if (%arg% == one)
  if (%self.north(DOOR)%)
   wait 1
   %send% %actor% You push the button labled one, but nothing happens.
   %echoaround% %actor% %actor.name% pushes the button labled one, but nothing happens.
   wait 1
   %send% %actor% The elevator doors are already open and you are already on that floor.
  else
   wait 1
   %send% %actor% You push the button labled one.
   %echoaround% %actor% %actor.name% pushes the button labled one.
   wait 1
   %echo% The elevator doors open.
   %door% 17048 0 flags a
   %door% 17051 2 flags a
   %at% 17051 %echo% The elevator doors open as a dinging noise is heard.
  end  
 elseif (%arg% == two)
  if (%self.north(DOOR)%)
   wait 1
   %send% %actor% You push the button labled two.
   %echoaround% %actor% %actor.name% pushes the button labled two.
   wait 1
   %echo% The elevator doors close and the elevator begins to move.
   %door% 17048 0 flags abc
   %door% 17051 2 flags abc
   wait 1 s
   %echo% The elevator doors open as it arrives on floor two.
   %teleport% all 17021
   %door% 17021 2 flags a
   %door% 17001 0 flags a
   %at% 17001 %echo% The elevator doors open as a dinging noise is heard.
  else
   %door% 17048 0 flags abc
   %door% 17051 2 flags abc
   wait 1
   %send% %actor% You push the button labled two.
   %echoaround% %actor% %actor.name% pushes the button labled two.
   wait 1 s
   %echo% The elevator doors open as it arrives on floor two.
   %teleport% all 17021
   %door% 17021 2 flags a
   %door% 17001 0 flags a
   %at% 17001 %echo% The elevator doors open as a dinging noise is heard.
  end
 elseif (%arg% == three)
  if (%self.north(DOOR)%)
   wait 1
   %send% %actor% You push the button labled three.
   %echoaround% %actor% %actor.name% pushes the button labled three.
   wait 1
   %echo% The elevator doors close and the elevator begins to move.
   %door% 17051 2 flags abc
   %door% 17048 0 flags abc
   wait 1 s
   %echo% The elevator doors open as it arrives on floor three.
   %teleport% all 17033
   %door% 17033 0 flags a
   %door% 17034 2 flags a
   %at% 17034 %echo% The elevator doors open as a dinging noise is heard.
  else
   %door% 17048 0 flags abc
   %door% 17051 2 flags abc
   wait 1
   %send% %actor% You push the button labled three.
   %echoaround% %actor% %actor.name% pushes the button labled three.
   wait 1 s
   %echo% The elevator doors open as it arrives on floor three.
   %teleport% all 17033
   %door% 17033 0 flags a
   %door% 17034 2 flags a
   %at% 17051 %echo% The elevator doors open as a dinging noise is heard.
  end
 else
  %send% %actor% Which button do you want to push? Try looking at the panel.
 end
elseif (%self.vnum% == 17033)
 if (%arg% == one)
  if (%self.north(DOOR)%)
   wait 1
   %send% %actor% You push the button labled one.
   %echoaround% %actor% %actor.name% pushes the button labled one.
   wait 1
   %echo% The elevator doors close and the elevator begins to move.
   %door% 17033 0 flags abc
   %door% 17034 2 flags abc
   wait 1 s
   %echo% The elevator doors open as it arrives on floor one.
   %teleport% all 17048
   %door% 17051 2 flags a
   %door% 17048 0 flags a
   %at% 17051 %echo% The elevator doors open as a dinging noise is heard.
  else
   %door% 17033 0 flags abc
   %door% 17034 2 flags abc   
   wait 1
   %send% %actor% You push the button labled one.
   %echoaround% %actor% %actor.name% pushes the button labled one.
   wait 1 s
   %echo% The elevator doors open as it arrives on floor one.
   %teleport% all 17048
   %door% 17051 2 flags a
   %door% 17048 0 flags a
   %at% 17051 %echo% The elevator doors open as a dinging noise is heard.
  end
 elseif (%arg% == two)
  if (%self.north(DOOR)%)
   wait 1
   %send% %actor% You push the button labled two.
   %echoaround% %actor% %actor.name% pushes the button labled two.
   wait 1
   %echo% The elevator doors close and the elevator begins to move.
   %door% 17033 0 flags abc
   %door% 17034 2 flags abc
   wait 1 s
   %echo% The elevator doors open as it arrives on floor two.
   %teleport% all 17021
   %door% 17021 2 flags a
   %door% 17001 0 flags a
   %at% 17001 %echo% The elevator doors open as a dinging noise is heard.
  else
   %door% 17033 0 flags abc
   %door% 17034 2 flags abc
   wait 1
   %send% %actor% You push the button labled two.
   %echoaround% %actor% %actor.name% pushes the button labled two.
   wait 1 s
   %echo% The elevator doors open as it arrives on floor two.
   %teleport% all 17021
   %door% 17021 2 flags a
   %door% 17001 0 flags a
   %at% 17001 %echo% The elevator doors open as a dinging noise is heard.
  end
 elseif (%arg% == three)
  if (%self.north(DOOR)%)
   wait 1
   %send% %actor% You push the button labled three, but nothing happens.
   %echoaround% %actor% %actor.name% pushes the button labled three, but nothing happens.
   wait 1
   %send% %actor% The elevator doors are already open and you are already on that floor.
  else
   wait 1
   %send% %actor% You push the button labled three.
   %echoaround% %actor% %actor.name% pushes the button labled three.
   wait 1
   %echo% The elevator doors open.
   %door% 17033 0 flags a
   %door% 17034 2 flags a
   %at% 17034 %echo% The elevator doors open as a dinging noise is heard.
  end
 else
  %send% %actor% Which button do you want to push? Try looking at the panel.
 end
end
~
#17005
Elevator Leave~
2 q 100
~
if (%self.vnum% == 17021)
 wait 1
 %door% 17001 0 flags abc
 %door% 17021 2 flags abc
 %echo% The elevator doors close.
 %at% 17001 %echo% The elevator doors close.
elseif (%self.vnum% == 17033)
 wait 1
 %door% 17033 0 flags abc
 %door% 17034 2 flags abc
 %echo% The elevator doors close.
 %at% 17034 %echo% The elevator doors close.
elseif (%self.vnum% == 17048)
 wait 1
 %door% 17048 0 flags abc
 %door% 17051 2 flags abc
 %echo% The elevator doors close.
 %at% 17051 %echo% The elevator doors close.
end
~
#17006
Doore Greet~
0 g 100
~
wait 1 s
say Hey what are you doing on this ship?
wait 2 s
say You are going to regret coming here...
~
#17007
Neiz Greet~
0 g 100
~
wait 1 s
say What! How did you get in here?
wait 2 s
say You are in for a world of hurt!
~
#17008
Salza Greet~
0 g 100
~
wait 1 s
%send% %actor% %self.name% looks at you with a raised eyebrow.
%echoaround% %actor% %self.name% looks at %actor.name% with a raised eyebrow.
~
#17009
Cooler Greet~
0 g 100
~
wait 2 s
say What kind of fool are you to challenge me?
~
#17010
Doore Fight~
0 k 8
~
eraser
~
#17011
Neiz Fight~
0 k 8
~
eval num %random.3%
switch (num)
 case 1
  crusher
  break
 default
  eraser
  break
done
~
#17012
Salza Fight~
0 k 8
~
crusher
~
#17013
Cooler Fight~
0 k 8
~
deathball
~
#17014
Cooler Transform~
0 l 65
~
if (%self.vnum% == 17010)
wait 1
say That's it! I am going to crush you with my power!
wait 2 s
say Why? Because i'm a prick!
wait 2 s
%echo% %self.name%'s head begins to expand so that four horn like protrusions jut from his skull. Sharp spikes form frim his forearms and his shoulders become more angular. Lastly his body grows to twice its former size and a shell like growth covers his mouth!
mtransform -17011
detach 17014 %self.id%
end
~
$~
