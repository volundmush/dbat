#6000
Do not delete this~
2 g 100
~
if (%actor.varexists(registered)%)
rdelete registered %actor.id%
end
~
#6015
yemma greet~
0 gh 100
~
if (%actor.is_pc%)
 if (%actor.align% >= -49)
  wait 2 s
  say Ah hello %actor.name%. You might be a bit surprised to find that you are dead.
  wait 5 s
  say This is the Afterlife and you are no longer part of the mortal coil.
  wait 4 s
  say You might find you enjoy being here more than you did living.
  wait 3 s
  say If you do not however there is something that can be done about it.
  wait 4 s
  say For a case such as your's I would rather you not be around here.
  wait 5 s
  say The specifics for that are none of your concern.
  wait 2 s
  say If you are interested then speak to me about revive.
 else
  wait 2 s
  say Oh... %actor.name%
  wait 4 s
  say Finally got the death you deserved huh?
  wait 3 s
  say Well you are not welcome around here.
  wait 3 s
  %send% %actor% Seems Yemma is not going to help you regain your former life. Perhaps
  %send% %actor% the Reaper in hell can help... Mwa ha ha ha...
 end
end
~
#6016
yemma teleport~
0 d 100
revive~
if (%speech% /= revive)
 eval num %actor.death%
 if (%actor.varexists(yrevive)% && %num% <= %ctime%)
  say Very Well %actor.name%, you have proven yourself worthy of returning to the living.
  wait 1 s
  say As I promised I shall send you back..
  wait 2 s
  raise %actor.name%
 elseif (%actor.varexists(yrevive)% && %num% > %ctime%)
  say You have proven yourself worthy of returning to life %actor.name%, however...
  wait 2 s
  %echo% %self.name% looks at his book.
  wait 2 s
  eval coun %num% - %ctime%
  eval count %coun% / 3600
  say I can't send you back just yet. You will need to wait about %count% hours (In Real Life)
  wait 4 s
  say Sorry it has to be that way or else the universe would descend into chaos (And death would be a slap on the wrist).
 elseif (%actor.align% < 0)
  wait 1 s
  say Your actions in life do not grant you any chance of revival from me.
  wait 3 s
  say You can go and beg the Reaper down in hell.
  wait 3 s
  say He may be merciful to scum like yourself.
  wait 3 s
  say However I will not.
 else
  wait 1 s
  say You haven't proven yourself worthy of revival.
  wait 3 s
  say To do so you will need to travel to the Grand Kai and speak with him.
  wait 4 s
  say He will instruct you on what to do to prove yourself worthy.
 end
end
~
#6017
Yemma Mute Revive~
0 c 100
revive~
if (%cmd% == revive)
 eval num %actor.death%
 if (%actor.varexists(yrevive)% && %num% <= %ctime%)
  say Very Well %actor.name%, you have proven yourself worthy of returning to the living.
  wait 1 s
  say As I promised I shall send you back..
  wait 2 s
  raise %actor.name%
 elseif (%actor.varexists(yrevive)% && %num% > %ctime%)
  say You have proven yourself worthy of returning to life %actor.name%, however...
  wait 2 s
  %echo% %self.name% looks at his book.
  wait 2 s
  eval coun %num% - %ctime%
  eval count %coun% / 3600
  say I can't send you back just yet. You will need to wait about %count% hours (In Real Life)
  wait 4 s
  say Sorry it has to be that way or else the universe would descend into chaos (And death would be a slap on the wrist).
 elseif (%actor.align% < 0)
  wait 1 s
  say Your actions in life do not grant you any chance of revival from me.
  wait 3 s
  say You can go and beg the Reaper down in hell.
  wait 3 s
  say He may be merciful to scum like yourself.
  wait 3 s
  say However I will not.
 else
  wait 1 s
  say You haven't proven yourself worthy of revival.
  wait 3 s
  say To do so you will need to travel to the Grand Kai and speak with him, or find another way.
  wait 4 s
  say He will instruct you on what to do to prove yourself worthy.
 end
end
~
#6029
Pilot Greeting~
0 g 100
~
if (%actor.align% >= -49)
 wait 1 s
 say Hello there. If you would like to see the wonderful world of the Grand Kai then give me 20 zenni for the fare.
else
 wait 1 s
 say You belong down below. Didn't Yemma talk to you yet?
end
~
#6030
Higher Plane Bribe~
0 m 0
~
wait 1
if (%actor.align% < -49)
 return 0
 say I don't want your money. The evil of your deeds has exempted you from visiting Otherworld.
else
 if (%amount% > 20)
  say Thanks for the tip. Hop aboard.
  %teleport% %actor% 6030
  mgoto 6030
  wait 1
  %force% %actor% look
  wait 2 s
  %send% %actor% %self.name% gets behind the flight-stick of the small plane and starts the engine.
  wait 4 s
  %send% %actor% The plane takes off the incredibly short runway flying through the yellow skies of the afterlife.
  wait 5 s
  %send% %actor% The plane sets down at otherworld gently and in an odd way.
  wait 3 s
  say I hope you enjoyed the flight.
  smile
  %teleport% %actor% 6800
  mgoto 6029
 elseif (%amount% < 20)
  return 0
  say That wasn't enough, keep the money...
 else
  say Hop aboard.
  %teleport% %actor% 6030
  mgoto 6030
  wait 1
  %force% %actor% look
  wait 2 s
  %send% %actor% %self.name% gets behind the flight-stick of the small plane and starts the engine.
  wait 4 s
  %send% %actor% The plane takes off the incredibly short runway flying through the yellow skies of the afterlife.
  wait 5 s
  %send% %actor% The plane sets down at otherworld gently and in an odd way.
  wait 3 s
  say I hope you enjoyed the flight.
  smile
  %teleport% %actor% 6800
  mgoto 6029
 end
end
~
#6031
Trudy Pilot Greet~
0 g 100
~
wait 1 sec
eval num %random.4%
switch (%num%)
 case 1
  %echo% %self.name% calmly watches a cloud float past.
  break
 case 2
  %echo% %self.name% hums softly to herself.
  break
 case 3
  %echo% %self.name% slowly counts a number of clouds that are floating nearby.
  break
 default
  %echo% %self.name% twirls a set of keys on one finger slowly.
  break
done
if (!%actor.varexists(trudy)%)
 wait 2 sec
 %echo% %self.name% seems to notice that she is not alone.
 wait 1 sec
 say AH! Are you a customer? I haven't had someone come by for a ride in ages...
 wait 3 sec
 say Everyone wants to go to Otherworld instead. Never to the Twilight Caverns. What a lousy job I have...
 wait 3 sec
 say Well if you want to go to the caverns just give me 100 zenni and I will fly you off there in a jiffy!
 set trudy
 remote trudy %actor.id%
end
~
#6032
Trudy Bribe Trigger~
0 m 1
~
if (%amount% < 100)
 wait 1
 say This isn't enough zenni.
 nop %actor.gold(%amount%)%
 nop %self.gold(-%amount%)%
elseif (%amount% > 100)
 wait 1
 say That's too much! I'm not greedy...
 nop %actor.gold(%amount%)%
 nop %self.gold(-%amount%)%
else
 wait 1
 say Alright, here we go!
 wait 1
 %teleport% %actor% 6030
 mgoto 6030
 wait 1
 %force% %actor% look
 wait 1 s
 %send% %actor% %self.name% gets behind the flight-stick of the small plane and starts the engine.
 wait 4 s
 %send% %actor% The plane takes off the incredibly short runway flying through the yellow skies of the afterlife.
 wait 5 s
 %send% %actor% The plane sets down at the entrance to Twilight Caverns gently and in an odd way.
 wait 3 s
 say I hope you enjoyed the flight.
 wait 1 s
 say If you want to get back all you need to do is hop down from here.
 smile
 %teleport% %actor% 7300
 mgoto 6027
end
~
#6040
Fortune Teller Baba greet~
0 g 100
~
if (%actor.is_pc%)
 if (%actor.align% >= -49)
  wait 2 s
  say Ah hello %actor.name%. Don't tell Yemma I said this but..
  wait 5 s
  say Grand Kai isn't the only one that can help you get back to the mortal realm.
  wait 4 s
  say And unlike him, I won't have you galavanting all around the afterlife either.
  wait 3 s
  say All you have to do is find me a new crystal ball, this one's on the fritz.
  wait 4 s
  say Bring me one and I'll have a word to that big oaf of an ogre about sending you back.
  wait 5 s
  emote gently taps her current crystal ball and looks into it, as if divining something.
  wait 2 s
  say Never know with this old piece of junk, but I'm inclined to think you might find one in Otherworld.
 else
  wait 2 s
  emote looks into her crystal ball, seemingly ignoring you.
 end
~
#6041
Fortune Teller Baba receive~
0 j 100
~
if (!%actor.varexists(yrevive)%)
  if (%object.vnum% == 7011)
    if (%actor.align% >= -49)
      wait 1
      %purge% %object%
      say Ahhh, yes. This one looks like it will do the trick.
      wait 2 s
      say Alright %actor.name%, you can go talk to Yemma.
      set yrevive
      remote yrevive %actor.id%
      wait 2 s
      say Go on, hurry up. Its not every day I put in a word for folks to be brought back to life!
  %send% %actor% @ROOC@n - @YIf you are a mute character, you can revive by typing @Rrevive@Y at King Yemma instead.@n
    else
      return 0
      wait 1 s
      say You are black of heart. I will not do a deal with you!
    end
  else
      return 0
      wait 1
      say I don't want that.
  end
else
  return 0
  wait 1
  say You are already set to be revived by Yemma silly!
end
~
$~
