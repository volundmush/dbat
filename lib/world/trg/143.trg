#14300
Deep Master Greet~
0 g 100
~
if (!%actor.varexists(proof)%)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me a hide and a pincer.
elseif (%actor.proof% /= deep)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings. You have already obtained my proof. I have nothing more for you. 
else
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me a hide and a pincer.
end
~
#14301
Deep Master Receive~
0 j 100
~
if (%object.vnum% == 13057)
 if (!%actor.varexists(deep1)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set deep1
  remote deep1 %actor.id%
 end
elseif (%object.vnum% == 13056)
 if (!%actor.varexists(deep2)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set deep2
  remote deep2 %actor.id%
 end
else
 return 0
 wait 1
 say This is not what is required to obtain my proof.
end
if (%actor.varexists(deep1)% && %actor.varexists(deep2)%)
 wait 1
 say You have proven yourself worthy of what I offer. Here is my proof.
 %load% obj 14300
 give proof %actor.name%
 rdelete deep1 %actor.id%
 rdelete deep2 %actor.id%
 if (!%actor.varexists(proof)%)
  set proof ^deep
  remote proof %actor.id%
 else
  set proof %actor.proof% ^deep
  remote proof %actor.id%
 end
end
~
#14302
Shallow Master Greet~
0 g 100
~
if (!%actor.varexists(proof)%)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me a tentacle and a fang.
elseif (%actor.proof% /= shallow)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings. You have already obtained my proof. I have nothing more for you. 
else
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me a tentacle and a fang
end
~
#14303
Shallow Master Receive~
0 j 100
~
if (%object.vnum% == 2501)
 if (!%actor.varexists(shal1)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set shal1
  remote shal1 %actor.id%
 end
elseif (%object.vnum% == 2700)
 if (!%actor.varexists(shal2)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set shal2
  remote shal2 %actor.id%
 end
else
 return 0
 wait 1
 say This is not what is required to obtain my proof.
end
if (%actor.varexists(shal1)% && %actor.varexists(shal2)%)
 wait 1
 say You have proven yourself worthy of what I offer. Here is my proof.
 %load% obj 14301
 give proof %actor.name%
 rdelete shal1 %actor.id%
 rdelete shal2 %actor.id%
 if (!%actor.varexists(proof)%)
  set proof ^shallow
  remote proof %actor.id%
 else
  set proof %actor.proof% ^shallow
  remote proof %actor.id%
 end
end
~
#14304
Body master Greet~
0 g 100
~
if (!%actor.varexists(proof)%)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me a leg and a wing.
elseif (%actor.proof% /= body)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings. You have already obtained my proof. I have nothing more for you. 
else
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me a leg and a wing.
end
~
#14305
Body Master Receive~
0 j 100
~
if (%object.vnum% == 4106)
 if (!%actor.varexists(body1)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set body1
  remote body1 %actor.id%
 end
elseif (%object.vnum% == 4409)
 if (!%actor.varexists(body2)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set body2
  remote body2 %actor.id%
 end
else
 return 0
 wait 1
 say This is not what is required to obtain my proof.
end
if (%actor.varexists(body1)% && %actor.varexists(body2)%)
 wait 1
 say You have proven yourself worthy of what I offer. Here is my proof.
 %load% obj 14302
 give proof %actor.name%
 rdelete body1 %actor.id%
 rdelete body2 %actor.id%
 if (!%actor.varexists(proof)%)
  set proof ^body
  remote proof %actor.id%
 else
  set proof %actor.proof% ^body
  remote proof %actor.id%
 end
end
~
#14306
Light Master Greet~
0 g 100
~
if (!%actor.varexists(proof)%)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me a tail and a claw.
elseif (%actor.proof% /= light)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings. You have already obtained my proof. I have nothing more for you. 
else
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me a tail and a claw.
end
~
#14307
Light Master Receive~
0 j 100
~
if (%object.vnum% == 9003)
 if (!%actor.varexists(light1)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set light1
  remote light1 %actor.id%
 end
elseif (%object.vnum% == 8512)
 if (!%actor.varexists(light2)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set light2
  remote light2 %actor.id%
 end
else
 return 0
 wait 1
 say This is not what is required to obtain my proof.
end
if (%actor.varexists(light1)% && %actor.varexists(light2)%)
 wait 1
 say You have proven yourself worthy of what I offer. Here is my proof.
 %load% obj 14303
 give proof %actor.name%
 rdelete light1 %actor.id%
 rdelete light2 %actor.id%
 if (!%actor.varexists(proof)%)
  set proof ^light
  remote proof %actor.id%
 else
  set proof %actor.proof% ^light
  remote proof %actor.id%
 end
end
~
#14308
Dark Master Greet~
0 g 100
~
if (!%actor.varexists(proof)%)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me a scale and a stinger.
elseif (%actor.proof% /= dark)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings. You have already obtained my proof. I have nothing more for you. 
else
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me a scale and a stinger.
end
~
#14309
Dark Master Receive~
0 j 100
~
if (%object.vnum% == 13207)
 if (!%actor.varexists(dark1)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set dark1
  remote dark1 %actor.id%
 end
elseif (%object.vnum% == 13208)
 if (!%actor.varexists(dark2)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set dark2
  remote dark2 %actor.id%
 end
else
 return 0
 wait 1
 say This is not what is required to obtain my proof.
end
if (%actor.varexists(dark1)% && %actor.varexists(dark2)%)
 wait 1
 say You have proven yourself worthy of what I offer. Here is my proof.
 %load% obj 14304
 give proof %actor.name%
 rdelete dark1 %actor.id%
 rdelete dark2 %actor.id%
 if (!%actor.varexists(proof)%)
  set proof ^dark
  remote proof %actor.id%
 else
  set proof %actor.proof% ^dark
  remote proof %actor.id%
 end
end
~
#14310
Sense Master Greet~
0 g 100
~
if (!%actor.varexists(proof)%)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me an eye and a tongue.
elseif (%actor.proof% /= sense)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings. You have already obtained my proof. I have nothing more for you. 
else
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me an eye and a tongue.
end
~
#14311
Sense Master Receive~
0 j 100
~
if (%object.vnum% == 12402)
 if (!%actor.varexists(sense1)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set sense1
  remote sense1 %actor.id%
 end
elseif (%object.vnum% == 12401)
 if (!%actor.varexists(sense2)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set sense2
  remote sense2 %actor.id%
 end
else
 return 0
 wait 1
 say This is not what is required to obtain my proof.
end
if (%actor.varexists(sense1)% && %actor.varexists(sense2)%)
 wait 1
 say You have proven yourself worthy of what I offer. Here is my proof.
 %load% obj 14305
 give proof %actor.name%
 rdelete sense1 %actor.id%
 rdelete sense2 %actor.id%
 if (!%actor.varexists(proof)%)
  set proof ^sense
  remote proof %actor.id%
 else
  set proof %actor.proof% ^sense
  remote proof %actor.id%
 end
end
~
#14312
Emotion Master Greet~
0 g 100
~
if (!%actor.varexists(proof)%)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me a heart and a brain.
elseif (%actor.proof% /= emotion)
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings. You have already obtained my proof. I have nothing more for you. 
else
 wait 1
 %echo% %self.name% chants quietly as %self.heshe% meditates.
 wait 2 s
 %echo% %self.name% opens %self.hisher% eyes slowly.
 wait 3 s
 say Greetings pilgrim of Temperance. Do you seek my proof on your journey to enlightenment?
 wait 5 s
 say If you seek my proof you must bring me two rare trophies from rare beasts found throughout the universe.
 wait 4 s
 say You must bring me a heart and a brain.
end
~
#14313
Emotion Master Receive~
0 j 100
~
if (%object.vnum% == 14101)
 if (!%actor.varexists(emot1)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set emot1
  remote emot1 %actor.id%
 end
elseif (%object.vnum% == 14201)
 if (!%actor.varexists(emot2)%)
  wait 1
  %purge% %object%
  say You have completed this part of obtaining my proof.
  set emot2
  remote emot2 %actor.id%
 end
else
 return 0
 wait 1
 say This is not what is required to obtain my proof.
end
if (%actor.varexists(emot1)% && %actor.varexists(emot2)%)
 wait 1
 say You have proven yourself worthy of what I offer. Here is my proof.
 %load% obj 14306
 give proof %actor.name%
 rdelete emot1 %actor.id%
 rdelete emot2 %actor.id%
 if (!%actor.varexists(proof)%)
  set proof ^emotion
  remote proof %actor.id%
 else
  set proof %actor.proof% ^emotion
  remote proof %actor.id%
 end
end
~
#14314
Proof Master Receive~
0 j 100
~
if (%object.vnum% == 14300)
 if (%actor.varexists(water1)%)
  return 0
  wait 1
  say You have already presented me with that proof. Why do you have another?
 elseif (%actor.varexists(water2)%)
  wait 1
  %purge% %object%
  say You have earned the Water Proof.
  wait 1 s
  rdelete water1 %actor.id%
  rdelete water2 %actor.id%
  %load% obj 14307
  give water %actor.name%
 else
  wait 1
  %purge% %object%
  say You have completed the first step towards the Water Proof.
  set water1
  remote water1 %actor.id%
 end
elseif (%object.vnum% == 14301)
 if (%actor.varexists(water2)%)
  return 0
  wait 1
  say You have already presented me with that proof. Why do you have another?
 elseif (%actor.varexists(water1)%)
  wait 1
  %purge% %object%
  say You have earned the Water Proof.
  wait 1 s
  rdelete water1 %actor.id%
  rdelete water2 %actor.id%
  %load% obj 14307
  give water %actor.name%
 else
  wait 1
  %purge% %object%
  say You have completed the first step towards the Water Proof.
  set water2
  remote water2 %actor.id%
 end
elseif (%object.vnum% == 14303)
 if (%actor.varexists(lum1)%)
  return 0
  wait 1
  say You have already presented me with that proof. Why do you have another?
 elseif (%actor.varexists(lum2)%)
  wait 1
  %purge% %object%
  say You have earned the Luminescence Proof.
  wait 1 s
  rdelete lum1 %actor.id%
  rdelete lum2 %actor.id%
  %load% obj 14308
  give lumin %actor.name%
 else
  wait 1
  %purge% %object%
  say You have completed the first step towards the Luminescence Proof.
  set lum1
  remote lum1 %actor.id%
 end
elseif (%object.vnum% == 14304)
 if (%actor.varexists(lum2)%)
  return 0
  wait 1
  say You have already presented me with that proof. Why do you have another?
 elseif (%actor.varexists(lum1)%)
  wait 1
  %purge% %object%
  say You have earned the Luminescence Proof.
  wait 1 s
  rdelete lum1 %actor.id%
  rdelete lum2 %actor.id%
  %load% obj 14308
  give lumin %actor.name%
 else
  wait 1
  %purge% %object%
  say You have completed the first step towards the Luminescence Proof.
  set lum2
  remote lum2 %actor.id%
 end
elseif (%object.vnum% == 14305)
 if (%actor.varexists(spirit1)%)
  return 0
  wait 1
  say You have already presented me with that proof. Why do you have another?
 elseif (%actor.varexists(spirit2)%)
  wait 1
  %purge% %object%
  say You have earned the Spirit Proof.
  wait 1 s
  rdelete spirit1 %actor.id%
  rdelete spirit2 %actor.id%
  %load% obj 14309
  give spirit %actor.name%
 else
  wait 1
  %purge% %object%
  say You have completed the first step towards the Spirit Proof.
  set spirit1
  remote spirit1 %actor.id%
 end
elseif (%object.vnum% == 14306)
 if (%actor.varexists(spirit2)%)
  return 0
  wait 1
  say You have already presented me with that proof. Why do you have another?
 elseif (%actor.varexists(spirit1)%)
  wait 1
  %purge% %object%
  say You have earned the Spirit Proof.
  wait 1 s
  rdelete spirit1 %actor.id%
  rdelete spirit2 %actor.id%
  %load% obj 14309
  give spirit %actor.name%
 else
  wait 1
  %purge% %object%
  say You have completed the first step towards the Spirit Proof.
  set spirit2
  remote spirit2 %actor.id%
 end
elseif (%object.vnum% == 14302)
 if (%actor.varexists(phys1)%)
  return 0
  wait 1
  say You have already presented me with that proof. Why do you have another?
 elseif (%actor.varexists(phys2)%)
  wait 1
  %purge% %object%
  say You have earned the Physical Proof.
  wait 1 s
  rdelete phys1 %actor.id%
  rdelete phys2 %actor.id%
  %load% obj 14310
  give physical %actor.name%
 else
  wait 1
  %purge% %object%
  say You have completed the first step towards the Physical Proof.
  set phys1
  remote phys1 %actor.id%
 end
elseif (%object.vnum% == 14307)
 if (%actor.varexists(phys2)%)
  return 0
  wait 1
  say You have already presented me with that proof. Why do you have another?
 elseif (%actor.varexists(phys1)%)
  wait 1
  %purge% %object%
  say You have earned the Physical Proof.
  wait 1 s
  rdelete phys1 %actor.id%
  rdelete phys2 %actor.id%
  %load% obj 14310
  give physical %actor.name%
 else
  wait 1
  %purge% %object%
  say You have completed the first step towards the Physical Proof.
  set phys2
  remote phys2 %actor.id%
 end
elseif (%object.vnum% == 14308)
 if (%actor.varexists(trans1)%)
  return 0
  wait 1
  say You have already presented me with that proof. Why do you have another?
 elseif (%actor.varexists(trans2)%)
  wait 1
  %purge% %object%
  say You have earned the Transcendent Proof.
  wait 1 s
  rdelete trans1 %actor.id%
  rdelete trans2 %actor.id%
  %load% obj 14311
  give transc %actor.name%
 else
  wait 1
  %purge% %object%
  say You have completed the first step towards the Transcendent Proof.
  set trans1
  remote trans1 %actor.id%
 end
elseif (%object.vnum% == 14309)
 if (%actor.varexists(trans2)%)
  return 0
  wait 1
  say You have already presented me with that proof. Why do you have another?
 elseif (%actor.varexists(trans1)%)
  wait 1
  %purge% %object%
  say You have earned the Transcendent Proof.
  wait 1 s
  rdelete trans1 %actor.id%
  rdelete trans2 %actor.id%
  %load% obj 14311
  give transc %actor.name%
 else
  wait 1
  %purge% %object%
  say You have completed the first step towards the Transcendent Proof.
  set trans2
  remote trans2 %actor.id%
 end
elseif (%object.vnum% == 14310)
 if (%actor.varexists(rare1)%)
  return 0
  wait 1
  say You have already presented me with that proof. Why do you have another?
 elseif (%actor.varexists(rare2)%)
  wait 1
  %purge% %object%
  say You have earned the Rare Hunter Proof!
  wait 1 s
  rdelete rare1 %actor.id%
  rdelete rare2 %actor.id%
  %load% obj 14312
  give rare %actor.name%
  set rareh
  remote rareh %actor.id%
 else
  wait 1
  %purge% %object%
  say You have completed the first step towards the Rare Hunter Proof.
  set rare1
  remote rare1 %actor.id%
 end
elseif (%object.vnum% == 14311)
 if (%actor.varexists(rare2)%)
  return 0
  wait 1
  say You have already presented me with that proof. Why do you have another?
 elseif (%actor.varexists(rare1)%)
  wait 1
  %purge% %object%
  say You have earned the Rare Hunter Proof!
  wait 1 s
  rdelete rare1 %actor.id%
  rdelete rare2 %actor.id%
  %load% obj 14312
  give rare %actor.name%
  set rareh
  remote rareh %actor.id%
 else
  wait 1
  %purge% %object%
  say You have completed the first step towards the Rare Hunter Proof.
  set rare2
  remote rare2 %actor.id%
 end
else
 return 0
 wait 1
 say This is not something I desire.
end
~
#14315
Guard Trigger~
0 q 100
~
if (%self.room.vnum% == 14350)
 if (%direction% == north && (!%actor.has_item(14312)% || !%actor.varexists(rareh)%))
   return 0
   say Only those who have proven themselves may enter.
 elseif (%direction% == north && %actor.carry%)
   return 0
   say Come in one at a time please.
 elseif (%actor.drag%)
   return 0
   say Come in one at a time please.
 end
else
 if (%direction% == south && (!%actor.has_item(14312)% || !%actor.varexists(rareh)%))
  return 0
  say Only those who have proven themselves may enter.
 elseif (%direction% == south && %actor.carry%)
   return 0
   say Come in one at a time please.
 elseif (%direction% == south && %actor.drag%)
   return 0
   say Come in one at a time please.
 end
end
~
#14316
Guardian Greet~
0 g 100
~
wait 1
say Greetings! I see you hold the Proof of Rare Hunter.
wait 2 s
say This means you may train with the master below and learn our most sacred technique. Also...
wait 4 s
say It means I may teach you a technique that goes hand in hand with what the master teaches. I do suggest you learn it first.
~
#14317
Temperance Master Greet~
0 g 100
~
wait 1
%echo% %self.name% mumbles as he meditates.
~
#14318
NO!~
2 g 100
~
if (%actor.drag%)
 return 0
 wait 1
 %send% %actor% You can't drag someone into this place.
end
if (%actor.level% < 60)
 return 0
 wait 1
 %send% %actor% You must be at least level 60 to enter this place.
end
~
$~
