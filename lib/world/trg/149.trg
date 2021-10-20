#14900
Leave Aquis City~
2 q 100
~
if (%direction% == west && %actor.room.vnum% == 14999)
 %echoaround% %actor% %actor.name% pushes through the gooey membrane and out into the Tamburs Ocean beyond!
 return 1
 wait 1
%echoaround% %actor% %actor.name% pushes through the gooey membrane and into the ocean here.
 %send% %actor% You push through the gooey membrane and emerge in the Tambrus Ocean on the other side!
elseif (%direction% == east && %actor.room.vnum% == 15099)
 %echoaround% %actor% %actor.name% pushes through the gooey membrane and out into the Tamburs Ocean beyond!
 return 1
 wait 1
%echoaround% %actor% %actor.name% pushes through the gooey membrane and into the ocean here.
 %send% %actor% You push through the gooey membrane and emerge in the Tambrus Ocean on the other side!
end
~
#14901
Enter Aquis City~
2 q 100
~
if (%direction% == east && %actor.room.vnum% == 15100)
 %echoaround% %actor% %actor.name% pushes through the gooey membrane and into Aquis City beyond!
 return 1
 wait 1
%echoaround% %actor% %actor.name% pushes through the gooey membrane and arrives into the city here.
 %send% %actor% You push through the gooey membrane and into Aquis City beyond!
elseif (%direction% == west && %actor.room.vnum% == 15204)
 %echoaround% %actor% %actor.name% pushes through the gooey membrane and into Aquis City beyond!
 return 1
 wait 1
%echoaround% %actor% %actor.name% pushes through the gooey membrane and arrives into the city here.
 %send% %actor% You push through the gooey membrane and into Aquis City beyond!
end
~
$~
