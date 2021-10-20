#9100
Mob Definition - Policeman~
0 n 100
~
set myClass Policeman
set Mood 5
set Corruption 0
set mySpeech "Howdy, Partner."
set justice 10
remote myClass %self.id%
remote Mood %self.id%
remote Corruption %self.id%
remote mySpeech %self.id%
remote justice %self.id%
~
#9101
Mob Definition:  AdultCitizen~
0 n 100
~
set myClass AdultCitizen
set Mood 5
set Corruption 10
set mySpeech I'm not used yet
remote myClass %self.id%
remote Mood %self.id%
remote Corruption %self.id%
remote mySpeech %self.id%
~
#9102
Mod Definition - Criminal~
0 n 100
~
set myClass Criminal
set Mood 5
set Corruption 10
set mySpeech I'm not used yet
set notoriety 0
remote myClass %self.id%
remote Mood %self.id%
remote Corruption %self.id%
remote mySpeech %self.id%
remote notoriety %self.id%
~
#9103
Reward RPP To Everyone In Room~
1 c 3
event~
if %arg% == rppreward
  wait 1
  set pplcount 0
  set playercount 0
  set mypeople %self.room.people%
  while %mypeople%
    set randrpp 1
    if %mypeople.is_pc%
      %echo% @GHello, %mypeople.name%@G.  You get @C(@Y%randrpp%@C)@G RPP for participating in this event!@n
      nop %mypeople.rpp(%randrpp%)%
      eval playercount %playercount% + 1
    end
    set mypeople %mypeople.next_in_room%
    eval pplcount %pplcount% + 1
  done
  %echo% @YThere are %pplcount% people in this room, %playercount% of which are player characters.@n
elseif %arg% == assistant
  %load% mob 9198
  %echo% @GAn assistant appears to help %actor.name%!@n
else
  wait 1
  %send% %actor% @C=======================[ @WCurrent Commands @C]=======================@n
  %send% %actor% @Y[@GRPPreward@Y]@W  Grants 1 RPP to everyone in this room.@n
  %send% %actor% @Y[@GAssistant@Y]@W  Summons an assistant to help with events.@n
  %send% %actor% @Y @n
  %send% %actor% @C==================================================================@n
  %send% %actor% @GUsage:  @YEvent @R<@Wcommand@R> @R[@Woptional@R] [@Woptional@R]@n
end
~
#9104
RP Assistant Load~
0 n 100
~
set player1 default
set player2 default
set player3 default
set player4 default
set player5 default
set player6 default
set player7 default
set player8 default
set player9 default
set player10 default
set player11 default
set player12 default
set player13 default
set player14 default
set player15 default
set player16 default
set player17 default
set player18 default
set player19 default
set player20 default
set currentturn 1
set currentname default
remote player1 %self.id%
remote player2 %self.id%
remote player3 %self.id%
remote player4 %self.id%
remote player5 %self.id%
remote player6 %self.id%
remote player7 %self.id%
remote player8 %self.id%
remote player9 %self.id%
remote player10 %self.id%
remote player11 %self.id%
remote player12 %self.id%
remote player13 %self.id%
remote player14 %self.id%
remote player15 %self.id%
remote player16 %self.id%
remote player17 %self.id%
remote player18 %self.id%
remote player19 %self.id%
remote currentturn %self.id%
remote currentname %self.id%
~
#9105
Roleplay Assistant - Scene Command~
0 c 100
scene~
* scene command
wait 1
if %arg% == enroll
  if %self.player1% == default
    set player1 %actor.name%
    remote player1 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
    if %self.currentname% == default
      set currentname %actor.name%
      remote currentname %self.id%
    end
  elseif %self.player2% == default
    set player2 %actor.name%
    remote player2 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player3% == default
    set player3 %actor.name%
    remote player3 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player4% == default
    set player4 %actor.name%
    remote player4 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player5% == default
    set player5 %actor.name%
    remote player5 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player6% == default
    set player6 %actor.name%
    remote player6 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player7% == default
    set player7 %actor.name%
    remote player7 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player8% == default
    set player8 %actor.name%
    remote player8 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player9% == default
    set player9 %actor.name%
    remote player9 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player10% == default
    set player10 %actor.name%
    remote player10 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player11% == default
    set player11 %actor.name%
    remote player11 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player12% == default
    set player12 %actor.name%
    remote player12 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player13% == default
    set player13 %actor.name%
    remote player13 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player14% == default
    set player14 %actor.name%
    remote player14 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player15% == default
    set player15 %actor.name%
    remote player15 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player16% == default
    set player16 %actor.name%
    remote player16 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player17% == default
    set player17 %actor.name%
    remote player17 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player18% == default
    set player18 %actor.name%
    remote player18 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  elseif %self.player19% == default
    set player19 %actor.name%
    remote player19 %self.id%
    %echo% @Y%actor.name% @Rhas enrolled in the scene!@n
  else
    wait 1
    %send% %actor% @ROnly 19 players are supported, sorry.  Wait for someone to drop out.@n
  end
elseif %arg% == turns
  if %player1% != default
    %send% %actor% @GPlayer 1:  @W%self.player1%@n
    if %currentturn% == 1
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player2% != default
    %send% %actor% @GPlayer 2:  @W%self.player2%@n
    if %currentturn% == 2
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player3% != default
    %send% %actor% @GPlayer 3:  @W%self.player3%@n
    if %currentturn% == 3
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player4% != default
    %send% %actor% @GPlayer 4:  @W%self.player4%@n
    if %currentturn% == 4
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player5% != default
    %send% %actor% @GPlayer 5:  @W%self.player5%@n
    if %currentturn% == 5
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player6% != default
    %send% %actor% @GPlayer 6:  @W%self.player6%@n
    if %currentturn% == 6
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player7% != default
    %send% %actor% @GPlayer 7:  @W%self.player7%@n
    if %currentturn% == 7
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player8% != default
    %send% %actor% @GPlayer 8:  @W%self.player8%@n
    if %currentturn% == 8
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player9% != default
    %send% %actor% @GPlayer 9:  @W%self.player9%@n
    if %currentturn% == 9
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player10% != default
    %send% %actor% @GPlayer 10:  @W%self.player10%@n
    if %currentturn% == 10
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player11% != default
    %send% %actor% @GPlayer 11:  @W%self.player11%@n
    if %currentturn% == 11
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player12% != default
    %send% %actor% @GPlayer 12:  @W%self.player12%@n
    if %currentturn% == 12
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player13% != default
    %send% %actor% @GPlayer 13:  @W%self.player13%@n
    if %currentturn% == 13
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player14% != default
    %send% %actor% @GPlayer 14:  @W%self.player14%@n
    if %currentturn% == 14
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player15% != default
    %send% %actor% @GPlayer 15:  @W%self.player15%@n
    if %currentturn% == 15
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player16% != default
    %send% %actor% @GPlayer 16:  @W%self.player16%@n
    if %currentturn% == 16
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player17% != default
    %send% %actor% @GPlayer 17:  @W%self.player17%@n
    if %currentturn% == 17
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player18% != default
    %send% %actor% @GPlayer 18:  @W%self.player18%@n
    if %currentturn% == 18
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
  if %player19% != default
    %send% %actor% @GPlayer 19:  @W%self.player19%@n
    if %currentturn% == 19
      %send% %actor% @R     ...and it's their turn!@n
    end
  end
elseif %arg% == endturn
  if %actor.name% == %currentname%
    %echo% @Y%currentname%@R's turn has ended!@n
    set currentname default
    set turncounter 0
    while %currentname% == default  
      if %currentturn% == 1
        set currentname %self.player2%
        set currentturn 2
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 2
        set currentname %self.player3%
        set currentturn 3
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 3
        set currentname %self.player4%
        set currentturn 4
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 4
        set currentname %self.player5%
        set currentturn 5
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 5
        set currentname %self.player6%
        set currentturn 6
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 6
        set currentname %self.player7%
        set currentturn 7
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 7
        set currentname %self.player8%
        set currentturn 8
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 8
        set currentname %self.player9%
        set currentturn 9
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 9
        set currentname %self.player10%
        set currentturn 10
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 10
        set currentname %self.player11%
        set currentturn 11
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 11
        set currentname %self.player12%
        set currentturn 12
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 12
        set currentname %self.player13%
        set currentturn 13
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 13
        set currentname %self.player14%
        set currentturn 14
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 14
        set currentname %self.player15%
        set currentturn 15
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 15
        set currentname %self.player16%
        set currentturn 16
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 16
        set currentname %self.player17%
        set currentturn 17
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 17
        set currentname %self.player18%
        set currentturn 18
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 18
        set currentname %self.player19%
        set currentturn 19
        eval turncounter %turncounter% + 1
      elseif %currentturn% == 19
        set currentname %self.player1%
        set currentturn 1
        eval turncounter %turncounter% + 1
      end  
      if %turncounter% > 30
        set currentname Noone
        remote currentname %self.id%
      end
      remote currentname %self.id%
      remote currentturn %self.id%
    done
    %echo% @Y%currentname%@R's turn has begun!@n  
  else
    %send% %actor% @RIt isn't your turn to end!  It's @Y%self.currentname%@R's turn!@n
  end
else
  %send% %actor% @GCurrent Commands:  @YEnroll@W, @YTurns@W, @YEndturn@n
end
~
#9106
Roleplay Assistant - Idle Poker~
0 b 100
~
if %currentname% != default
  set idlename %currentname%
  remote idlename %self.id%
  wait 300s
  if %self.currentname% == %self.idlename%
    tell %self.currentname% Hey, you!  It's your turn!  -poke, poke-
  end
end
~
#9110
Policeman Greet OmniScript~
0 h 100
~
@C********************************************************@n
@C*@W Set some variables to determine who this mob is@n
@C*@W and who they are dealing with...@n
@C********************************************************@n
@C********************************************************@n
@C*@W First, we define self class and mood, etc.@n
@C*@W These are set by the mob load script that should be@n
@C*@W attached to the mob already.@n
@C********************************************************@n
set myClass %self.myClass%
set myMood %self.Mood%
@C********************************************************@n
@C*@W Get the info of the person that walked in
@C********************************************************@n
if ( %actor.varexists(myClass)% )
  set targClass %actor.myClass%
  set myJustice %self.justice%
else
  set targClass default
end
if ( %targClass% == Criminal )
  set targNotoriety %actor.notoriety%
  if ( %targNotoriety% <= %myJustice% )
    say GET ON THE GROUND, %actor.name%!
    emote rushes forward and tackles %actor.name% before %actor.heshe% has a chance to react.  Bringing %self.hisher% knee to %actor.hisher% back, he pins %actor.himher% down and reaches for the radio on the front of %self.hisher% vest.
    wait 1s
    %purge% %actor%
    say Headquarters, I got a criminal here at %self.room.name%@C.  I'm sending them to you for processing...
    wait 5s
    emote stands up after the criminal is shipped off.  @C'@WAnother punk off the streets...@C'@n
    eval justice %myJustice% + 1
    remote justice %self.id%
  elseif
    emote eyes %actor.name% warily, clearly afraid of %actor.himher%.  %self.name% shakes %self.hisher% head, remaining alert but too scared to do anything.
  end
end
~
#9111
Criminal Greet Omniscript~
0 h 100
~
@C********************************************************@n
@C*@W Set some variables to determine who this mob is@n
@C*@W and who they are dealing with...@n
@C********************************************************@n
@C********************************************************@n
@C*@W First, we define self class and mood, etc.@n
@C*@W These are set by the mob load script that should be@n
@C*@W attached to the mob already.@n
@C********************************************************@n
set myClass %self.myClass%
set myMood %self.Mood%
@C********************************************************@n
@C*@W Get the info of the person that walked in
@C********************************************************@n
if ( %actor.varexists(myClass)% )
  set targClass %actor.myClass%
else
  set targClass default
end
if ( %targClass% == AdultCitizen )
  set myNotoriety %self.notoriety%
  if ( %myNotoriety% >= 30 )
    wait 1s
    emote slips up behind %actor.name% and slides a blade into their back while reaching around to cover their mouth.  %self.name% drags %actor.name% out of view and then returns with a smile on %self.hisher% face.
    %damage% %actor% 10000000000
    eval notoriety %myNotoriety% + 1
    remote notoriety %self.id%
  elseif ( %myNotoriety% >= 20 )
    wait 1s
    emote eyes %actor.name% for a moment before making a sign to someone in the distance.  Smiling to himself, he watches %actor.name% maliciously.
    eval notoriety %myNotoriety% + 1
    remote notoriety %self.id%
  else
    wait 1s
    emote blinks as %actor.name% enters.  He takes out his iMuffin and rapidly types a few things on it, then averts his attention elsewhere.
    eval notoriety %myNotoriety% + 1
    remote notoriety %self.id%
  end
end
~
#9140
Mob Definition:  POIs~
0 n 100
~
set Vegeta1 Seddy
remote Vegeta1 %self.id%
~
#9180
POI Check~
1 c 1
poicheck~
*************************************************
* Load mob 9190, who has vars set by trigger 9181
*************************************************
%load% mob 9190
*************************************************
* Wait for the mob to load...
*************************************************
wait 1
*************************************************
* Now steal all the vars...
*************************************************
set mypeople %self.room.people%
set Earth_1 %mypeople.Earth_1%
set Earth_2 %mypeople.Earth_2%
set Earth_3 %mypeople.Earth_3%
set Earth_4 %mypeople.Earth_4%
set Earth_5 %mypeople.Earth_5%
set Earth_6 %mypeople.Earth_6%
set Vegeta_1 %mypeople.Vegeta_1%
set Vegeta_2 %mypeople.Vegeta_2%
set Vegeta_3 %mypeople.Vegeta_3%
set Vegeta_4 %mypeople.Vegeta_4%
set Vegeta_5 %mypeople.Vegeta_5%
set Vegeta_6 %mypeople.Vegeta_6%
set Konack_1 %mypeople.Konack_1%
set Konack_2 %mypeople.Konack_2%
set Konack_3 %mypeople.Konack_3%
set Konack_4 %mypeople.Konack_4%
set Konack_5 %mypeople.Konack_5%
set Konack_6 %mypeople.Konack_6%
set Namek_1 %mypeople.Namek_1%
set Namek_2 %mypeople.Namek_2%
set Namek_3 %mypeople.Namek_3%
set Namek_4 %mypeople.Namek_4%
set Namek_5 %mypeople.Namek_5%
set Namek_6 %mypeople.Namek_6%
set Frigid_1 %mypeople.Frigid_1%
set Frigid_2 %mypeople.Frigid_2%
set Frigid_3 %mypeople.Frigid_3%
set Frigid_4 %mypeople.Frigid_4%
set Frigid_5 %mypeople.Frigid_5%
set Frigid_6 %mypeople.Frigid_6%
**************************************************
* Don't worry, the mob purges itself after...
**************************************************
**************************************************
* K.  Let's do stuff.
**************************************************
if %arg% == Earth
  %send% %actor% @G=====================================================@n
  %send% %actor% @G=      Showing Planetary Leadership for Earth       =@n
  %send% %actor% @G=====================================================@n
  %send% %actor% @Y-@R>@G Earth 1:@W %Earth_1%
  %send% %actor% @Y-@R>@G Earth 2:@W %Earth_2%
  %send% %actor% @Y-@R>@G Earth 3:@W %Earth_3%
  %send% %actor% @Y-@R>@G Earth 4:@W %Earth_4%
  %send% %actor% @Y-@R>@G Earth 5:@W %Earth_5%
  %send% %actor% @Y-@R>@G Earth 6:@W %Earth_6%
else
  %send% %actor% @RThere is no information for %arg%@n
end
~
#9181
POI Settings~
0 n 100
~
* POI Control Interface
* ***********
*  Earth
* ***********
set Earth_1 King Furry
set Earth_2 King Furry
set Earth_3 King Furry
set Earth_4 King Furry
set Earth_5 King Furry
set Earth_6 King Furry
* ***********
*  Vegeta
* ***********
set Vegeta_1 King Vegeta
set Vegeta_2 King Vegeta
set Vegeta_3 King Vegeta
set Vegeta_4 King Vegeta
set Vegeta_5 King Vegeta
set Vegeta_6 King Vegeta
* ***********
*  Konack
* ***********
set Konack_1 Council of Konack
set Konack_2 Council of Konack
set Konack_3 Council of Konack
set Konack_4 Council of Konack
set Konack_5 Council of Konack
set Konack_6 Council of Konack
* ***********
*  Namek
* ***********
set Namek_1 Guru
set Namek_2 Guru
set Namek_3 Guru
set Namek_4 Guru
set Namek_5 Guru
set Namek_6 Guru
* ***********
*  Frigid
* ***********
set Frigid_1 King Cold
set Frigid_2 King Cold
set Frigid_3 King Cold
set Frigid_4 King Cold
set Frigid_5 King Cold
set Frigid_6 King Cold
* ********************************************
*  remote the variables to the POI Control Mob
* ********************************************
remote Earth_1 %self.id%
remote Earth_2 %self.id%
remote Earth_3 %self.id%
remote Earth_4 %self.id%
remote Earth_5 %self.id%
remote Earth_6 %self.id%
remote Vegeta_1 %self.id%
remote Vegeta_2 %self.id%
remote Vegeta_3 %self.id%
remote Vegeta_4 %self.id%
remote Vegeta_5 %self.id%
remote Vegeta_6 %self.id%
remote Konack_1 %self.id%
remote Konack_2 %self.id%
remote Konack_3 %self.id%
remote Konack_4 %self.id%
remote Konack_5 %self.id%
remote Konack_6 %self.id%
remote Namek_1 %self.id%
remote Namek_2 %self.id%
remote Namek_3 %self.id%
remote Namek_4 %self.id%
remote Namek_5 %self.id%
remote Namek_6 %self.id%
remote Frigid_1 %self.id%
remote Frigid_2 %self.id%
remote Frigid_3 %self.id%
remote Frigid_4 %self.id%
remote Frigid_5 %self.id%
remote Frigid_6 %self.id%
* *********************************************
*  wait long enough for stuff to get my vars
***********************************************
wait 4
* *********************************************
*  Then purge myself for so no evidence :)
***********************************************
%purge% %self%
~
#9190
Mob Interaction - Greet~
0 h 100
~
@C********************************************************@n
@C*@W Set some variables to determine who this mob is@n
@C*@W and who they are dealing with...@n
@C********************************************************@n
@C********************************************************@n
@C*@W First, we define self class and mood, etc.@n
@C*@W These are set by the mob load script that should be@n
@C*@W attached to the mob already.@n
@C********************************************************@n
if ( %self.varexists(myClass)% )
  set myClass %self.myClass%
else
  set myClass default
end
if ( %self.varexists(Mood)% )
  set myMood %self.Mood%
else
  set myMood 5
end
if ( %actor.varexists(myClass)% )
  set targClass %actor.myClass%
else
  set targClass default
end
@C********************************************************@n
@C* @WSetting a random number for use later...@n
@C********************************************************@n
set smallRand %random.10%
@C********************************************************@n
@C* @WSetting a random number for use later...@n
@C********************************************************@n
set bigRand %random.100%
@C********************************************************@n
@C* @WNow, let's decide what we're doing if the mob is a@n
@C* @Wcertain class...@n
@C********************************************************@n
@C********************************************************@n
@C* @WIf the mob is a policeman type...@n
@C********************************************************@n
if ( %myClass% == Policeman )
  @C********************************************************@n
  @C* @WIf the target is a policeman type...@n
  @C********************************************************@n
  if ( %targClass% == Policeman )
    if ( %myMood% <= 9 )
      eval myMood %myMood% + 1
      set Mood %myMood%
      remote Mood %self.id%
    end
    switch %myMood%
      case 1
        say About freaking time, %actor.name%!  I was beginning to think I was the only person working!
      break
      case 2
        say Good!  You're here, %actor.name%!  Now I don't have to do everything myself.
      break
      case 3
        say Oh!  So I guess there are other people on duty!
      break
      case 8
        say Representing the LAW!  We got this, %actor.name%
      break
      case 9
        say Nothing can take us down now that you're here, %actor.name%!
      break
      case 10
        say Hell yeah!  I love you, %actor.name%!  I mean... plutonic and all.  Yeah...
      break
      default
      done
      @C********************************************************@n
      @C* @WIf the target is a criminal type...@n
      @C********************************************************@n
    elseif ( %targClass% == Criminal )
      if ( %myMood% >= 1 )
        eval myMood %myMood% - 1
        set Mood %myMood%
        remote Mood %self.id%
      end
      switch %myMood%
        case 0
        say %actor.name%!  Please.  I beg you to break a law.  I wanna break your face so bad...
      break
      case 1
        say What a piece of crap.  Dare you to mess up and give me a reason to beat you, %actor.name%!
      break
      case 2
        say Great.  Another crap piece of criminal like %actor.name% I have to watch...
      break
      case 3
        say I've got my eye on you, you piece of crap, %actor.name%.
      break
      case 8
        say Keep your nose clean, %actor.name%!
      break
      case 9
        say I'm in such a good mood, I'm not even going to warn you, %actor.name%.
      break
      default
        say Damn criminals...
      done
    else
      wait 1s
      emote eyes *%actor.name% critically.
      wait 1s
      say I've got my eye on you.  Don't be causing trouble on my watch...
    end
    @C********************************************************@n
    @C* @WIf the mob is a criminal type...@n
    @C********************************************************@n
  elseif ( %myClass% == Criminal )
    @C********************************************************@n
    @C* @WIf the target is a policeman type...@n
    @C********************************************************@n
    if ( %targClass% == Policeman )
      if ( %myMood% >= 1)
        eval myMood %myMood% - 1
        set Mood %myMood%
        remote Mood %self.id%
      else
        wait 1s
        say Ahhhh, screw it... 
        wait 2s
        steal zenni %actor.alias%
      end
      @C********************************************************@n
      @C* @WIf the target is a Criminal type...@n
      @C********************************************************@n
    elseif ( %targClass% == Criminal )
      if ( %myMood% <= 9 )
        eval myMood %myMood% + 1
        set Mood %myMood%
        remote Mood %self.id%
      end
    end
    @C********************************************************@n
    @C* @WIf the mob is a AdultCitizen type...@n
    @C********************************************************@n
  elseif %myClass% == AdultCitizen
  else
    @C********************************************************@n
    @C* @WIf the mob has this script, but doesn't have a class@n
    @C* @Wset, this fires.  A class needs to be added to mob@n
    @C********************************************************@n
    wait 1s
    say I don't have a defined class.  You should get an Imm to check my triggers...
  end
~
#9191
Mob Interaction:  Enter New Room~
0 i 100
~
@C********************************************************@n
@C* @WSet some variables to determine who this mob is@n
@C* @Wand who they are dealing with...@n
@C********************************************************@n
if ( %self.varexists(myClass)% )
  set myClass %self.myClass%
else
  set myClass default
end
set myPeople %self.room.people%
set entryTarget default
if %self.room.contents(65535)%
  set myCorpse %self.room.contents(65535)%
else
  set myCorpse nothing
end
if %myCorpse.name% /= corpse
  @C********************************************************@n
  @C* @WIf there's a corpse here..@n
  @C********************************************************@n
  if %myClass% == AdultCitizen
    @C********************************************************@n
    @C* @WIf I'm an AdultCitizen@n
    @C********************************************************@n
    wait 3
    say Ahhh!  A corpse!  I don't wanna die!
    flee
    wait 2s
    emote breathes heavily, placing a hand on %self.hisher% chest.  @C'@WI can't believe there's a corpse back there...@C'@n
  elseif %myClass% == Policeman
    @C********************************************************@n
    @C* @WIf I'm a Policeman@n
    @C********************************************************@n
    wait 3
    emote gasps and rushes over to a corpse.  @C'@YHmm... I should investigate this.  Something nefarious is afoot...@C'@n    
    wait 4s
    say I probably shouldn't leave this here.  I'll take it to the coroner for an autopsy...
    get corpse
  else
    @C********************************************************@n
    @C* @WIf I'm not an AdultCitizen@n
    @C********************************************************@n
  end
elseif %myCorpse.name% /= zenni
  wait 1s
  say Cha-Ching!
  get zenni
else
  @C********************************************************@n
  @C* @WIf there's no corpse here..@n
  @C********************************************************@n
  while %myPeople%
    set temp_person %myPeople.next_in_room%
    if ( %myPeople.id% != %self.id% )
      set entryTarget %myPeople%
    end
    set myPeople %temp_person%
  done
end
~
#9198
drop my gauntlet~
1 g 100
~
if ( %actor.name% != Seddy )
  wait 1
  %force% %actor.name% say I am not worthy...
  %force% %actor.name% drop %self.name%
end
~
#9199
var fixer~
1 c 3
fixme~
set notoriety 50
remote notoriety %actor.id%
%force% %actor% emote smiles, feeling something within them was fixed.
~
$~
