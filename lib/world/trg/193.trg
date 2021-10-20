#19300
Shadow Lantern~
1 j 100
~
wait 1
%echo% An eerie glow emits from the lantern...
~
#19301
Chief Talk~
0 g 100
~
if (%actor.is_pc%)
   wait 1 s
   emote nods his head at *%actor.name%. &1Have you come to take the Trial?&2
   wait 5
   %send% %actor% <-------------------------------------->
   %send% %actor% [ Trial?                               ]
   %send% %actor% [ Yes.                                 ]
   %send% %actor% [ Tell me about the village.           ]
   %send% %actor% [ I have completed the Trial.          ]
   %send% %actor% <-------------------------------------->
   %send% %actor% Use 'select <option>'.
end
~
#19302
Selection~
0 c 100
select~
if (%arg% == Trial? || %arg% == trial || %arg% ==trial?)
  %echoaround% %actor% %actor.name% asks about the Trial.
  %send% %actor% You ask about the Trial.
  wait 1 s
  emote gives %actor.name% a weird look. &1You mean you came all this way without even knowing what the Trial is?&2
  wait 3 s
  emote moves into a more comfortable position before speaking. &1This temple was built long ago. We don't know by whom, for the knowledge was lost in the currents of time, but we know that our people were assigned to guard it. They designed the temple to  wait 2 s
  emote sighs and shakes his head. &1I can still sometimes hear the screams of those who do not make it.&2
  wait 3 s
  say Within the temple lies Shadowmist Maze, which is filled with traps and the Shadows of warriors who have come and gone from all over the galaxy. While in the maze, you may find that abilities you have relied on in the past no longer work. Not even we  wait 3 s
  emote yawns for a second before resuming, &1There are also some rumors floating around about how the Temple itself will keep you from performing some actions. Though I do not know if this rumor is true.&2
  wait 10
  %send% %actor% <-------------------------------------->
  %send% %actor% [ I want to take it.                   ]
  %send% %actor% [ Have you entered?                    ]
  %send% %actor% <-------------------------------------->
  
  
elseif (%arg% == I want to take it. || %arg% == yes || %arg% == yes. || %arg% == i want to take it || %arg% == i want to take it. || %arg% == i want to take the trial || %arg% == I want to take the trial)
  %echoaround% %actor% %actor.name% tells the Chief %actor.heshe% wants to take on the Trial.
  %send% %actor% You inform the Chief that you wish to take the Trial.
  if (!%actor.varexists(smazecomplete)%)
    if (!%actor.varexists(smazeallowed)%)
      wait 1 s
      emote studies %actor.name%. &1So you think you are strong enough to take on the Trial, eh?&2
      wait 20
      if (%actor.level% >= 90)
        emote nods his head. &1Yes, you seem like you are strong enough to take the Trial.&2
        wait 2 s
        say I warn you, once you enter the Maze, you will not be able to leave. So you best be ready when you enter.
        wait 2 s
        emote nods his head at one of his Aides. They rush out of the room. &1I have sent word to the guard to let him know that you are allowed through. Good luck.&2
        set smazeallowed
        remote smazeallowed %actor.id%
      else
        say Sorry, but you are too weak to enter.
      end
    elseif (%actor.varexists(smazeallowed)%)
      wait 2 s
      say But you're already allowed in!
    end
  elseif (%actor.varexists(smazecomplete)%)
    wait 2 s
    emote chuckles, &1You've already beaten the maze, there would be no point in sending you back in!&2
  end
  
  
elseif (%arg% == Have you entered? || %arg% == have you entered || %arg% == have you entered?)
  %echoaround% %actor% %actor.name% asks if the Chief has ever entered.
  %send% %actor% You ask the Chief if he has ever entered.
  wait 1 s
  emote shakes his head. &1We are forbidden to take the Trial. I do want to see what is inside the maze, though.&2
  wait 10
  %send% %actor% <-------------------------------------->
  %send% %actor% [ I want to take it.                   ]
  %send% %actor% <-------------------------------------->
  
  
elseif (%arg% == tell me about the village || %arg% == Tell me about the village. || %arg% == Tell me about the village)
  %echoaround% %actor% %actor.name% asks the Chief about the village.
  %send% %actor% You ask the Chief about the village.
  wait 1 s
  emote rubs his chin. &1You want to know about the Village, eh?&2 He lets out a low 'hmm' before he resumes speaking, &1Well there isn't much to tell, really. This village has been around longer than anyone here can remember.&2
  wait 2 s
  emote shifts in his chair, &1Generations have lived here. We don't get too many visitors, and sometimes people decide to pack up and leave. I'm sure you've probably seen the empty houses in town.&2
  wait 2 s
  emote rubs one of his temples as he sighs, &1I can remember when this was a grand village, when warriors would come here all the time to challenge themselves in the maze. Not many came out, mind you, but that didn't stop them.&2
  wait 2 s
  %send% %actor% <-------------------------------------->
  %send% %actor% [ Trial?                               ]
  %send% %actor% [ I want to take the trial.            ]
  %send% %actor% <-------------------------------------->
  
elseif (%arg% == I have completed the Tral. || %arg% == i have completed the Trial || %arg% == i have completed the Trial. || %arg% == I have completed the Trial)
  %echoaround% %actor% %actor.name% informs the Chief he has completed the Trial.
  %send% %actor% You you inform the Chief you have completed the trial.
  if (%actor.varexists(smazecomplete)%)
    if (%actor.inventory(19405)% && !%actor.varexists(gotboots)%)
      wait 2 s
      emote leans forward in his seat, &1Well I'll be. You've actually completed the Trial!&2
      wait 2 s
      %send% %actor% %self.name% takes the Shadow Orb from you, '@CI never thought I would actually see this thing. I've heard tales about it but never seen it.@n'
      %echoaround% %actor% %self.name% takes the Shadow Orb from %actor.name%, '@CI never thought I would actually see this thing. I've heard tales about it but never seen it.@n'
      %force% %actor% give orb chief
      %purge% orb
      wait 2 s
      emote puts the Orb on top of a pedastel to his right and it quickly vanishes, and in it's place are a pair of boots, &1So this is what people get. Seems to be made of some fine material.&2
      wait 1 s
      %send% %actor% %self.name% takes the boots and hands them to you, '@CWell they are yours now. Good luck to you in what ever you do.@n'
      %echoaround% %actor% %self.name% takes the boots and hands them to %actor.name%, '@CWell they are yours now. Good luck to you in what ever you do.@n'
      %load% obj 19401
      give boots %actor.name%
      set gotboots yes
      remote gotboots %actor.id%
    else
      wait 2 s
      say Didn't I already give you your reward?
    end
  else
    wait 2 s
    say You haven't even completed the trial!
  end
else
  %send% %actor% Select what?
end
~
#19303
Pamp Test~
0 c 3
pread~
if (%arg% == one)
    %send% %actor%   @YAre you tired of having no where to sleep? Or having to pay for a hotel room
    %send% %actor% @Ywith your hard earned zenni? Do you long for a place to call home? If you have
    %send% %actor% @Yanswered yes to any of these, get in contact with an immortal today to begin the
    %send% %actor% @Yprocess.
    %send% %actor% @n
    %send% %actor% @YLocation Surcharges
    %send% %actor% @W-------------------------
    %send% %actor% @GEarth:    @D0  Percent
    %send% %actor% @CFrigid:   @D15 Percent
    %send% %actor% @RVegeta:   @D10 Percent
    %send% %actor% @gNamek:    @D5  Percent
    %send% %actor% @mSpace:    @D30 Percent
    %send% %actor% @MKonack:   @D10 Percent
    %send% %actor% @bKanassa:  @D20 Percent
    %send% %actor% @cOther:    @D25 Percent
    %send% %actor% @n  
    %send% %actor%    @YA location surcharge will increase house price packages by the percentage
    %send% %actor% @Ylisted. Space is defined as anywhere in space (so your house could be a secret
    %send% %actor% @Yasteroid base). Other is defined as any other planet built but not currently
    %send% %actor% @Ylisted. 
    %send% %actor% @n
    %send% %actor% @YLand and Building
    %send% %actor% @W-----------------
    %send% %actor% @CSmall House: @W40,000 zenni. Basic five room house.
    %send% %actor% @CRegular House: @W70,000 zenni. Basic ten room house.
    %send% %actor% @CLarge House: @W100,000 zenni. Basic fifteen room house.
    %send% %actor% @CHouse Upgrade: @W30,000 zenni. Upgrades room size, if not already large.
    %send% %actor% @CPrivate Landing Pad: @W20,000 zenni. Adds landing pad to house.
    %send% %actor% @CKey-locked door: @W2,500 zenni. Adds a keylocked door to house.
    %send% %actor% @CLarge House RPP version: @W10 RPP. Includes fifteen rooms, two misc items (Food
    %send% %actor%  @Wwithin reason, furniture etc. No equipment.), and two misc scripts.
    %send% %actor% @n
    %send% %actor% @YPage One - Type 'pread two' for page two, or 'pread three' for page three.@n
elseif (%arg% == two)
    %send% %actor% @YHome Decor
    %send% %actor% @W-----------------------
    %send% %actor%    @YAbout to buy a home or already have one and want to spruce it up? For
    %send% %actor% @Yadditional zenni, we offer a large list of decor items for you to choose from.
    %send% %actor% @n
    %send% %actor% @YThere is a limit of 15 items per house. Includes things like keys also. 
    %send% %actor% @n
    %send% %actor% @CExtra Key: @W500 zenni.
    %send% %actor% @CChair: @W500 Zenni. (Copy of an existing one)
    %send% %actor% @CBed: @W1,000 Zenni. (Copy of an existing one)
    %send% %actor% @CCustom Chair/Bed: @WAdd 500 zenni to above prices.
    %send% %actor% @CSmall Container: @W2,000 zenni. Holds 300 weight.
    %send% %actor% @CMedium Container: @W5,000 zenni. Holds 600 weight.
    %send% %actor% @CLarge Container: @W12,000 zenni. Holds 1000 weight.
    %send% %actor% @CKey-locked container: @WAdd 1,000 zenni to container prices.
    %send% %actor% @CHealing Tank: @W15,000 zenni. (Max 1)
    %send% %actor% @CGravity Chamber: @W10,000 zenni. (Max 1)
    %send% %actor% @CFountain: @W2,500 zenni. (Max 1)
    %send% %actor% @CRefridgerator: @W5,000 zenni (Max 1. Includes 1 food and drink item)
    %send% %actor% @COther Item: @W1,500 zenni.
    %send% %actor% @n
    %send% %actor%    @YNPCs can also be purchased (max 3), though they are basic and will NOT be
    %send% %actor% @Ytraining NPCs. At a very high cost, basic scripting (greeting and stuff like
    %send% %actor% @Ythat) can be purchased for the NPCs. 
    %send% %actor% @n
    %send% %actor% @CNPC: @W2,500 zenni.
    %send% %actor% @CBasic Scripting: @W50,000 zenni.
    %send% %actor% @n
    %send% %actor% @YExtras
    %send% %actor% @W-----------------------
    %send% %actor%   @YLooking for an extra addon not mentioned above anywhere? Here is a
    %send% %actor% @Ylist of some things that have been added in.
    %send% %actor% @n
    %send% %actor% @CAnti-IT: @W50,000 per applied room. This will keep anyone in that room from ITing out.
    %send% %actor% @n
    %send% %actor% @YPage Two - Type 'pread one' to go look at page one, or 'pread three' to look at page three.@n
elseif (%arg% == three)
   %send% %actor% @YThings you can't do with a house
   %send% %actor% @W-----------------------
   %send% %actor%   @YHere is a list of things that you CANNOT do with a house.
   %send% %actor% @n
   %send% %actor% @CShops: @WShops will not be added into houses.
   %send% %actor% @CVillages: @WHouses cannot be some form of a village.  Having them as an open
   %send% %actor% @Wbuilding like a hotel or a castle is fine, but not a village.
   %send% %actor% @n
   %send% %actor% @YPage One - Type 'pread two' for page two, or 'pread three' for page three.@n
else
  %send% %actor% Read what?
end
~
#19304
Door Open~
0 d 100
Enter~
if (%actor.varexists(smazeallowed)%)
  wait 1 s
  emote nods, &1As you wish.&2
  wait 2 s
  unlock gate
  open gate
  wait 5 s
  close gate
  lock gate
else
  wait 1 s
  say No, you're not allowed in at this time.
end
~
#19308
Test~
2 c 3
trip~
%send% %actor% You feel something restricting you...
~
#19354
Troll Quest Part 2~
0 j 100
~
if (!%actor.varexists(ChefTrolled)%)
   wait 1 s
   %purge% %object.name%
   say You actually found one?
   wait 1 s
   %echo% The Head Chef looks over the vial before handing it off. He then looks at %actor.name%.
   wait 2 s
   say What? Did you expect a reward of some sort?
   wait 1 s
   laugh
   wait 1 s
   say If you find anymore, I could use them.
   set ChefTrolled
   remote ChefTrolled %actor.id%
  elseif (%object.vnum% != 19384) 
    return 0
    wait 1 s
    say Why are you givin' me this? I don't need it.
else
   wait 1 s
   %purge% %object.name%
   %echo% The Head Chef looks at the vial, '@CAh you found some more.@w' He looks over at %actor.name%, '@CWhy are you still hanging around here? Did you expect me to actually give you something this time?@w' The Head Chef bursts out laughing as he hands the spices to another chef.
end
~
#19355
Troll Quest~
0 g 100
~
if (%actor.is_pc%)
  if (!%actor.varexists(ChefTrolled)%)
   wait 2
   %echo% The Head Chef rushes right past you before turning around and stomping his way back.
   wait 2 s
   Say What do ya want? Can't you see we're busy?
   wait 1 s
   %echo% A loud Fwoosh comes from somewhere deeper in the kitchens.
    wait 1 s
    emote shakes his head and sighs. &1Damn newbies, they're always messing up...&2 The chef looks up at you. &1Say, could you help us out and go into the basement  and get some supplies for us? I'd go my self, but someone has to watch over these useless guys...&2
   wait 2 s
   emote mumbles under his breath, &1And the rats...&2
 else
  wait 1 s
  %echo% The Head Chef laughs at %actor.name% as he rushes past.
  end
end
~
#19358
test~
1 ac 100
player1~
%send% %actor% Player1 set to %arg%
set player1 %arg%
global player1
%send% %actor% Player1 is %player1%
~
#19359
new trigger~
1 ac 100
spin~
set targ %random.char%
while %targ%
  if (%targ.is_pc%)
     eval sel %random.3%
     switch (%sel%)
     case 1
      %send% %actor% You spin the bottle.
      %echoaround% %actor% %actor.name% reaches out and spins the bottle. Where it stops, no one knows!
      wait 3 s
      %echo% The bottle lands on %targ.name%!
      unset targ
      break
     case 2
 %send% %actor% You spin the bottle.
      %echoaround% %actor% %actor.name% reaches out and spins the bottle. Where it stops, no one knows!
      wait 3 s
      %echo% The bottle lands on %targ.name%!
      unset targ
      break
     default
      %send% %actor% You spin the bottle.
      %echoaround% %actor% %actor.name% reaches out and spins the bottle. Where it stops, no one knows!
       wait 3 s
       %echo% The bottle fails to land on anyone.
       unset targ
     done
  end
done
~
#19360
flip~
2 c 3
flip~
if (%findobj.19398(19302)% == 1)
   %send% %actor% You turn the doorbell off.
   %echoaround% %actor% %actor.name% turns off the doorbell.
   %at% 19398 %purge% doorbell
else
   %send% %actor% You turn the doorbell on.
   %echoaround% %actor% %actor.name% turns on the doorbell.
   %at% 19398 %load% obj 19302
end
~
#19361
press~
2 ac 3
press~
if (%findobj.19398(19302)% == 0)
  %send% %actor% You push the botton, but nothing happens.
else
  %send% %actor% You push the and a soft tone can be heard coming from inside.
end
~
#19362
Intercom test~
2 c 3
talk~
if (%findobj.19398(19303)% == 1)
  %send% %actor% You press a button on the intercom and speak into it, '@C%arg%@n'
  %echoaround% %actor% %actor.name% presses a button on the intercom and says, '@C%arg%@n'
  %at% 19398 %echo% A %actor.sex% sounding voice speaks over the intercom, '@C%arg%@n'
else
  %send% %actor% You press a button on the intercom, but nothing happens.
  %echoaround% %actor% %actor.name% presses a button on the intercom, but nothing happens.
end
~
#19363
Intlock test~
2 c 3
intlock~
if (%findobj.19398(19303)% == 0)
   %send% %actor% You press a button on the Master Intercom panel and a few lights blink into existance.
   %echoaround% %actor% You press a button on the Master Intercom Panel and a few lights blink into existance.
   %at% 19343 %echo% The lights on the intercom blink into existance.
   %at% 19398 %load% obj 19303
else
   %at% 19398 %purge% boulder
   %send% %actor% The lights on the intercom blink out of existance as you turn off the intercom system.
   %echoaround% %actor% %actor.name% presses a button on the Master Intercom's panel and the lights on it blink out of existance.
   %at% 19343 %echo% The lights on the intercom blink out of existance.
end
~
#19365
Guard Greet~
0 g 100
~
if (%actor.is_pc%)
  if (%actor.varexists(smazeallowed)%)
    wait 2 s
    emote stands up at attention as %actor.name% approaches, &1I've just gotten word that you have permission to enter. Let me know when you want to enter and I'll open the door for you.&2
    
    
  elseif(%actor.varexists(smazecomplete)%)
    if (%actor.sex% == male)
      wait 2 s
      emote stands up at attention as %actor.name% approaches and stares that them in awe, &1I'm sorry sir, but I just can't let you enter again. I'm sure you could handle yourself again, but it just isn't allowed... I'd let you in if I could though...&2
      
    elseif (%actor.sex% == female)
      wait 2 s
      emote stands up at attention as %actor.name% approaches and stares at them with confusion on his face, &1Sorry ma'am, I can't let you enter again, it's against the rules.&2
      wait 1
      emote adds under his breath, &1I'm surprised a girl managed to make it through...&2
    else
      wait 2 s
      emote stands up at attention as %actor.name% approaches and nods, &1Sorry Sir... Ma'am... Uh, it? Anyway, I can't let you enter again, it's against the rules to enter after you've completed the trial.&2
    end
  else
    wait 2 s
    emote looks up from his chair, &1If you want to get inside, talk to the Chief upstairs.&2
    
  end
end
~
#19384
Spice Load~
0 f 100
~
%load% obj 19384
~
#19393
new trigger~
2 c 100
xzwdeathxzw~
* No Script
~
$~
