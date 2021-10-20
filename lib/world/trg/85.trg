#8500
Mysterious Man greet~
0 g 100
~
if %actor.level% > 79 && !%actor.varexists(mysteriousman)%
  wait 2 s
  %send% %actor% A mysterious man whispers to you, "You look capable. I have work if you are interested. Just ask me about the work and we'll come to an arrangement."
  %echoaround% %actor% %self.name% whispers something to %actor.name%
end
~
#8501
Inducts for Mysterious Man as well as offers work~
0 d 100
*~
if %speech% == work
  if %actor.level% > 79 && !%actor.varexists(mysteriousman)
    wait 2 s
    say Alright, %actor.name%. I will now give you jobs.
    set mysteriousman
    remote mysteriousman %actor.id%
    set mysteriousmanwork one
    remote mysteriousmanwork %actor.id%
    wait 2 s
    say Just say phoenix and I will give you further instructions.
  end
elseif %speech% == phoenix
  if (%actor.varexists(mysteriousman)% && %actor.varexists(mysteriousmanwork)%)
    if (%actor.mysteriousmanwork% == five && !%actor.varexists(doingmmfive)%)
      wait 2 s
      say I've got another job for you, %actor.name%.
      wait 2 s
      say On planet Yardrat, there is a creature called Gurs.
      wait 2 s
      set doingmmfive
      remote doingmmfive %actor.id%
      say Kill Gurs and bring me a trophy.
    elseif (%actor.mysteriousmanwork% == four && !%actor.varexists(doingmmfour)%)
      wait 2 s
      say I've got another job for you, %actor.name%.
      wait 2 s
      say Head to Frigid and hunt down a man known as Grandmaster Joffre.
      set doingmmfour
      remote doingmmfour %actor.id%
      wait 2 s
      say Kill Joffre and bring me back a trophy.
    elseif (%actor.mysteriousmanwork% == three && !%actor.varexists(doingmmthree)%)
      wait 2 s
      say I've got another job for you, %actor.name%.
      wait 2 s
      say Some say the Truffles are extinct.. not entirely true.
      wait 2 s
      set doingmmthree
      remote doingmmthree %actor.id%
      say Find a Truffle named Doctor Radek and kill him.
    elseif (%actor.mysteriousmanwork% == two && !%actor.varexists(doingmmtwo)%)
      wait 2 s
      say I've got another job for you, %actor.name%.
      wait 2 s
      say There is a being known as the Archvile.
      wait 2 s
      set doingmmtwo
      remote doingmmtwo %actor.id%
      say Find him and kill him.
    elseif (%actor.mysteriousmanwork% == one && !%actor.varexists(doingmmone)%)
      wait 2 s
      say I've got a job for you, %actor.name%.
      wait 2 s
      say I need to you to kill one of Cooler's top men.
      wait 2 s
      set doingmmone
      remote doingmmone %actor.id%
      say Kill Salza and return to me with a trophy as proof.
    else
      wait 2 s
      say I don't have any work for you.
    end
  end
end
~
#8502
***Not in Use***~
0 d 100
*~
if %speech% == phoenix
  if (%actor.varexists(mysteriousman)% && %actor.varexists(mysteriousmanwork)%)
    if (%actor.mysteriousmanwork% == five && !%actor.varexists(doingmmfive)%)
      wait 2 s
      say I've got another job for you, %actor.name%.
      wait 2 s
      say On planet Yardrat, there is a creature called Gurs.
      wait 2 s
      set doingmmfive
      remtoe doingmmfive %actor.id%
      say Kill Gurs and bring me a trophy.
    elseif (%actor.mysteriousmanwork% == four && !%actor.varexists(doingmmfour)%)
      wait 2 s
      say I've got another job for you, %actor.name%.
      wait 2 s
      say Head to Frigid and hunt down a man known as Grandmaster Joffre.
      set doingmmfour
      remote doingmmfour %actor.id%
      wait 2 s
      say Kill Joffre and bring me back a trophy.
    elseif (%actor.mysteriousmanwork% == three && !%actor.varexists(doingmmthree)%)
      wait 2 s
      say I've got another job for you, %actor.name%.
      wait 2 s
      say Some say the Truffles are extinct.. not entirely true.
      wait 2 s
      set doingmmthree
      remote doingmmthree %actor.id%
      say Find a Truffle named Doctor Radek and kill him.
    elseif (%actor.mysteriousmanwork% == two && !%actor.varexists(doingmmtwo)%)
      wait 2 s
      say I've got another job for you, %actor.name%.
      wait 2 s
      say There is a being known as the Archvile.
      wait 2 s
      set doingmmtwo
      remote doingmmtwo %actor.id%
      say Find him and kill him.
    elseif (%actor.mysteriousmanwork% == one && !%actor.varexists(doingmmone)%)
      wait 2 s
      say I've got a job for you, %actor.name%.
      wait 2 s
      say I need to you to kill one of Cooler's top men.
      wait 2 s
      set doingmmone
      remote doingmmone %actor.id%
      say Kill Salza and return to me with a trophy as proof.
    else
      wait 2 s
      say I don't have any work for you.
    end
  end
end
~
#8503
Mysterious Man complete work~
0 g 100
~
if %actor.varexists(completedmmfive)%
  wait 2 s
  say I can tell you have killed Gurs, %actor.name%.
  wait 2 s
  say And so, here is your reward.
  wait 2 s
  emote hands %actor.name% a pouch of zenni and takes the trophy from them.
  nop %actor.gold(80000)%
  %actor.exp(20000000)%
  rdelete completedmmfive %actor.id%
  set mysteriousmanwork six
  remote mysteriousmanwork %actor.id%
elseif %actor.varexists(completedmmfour)%
  wait 2 s
  say I can tell you have killed Grandmaster Joffre, %actor.name%.
  wait 2
  emote hands %actor.name% a pouch of zenni and takes the trophy from them.
  nop %actor.gold(60000)%
  %actor.exp(15000000)%
  rdelete completedmmfour %actor.id%
  set mysteriousmanwork five
  remote mysteriousmanwork %actor.id%
elseif %actor.varexists(completedmmthree)%
  wait 2 s
  say I can tell you have killed Doctor Radek, %actor.name%.
  wait 2
  emote hands %actor.name% a pouch of zenni and takes the trophy from them.
  nop %actor.gold(50000)%
  %actor.exp(12500000)%
  rdelete completedmmthree %actor.id%
  set mysteriousmanwork four
  remote mysteriousmanwork %actor.id%
elseif %actor.varexists(completedmmtwo)%
  wait 2 s
  say I can tell you have killed the Archvile, %actor.name%.
  wait 2
  emote hands %actor.name% a pouch of zenni and takes the trophy from them.
  nop %actor.gold(40000)%
  %actor.exp(10000000)%
  rdelete completedmmtwo %actor.id%
  set mysteriousmanwork three
  remote mysteriousmanwork %actor.id%
elseif %actor.varexists(completedmmone)%
  wait 2 s
  say I can tell you have killed Salza, %actor.name%.
  wait 2
  emote hands %actor.name% a pouch of zenni and takes the trophy from them.
  nop %actor.gold(30000)%
  %actor.exp(7500000)%
  rdelete completedmmone %actor.id%
  set mysteriousmanwork two
  remote mysteriousmanwork %actor.id%
end
~
#8504
Gurs death trigger for Mysterious Man~
0 f 100
~
if %actor.varexists(doingmmfive)%
  rdelete doingmmfive %actor.id%
  set completedmmfive
  remote completedmmfive %actor.id%
  wait 2 s
  %send% %actor% You take a trophy from the corpse of Gurs.
  wait 2 s
  %send% %actor% You should return to the Mysterious Man to get your reward.
end
~
#8505
Joffre death trigger for Mysterious Man work~
0 f 100
~
if %actor.varexists(doingmmfour)%
  rdelete doingmmfour %actor.id%
  set completedmmfour
  remote completedmmfour %actor.id%
  wait 2 s
  %send% %actor% You take a trophy from the corpse of Joffre.
  wait 2 s
  %send% %actor% You should return to the Mysterious Man to get your reward.
end
~
#8506
Radek Death trigger for Mysterious Man Work~
0 f 100
~
if %actor.varexists(doingmmthree)%
  rdelete doingmmthree %actor.id%
  set completedmmthree
  remote completedmmthree %actor.id%
  wait 2 s
  %send% %actor% You take a trophy from the corpse of Radek.
  wait 2 s
  %send% %actor% You should return to the Mysterious Man to get your reward.
end
~
#8507
The Archvile Death trigger of Mysterious Man work~
0 f 100
~
if %actor.varexists(doingmmtwo)%
  rdelete doingmmtwo %actor.id%
  set completedmmtwo
  remote completedmmtwo %actor.id%
  wait 2 s
  %send% %actor% You take a trophy from the corpse of the Archvile.
  wait 2 s
  %send% %actor% You should return to the Mysterious Man to get your reward.
end
~
#8508
Salza deathtrigger for Mysterious Man work~
0 f 100
~
if %actor.varexists(doingmmone)%
  rdelete doingmmone %actor.id%
  set completedmmone
  remote completedmmone %actor.id%
  wait 2 s
  %send% %actor% You take a trophy from the corpse of Salza.
  wait 2 s
  %send% %actor% You should return to the Mysterious Man to get your reward.
end
~
$~
