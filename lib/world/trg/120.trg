#12000
Kibito Greet~
0 g 100
~
if (%actor.level% <= 2)
  wait 1
  say Welcome %actor.name%, I look forward to teaching you. On the way out, Sensuro should give you some advice. Oh, I'll also give you some @Gfood@C if you should ask for it.@n
  if !%actor.varexists(kibitoeq)%
    wait 2 sec
    say Now %actor.name% here is the official symbol of my students.
    wait 1 sec
    mload obj 12000
    give symbol %actor.name%
    set kibitoeq
    remote kibitoeq %actor.id%
  end
elseif (%actor.level% < 5)
  wait 1
  say %actor.name%, you will need to channel your enthusiasm into becoming much stronger.
  if %actor.varexists(kibitoeq)%
    rdelete kibitoeq %actor.id%
  end
elseif (%actor.level% < 10)
  wait 1
  say Strength is not key to becoming a great warrior. You must first achieve inner peace.
elseif (%actor.level% < 20)
  wait 1
  say You have now firmly planted your feet on the road to success, %actor.name%.
elseif (%actor.level% < 40)
  wait 1
  say %actor.name%, you've come far in your time as my pupil but there is still much to learn.
elseif (%actor.level% < 75)
  wait 1
  say Impressive, %actor.name%. I didn't think you had it in you to surpass my original expectations for you!
elseif (%actor.level% < 100)
  wait 1
  say You are amongst the finest warriors I have ever taught, %actor.name%.
elseif (%actor.level% == 100)
  wait 1
  say There is nothing left for me to teach you. Use your skills wisely.
else
  wait 1
  say %actor.name% I don't know what to say about you.
end
~
#12001
Sensuro Block~
0 q 100
~
if (%direction% == east && %actor.class% != Kibito)
 return 0
 wait 1
 say You do not have permission to enter further into the park. Please leave.
* elseif (%actor.carry%)
* return 0
* wait 1
* Say You may not carry anyone inside.
elseif (%actor.drag%)
 return 0
 wait 1
 say You may not drag anyone inside.
elseif (%direction% == east && %actor.class% == Kibito)
wait 1
 say Please continue, do not keep Kibito waiting too long.
%send% %actor% Sensuro says, 'Please continue, do not keep Kibito waiting too long.'
return 1
end
~
#12002
Sensuro noob quest greet~
0 g 100
~
if (%actor.class% == Kibito)
    if (%actor.level% <= 2)
        wait 1
        say Oh, hello there. You must be a new student of Kibito. Pleased to meet you.
        wait 2 s
        say The first place you should go to for training is the Kinetic Dojo. It's near by, the signs will tell you the way.
        wait 2 s
        say If you come back after you've trained there, I shall have a task to give you.
    
    elseif (%actor.level% >= 5 && %actor.level% <= 9 && !%actor.varexists(SenQ1Complete)% && !%actor.varexists(onSenQ1) && !%actor.varexists(SenQComplete)%)
        wait 1
        emote slightly bows to %actor.name%, &1Greetings, %actor.name%. I see your training is going well. I have a task for you.&2
        wait 2 s
say Go back to the dojo and fight the intermediate students. They will have a badge that you need to give to me.
        set onSenQ1
        remote onSenQ1 %actor.id%
        
    elseif (%actor.level% >= 10 && %actor.level% <= 14 && !%actor.varexists(SenQ2Complete)% && !%actor.varexists(onSenQ2)% && !%actor.varexists(SenQComplete)%)
        wait 1
        emote slightly bows to %actor.name%, &1Greetings, %actor.name%. You've grown a bit, I'm glad. I have another task for you.&2
        wait 2 s
        say To the east of the Park there is a beach, and on the beach are some crabs. Master Kibito would like some crab meat, as the crabs on Aether are the best around.
        set onSenQ2
        remote onSenQ2 %actor.id%
        
    elseif (%actor.level% >= 15 && %actor.level% <= 19 && !%actor.varexists(SenQ3Complete)% && !%actor.varexists(onSenQ3)% && !%actor.varexists(SenQComplete)%)
        wait 1
        emote slightly bows to %actor.name%, &1Greetings, %actor.name%. You've grown yet again. I'm sure Kibito is proud of your progress. I have another task for you to do.&2
        wait 2 s
        say To show your progress, I'd like you to goto the Advanced Kinetic Dojo and get the Advanced Badge from an instructor there.
        wait 2 s
        say The Advanced Kinetic Dojo is right next to the Kinetic Kai Dojo.
        set onSenQ3
        remote onSenQ3 %actor.id%
        
    elseif (%actor.level% >= 20 && !%actor.varexists(SenQ4Complete)% && !%actor.varexists(onSenQ4)% && !%actor.varexists(SenQComplete)%)
        wait 1
        emote slightly bows to %actor.name%, &1Greetings, %actor.name%. You've grown so much since I first met you. I have one final task for you.&2
        wait 2 s
        say I want you to head into the Kaiju forest and face the Makai there. Bring me back a Makai symbol.
        set onSenQ4
        remote onSenQ4 %actor.id%     
        
    else
        if (!%actor.varexists(onSenQ1)% && !%actor.varexists(onSenQ2)% &&  !%actor.varexists(onSenQ3)%  && !%actor.varexists(onSenQ4)%)
            wait 1
            say Hello, %actor.name%. Don't keep Kibito long, he has a lot to do.
        end
    end 
else
    wait 1
    say I'm sorry, but I can't let you inside.
end
~
#12003
Kibito Food Give~
0 d 100
food~
if (!%actor.varexists(kibitofood1)% && !%actor.varexists(kibitofood)%)
  wait 1 sec
  say Okay. I'll make some fresh for you.
  wait 2 sec
  emote holds out his hand and conjures up some food and drink.
  %load% obj 1101
  %load% obj 1101
  %load% obj 1101
  %load% obj 1101
  %load% obj 12028
  give all.cake %actor.name%
  give all.apple %actor.name%
  set kibitofood1
  remote kibitofood1 %actor.id%
elseif (!%actor.varexists(kibitofood2)% && !%actor.varexists(kibitofood)%)
  wait 1 sec
  emote chuckles, &1Alright, I can give you some more.&2
  wait 2 sec
  emote holds out his hand and conjures up some food and drink.
  %load% obj 1101
  %load% obj 1101
  %load% obj 1101
  %load% obj 1101
  %load% obj 12028
  give all.cake %actor.name%
  give all.apple %actor.name%
  set kibitofood2
  remote kibitofood2 %actor.id%
elseif (!%actor.varexists(kibitofood3)% && !%actor.varexists(kibitofood)%)
  wait 1 sec
  emote nods his head, &1I can give you some more, but after this you need to find your own food and drink source.&2
  wait 2 sec
  emote holds out his hand and conjures up some food and drink.
  %load% obj 1101
  %load% obj 1101
  %load% obj 1101
  %load% obj 1101
  %load% obj 12028
  give all.cake %actor.name%
  give all.apple %actor.name%
  rdelete kibitofood1 %actor.id%
  rdelete kibitofood2 %actor.id%
  set kibitofood
  remote kibitofood %actor.id%
else
  wait 1 sec
  say I'm sorry, %actor.name%, but it's time for you to fend for yourself in that regard.
end
~
#12004
Sensuro Quest Checkers~
0 g 100
~
if (%actor.varexists(onSenQ1)%)
    wait 1
    say Hello there, have you forgotten what I have asked of you?
    wait 2 s
@n    say Go to the Kinetic Kai Dojo and get a badge from one of the intermediates. Just follow the signs if you don't know how to get there.
 
elseif (%actor.varexists(onSenQ2)%)
    wait 1
    say Hello there, have you forgotten what I have asked of you?
    wait 2 s
    say Head to the beach to the east of the Park and bring back some Crab meat.
 
elseif (%actor.varexists(onSenQ3)%)
    wait 1
    say Hello there, have you forgotten what I have asked of you?
    wait 2 s
    say Go to the Advanced Kinetic Dojo and bring back an Advanced Medal from an instructor.
 
elseif (%actor.varexists(onSenQ4)%)
    wait 1
    say Hello there, have you forgotten what I have asked of you?
    wait 2 s
    say Head into the Forest to the South and bring back a Makai Symbol.
end
save
~
#12005
Sensuro Quest Recieve~
0 j 100
~
if (%actor.varexists(onSenQ1)% && %object.vnum% == 12031)
    %echoaround% %actor% %actor.name% gives Sensuro a small badge
    %purge% %object%
    wait 1
    say Very good. I have some Zenni here for you. Please return later and I shall have more for you to do.
    wait 1 s
    emote hands %actor.name% a small bag of Zenni.
    nop %actor.gold(500)%
    rdelete onSenQ1 %actor.id%
    set SenQ1Complete
    remote SenQ1Complete %actor.id%
    
elseif (%actor.varexists(onSenQ2)% && %object.vnum% == 12101)
    %echoaround% %actor% %actor.name% gives Sensuro some crab meat.
    %purge% %object%
    wait 1
    say Very good. I have some more Zenni for you, and some tickets.
    wait 2 s
    say The tickets are used at the Spaceport to take you to a Space Station that can take you all over the place to new planets. It's also a good place to meet new people.
    wait 1 s
    emote hands over a small sack of Zenni, and some tickets to %actor.name%
    nop %actor.gold(500)%
    %load% obj 3999
    %load% obj 3999
    %load% obj 3999
    %load% obj 3999
    give all.ticket %actor.name%
    wait 1
    say The spaceport is to the North East of here. Just follow the road to the west north, east, and then north east. And please return later for another task.
    rdelete onSenQ2 %actor.id%
    set SenQ2Complete
    remote SenQ2Complete %actor.id%
 
elseif (%actor.varexists(onSenQ3)% && %object.vnum% == 17700)
    %echoaround% %actor% %actor.name% gives Sensuro an advanced medal.
    %purge% %object%
    wait 1
    say Very good. I have a bigger reward for you this time. And be sure to return later for another task.
    wait 1 s
    emote hands %actor.name% a large sack of Zenni.
    nop %actor.gold(2000)%
    rdelete onSenQ3 %actor.id%
    set SenQ3Complete
    remote SenQ3Complete %actor.id%
 
elseif (%actor.varexists(onSenQ4)% && %object.vnum% == 12302)
    %echoaround% %actor% %actor.name% gives Sensuro a Makai Symbol.
    %purge% %object%
    wait 1
    say Very good. I'm glad you were up to the final task I had set for you.
    wait 2 s
    emote holds out his hand and a red Gi with Kibito's symbol on the front of it appears in his hand,  &1Please take this as your reward. It's a sign of a true Kibito student.&2   
    %load% obj 12032
    give gi %actor.name%
    rdelete onSenQ4 %actor.id%
    rdelete SenQ1Complete %actor.id%
    rdelete SenQ2Complete %actor.id%
    rdelete SenQ3Complete %actor.id%
    set SenQComplete
    remote SenQComplete %actor.id%
 
else
    return 0
    wait 1
    %echoaround% %actor% %actor.name% attempts to give Sensuro something.
    say I don't want this.
end
~
#12006
Kinetic Kai Instructor Drop~
0 f 100
~
if (%actor.varexists(onSenQ1)%)
    %load% obj 12031
end
~
#12007
Kai Battle Instructor Greet~
0 g 100
~
* @G*********************************************************@n
* @G First, we need to see what the status is of the actor@n
* @G and set it to a variable for us to use.@n
* @G*********************************************************@n
if %actor.varexists(kbmeadowtraining)%
  set kbmeadowtraining %actor.kbmeadowtraining%
else
  set kbmeadowtraining New
end
* @G*********************************************************@n
* @G Now, let's get their race and class so we can@n
* @G discriminate...@n
* @G*********************************************************@n
set actrace %actor.race%
set actclass %actor.class%
* @G*********************************************************@n
* @G Wait a second so room spam clears when they enter...@n
* @G*********************************************************@n
wait 1s
* @G*********************************************************@n
* @G Now, we check our variables to see how we should greet @n
* @G the actor...@n
* @G*********************************************************@n
* @G*********************************************************@n
* @G Are they coming from outside the meadow?
* @G*********************************************************@n
if %direction% == East
  * @G*********************************************************@n
  * @G Are they a not a Kai?@n
  * @G*********************************************************@n
  if %actrace% != Kai
    
    * @G*********************************************************@n
    * @G Are they not a Kibito student?@n
    * @G*********************************************************@n
    if %actclass% != Kibito
      say Sorry.  You are neither Kai nor a student of Kibito, so we have no intention of teaching you the ways of the Gentle Fist.
    else
      * @G*********************************************************@n
      * @G If they are a student of Kibito but not a Kai...@n
      * @G*********************************************************@n
      
      say Ahh, a student of Kibito.  Please, enter if you wish to study the ways of the Gentle Fist.
    end
  else
    * @G*********************************************************@n
    * @G If they are a Kai...@n
    * @G*********************************************************@n
    say Ahh, a fellow Kai.  Please, enter if you wish to study the ways of the Gentle Fist.
  end
else
  say Farewell, friend.  May peace go with you.
end
~
#12008
Combat Meadow Block~
2 q 100
w~
* @G*********************************************************@n
* @G Let's get their race and class so we can@n
* @G discriminate...@n
* @G*********************************************************@n
set actrace %actor.race%
set actclass %actor.class%
if %direction% == west
  if %actrace% != Kai
    if %actclass% != Kibito
      %echo% @RPsychic energy seems to prevent you from entering the training area of the Kai's...@n
      return 0
    else
      return 1
    end
  else
    return 1
  end
end
~
#12033
Kibito Mute~
0 c 3
point~
if %arg% == food
    if (!%actor.varexists(kibitofood1)% && !%actor.varexists(kibitofood)%)
        wait 1 sec
        say Okay. I'll make some fresh for you.
        wait 2 sec
        emote holds out his hand and conjures up some food and drink.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 12028
        give all.cake %actor.name%
        give all.apple %actor.name%
        set kibitofood1
        remote kibitofood1 %actor.id%
    elseif (!%actor.varexists(kibitofood2)% && !%actor.varexists(kibitofood)%)
        wait 1 sec
        emote chuckles, &1Alright, I can give you some more.&2
        wait 2 sec
        emote holds out his hand and conjures up some food and drink.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 12028
        give all.cake %actor.name%
        give all.apple %actor.name%
        set kibitofood2
        remote kibitofood2 %actor.id%
    elseif (!%actor.varexists(kibitofood3)% && !%actor.varexists(kibitofood)%)
        wait 1 sec
        emote nods his head, &1I can give you some more, but after this you need to find your own food and drink source.&2
        wait 2 sec
        emote holds out his hand and conjures up some food and drink.
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 1101
        %load% obj 12028
        give all.cake %actor.name%
        give all.apple %actor.name%
        rdelete kibitofood1 %actor.id%
        rdelete kibitofood2 %actor.id%
        set kibitofood
        remote kibitofood %actor.id%
    else
        wait 1 sec
        say I'm sorry, %actor.name%, but it's time for you to fend for yourself in that regard.
    end
else
    %send% %actor% Point at what?
end
~
#12050
Aether Punch Training~
0 c 100
quest~
if %actor.varexists(QKizonna)%
  set QKizonna %actor.QKizonna%
else
  set QKizonna 0
end
set QArg %arg%
eval stamrequired %actor.move% * 0.7
set victpunch %actor.skill(punch)%
eval victpunchtrained %victpunch% + 1
wait 1
if !%QArg%
  say Oh, so you seek to learn more about punching within the Gentle Fist?  
  wait 1s
  if %victpunch% > 50
    wait 1s
    say It seems like you already have the basics down.  Theory can only get you so far, you can only learn now from practicing what you have learned.
  else
    wait 1s
    if %actor.move% > %stamrequired%
      emote nods.  @C'Very well.  Let us begin, then...'@n  *%actor.name% and %self.name% practice for a bit in the ways of the Gentle Fist.  Their movements are graceful and non lethal.
      wait 2s
      %nop% %actor.move(-%stamrequired%)%
      %nop% %actor.skillset(punch %victpunchtrained%)%
      wait 1
      %send% %actor% @RYou feel your punch skill improve! @W(@Y%actor.skill(punch)%@W)@n
    else
      %send% %actor% @RYou don't seem to have enough stamina to train your @Ypunch@R skill right now... @W(@Y%stamrequired%@W)@n
    end
  end
else
  say Argument is %QArg%
end
~
#12098
Heiwana Wear Symbol~
0 n 100
~
* No Script
~
#12099
Respawn Me~
0 f 100
~
%load% m %self.vnum%
~
$~
