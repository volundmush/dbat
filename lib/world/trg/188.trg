#18800
Maintenance~
0 ab 100
~
set tank %self.room.contents(65)%
if (%tank%)
 if (%tank.health% <= 0)
  wait 1
  %echo% %self.name% takes a look at the broken healing tank for a few minutes before getting out his tools and begins to work on it.
  wait 5 s
  %echo% Removing a broken panel on the healing tank %self.name% then carefully bolts a new one in its place.
  wait 5 s
  %echo% %self.name% replaces some wiring inside the healing tank.
  wait 5 s
  %echo% %self.name% replaces the busted glass window of the healing tank. With that done the healing tank is now good as new again!
  %tank.health(100)%
 end
end
~
#18830
Kinoko Greet~
0 g 100
~
wait 2 s
say Welcome to the team headquarters, keep fighting and stay alive!
~
#18831
Serori greet~
0 g 100
~
wait 2 s
say We can't allow you to be pathetic. If you are hurt, then get in here, if not then leave!
~
$~
