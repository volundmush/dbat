#14100
Gurs Greet~
0 g 100
~
wait 1
%echo% Mumbling can be heard from Gurs as he meditates.
wait 3 s
%echo% The entire tree stump begins to shake as Gurs releases some of his ki.
~
#14101
Gurs Fight~
0 k 25
~
wait 1
%echo% Gurs places two fingers on his forehead and disappears!
eval num 14100 + %random.88%
 if (%num% + %self.room.vnum% > 14189)
  mgoto 14189
elseif (%num% == %self.room.vnum%)
eval num2 %num% - 1
mgoto %num2%
 else
  mgoto %num%
 end
~
$~
