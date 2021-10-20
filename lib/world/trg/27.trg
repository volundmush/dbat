#2700
Lieutenant Trigger~
0 g 100
~
wait 2 sec
%echo% %self.name% looks at a report.
wait 3 sec
say How the hell did they let that happen?
wait 3 sec
grumble
~
#2701
Zaltia Trigger~
0 g 100
~
eval num %random.3%
switch (%num%)
 case 1
 wait 2 sec
 %echo% The radio crackles as a voice can be heard over it.
 wait 2 sec
 %echo% Radio Voice 'Squad B requesting backup at last reported location, over.'
 wait 5 sec
 %echo% %self.name% presses a button and speaks into her microphone.
 wait 4 sec
 say Roger that Squad B, over.
 wait 3 sec
 %echo% %self.name% releases the button.
 wait 3 sec
 say Like hell you bastard. That will teach you to mess with me...
 break
 case 2
 wait 2 sec
 %echo% %self.name% takes a look at a chart before inputing data into the main computer terminal.
 wait 6 sec
 say This, this, and thiiiiis. Finished.
 break
 case 3
 wait 2 sec
 %echoaround% %actor% %self.name% looks up from her work at %actor.name%
 %send% %actor% %self.name% looks up from her work at you.
 wait 4 sec
 say What the hell do you want? I am busy...
 break
done
~
#2702
Grape Greet~
0 g 100
~
wait 2 sec
say Damn that Gourd.
wait 3 sec
say What does he think he is doing...
wait 3 sec
say How could Ryo make such an idiot his second in command?
wait 5 sec
curse
~
$~
