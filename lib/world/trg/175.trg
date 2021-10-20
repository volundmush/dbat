#17500
Farmer No get~
1 g 100
~
if (!%actor.is_pc%)
 return 0
end
~
#17501
Farmers~
0 bg 20
~
wait 1 s
eval num %random.4%
switch (%num%)
 case 1
 %echo% %self.name% reaches down and inspects a plant carefully.
 break
 case 2
 %echo% %self.name% carefully waters a plant.
 break
 default
 %echo% %self.name% stops to wipe %self.hisher% forehead.
break
done
~
#17502
Gustaf Greet~
0 g 100
~
if (!%actor.varexists(gustof)%)
 wait 1 s
 %echo% %self.name% looks up from his paper.
 wait 2 s
 say Ah, you must be the person I sent for.
 wait 3 s
 say Well? Are you? @GYes@C or @Gno@C?@n
 set gustof new
 remote gustof %actor.id%
else
 if (%actor.gustof% == new)
  wait 1 s
  %echo% %self.name% looks up from his paper.
  wait 2 s
  say You again? You still haven't answered my question...
 elseif (%actor.gustof% == yes)
  wait 1 s
  %echo% %self.name% looks up from his paper.
  wait 2 s
  say Those goods are still needing to be delivered...
 elseif (%actor.gustof% == no)
  wait 1 s
  %echo% %self.name% looks up from his paper.
  wait 2 s
  %echo% %self.name% What are you doing here? I told you to leave already!
 end
end
~
#17503
Gustof Speech~
0 d 100
yes~
if (!%actor.varexists(gustof)%)
 return 0
else
 if (%actor.gustof% == new)
  wait 1 s
  say Ah good. This is what I needed done and why I sent for someone of your qualifications. I need you to go to earth and purchase 100 loaves of wheat bread.
  wait 3 s
  say Bring that bread and deliver it to Greg's supply shop. He will take care of the payment. Once he has given you what I want return to me and give me the treasure he gave you. I will then pay you for a job well done.
  wait 3 s
  say Hurry on now...
  set gustof yes
  remote gustof %actor.id%
 elseif (%actor.gustof% == no)
  wait 1 s
  say You already said no. Don't be a liar now...
 elseif (%actor.gustof% == done)
  wait 1 s
  say You already completed the job. Get lost.
 elseif (%actor.gustof% == paid)
  wait 1 s
  say Give me what Greg gave you!
 end
end
~
#17504
Gustof No Speech~
0 d 100
no~
if (!%actor.varexists(gustof)%)
 return 0
else
 if (%actor.gustof% == new)
  wait 1 s
  say Fine then get lost.
  set gustof no
  remote gustof %actor.id%
 elseif (%actor.gustof% == yes)
  return 0
 elseif (%actor.gustof% == paid)
  wait 1 s
  say Don't you dare quit on the job before you give me the payment!
 elseif (%actor.gustof% == done)
  wait 1 s
  say You already finished the job, what are you saying no to?
 end
end
~
#17505
Gustof Final~
0 j 100
~
if (%object.vnum% != 17512)
 return 0
 wait 1 s
 say What is this? I don't need that...
else
 if (!%actor.varexists(gustof)%)
  %purge% %object%
  wait 1 s
  say I never made any deal with you, thanks for the free gem sucker.
 elseif (%actor.gustof% == no)
  %purge% %object%
  wait 1 s
  say I never made any deal with you, thanks for the free gem sucker.
 elseif (%actor.gustof% == yes)
  %purge% %object%
  wait 1 s
  say This isn't the gem Greg promised me, this must be a different one. Thanks for the free gem.
 elseif (%actor.gustof% == done)
  %purge% %object%
  wait 1 s
  say You're giving me another gem for free? Thanks!
 elseif (%actor.gustof% == paid)
  %purge% %object%
  wait 1 s
  say Ah good you completed the job. Here's your payment...
  set gustof done
  remote gustof %actor.id%
  %load% obj 160
  drop token
  %force% %actor% get token
 end
end
~
#17506
Greg's Receive~
0 j 100
~
if (!%actor.varexists(gustof)%)
 return 0
 wait 1 s
 say I don't want anything from you unless you are selling it legitimately, pal.
elseif (%actor.gustof% != yes)
 return 0
 wait 1 s
 say I don't feel right about receiving this from you right now....
else
 if (%object.vnum% == 543)
  %purge% %object%
  wait 1 s
  say Ah so Gustaf sent you to complete his deal, huh?
  wait 2 s
  say Alright here is my end. This is the only gem of Orium this size found on the entire planet.
  %load% obj 17512
  drop orium
  %force% %actor% get orium
  set gustof paid
  remote gustof %actor.id%
 else
  return 0
  wait 1 s
  say I don't want that.
 end
end
~
#17507
Yuu Greet~
0 g 100
~
if (%actor.varexists(gytalis)%)
  if (%actor.gytalis% != never)
    wait 1 s
    %echo% %self.name% looks up.
    wait 3 s
    say I sense something very wrong with you. You've been around that dullard Gytalis haven't you? I don't want any student of his in my presence.
    wait 2 s
    say LEAVE!
  elseif (%actor.varexists(yuutrain)%) 
    if (%actor.yuutrain% == orium)
      wait 1 s
      say Well are you going to get me that Orium or what?
      wait 3 s
      say I won't train you unless you do.
    elseif (%actor.yuutrain% == silver)
      wait 1 s
      say Haven't you fetched me that Fullmoon Silver Ore yet?
    elseif (%actor.yuutrain% == fruit)
      wait 1 s
      say Where is that golden fruit I asked for?
    elseif (%actor.yuutrain% == never)
      wait 1 s
      say You fool! You betray me and then come back for more!?
      round %actor.name%
    else
      wait 1 s
      say Ah my trusted apprentice....
    end
  else
    wait 1 s
    %echo% %self.name% looks up.
    wait 3 s
    say Who are you?
    %echo% [@mOOC Hint@n: Maybe you should @Ggreet@n him.]
  end
elseif (!%actor.varexists(yuutrain)%)
  wait 1 s
  %echo% %self.name% looks up.
  wait 3 s
  say Who are you?
  %echo% [@mOOC Hint@n: Maybe you should @Ggreet@n him.]
else
  if (%actor.yuutrain% == orium)
    wait 1 s
    say Well are you going to get me that Orium or what?
    wait 3 s
    say I won't train you unless you do.
  elseif (%actor.yuutrain% == silver)
    wait 1 s
    say Haven't you fetched me that Fullmoon Silver Ore yet?
  elseif (%actor.yuutrain% == fruit)
    wait 1 s
    say Where is that golden fruit I asked for?
  elseif (%actor.yuutrain% == never)
    wait 1 s
    say You fool! You betray me and then come back for more!?
    round %actor.name%
  else
    wait 1 s
    say Ah my trusted apprentice....
  end
end
~
#17508
Yuu Introduction~
0 c 100
greet~
wait 1 s
%send% %actor% You introduce yourself to Yuu.
%echoaround% %actor% %actor.name% introduces %actor.himher%self to Yuu.
if (%actor.varexists(gytalis)%)
  if (%actor.gytalis% != never)
    wait 2 s
    say I've heard of you. You are a pupil of that fool Gytalis. Begone...
  elseif (!%actor.varexists(yuutrain)%)
    wait 2 s
    say Ah. So have you come to train under me then?
    wait 2 s
    say I have much to teach a willing pupil. Though if you wish to train under me you will have to prove yourself.
    wait 3 s
    say There are some things I need, but I am currently constrained in where I can go. I was exiled here by the selfish leadership of Aether. Once I have what I need though that will not be a problem anymore.
    wait 4 s
    say I will tell you the first thing I require if you accept the deal. You bring me what I need in exchange for my passing on exclusive knowledge I possess to you.
    wait 4 s
    say Do you @Gaccept@C?@n
  end
elseif (%actor.varexists(yuutrain)%)
  if (%actor.yuutrain% == never)
    wait 1 s
    say SCUM!
  elseif (%actor.yuutrain% == orium)
    wait 1 s
    say I know who you are already! Where's that orium?
  elseif (%actor.yuutrain% == silver)
    wait 1 s
    say I know who you are already! Where's that Fullmoon Silver Ore?
  elseif (%actor.yuutrain% == fruit)
    wait 1 s
    say I know who you are already! Where's that Golden Fruit?
  else
    wait 1 s
    say I already know who you are, my apprentice...
  end
else
  wait 2 s
  say Ah. So have you come to train under me then?
  wait 2 s
  say I have much to teach a willing pupil. Though if you wish to train under me you will have to prove yourself.
  wait 3 s
  say There are some things I need, but I am currently constrained in where I can go. I was exiled here by the selfish leadership of Aether. Once I have what I need though that will not be a problem anymore.
  wait 4 s
  say I will tell you the first thing I require if you accept the deal. You bring me what I need in exchange for my passing on exclusive knowledge I possess to you.
  wait 4 s
  say Do you accept?
end
~
#17509
Yuu Accept~
0 c 100
accept~
set trainername none
if (%actor.varexists(yuutalis)%)
  if (%actor.yuutalis% == gytalis)
    set trainername gytalis
  elseif (%actor.yuutalis% == yuu)
    set trainername yuu
  end
end
if (%trainername% == gytalis)
  wait 1 s
  say You are a pupil of that fool Gytalis. BEGONE!
elseif (%trainername% == yuu || %trainername% == none)
  if (%actor.varexists(yuutrain)%)
    if (%actor.yuutrain% == never)
      wait 1 s
      say SCUM!
    elseif (%actor.yuutrain% == orium)
      wait 1 s
      say Where's that orium?
    elseif (%actor.yuutrain% == silver)
      wait 1 s
      say Where's that Fullmoon Silver Ore?
    elseif (%actor.yuutrain% == fruit)
      wait 1 s
      say Where's that Golden Fruit?
    else
      wait 1 s
      say What? Do you want to train or not?
    end
  else
    wait 1 s
    say Good.  Now the first thing I need you to obtain for me is an Orium Gem.  I've heard talk that a large one has been found recently and even the mayor of this wretched town is looking to obtain it.  I don't care how you get it.  Just bring it to me.
    set yuutrain orium
    set yuutalis yuu
    remote yuutalis %actor.id%
    remote yuutrain %actor.id%
  end
end
~
#17510
Yuu Final~
0 j 100
~
if (%actor.varexists(gytalis)%)
  if (%actor.gytalis% != never)
    return 0
    wait 1
    say I want NOTHING from you.
  elseif (!%actor.varexists(yuutrain)%)
    return 0
    wait 1
    say I don't want anything from you until you accept my deal.
  else
    if (%actor.yuutrain% == never)
      return 0
      wait 1 s
      say SCUM! Like I want anything from you!
    elseif (%actor.yuutrain% == orium)
      if (%object.vnum% == 17512)
        %purge% %object%
        wait 1 s
        say Good. Gooooood. This is a fine sample and it will serve me well.
        wait 2 s
        grin
        wait 2 s
        say Well anyway, next task for you to complete. On planet Earth I have heard there is a type of fine silver not found anywhere else. Bring me some of this silver ore.
        set yuutrain silver
        remote yuutrain %actor.id%
      else
        return 0
        wait 1 s
        say This is not what I want. Bring me the orium!
      end
    elseif (%actor.yuutrain% == silver)
      if (%object.vnum% == 2009)
        %purge% %object%
        wait 1 s
        say Yes this silver is of the highest quality. I have never seen silver this pure. You have done well!
        wait 2 s
        grin
        wait 2 s
        say The next and last task I have for you is to obtain the sacred Golden Fruit on planet Aether. This fruit is dear to those who betrayed me. Taking it from them serves two purposes for me. One it will piss them off, and secondly I need it to complete the ritual.
        set yuutrain fruit
        remote yuutrain %actor.id%
      else
        return 0
        wait 1 s
        say This is not what I want. Bring me that rare silver ore!
      end
    elseif (%actor.yuutrain% == fruit)
      if (%object.vnum% == 12301)
        %purge% %object%
        wait 1 s
        say EXCELLENT! You have done well my new apprentice. You may train with me whenever you wish. Just wait till after I complete this ritual...
        set ritualongoing
        remote ritualongoing %actor.id%
        set yuutrain complete
        remote yuutrain %actor.id%
        wait 4 s
        %echo% %self.name% takes the three items and walks over to a nearby table. %self.name% raises his hand above the table and forms a black iron cauldron out of thin air.
        wait 6 s
        %echo% %self.name% drops the Orium Gem into the cauldron. He then grabs both sides of the cauldron with his hands and begins to infuse energy into it.
        wait 8 s
        %echo% As the cauldron begins to glow bright red the Orium Gem inside melts into a bright green glowing liquid.
        wait 6 s
        %echo% %self.name% raises one hand from the cauldron and takes the Fullmoon Silve Ore. Holding it above the cauldron he lets it drop into the molten orium.
        wait 8 s
        %echo% Not wasting anytime %self.name% releases the cauldron completely and grabs the Golden Fruit. Tearing it in half he drops both halves into the mixture of silver and orium.
        wait 9 s
        %echo% A large explosion of energy erupts from the top of the cauldron for a brief second before disappearing. %self.name% quickly materializes a wooden mold next to the cauldron.
        wait 7 s
        %echo% Taking the cauldron %self.name% pours its contents into the mold and then begins cooling it quickly with superspeed swipes of air from his hands.
        wait 6 s
        %echo% %self.name% picks up the Universal Scepter from the mold as the ritual is now complete.
        %load% obj 17514
        wait 2 s
        kiss sceptre
        %echo% %self.name% cackles gleefuly.
        rdelete ritualongoing %actor.id%
      else
        return 0
        wait 1 s
        say This is not what I want. Bring me the Golden Fruit!
      end
    else
      wait 1 s
      say What? Do you want to train or not?
    end
  end
else
    if (%actor.yuutrain% == never)
      return 0
      wait 1 s
      say SCUM! Like I want anything from you!
    elseif (%actor.yuutrain% == orium)
      if (%object.vnum% == 17512)
        %purge% %object%
        wait 1 s
        say Good. Gooooood. This is a fine sample and it will serve me well.
        wait 2 s
        grin
        wait 2 s
        say Well anyway, next task for you to complete. On planet Earth I have heard there is a type of fine silver not found anywhere else. Bring me some of this silver ore.
        set yuutrain silver
        remote yuutrain %actor.id%
      else
        return 0
        wait 1 s
        say This is not what I want. Bring me the orium!
      end
    elseif (%actor.yuutrain% == silver)
      if (%object.vnum% == 2009)
        %purge% %object%
        wait 1 s
        say Yes this silver is of the highest quality. I have never seen silver this pure. You have done well!
        wait 2 s
        grin
        wait 2 s
        say The next and last task I have for you is to obtain the sacred Golden Fruit on planet Aether. This fruit is dear to those who betrayed me. Taking it from them serves two purposes for me. One it will piss them off, and secondly I need it to complete the ritual.
        set yuutrain fruit
        remote yuutrain %actor.id%
      else
        return 0
        wait 1 s
        say This is not what I want. Bring me that rare silver ore!
      end
    elseif (%actor.yuutrain% == fruit)
      if (%object.vnum% == 12301)
        %purge% %object%
        wait 1 s
        say EXCELLENT! You have done well my new apprentice. You may train with me whenever you wish. Just wait till after I complete this ritual...
        set ritualongoing
        remote ritualongoing %actor.id%
        set yuutrain complete
        remote yuutrain %actor.id%
        wait 4 s
        %echo% %self.name% takes the three items and walks over to a nearby table. %self.name% raises his hand above the table and forms a black iron cauldron out of thin air.
        wait 6 s
        %echo% %self.name% drops the Orium Gem into the cauldron. He then grabs both sides of the cauldron with his hands and begins to infuse energy into it.
        wait 8 s
        %echo% As the cauldron begins to glow bright red the Orium Gem inside melts into a bright green glowing liquid.
        wait 6 s
        %echo% %self.name% raises one hand from the cauldron and takes the Fullmoon Silve Ore. Holding it above the cauldron he lets it drop into the molten orium.
        wait 8 s
        %echo% Not wasting anytime %self.name% releases the cauldron completely and grabs the Golden Fruit. Tearing it in half he drops both halves into the mixture of silver and orium.
        wait 9 s
        %echo% A large explosion of energy erupts from the top of the cauldron for a brief second before disappearing. %self.name% quickly materializes a wooden mold next to the cauldron.
        wait 7 s
        %echo% Taking the cauldron %self.name% pours its contents into the mold and then begins cooling it quickly with superspeed swipes of air from his hands.
        wait 6 s
        %echo% %self.name% picks up the Universal Scepter from the mold as the ritual is now complete.
        %load% obj 17514
        wait 2 s
        kiss sceptre
        %echo% %self.name% cackles gleefuly.
        rdelete ritualongoing %actor.id%
      else
        return 0
        wait 1 s
        say This is not what I want. Bring me the Golden Fruit!
      end
  else
    wait 1 s
    say What? Do you want to train or not?
  end
end
~
#17511
Yuu Betray~
0 k 100
~
if (!%actor.varexists(yuutrain)%)
  if (%actor.varexists(gytalis)%)
    if (%actor.gytalis% != never)
      wait 1
      say You fool! Gytalis won't be able to save you from my wrath!
      set yuutrain never
      remote yuutrain %actor.id%
    else
      wait 1
      say You fool! Now I will never teach you the mysteries of psychic dominance!
      set yuutrain never
      remote yuutrain %actor.id%
    end
  else
   wait 1
   say You fool! Now I will never teach you the mysteries of psychic dominance!
   set yuutrain never
   remote yuutrain %actor.id%
  end
elseif (%actor.varexists(yuutrain)%)
  if (%actor.yuutrain% != never)
    wait 1
    say YOU FOOL! I would have taught you all I know!
    set yuutrain never
    remote yuutrain %actor.id%
  end
end
~
#17512
Yuu Practice~
0 c 100
pr~
if (!%actor.varexists(yuutrain)%)
  if (%actor.varexists(gytalis)%)
    if (%actor.gytalis% != never)
      return 1
      wait 1
      say I would never train you! Go train with that fool master of your's.
    else
      return 1
      wait 1
      say I don't even know you, and I have not made any deal with you for training.
    end
  else
   return 1
   wait 1
   say I don't even know you, and I have not made any deal with you for training.
  end
else
  if (%actor.yuutrain% == complete && !%actor.varexists(ritualongoing)%)
    return 0
    while (%self.has_item(17514)%)
      %purge% %self.inventory(17514)%
    done
  elseif (%actor.yuutrain% == never)
    return 1
    wait 1
    say I would never train someone who BETRAYED ME!
  elseif (%actor.varexists(ritualongoing)%)
    return 1
    wait 1
    say LET ME FINISH THE RITUAL!
  else
    return 1
    wait 1
    set object blank
    if (%actor.yuutrain% == orium)
      set object Orium Gem
    elseif (%actor.yuutrain% == silver)
      set object rare silver ore
    elseif (%actor.yuutrain% == fruit)
      set object golden fruit
    end
    say You haven't completed the task I gave you! Bring me the %object%!
  end
end
~
#17513
Gytalis Greet~
0 g 100
~
if (!%actor.varexists(gytalis)%)
 if (%actor.varexists(yuutrain)%)
  if (%actor.yuutrain% != never)
   wait 1
   say I want nothing to do with you. Yuu is evil and you serve him. Begone.
  else
   wait 1
   say Hello there. Are you here to train with me? I've not had a pupil in a long time now. Few know where to look for me afterall...
   wait 5 s
   say If you want to train with me though I will require you to prove yourself first. I'm planning on making the rarest most high quality scabbard to ever exist. Any fool can make a high quality sword, but what good is that without an equally high quality home for the sword?
   wait 6 s
   say So if you @Gaccept@C this deal I will tell you what to do. When you are done I will train you. Sound good, youngster?@n
  end
 else
  wait 1
  say Hello there. Are you here to train with me? I've not had a pupil in a long time now. Few know where to look for me afterall...
  wait 5 s
  say If you want to train with me though I will require you to prove yourself first. I'm planning on making the rarest most high quality scabbard to ever exist. Any fool can make a high quality sword, but what good is that without an equally high quality home for the sword?
  wait 6 s
  say So if you @Gaccept@C this deal I will tell you what to do. When you are done I will train you. Sound good, youngster?@n
 end
else
   if (%actor.gytalis% == diamond)
    wait 1
    say I'm still waiting on that Konatsu diamond, youngster.
   elseif (%actor.gytalis% == husk)
    wait 1
    say I'm still waiting on that especially large Karinak husk from Konack, youngster.
   elseif (%actor.gytalis% == ruby)
    wait 1
    say I'm still waiting on that massive ruby, youngster.
   elseif (%actor.gytalis% == never)
    wait 1
    say You betrayed me. What a fool you are... Please leave...
   else
    wait 1
    say You ready for more training?
   end
end
~
#17514
Gytalis Accept~
0 c 100
accept~
set trainername none
if (%actor.varexists(yuutalis)%)
  if (%actor.yuutalis% == gytalis)
    set trainername gytalis
  elseif (%actor.yuutalis% == yuu)
    set trainername yuu
  end
end
if (%trainername% == yuu)
  wait 1
  say Get lost. Go ask Yuu for training.
elseif (%trainername% == gytalis || %trainername% == none)
  if (%actor.varexists(gytalis)%)
    if (%actor.gytalis% == complete)
      wait 1
      say You already can train with me!
    elseif (%actor.gytalis% == never)
      wait 1
      say Leave my presence traitor.
    else
      wait 1
      set object blank
      if (%actor.gytalis% == diamond)
        set object diamond
      elseif (%actor.gytalis% == husk)
        set object Karinak husk
      elseif (%actor.gytalis% == ruby)
        set objet massive ruby
      end
      say I'm still waiting on that %object%.
    end
  else
    wait 1s
    say Good. Now here is what I want from you first. On Frigid in a certain mine there are various precious stones to be found. I want you to bring me a diamond from that mine.
    set gytalis diamond
    set yuutalis gytalis
    remote gytalis %actor.id%
    remote yuutalis %actor.id%
  end
~
#17515
Gytalis Receive~
0 j 100
~
if (!%actor.varexists(gytalis)%)
 if (%actor.varexists(yuutrain)%)
  if (%actor.yuutrain% != never)
   return 0
   wait 1
   say I want nothing from a pupil of Yuu!
  else
   return 0
   wait 1
   say I don't want anything from you until you @Gaccept@n my deal.
  end
 else
  return 0
  wait 1
  say I don't want anything from you until you @Gaccept@n my deal.
 end
else
 if (%actor.gytalis% == diamond)
  if (%object.vnum% == 5602)
   %purge% %object%
   wait 1
   say Excellent work. Now what I require from you is an exceptionally large Karinak Husk from Konack. Bring me the biggest one you can find.
   set gytalis husk
   remote gytalis %actor.id%
  else
   return 0
   wait 1
   say I want the diamond I asked for. Not this.
  end
 elseif (%actor.gytalis% == husk)
  if (%object.vnum% == 8401)
   %purge% %object%
   wait 1
   say Excellent work. The last thing I need you to bring me is a massive ruby. I'm not sure where you should look for that though. Good luck...
   set gytalis ruby
   remote gytalis %actor.id%
  else
   return 0
   wait 1
   say I want the Mighty Karinak Husk I asked for. Not this.
  end
 elseif (%actor.gytalis% == ruby)
  if (%object.vnum% == 3202)
   %purge% %object%
   wait 1
   set ritualgytalis
   remote ritualgytalis %actor.id%
   set gytalis complete
   remote gytalis %actor.id%
   say Yes, this is a very good ruby. Please excuse me a moment...
   wait 2 s
   %echo% %self.name% walks over to a work table in the corner of the cave.
   wait 4 s
   %echo% %self.name% takes out a specialized gem cutting tool and begins to cut the massive ruby into circular pieces.
   wait 6 s
   %echo% %self.name% inspects the ruby pieces carefully before moving on to the next step. Taking out a pair of huge sheers he begins to cut the husk into two halves of a scabbard.
   wait 7 s
   %echo% %self.name% takes the two halves and begins joining them together with the ruby pieces. He then takes the diamond and begins to attach it to the very bottom of the scabbard.
   wait 6 s
   %echo% %self.name% inspects the finished Dragon's Blood Scabbard and smiles.
   %load% obj 17515
   rdelete ritualgytalis %actor.id%
   wait 1 s
   say You may now train with me.
  else
   return 0
   wait 1
   say I want the massive ruby I asked for. Not this.
  end
 elseif (%actor.gytalis% == never)
  return 0
  wait 1
  say I want nothing from you traitor!
 else
  return 0
  wait 1
  say I don't want gifts from you my student. Are you going to train?
 end
end
~
#17516
Gytalis Practice~
0 c 100
pr~
if (%actor.varexists(yuutalis)%)
  if (%actor.yuutalis% == gytalis)
    if (!%actor.varexists(gytalis)%)
      if (%actor.varexists(yuutrain)%)
        if (%actor.yuutrain% != never)
          return 1
          wait 1
          say I would never train you! Go beg your evil master Yuu for lessons!
        else
          return 1
          wait 1
          say I have not made any deal with you for training.
        end
      else
        return 1
        wait 1
        say I have not made any deal with you for training.
      end
    else
      if (%actor.gytalis% == complete && !%actor.varexists(ritualgytalis)%)
        return 0
        while (%self.has_item(17515)%)
          %purge% %self.inventory(17515)%
        done
      elseif (%actor.gytalis% == never)
        return 1
        wait 1
        say I would never train someone who BETRAYED ME!
      elseif (%actor.varexists(ritualgytalis)%)
        return 1
        wait 1
        say I'm busy at the moment!
      else
        return 1
        wait 1
        set object blank
        if (%actor.gytalis% == diamond)
          set object diamond
        elseif (%actor.gytalis% == husk)
          set object Mighty Karinak Husk
        elseif (%actor.gytalis% == ruby)
          set object Massive Ruby
        end
        say You haven't completed the task I gave you. Bring me the %object%!
      end
    end
  else
    say I would never train you!  Go beg your evil master Yuu for lessons!
  end
end
~
#17517
Gytalis Betray~
0 k 100
~
if (!%actor.varexists(gytalis)%)
 if (%actor.varexists(yuutrain)%)
  if (%actor.yuutrain% != never)
   wait 1
   say You fool!
   set gytalis never
   remote gytalis %actor.id%
  else
   wait 1
   say You fool!
   set gytalis never
   remote gytalis %actor.id%
  end
 else
  wait 1
  say You fool!
  set gytalis never
  remote gytalis %actor.id%
 end
elseif (%actor.gytalis% != never)
 wait 1
 say You fool! I would have taught you so much. You've thrown it all away!
 set gytalis never
 remote gytalis %actor.id%
end
~
#17518
Trainer Death~
0 f 100
~
if (%self.vnum% == 17511)
 if (!%actor.varexists(yuutrain)%)
  set yuutrain never
  remote yuutrain %actor.id%
 elseif (%actor.yuutrain% != never)
  set yuutrain never
  remote yuutrain %actor.id%
 end
elseif (%self.vnum% == 17515)
 if (!%actor.varexists(gytalis)%)
  set gytalis never
  remote gytalis %actor.id%
 elseif (%actor.gytalis% != never)
  set gytalis never
  remote gytalis %actor.id%
 end
end
~
#17519
Gerrad Greet~
0 g 100
~
if (!%actor.varexists(gerrad)%)
 wait 2 s
 say Well hello there stranger.
 wait 3 s
 say You better be careful, this world's not entirely settled and can be quite dangerous.
 wait 5 s
 say Just a week ago we lost a man from the colony while he was on expedition into the nearby Orium Cave.
 wait 5 s
 say Greg the supply store owner was working with him when there was a collapse of the cave. I've ordered the cave off limits to colonists while the investigation is pending.
 wait 6 s
 say Some suspect Greg might have been involved, but the mayor has assured me that can't be the case. I wouldn't be doing my job if I didn't follow all leads though.
 wait 5 s
 say I wish I could get into that cave but I'm deathly afraid of those giant worms living in there. Hey, you look pretty strong. Could you go in and try to find me some evidence? I'll reward you appropriately. I have to know what happened to Paul. If you can't do it I may have to work up the nerve to look myself.
else
 wait 2 s
 say Thanks to the evidence you brought me I'm more certain than ever Greg was involved in Paul's death.
 wait 5 s
 say I just need to gather some concrete proof to prove it. Leave that up to me, you've done more than enough already.
end
~
#17520
Gerrad Receive~
0 j 100
~
if (!%actor.varexists(gerrad)%)
 if (%object.vnum% == 7803)
  %purge% %object%
  wait 1
  %echo% %self.name% takes the note and begins to read it.
  wait 3 s
  %echo% %self.name% looks up with a concerned expression on his face.
  wait 5 s
  say This note isn't clear but it appears to imply that Greg was involved in the collapse and ran off...
  wait 5 s
  say Poor Paul. Betrayed by a friend and he wasn't even aware of the betrayal despite surviving it for a short time. I hope his death wasn't too painful...
  wait 6 s
  say Well, this isn't enough evidence to link Greg to Paul's death. However it has put me on the right path. In fact the mayor might be involved on some level...
  wait 3 s
  say Anyway, here's your reward. Leave the rest of the investigation to me.
  set gerrad
  remote gerrad %actor.id%
  if (%actor.level% < 100)
   if (%actor.level% > 50)
    %actor.exp(5000000)%
    %send% %actor% You gain 5,000,000 exp!
   elseif (%actor.level% > 30)
    %actor.exp(1000000)%
    %send% %actor% You gain 1,000,000 exp!
   else
    %actor.exp(100000)%
    %send% %actor% You gain 100,000 exp!
   end
  end
  %load% obj 151
  drop token
  %force% %actor% get token
 else
  return 0
  say What would I want with that!?
 end
else
 return 0
 wait 1
 say I've already received evidence from you. Leave the rest to me.
end
~
#17521
Copy of 17507 Original (Yuu Greet)~
0 g 100
~
if (%actor.varexists(gytalis)%)
 if(%actor.gytalis% != never)
  wait 1 s
  %echo% %self.name% looks up.
  wait 3 s
  say I sense something very wrong with you. You've been around that dullard Gytalis haven't you? I don't want any student of his in my presence.
  wait 2 s
  say LEAVE!
 else
  if (%actor.yuutrain% == orium)
   wait 1 s
   say Well are you going to get me that Orium or what?
   wait 3 s
   say I won't train you unless you do.
  elseif (%actor.yuutrain% == silver)
   wait 1 s
   say Haven't you fetched me that Fullmoon Silver Ore yet?
  elseif (%actor.yuutrain% == fruit)
   wait 1 s
   say Where is that golden fruit I asked for?
  elseif (%actor.yuutrain% == never)
   wait 1 s
   say You fool! You betray me and then come back for more!?
   round %actor.name%
  else
   wait 1 s
   say Ah my trusted apprentice....
  end
 end
elseif (!%actor.varexists(yuutrain)%)
 wait 1 s
 %echo% %self.name% looks up.
 wait 3 s
 say Who are you?
 %echo% @D[@mOOC Hint@W: @wMaybe you should @Ggreet@w him.@D]@n
else
 if (%actor.yuutrain% == orium)
  wait 1 s
  say Well are you going to get me that Orium or what?
  wait 3 s
  say I won't train you unless you do.
 elseif (%actor.yuutrain% == silver)
  wait 1 s
  say Haven't you fetched me that Fullmoon Silver Ore yet?
 elseif (%actor.yuutrain% == fruit)
  wait 1 s
  say Where is that golden fruit I asked for?
 elseif (%actor.yuutrain% == never)
  wait 1 s
  say You fool! You betray me and then come back for more!?
  round %actor.name%
 else
  wait 1 s
  say Ah my trusted apprentice....
 end
end
~
#17522
Copy of 17508~
0 c 100
greet~
wait 1 s
%send% %actor% You introduce yourself to Yuu.
%echoaround% %actor% %actor.name% introduces %actor.himher%self to Yuu.
if (%actor.varexists(gytalis)% && %actor.gytalis% != never)
 wait 2 s
 say I've heard of you. You are a pupil of that fool Gytalis. Begone...
elseif (!%actor.varexists(yuutrain)%)
 wait 2 s
 say Ah. So have you come to train under me then?
 wait 2 s
 say I have much to teach a willing pupil. Though if you wish to train under me you will have to prove yourself.
 wait 3 s
 say There are some things I need, but I am currently constrained in where I can go. I was exiled here by the selfish leadership of Aether. Once I have what I need though that will not be a problem anymore.
 wait 4 s
 say I will tell you the first thing I require if you accept the deal. You bring me what I need in exchange for my passing on exclusive knowledge I possess to you.
 wait 4 s
 say Do you @Gaccept@C?@n
else
 if (%actor.yuutrain% == never)
  wait 1 s
  say SCUM!
 elseif (%actor.yuutrain% == orium)
  wait 1 s
  say I know who you are already! Where's that orium?
 elseif (%actor.yuutrain% == silver)
  wait 1 s
  say I know who you are already! Where's that Fullmoon Silver Ore?
 elseif (%actor.yuutrain% == fruit)
  wait 1 s
  say I know who you are already! Where's that Golden Fruit?
 else
  wait 1 s
  say I already know who you are, my apprentice...
 end
end
~
#17523
Copy of 17509~
0 c 100
accept~
if (%actor.varexists(gytalis)% && %actor.gytalis% != never)
 wait 1 s
 say You are a pupil of that fool Gytalis. BEGONE!
elseif (!%actor.varexists(yuutrain)%)
 wait 1 s
 say Good. Now the first thing I need you to obtain for me is an Orium Gem. I've heard talk that a large one has been found recently and even the mayor of this wretched town is looking to obtain it. I don't care how you get it. Just bring it to me. I'll tell you what to do next afterward.
 set yuutrain orium
 remote yuutrain %actor.id%
else
 if (%actor.yuutrain% == never)
  wait 1 s
  say SCUM!
 elseif (%actor.yuutrain% == orium)
  wait 1 s
  say Where's that orium?
 elseif (%actor.yuutrain% == silver)
  wait 1 s
  say Where's that Fullmoon Silver Ore?
 elseif (%actor.yuutrain% == fruit)
  wait 1 s
  say Where's that Golden Fruit?
 else
  wait 1 s
  say What? Do you want to train or not?
 end
end
~
#17524
Copy of 17510~
0 j 100
~
if (%actor.varexists(gytalis)% && %actor.gytalis% != never)
 return 0
 wait 1
 say I want NOTHING from you.
elseif (!%actor.varexists(yuutrain)%)
 return 0
 wait 1
 say I don't want anything from you until you @Gaccept@C my deal.@n
else
 if (%actor.yuutrain% == never)
  return 0
  wait 1 s
  say SCUM! Like I want anything from you!
 elseif (%actor.yuutrain% == orium)
  if (%object.vnum% == 17512)
   %purge% %object%
   wait 1 s
   say Good. Gooooood. This is a fine sample and it will serve me well.
   wait 2 s
   grin
   wait 2 s
   say Well anyway, next task for you to complete. On planet Earth I have heard there is a type of fine silver not found anywhere else. Bring me some of this silver ore.
   set yuutrain silver
   remote yuutrain %actor.id%
  else
   return 0
   wait 1 s
   say This is not what I want. Bring me the orium!
  end
 elseif (%actor.yuutrain% == silver)
if (%object.vnum% == 2009)
   %purge% %object%
   wait 1 s
   say Yes this silver is of the highest quality. I have never seen silver this pure. You have done well!
   wait 2 s
   grin
   wait 2 s
   say The next and last task I have for you is to obtain the sacred Golden Fruit on planet Aether. This fruit is dear to those who betrayed me. Taking it from them serves two purposes for me. One it will piss them off, and secondly I need it to complete the ritual.
   set yuutrain fruit
   remote yuutrain %actor.id%
  else
   return 0
   wait 1 s
   say This is not what I want. Bring me that rare silver ore!
  end
 elseif (%actor.yuutrain% == fruit)
if (%object.vnum% == 12301)
   %purge% %object%
   wait 1 s
   say EXCELLENT! You have done well my new apprentice. You may train with me whenever you wish. Just wait till after I complete this ritual...
   set ritualongoing
   remote ritualongoing %actor.id%
   set yuutrain complete
   remote yuutrain %actor.id%
   wait 4 s
   %echo% %self.name% takes the three items and walks over to a nearby table. %self.name% raises his hand above the table and forms a black iron cauldron out of thin air.
   wait 6 s
   %echo% %self.name% drops the Orium Gem into the cauldron. He then grabs both sides of the cauldron with his hands and begins to infuse energy into it.
   wait 8 s
   %echo% As the cauldron begins to glow bright red the Orium Gem inside melts into a bright green glowing liquid.
   wait 6 s
   %echo% %self.name% raises one hand from the cauldron and takes the Fullmoon Silve Ore. Holding it above the cauldron he lets it drop into the molten orium.
   wait 8 s
   %echo% Not wasting anytime %self.name% releases the cauldron completely and grabs the Golden Fruit. Tearing it in half he drops both halves into the mixture of silver and orium.
   wait 9 s
   %echo% A large explosion of energy erupts from the top of the cauldron for a brief second before disappearing. %self.name% quickly materializes a wooden mold next to the cauldron.
   wait 7 s
   %echo% Taking the cauldron %self.name% pours its contents into the mold and then begins cooling it quickly with superspeed swipes of air from his hands.
   wait 6 s
   %echo% %self.name% picks up the Universal Scepter from the mold as the ritual is now complete.
   %load% obj 17514
   wait 2 s
   kiss sceptre
   %echo% %self.name% cackles gleefuly.
   rdelete ritualongoing %actor.id%
  else
   return 0
   wait 1 s
   say This is not what I want. Bring me the Golden Fruit!
  end
 else
  wait 1 s
  say What? Do you want to train or not?
 end
end
~
#17525
copy of 17511~
0 k 100
~
if (!%actor.varexists(yuutrain)%)
 if (%actor.varexists(gytalis)% && %actor.gytalis% != never)
  wait 1
  say You fool! Gytalis won't be able to save you from my wrath!
  set yuutrain never
  remote yuutrain %actor.id%
 else
  wait 1
  say You fool! Now I will never teach you the mysteries of psychic dominance!
  set yuutrain never
  remote yuutrain %actor.id%
 end
elseif (%actor.yuutrain% != never)
 wait 1
 say YOU FOOL! I would have taught you all I know!
 set yuutrain never
 remote yuutrain %actor.id%
end
~
#17526
copy of 17512~
0 c 100
pr~
if (!%actor.varexists(yuutrain)%)
 if (%actor.varexists(gytalis)% && %actor.gytalis% != never)
  return 1
  wait 1
  say I would never train you! Go train with that fool master of your's.
 else
  return 1
  wait 1
  say I don't even know you, and I have not made any deal with you for training.
 end
else
 if (%actor.yuutrain% == complete && !%actor.varexists(ritualongoing)%)
  return 0
  while (%self.has_item(17514)%)
   %purge% %self.inventory(17514)%
  done
 elseif (%actor.yuutrain% == never)
  return 1
  wait 1
  say I would never train someone who BETRAYED ME!
 elseif (%actor.varexists(ritualongoing)%)
  return 1
  wait 1
  say LET ME FINISH THE RITUAL!
 else
  return 1
  wait 1
  set object blank
  if (%actor.yuutrain% == orium)
   set object Orium Gem
  elseif (%actor.yuutrain% == silver)
   set object rare silver ore
  elseif (%actor.yuutrain% == fruit)
   set object golden fruit
  end
  say You haven't completed the task I gave you! Bring me the %object%!
 end
end
~
#17527
Copy of 17513~
0 g 100
~
if (!%actor.varexists(gytalis)%)
 if (%actor.varexists(yuutrain)% && %actor.yuutrain% != never)
  wait 1
  say I want nothing to do with you. Yuu is evil and you serve him. Begone.
 else
  wait 1
  say Hello there. Are you here to train with me? I've not had a pupil in a long time now. Few know where to look for me afterall...
  wait 5 s
  say If you want to train with me though I will require you to prove yourself first. I'm planning on making the rarest most high quality scabbard to ever exist. Any fool can make a high quality sword, but what good is that without an equally high quality home for the sword?
  wait 6 s
  say So if you @Gaccept@C this deal I will tell you what to do. When you are done I will train you. Sound good, youngster?@n
 end
else
 if (%actor.gytalis% == diamond)
  wait 1
  say I'm still waiting on that Konatsu diamond, youngster.
 elseif (%actor.gytalis% == husk)
  wait 1
  say I'm still waiting on that especially large Karinak husk from Konack, youngster.
 elseif (%actor.gytalis% == ruby)
  wait 1
  say I'm still waiting on that massive ruby, youngster.
 elseif (%actor.gytalis% == never)
  wait 1
  say You betrayed me. What a fool you are... Please leave...
 else
  wait 1
  say You ready for more training?
 end
end
~
#17528
Copy of 17514~
0 c 100
accept~
if (!%actor.varexists(gytalis)%)
 if (%actor.varexists(yuutrain)% && %actor.yuutrain% != never)
  wait 1
  say Get lost. Go ask Yuu for training.
 else
  wait 1
say Good. Now here is what I want from you first. On Frigid in a certain mine there are various precious stones to be found. I want you to bring me a diamond from that mine.
  set gytalis diamond
  remote gytalis %actor.id%
 end
elseif (%actor.gytalis% == complete)
 wait 1
 say You already can train with me!
elseif (%actor.gytalis% == never)
 wait 1
 say Leave my presence traitor.
else
 wait 1
 set object blank
 if (%actor.gytalis% == diamond)
  set object diamond
 elseif (%actor.gytalis% == husk)
  set object Karinak husk
 elseif (%actor.gytalis% == ruby)
  set objet massive ruby
 end
 say I'm still waiting on that %object%.
end
~
#17529
Copy of 17515~
0 j 100
~
if (!%actor.varexists(gytalis)%)
 if (%actor.varexists(yuutrain)% && %actor.yuutrain% != never)
  return 0
  wait 1
  say I want nothing from a pupil of Yuu!
 else
  return 0
  wait 1
  say I don't want anything from you until you @Gaccept@n my deal.
 end
else
 if (%actor.gytalis% == diamond)
  if (%object.vnum% == 5602)
   %purge% %object%
   wait 1
   say Excellent work. Now what I require from you is an exceptionally large Karinak Husk from Konack. Bring me the biggest one you can find.
   set gytalis husk
   remote gytalis %actor.id%
  else
   return 0
   wait 1
   say I want the diamond I asked for. Not this.
  end
 elseif (%actor.gytalis% == husk)
  if (%object.vnum% == 8401)
   %purge% %object%
   wait 1
   say Excellent work. The last thing I need you to bring me is a massive ruby. I'm not sure where you should look for that though. Good luck...
   set gytalis ruby
   remote gytalis %actor.id%
  else
   return 0
   wait 1
   say I want the Mighty Karinak Husk I asked for. Not this.
  end
 elseif (%actor.gytalis% == ruby)
  if (%object.vnum% == 3202)
   %purge% %object%
   wait 1
   set ritualgytalis
   remote ritualgytalis %actor.id%
   set gytalis complete
   remote gytalis %actor.id%
   say Yes, this is a very good ruby. Please excuse me a moment...
   wait 2 s
   %echo% %self.name% walks over to a work table in the corner of the cave.
   wait 4 s
   %echo% %self.name% takes out a specialized gem cutting tool and begins to cut the massive ruby into circular pieces.
   wait 6 s
   %echo% %self.name% inspects the ruby pieces carefully before moving on to the next step. Taking out a pair of huge sheers he begins to cut the husk into two halves of a scabbard.
   wait 7 s
   %echo% %self.name% takes the two halves and begins joining them together with the ruby pieces. He then takes the diamond and begins to attach it to the very bottom of the scabbard.
   wait 6 s
   %echo% %self.name% inspects the finished Dragon's Blood Scabbard and smiles.
   %load% obj 17515
   rdelete ritualgytalis %actor.id%
   wait 1 s
   say You may now train with me.
  else
   return 0
   wait 1
   say I want the massive ruby I asked for. Not this.
  end
 elseif (%actor.gytalis% == never)
  return 0
  wait 1
  say I want nothing from you traitor!
 else
  return 0
  wait 1
  say I don't want gifts from you my student. Are you going to train?
 end
end
~
#17530
Copy of 17516~
0 c 100
pr~
if (!%actor.varexists(gytalis)%)
 if (%actor.varexists(yuutrain)% && %actor.yuutrain% != never)
  return 1
  wait 1
  say I would never train you! Go beg your evil master Yuu for lessons!
 else
  return 1
  wait 1
  say I have not made any deal with you for training.
 end
else
 if (%actor.gytalis% == complete && !%actor.varexists(ritualgytalis)%)
  return 0
  while (%self.has_item(17515)%)
   %purge% %self.inventory(17515)%
  done
 elseif (%actor.gytalis% == never)
  return 1
  wait 1
  say I would never train someone who BETRAYED ME!
 elseif (%actor.varexists(ritualgytalis)%)
  return 1
  wait 1
  say I'm busy at the moment!
 else
  return 1
  wait 1
  set object blank
  if (%actor.gytalis% == diamond)
   set object diamond
  elseif (%actor.gytalis% == husk)
   set object Mighty Karinak Husk
  elseif (%actor.gytalis% == ruby)
   set object Massive Ruby
  end
  say You haven't completed the task I gave you. Bring me the %object%!
 end
end
~
#17531
Copy of 17517~
0 k 100
~
if (!%actor.varexists(gytalis)%)
 if (%actor.varexists(yuutrain)% && %actor.yuutrain% != never)
  wait 1
  say You fool!
  set gytalis never
  remote gytalis %actor.id%
 else
  wait 1
  say You fool!
  set gytalis never
  remote gytalis %actor.id%
 end
elseif (%actor.gytalis% != never)
 wait 1
 say You fool! I would have taught you so much. You've thrown it all away!
 set gytalis never
 remote gytalis %actor.id%
end
~
#17532
Varstat Adjuster (REMOVE FROM ROOM WHEN DONE)~
2 c 100
resetme~
set gytrain diamond
remote gytrain %actor.id%
%send% %actor% @CVar 'yuutrain' has been reset.@n
set gytalis diamond
remote gytalis %actor.id%
unset gytalis %actor.id%
~
#17533
VarFixer for those Already In Quest~
2 c 100
fixme~
set yuutalis gytalis
remote yuutalis %actor.id%
%echo% @RYou have been fixed.  Report to Sedalis so that he may check.@n
~
#17540
Damanos Greet~
0 g 50
~
if %actor.level% >= 50
 if (!%actor.varexists(damanosrequest)% && !%actor.varexists(damanoscomplete)%)
  wait 1 s
  %send% %actor% @WDamanos whispers to you, "@CYou look like the sort that might be interested in acquiring an orium gem.@W"@n
  wait 2 s
  %send% %actor% @WDamanos whispers to you, "@CJust so happens that I might be the kind of person who has one.@W"@n
  wait 2 s
  %send% %actor% @WDamanos whispers to you, "@CSay @Ryes@C if you are indeed the sort of person who would like to own an orium gem.@W"@n
  end
end
~
#17541
Damanos Speech~
0 d 100
yes~
if (!%actor.varexists(damanoscomplete)%)
 if (!%actor.varexists(damanosrequest)%)
  set damanosrequest
  remote damanosrequest %actor.id%
  wait 1 s
  %send% %actor% @WDamanos whispers to you, "@CGood. I'll keep it simple. There's a rather costly painting I want.@W"@n
  wait 2 s
  %send% %actor% @WDamanos whispers to you, "@CIt is on sale at Giovanni's Oddities in the space mall. Buy it and bring it to me.@W"@n
 else
  wait 1 s
  %send% %actor% @WDamanos whispers to you, "@CI already told you what to do. Get the painting from Giovanni's Oddities and bring it to me.@W"@n
  end
else
 wait 1 s
 %send% %actor% @WDamanos whispers to you, "@CYou've already got your orium gem. Now bugger off.@W"@n
end
~
#17542
Damanos quest complete~
0 j 100
~
if (%object.vnum% == 17208)
 if (!%actor.varexists(damanoscomplete)%)
  set damanoscomplete
  remote damanoscomplete %actor.id%
  %purge% %object%
  wait 2 s
  %send% %actor% @WDamanos whispers to you, "@CWell, what do you know. You must really have wanted this orium gem. Quite the exquisite painting, no?@W"@n
  wait 2 s
  %send% %actor% @WDamanos whispers to you, "@COh, who am I kidding. You don't look the type to appreciate fine art. But here's your reward. One orium gem.@W"@n
  wait 1 s
  %load% obj 17512
  drop orium
  %force% %actor% get orium
  if (%actor.varexists(damanosrequest)%)
   rdelete damanosrequest %actor.id%
  end
 else
  return 0
  %send% %actor% @WDamanos whispers to you, "@CDon't be foolish. You've already given me the real painting. Begone with your fakes.@W"@n
 end
else
 return 0
 %send% %actor% @WDamanos whispers to you, "@CI don't want %object.name%!@w"@n
end
~
$~
