#16000
Starport Guard~
0 q 100
~
if (%direction% == northwest && %actor.race% != Arlian && !%actor.inventory(16000)%)
return 0
say You can only enter Janacre if you have a pass, outsider.
wait 2 s
say I sell passes, if you want one. Otherwise, you'll have to go back where you came from.
wait 2 s
%send% %actor% Type list
elseif (%direction% == northwest && %actor.race% != Arlian && %actor.inventory(16000)%)
say Welcome to Janacre, outsider. Enjoy your stay and remember, keep to the main roads.
return 1
end
~
#16001
Aggro guard~
0 g 100
~
if (%actor.race% != Arlian && !%actor.inventory(16000)%)
say Stop, where's your pass? Oh, you don't have one! Die scum!
punch %actor.name%
elseif (%actor.race% != Arlian && %actor.inventory(16000)%)
say Carry on outsider.
end
~
#16002
Sizzez Shop Greet (foods)~
0 g 100
~
wait 2s
emote looks at *%actor.name% and her wings flutter quickly a couple times in excitement. "@RWelcome to Endoskelefoods, home of the Maggot Popper!@n"
~
$~
