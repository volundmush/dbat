#7000
Grand Kai Greet~
0 g 100
~
if (!%actor.varexists(yrevive)%)
 wait 1 s
 say Ah %actor.name%, King Yemma told me you would be stopping by.
 wait 3 s
 say So you wish to return to the land of the living do you?
 wait 5 s
 say Groovy! However I can't just allow people to be returned to life so easily.
 wait 5 s
 say I need you to bring me something in order to prove your worth.
 wait 4 s
 say This is also to be fair to the other warriors on Otherworld who never got this chance.
 wait 4 s
 say What I want you to bring me is Princess Snake's golden teacup.
 wait 5 s
 say That teacup is part of a set that I want to complete.
 wait 3 s
 say She however hates me so much that she refuses to trade it to me.
 wait 4 s
 say Go @gtrade@C with her to get it.
 wait 3 s
 say When you have it give it to me and I will grant Yemma to revive you.
else
 wait 1 s
 say What are you still doing hanging around here %actor.name%?
end
~
#7001
Grand Kai Receive~
0 j 100
~
if (!%actor.varexists(yrevive)%)
 if (%object.vnum% == 7002)
  wait 1
  %purge% %object%
  say Ah finally! I have completed this set.
  wait 3 s
  say Alright %actor.name%, you can go talk to Yemma.
  set yrevive
  remote yrevive %actor.id%
  wait 2 s
  say I give permission for you to be sent back to the living world.
 else
  return 0
  wait 1
  say I don't want that.
 end
else
 wait 1
 %purge% %object%
 say You are already set to be revived by Yemma.
 wait 2 s
 say Hurry and go talk to him.
end
~
#7002
Grand Kai Sit~
0 n 100
~
wait 1
sit throne
~
#7005
Grand Trainer~
0 g 100
~
wait 1 s
say Ah hello. The Grand Kai is north of here.
wait 2 s
say I on the other hand can help you achieve a higher level! Though I don't know any skills...
~
$~
