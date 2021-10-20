#64300
Galaxy Travel~
2 c 100
travel~
if ("%arg%" != "planet" && "%arg%" != "remnants")
    wait 1s
     %send% %actor% @nThere are @Rremnants@n of a single @Rplanet@n that appears close enough to @Ytravel@n to.
   else
    wait 1s
      %send% %actor%  You hover at the mouth of the wormhole, and before you the vast distance stretches out before you, the shattered remains of the @WS@no@Wu@nt@Wh@n @DG@na@Wl@Da@nx@Wy@n.
      wait 2s
      %send% %actor% Using the ever faint traces of life to guide you, you gather your energy and propel yourself forward at full speed!
      wait 10s
      %echoaround% %actor% %actor.name% gathers their energy and blasts away into the dark vastness of space!
            wait 15s
      %send% %actor% The blackness surrounding you is eerie and foreboding, not even the usual faint blink of a distant star to comfort you as you travel.
            wait 15s
      %send% %actor% At last the signs of life become more clear, and the sight of a fading star, and the half cracked remains of a planet thats all but been destroyed come into sight!
      wait 10s
      %teleport% %actor% 64301
      wait 2s
%send% %actor% Approaching the remains of this planet, you find the only stable place in its atmosphere to land.
      %echoaround% %actor% %actor.name% Lands from outer space!
      %force% %actor% look
end
~
#64301
Galaxy travel 2~
2 c 100
travel~
if ("%arg%" != "outskirts" && "%arg%" != "wormhole")
    wait 1s
     %send% %actor% @nThe @Rwormhole@n you came from can only be reached by @Yreturning@n to the @Routskirts@n of the @WS@no@Wu@nt@Wh@n @DG@na@Wl@Da@nx@Wy@n.
       else
    wait 1s
      %send% %actor%  Focusing your energy, you wait to find a momentary lapse of turbulence in the atmosphere before propelling yourslef back into space! You aim yourself for the very edge of the @WS@no@Wu@nt@Wh@n @DG@na@Wl@Da@nx@Wy@n.
      wait 2s
      %send% %actor% Gaining speed, you achieve maximum speed as you zero in on your destination!
      wait 10s
      %echoaround% %actor% %actor.name% gathers their energy and blasts away into the dark vastness of space!
            wait 15s
      %send% %actor% The blackness surrounding you is eerie and foreboding, not even the usual faint blink of a distant star to comfort you as you travel.
            wait 15s
      %send% %actor% At last the sight of the strange wormhole comes into view, as the very outer reaches of space lie just beyond!
      wait 10s
      %teleport% %actor% 64300
      wait 2s
%send% %actor% Approaching the very edge of the @WS@no@Wu@nt@Wh@n @DG@na@Wl@Da@nx@Wy@n, you bring yourself to halt near the edge of a wormhole.
      %echoaround% %actor% %actor.name% halts their momentum near the edge of a wormhole!
      %force% %actor% look
end
~
#64302
Raptor Drop~
0 f 100
~
 eval num %random.1000% 
    if (%num% >= 975)
       %load% obj 64301 
   Else
end
~
#64303
Grolm drop~
0 f 100
~
 eval num %random.1000% 
    if (%num% >= 975)
       %load% obj 64302 
   Else
end
~
#64304
Wanderer Greet~
0 g 100
~
  if (!%actor.varexists(ondwq)% && !%actor.varexists(dwqcomp)%)
      wait 2s
      %echoaround% %self% @nThe figure @npauses in his stride slowly, turning the depths of his cowl to stare at %actor.name%.
      wait 3s
      %echoaround% %self% @n"@DYou. You may be strong enough ... just.@n" The voice slowly croaks from within the figure's cowl.
      wait 2s
      %echoaround% %self% @nThe figure turns to regard %actor.name% more fully. "@DTo set HIM on a road that doesn't lead to your Galaxy, the death of the father must be delivered to the son.@n"
      wait 3s
      %echoaround% %self% @nHe waits for a time in silence, as the impact of this sinks in. "@DIf you have the will, @Rcommit@D to my cause.@n"
      wait 2s
      %echoaround% %self% @nThe figure turns away, returning to his wanderings.
          elseif (%actor.varexists(dwqcomp)%)
            wait 2s
            %echoaround% %self% @nThe robed figure pauses his wandering with a dark chuckle, the cowl of his robe turning to regard %actor.name%.
            wait 3s
            %echoaround% %self% "@DHeh ... you have already furthered my goals. Away with you.@n"
       else
         wait 2s
         %echoaround% %self% @nThe robed figure pauses his wandering with a slow, menacing pace. The depths of his cowl turn to regard %actor.name%.
         wait 3s
         %echoaround% %self% "@DHave you brought me the @Rtablet@D? If not then I have no use of you.@n"
         wait 2s
         %echoaround% %self% The figure turns away from %actor.name% to return to his wandering.
  End
~
#64305
Wanderer Commit~
0 cd 100
commit~
If (%actor.varexists(ondwq)%)
  return 0
  wait 4s
   say @DYou've already agreed to help me. Why aren't you off doing just that?@n
    Elseif (%actor.varexists(dwqcomp)%)
      return 0
      wait 4s
      say @DYou've already served your purpose. Be away with you before you become annoying.@n
Else
  wait 2s
  %echoaround% %self% @nThe robed figure dips his head in acknowledgement, the cowl dipping downward.
  wait 4s
  say @DVery well. Find the father, @RParagus@D, and destroy him.@n
  wait 3s
  say @DBring to me the writings he carries, and I will deliver the news to HIM. They are inscribed upon a stone @Rtablet@D.@n
  Set ondwq
  remote ondwq %actor.id%
end
~
#64306
Wanderer Receive~
0 j 100
~
If (!%actor.varexists(ondwq)% && !%actor.varexists(dwqcomp)%)
      return 0
      wait 3s
      say @DYou haven't yet @Rcommited@D yourself to my goals.@n
         Elseif (%actor.varexists(dwqcomp)%)
            return 0
            wait 3s
            say @DYou have already served your purpose. Go away.@n
    else
       If (%object.vnum% != 64315)
           return 0
           wait 3s
           say @DThis isn't what I asked you for. What incompetence ...@n
           wait 2s
           %echoaround% %self% @nThe robed figure turns his attention away from %actor.name%.
     Else
       wait 3s
       %echoaround% %self% The robed figure takes the tablet from %actor.name% with hands that peek out from the robe to grasp the stone with scaled, clawed hands.@n
       wait 3s
       say @DYes. This is what I need ...@n
       wait 2s
       %purge% %object.name%
       %echoaround% %self% @nThe robed figure @nchuckles darkly with a horrible hissing sound that follows.
       wait 3s
       %echoaround% %self% The cowled head turns to regard %actor.name% for a long moment in silence.
       wait 3s
       rdelete ondwq %actor.id%
       wait 1s
       say @DTake this, and trust in the choice you've made.@n
         eval num %random.1000%
          if (%num% >= 990)
               wait 3s
               %echoaround% %self% @nA pair of pants materalize before %actor.name% as they @Ys@Wh@Yi@Wmm@Ye@Wr@ys@n into existence!
               wait 2s
               set dwqcomp
               remote dwqcomp %actor.id%
               %load% obj 64398
               give pants %actor.name%
               wait 3s
               %echoaround% %self% @nThe figure turns away, no longer interested in %actor.name%.
        Else
           wait 3s
           %echoaround% %self% @nA golden crown materalize before %actor.name% as it @Ys@Wh@Yi@Wmm@Ye@Wr@ys@n into existence!
           wait 2s
           set dwqcomp
           remote dwqcomp %actor.id%
           %load% obj 64397
           give diadem %actor.name%
           wait 3s
           %echoaround% %self% @nThe figure turns away, no longer interested in %actor.name%.
         End
      End
  End
~
#64307
koraam exercise~
0 b 10
~
%echoaround% %self% @nThe tall redheaded warrior suddenly crouches, only to leap as high as he can up into the air!@n
wait 2s
%echoaround% %self% @nWith a loud yell, the @Dveiled@n warrior screams out "@CHyahh!@n" as his foot kicks out straight in front of him trying to reach as high as he can!@n
wait 2s
%echoaround% %self% @nReaching the peak of his kick, the warrior nimbly drops back to the ground onto the balls of his feet and lands in a battle ready crouched stance.@n
~
#64308
koraam greet~
0 g 100
~
If (%actor.level% != 100)
   wait 5s
   %echoaround% %self% @nAs %actor.name% enters, this tall man looks up to regard them.
   wait 15s
   %echoaround% %self% @nAfter a long and careful study, the tall veiled warrior looks to %actor.name% and shakes his head slightly.@n
   wait 10s
   %echoaround% %self% @nThe man dips his head respectfully toward %actor.name%. "@CI will not deny you the honor of the dance, but I do not think you are ready.@n"
   wait 10s
   %echoaround% %self% @nThe redheaded man turns his @Dg@wr@De@wy@n eyes back toward the glow of the fire.@n
Else
   wait 5s
   %echoaround% %self% @nAs %actor.name% enters, this tall man looks up to regard them.
   wait 15s
   %echoaround% %self% @nAfter a long and careful study, the tall veiled warrior looks to %actor.name% and nods his head slightly in grim approval.@n
   wait 10s
   %echoaround% %self% @nThe man dips his head respectfully toward %actor.name%. "@CIf you wish, I will dance the spears with you.@n"
   wait 10s
   %echoaround% %self% @nThe man waits with patience, eventually turning his @Dg@wr@De@wy@n eyes back to the glow of the fire.@n
End
~
#64309
koraam death~
0 f 100
~
 eval num %random.1000% 
    if (%num% >= 975)
       %load% obj 64314 
   Else
end
~
#64310
Broly plushie~
1 c 100
~
if ("%arg%" != "button")
  wait 1s
  %send% %actor% @RTry pressing the button!@n
else
  %send% %actor% You press the button.
  %echoaround% %actor% %actor.name% presses the button on the @YL@Ce@Wg@Ye@Cn@Wd@Ya@Cr@Wy S@nu@Wp@ne@Wr @YS@ya@Yi@yy@Ya@yn@n plushie.
  wait 1s
  eval nuz %random.1000%
  if (%nuz% >= 990)
    wait 3s
    %send% %actor% The plushie shouts, "@CI am the Legendary Super Saiyan! No One can withstand my Power!@n"
    %echoaround% %actor% The plushie shouts, "@CI am the Legendary Super Saiyan! No One can withstand my Power!@n"
  elseif (%nuz% >= 750)
wait 2s
    %send% %actor% The plushie shouts "@CI will destroy Kakarot!@n"
    %echoaround% %actor% The plushie shouts "@CI will destroy Kakarot!@n"
  elseif (%nuz% >= 500)
    wait 2s
    %send% %actor% The plushie shouts, "@CShell Blaster!@n"
    %echoaround% %actor% The plushie shouts, "@CShell Blaster!@n"
  elseif (%nuz% >= 250)
    wait 2s
    %send% %actor% The plushie shouts, "@CShell Blaster!@n"
    %echoaround% %actor% The plushie shouts, "@CShell Blaster!@n"
  else
    wait 2s
    %send% %actor% The plushie shouts, "@CYou are beneath me!@n"
    %echoaround% %actor% The plushie shouts, "@CYou are beneath me!@n"
  end
End
~
#64311
Columns~
2 c 100
delve~
If ("%arg%" != "column" && "%arg%" != "columns")
          %send% %actor% @d     @n
         wait 3s
         %send% %actor% @DTry to @Rdelve@D the @Rcolumns@n.
else
  If (!%actor.varexists(rhuq)%)
     %send% %actor% @d     @n
     wait 3s
     %send% %actor% @DThe columns do not accept you, and you find them of little use.@n
  Elseif (%actor.varexists(rhucom)%)
     %send% %actor% @d     @n
     wait 3s
     %send% %actor% @DThe columns have shown you all that they can.@n
  Else
     wait 3s
     %send% %actor% @DYou take your first step into the row of columns, and all goes black. Time seems to freeze around you before your vision begins to return, blurry and unclear.
     %echoaround% %self% @n%actor.name% takes their first steps into the columns, and they are enraptured by some great magic or science! Their eyes stare blankly ahead but they do not see and any attempts to call out to them go unheard.@n
     wait 5s
     %send% %actor% @DYour eyes open to the sight of the vast void of space. A harsh world lies below, almost entirely sand and the pinpricks of light that represent your people's pods falling into the atmosphere. '@GHarsh or not we the people of Kesh claim this world for our new home!@D' The thought is not your own and yet it runs through your head as if it were, followed with a surge of determination and hope.@n
     wait 3s
     %send% %actor% @DYour vision changes, shifting jarringly to show crowds and throngs of people. A Tent city that stretches as far as the eye can see. "@GAll of us cannot remain in one place for much longer. Our one Tribe must become many.@D" The words tumble forth from your lips that are not your lips. The sense of surety and justification fill you, and you know that what you do is for the best for your people.@n
     wait 4s
     %send% %actor% @DYour vision flickers again and you watch from above as small groups here and there, Families among your great people that begin to spread in different directions like an exedus. Trails of lights in the dimming desert light that thread their ways into the night like serpents. For the first time your people are divided.@n
     %echoaround% %self% %actor.name% @nhas only taken one or two steps through the slender hallway of columns despite having started the journey several minutes ago. Their face is covered in a grim expression, as if fighting some kind of intrusion.@n
     wait 5s
     %send% %actor% @DTime passes but you're not sure how much. Many flashes of vision, watching as your people turn into selfish and self serving tribes. Warring with each other and fighting over squabbles. The harshness of this harsh land has turned them hard and callous.@n
     wait 2s
     %send% %actor% @YFLASH@D. Raids and fueds that last generations, until each segment of his people dwindle into only the core of what once was. There is no forgiveness in the desert, and the forgiveness and mercy of your people dwindle still.
     wait 4s
     %send% %actor% @YFLASH@D. Many years have passed and you look through the world with the eyes of an old man. Your age weighs upon you and you feel tired, long and harsh living in this wasteland has in the end turned you hard as well. This day is special in your mind and memory. The Strangers came, with an offer and a demand. They demanded your people raise the infant they'd brought with them and they offered you immortality. The chance to live forever so that you might see the task completed. Without 
understanding you jokingly agreed and they brought forth seven glowing orbs ... @n
     %echoaround% %self% @n%actor.name% continues with another step, each one harder than the last. Sweat pours from their face and drips to the sand below with soft hisses as if they were running at full speed! Whatever they're seeing brings a look of fright or terror to their face.@n
     wait 5s
     %send% %actor% @YFLASH@D. The boy is a small child, always angry but incredibly strong. The parents would never return, you know that in your heart. The boy's strength is sure to do with it. He is different from the rest, humanoid like his people but not human. A tail and a strange power inside that made him much stronger than even your finest spear brothers. You work him hard, teach him the harsh ways of living in the desert here. You hide the fact that your age is not increasing, that your body no l
onger aches the same or fails you as it used to.@n
     wait 4s
     %send% %actor% @YFLASH@D. The boy grows into a teen, a young man beginning his combat training. You watch as your people teach him as one of their own the strange style of knees elbows and kicks always preferring to attack from the odd angle. You watch as he learns weapons training, the short spear and the buckler. You've watched him grow and learn and you feel the love of a father in your heart.@n
     wait 2s
     %send% %actor% @YFLASH@D. A first hunt. @YFLASH@D. A first trial by combat. @YFLASH@D. A hundred, a million memories flash by as you watch the child grow. A million memories assault you over and over as you can do nothing to run from them. As each settles into you as if you live them anew you feel the weight of them pressing you down.@n
     %echoaround% %self% %actor.name% has fallen to a knee, still struggling to move forward. Always forward as the sweat continues to pour from them the color draining from their skin.@n
     wait 5s
     %send% %actor% @DMore decades have passed, your son long gone from this world. His race can travel long distances through the skies and his power was always beyond words. As the years have passed your people have noticed that stars have begun to disappear from the sky. Something in your heart knows, and begins to worry.@n
     wait 3s
     %send% %actor% @DEvery fading star is like a wound to your heart, for who else could have the strength to kill a star? You know it's only a matter of time before he returns and the thoughts of it keep you awake at night. What will you do, where you will go, unable to die even in the fact of his power? Your soul weeps.@n
     wait 3s
     %send% %actor% @DDecades pass. Centuries. Millenia. You lie waiting, worrying. Feeling the looming shadow of a destruction that never comes. Lives upon lives worth of time, wasted. Still, you cannot help but wonder, cannot help but feel that his presence must be remembered. You go out in parley, to find what remains of the other fractions of your people to search out their Wisest. But there is nothing to find. Your Immortal wanderings find only bones and dust and you know that you are the last of the 
people of Kesh.@n
     set rhucom
     remote rhucom %actor.id%
    End
End 
~
#64312
Kesh Solider random~
0 b 2
~
%echoaround% %self% @nThe soldier grumbles under his breath in a gruff manner, his eyes shifting cautiously from side to side.
wait 2s
%echoaround% %self% @nWith an uneasy shift of his gait, the soldier shifts his gear for comfort and continues his patrol, all the while grumbling under his breath.@n
~
#64313
columns return~
2 c 100
contemplate~
If ("%arg%" != "history")
   wait 1s
   %send% %actor% @DTry to @Rcontemplate@D on the right subject!@n
Else
   %send% %actor% @d @n
   wait 2s
   %send% %actor% @DYou sit and begin to contemplate the information and history before you.@n
   wait 4s
   %send% %actor% @DAs your mind focuses and your body relaxes, you feel a strange sensation ...@n
   wait 2s
   %send% %actor% @DYou find yourself once again standing within the ring of glass columns.@n
   %teleport% %actor% 64305
   %force% %actor% look
End
~
#64314
Column Return 2~
2 c 100
contemplate~
If (!%actor.varexists(rhucom)%)
   wait 3s
   %send% %actor% @DYou feel a sense of something in the room but it passes.@n
  Else
    If ("%arg%" != "history")
    wait 1s
    %send% %actor% @DTry to @Rcontemplate@D on the right subject!@n
      Else
      %send% %actor% @d @n
      wait 2s
      %send% %actor% @DYou sit and begin to contemplate the information and history before you.@n
      wait 4s
      %send% %actor% @DAs your mind focuses and your body relaxes, you feel a strange sensation ...@n
      wait 2s
      %send% %actor% @DYou find yourself once again standing within the ring of glass columns.@n
      %teleport% %actor% 64305
      %force% %actor% look
   End
End
~
#64315
Paragus Drop~
0 f 100
~
Eval num %random.1000%
    if (%num% >= 800)
       %load% obj 64343
    Elseif (%num% >= 500)
       %load% obj 64344
    Else
End
~
#64320
Regular Broly Drop~
0 f 100
~
Eval num %random.1000%
    if (%num% >= 800)
       %load% obj 64345 
    Elseif (%num% >= 500)
       %load% obj 64346
    Else
End
~
#64321
Super Broly Drop~
0 f 100
~
Eval num %random.1000% 
    if (%num% >= 800)
       %load% obj 64398
    Elseif (%num% >= 500)
       %load% obj 64345
    Elseif (%num% >= 300)
       %load% obj 64346
    Elseif (%num% >= 250)
       %load% obj 64399
    Else
End
~
#64322
Paragus Location~
0 b 2
~
Eval num %random.1000%
    If (%num% >= 500)
      Eval pnum %random.1000%
       If (%pnum% >= 750)
        %teleport% paragus 2298
       Elseif (%pnum% >= 375)
        %teleport% paragus 4169
       Else
        %teleport% paragus 16302
    Else
End
End
~
#64323
Wanderer Location~
0 b 2
~
Eval num %random.1000%
    If (%num% >= 500)
      Eval wnum %random.1000%
       If (%wnum% >= 750)
        %teleport% wanderer 64367
       Elseif (%wnum% >= 375)
        %teleport% wanderer 17260
       Else
        %teleport% wanderer 9227
    Else
End
~
#64355
Cliff Climb~
2 c 100
climb~
if ("%arg%" != "cliff" && "%arg%" != "rock")
wait 1s
%send% %actor% @DTry @Rclimb@Ding the cliff.@n
   else
         eval cliff %random.1000%
         if (%cliff% >= 925) 
        %send% %actor% @DYou begin to climb up the cliffs jagged face with ease!@n
        %echoaround% %actor% %actor.name% begins to climb up the jagged cliff face with ease!
        Wait 10s
        %send% %actor% @DYou continue to climb up the face of the cliff and it seems to go on forever! Still you climb with ease your strength never failing you!@n
        %echoaround% %actor% %actor.name% @ncontinues to climb up the jagged cliff face with surprising ease as the wind buffets them mercilessly! They grow small in the distance.@n
        Wait 10s
        %send% %actor% @DYou see the peaks of the cliff in sight! Scrambling with a final burst of effort you are soon pulling yourself over the top!@n
       Wait 3s
       %teleport% %actor% 64356
       %force% %actor% look
 elseif (%cliff% >= 850)
       %send% %actor% @DYou begin to climb up the cliffs jagged face with relative ease. The climb looks very long as you begin.@n
        %echoaround% %actor% %actor.name% @nbegins the long climb up the jagged cliff face. It's a long way but they seem determined to make it.@n
        Wait 15s
        %send% %actor% @DYou continue to doggedly climb up the cliff face as the howling @Wwinds@D buffet all around you and threaten to dislodge your grip! Still you climb the daunting task.@n
        %echoaround% %actor% %actor.name% @ncontinues with a tenuous grip to climb up the cliffs. Eventually they become small to the eye, only visible by the lightning flashes in the atmosphere above.@n
        Wait 30s
        %send% %actor% @DYou finally find the top of the cliffs in sight. Your hands are sore and your body is tired as you pull yourself haggardly over the edge at the top.@n
        Wait 5s
        %teleport% %actor% 64356
        %force% %actor% look
 elseif (%cliff% >= 500)
                          %send% %actor% @DYou begin to climb up the cliff face and it's tough. The wind whips at you and makes it hard to hold onto to the jagged rocks that cut into your hands.@n
                          %echoaround% %actor% %actor.name% @nbegins the long climb up the jagged cliff face. It's a long way but they seem to be having a rough time of it already.@n
     Wait 20s
                           %send% %actor% @DYou continue to doggedly climb up the cliff face as the howling @Wwinds@D buffet all around you and threaten to dislodge your grip! Still you climb the daunting task.@n
                           %echoaround% %actor% %actor.name% @ncontinues with a tenuous grip to climb up the cliffs. Eventually they become small to the eye, only visible by the lightning flashes in the atmosphere above.@n
    Wait 20s
                           %send% %actor% @DYou continue to struggle up the cliff, but suddenly the howling @Wwinds@D catch you and suddenly you're flying through the air ...@n
                           %echoaround% %actor% %actor.name% @ncontinues to climb up the rock face but soon they're ripped from it by an enormous gust of wind to plummet through the air!@n
   Wait 2s
                           %send% %actor% @DFalling ...@n
                           %echoaround% %actor% %actor.name% @nis falling ...@n
  Wait 2s
                           %send% %actor% @DFalling ...@n
                           %echoaround% %actor% %actor.name% @nis falling ...@n
  Wait 2s
                           %send% %actor% @DYou slam painfully into the ground, but manage to break your fall!
   Eval cliffdamage %actor.hitp% / 10
                           %damage% %actor% %cliffdamage%
                           %force% %actor% sit
                           %echoaround% %actor% %actor.name% @nslams painfully into the ground, but manage to break their fall!@n
     elseif (%cliff% >= 250)
                           %send% %actor% @DYou begin to climb up the cliff face and it's tough. The wind whips at you and makes it hard to hold onto to the jagged rocks that cut into your hands.@n
                           %echoaround% %actor% %actor.name% @nbegins the long climb up the jagged cliff face. It's a long way but they seem to be having a rough time of it already.@n
    Wait 20s
                           %send% %actor% @DYou continue to doggedly climb up the cliff face as the howling @Wwinds@D buffet all around you and threaten to dislodge your grip! Still you climb the daunting task.@n
                           %echoaround% %actor% %actor.name% @ncontinues with a tenuous grip to climb up the cliffs. Eventually they become small to the eye, only visible by the lightning flashes in the atmosphere above.@n
       Wait 20s
                            %send% %actor% @DYou continue to struggle up the cliff, but suddenly the howling @Wwinds@D catch you and suddenly you're flying through the air!@n
                            %echoaround% %actor% %actor.name% @ncontinues to climb up the rock face but soon they're ripped from it by an enormous gust of wind to plummet through the air!@n
       Wait 2s
                            %send% %actor% @DFalling ...@n
                            %echoaround% %actor% %actor.name% @nis falling ...@n
       Wait 2s
                            %send% %actor% @DFalling ...@n
                            %echoaround% %actor% %actor.name% @nis falling ...@n
       Wait 2s
                            %send% %actor% @DYou slam painfully into the ground, with a massive impact!@n
    Eval cliffdamage %actor.hitp% / 4
                            %damage% %actor% %cliffdamage%
                            %force% %actor% sit
                            %echoaround% %actor% %actor.name% @nslams painfully into the ground, with a massive impact!@n
   Else
                            %send% %actor% @DYou begin to climb up the cliff face and it's tough. The wind whips at you and makes it hard to hold onto to the jagged rocks that cut into your hands and you just can't find a good route. You'll want to take a minute and try again.@n
                            %echoaround% %actor% %actor.name% @nbegins the long climb up the jagged cliff face, but they can't seem to find a good route or hold in the wind. They climb back down.@n
     End 
End
~
#64377
wipe flag~
2 c 100
wipeme~
rdelete dwqcomp %actor.id%
~
#64387
Hunter Betray~
0 f 100
~
If (%actor.varexists(sgh)%)
  rdelete sgh %actor.id%
  set sgb 
  remote sgb %actor.id%
      Elseif (%actor.varexists(sgb)%)
        return 0
   Else
     set sgb
     remote sgb %actor.id%
End
~
#64388
Hunter Greet~
0 g 100
~
if (%actor.varexists(sgh)%)
     Wait 4s
     Say @GHey there. Did you bring anything? Hurry it up or get the hell out.@n
     wait 4s
     %echoaround% %self% @nThe hunter grumbles to himself for a moment looking at you expectantly, before he tsks and turns back to sifting through the sand.@n
           Elseif (%actor.varexists(sgb)%)
                    Wait 4s
                    Say @GGah, I don't need anything from the likes of you.@n
                      Else 
                    Wait 4s
                   Say @GHey, you. What the hell are you doing out here?!@n
                  Wait 4s
                Say @GIf he hasnt noticed you, you should run now ... while you can.@n
               wait 4s
          say @GBut ... if you're going to stick around, I might be able to offer you something if you can bring me something.@n
        wait 3s
     %echoaround% %self% @nThe grizzled and wily hunter glances to either side before continuing to speak.@n
   wait 4s
  say @GBring me a Grolm's eye and we can be friends, do we @Ragree?@n
End
~
#64389
Hunter Agree~
0 c 100
agree~
If (%actor.varexists(sgh)%)
  wait 4s
   say @GYou've already agreed to help me!@n
    Elseif (%actor.varexists(sgb)%)
      return 0
      wait 4s
      say @GI told you, I don't want anything from the likes of you.@n
Else
  return 0
  wait 4s
  say @GWell alright.@n
  Set sgh
  remote sgh %actor.id%
end
~
#64390
hunter receive~
0 j 100
~
If (!%actor.varexists(sgh)% && !%actor.varexists(sgb)%)
      return 0
      wait 1s
      say @GYou haven't agreed to help me, so I don't trust ya.@n
         Elseif (%actor.varexists(sgb)%)
         return 0
         wait 4s
         say @GI told you I don't need anything from the likes of you.@n
              else
                 If (%object.vnum% != 64301 && %object.vnum% != 64302)
                 return 0
                 wait 3s
                 say @GThis isn't what the hell I asked for!@n 
              Else
            %purge% %object.name%
           wait 3s
        say @GThanks, this looks just right! Here's something for your trouble.@n 
        wait 3s
       %echoaround% %self% @nThe grizzled hunter digs a strange coin from his pocket, flicking it with his thumb.@n
     %load% obj 64304
 give coin %actor.name%
  END
END
~
#64391
legend bracer removal~
1 l 100
~
return 0
wait 3s
%force% %actor.name% emote @ntries to remove the bracers from their arms, and the rubies inset along them begin to @Rglow@n a vicious color!
wait 3s
%force% %actor.name% smote @nAn equally @Rred@n aura flashes for a moment around the bracers as they refuse to be removed, and %actor.name% roars out in pain.
eval bracedmg %actor.hitp% / 10
%damage% %actor% %bracedmg%
~
#64392
bracer influence~
1 b 2
~
if (%self.worn_by%)
  eval ch %self.worn_by%
  %send% %ch% @nYou feel the sudden and savage urge to give in to the rage that inexpicably builds up within your soul!@n
  wait 2s
else
end
~
#64394
Tamatori secret receive~
0 j 100
~
 If (%object.vnum% != 64321)
    return 0
    wait 3s
    %echoaround% %self% @nThe Divine Kaio warrioress just turns away from %actor.name%@n.
         Else
           wait 3s
           %echoaround% %self% @nThe Divine Kaio Warrioress nods at %actor.name% in relief, handing them something before turning away.@n
           wait 1s
           %load% obj 64322
           give lousy %actor%
end
~
#64395
Broly secret receive~
0 j 100
~
 If (%object.vnum% != 64320)
    return 0
    wait 3s
    %echoaround% %self% @nThe legendary Saiyan shoves you away with a crazed grunt.@n
         Else
           wait 3s
           %echoaround% %self% @nThe legendary Saiyan shoves you away with a crazed grunt, flinging a T-shirt at your face!@n
           wait 1s
           %load% obj 64323
           give buddy %actor%
end
~
#64396
Tamatori death heart~
0 f 100
~
 eval num %random.1000% 
    if (%num% >= 700)
       %load% obj 64320 
   Else
end 
~
#64397
Broly Death head~
0 f 100
~
eval num %random.1000%
if (%num% >= 700)
%load% obj 64321
else
end
~
#64398
Power shake~
2 ab 3
~
%zoneecho% 64300 @RThe blasted pieces of this planet quake and tremble suddenly as an unimagineable power level threatens to crush you under its weight!@n
~
#64399
Say my name~
0 d 100
kakarot~
return 0
  wait 3s
  %echoaround% %self% @nThe massively muscled Super Saiyan @YB@Wr@Yo@Wl@Yy@n whirls about at the sound of @RTHAT@n name.
  wait 2s
  %echoaround% %self% @nThe @YL@Ce@Wg@Ye@Cn@Wd@Ya@Cr@Wy@n @WS@nu@Wp@ne@Wr @S@ya@Yi@yy@Ya@yn@n settles a crazed look onto %actor.name%@n.
  wait 3s
  %echoaround% %self% @nWith an enormous yell, @YB@Wr@Yo@Wl@Yy@n bellows out with @Rrage@n! The ground quakes and the skies thunder! Lightning cracks down from above and the pressure of a power level larger than anything ever before crushes your very will to live as this madman unleashes the full force of @nThe @YL@Ce@Wg@Ye@Cn@Wd@Ya@Cr@Wy@n @WS@nu@Wp@ne@Wr @YS@ya@Yi@yy@Ya@yn@n!
  mtransform -64397
  wait 3s
  %echoaround% %self% @YB@Wr@Yo@Wl@Yy@n seethes with breath as his now massive form stares down at %actor.name% with a killing rage in his eyes.
~
$~
