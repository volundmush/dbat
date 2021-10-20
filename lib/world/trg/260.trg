#64700
Skill Check: Search~
2 g 100
~
If (%actor.skill(Spot)% > 50)
  wait 0s
  %send% %actor% @YYou @Rnotice@Y something to the @RNorthwest@Y.@n
End
~
#64701
Talk: [64705] Orlen~
0 c 100
Talk~
if (%actor.class% == Frieza || %actor.class% == Ginyu) 
  if (%cmd% == Talk)
    if %arg% == Orlen
      %send% %actor% @D------------------------------------@n
      %send% %actor% @yOption @G1@c) @WWhat are you doing here?@n
      %send% %actor% 
      %send% %actor% @RSyntax@w: @cTalk Orlen @G<option number>@n
      %send% %actor% @D------------------------------------@n
    elseif %arg% == Orlen 1
      wait 1 s
      %echoaround% %actor% @c%actor.name%@w asks what is %self.name% doing here.@n
      %send% %actor% @cYou@w ask %self.name% what he is doing here.@n
      wait 2 s
      whisper %actor.name% I am working undercover. Don't question me and blow my cover.
    end
  end
else
  if (%cmd% == Talk)
    if %arg% == Orlen
      %send% %actor% @D------------------------------------@n
      %send% %actor% @yOption @G1@c) @WWhat are you doing here?@n
      %send% %actor% @yOption @G2@c) @WWhy did you defect from the World Trade?@n
      %send% %actor% @yOption @G3@c) @WIs there anything else?@n
      %send% %actor% 
      %send% %actor% @RSyntax@w: @cTalk Orlen @G<option number>@n
      %send% %actor% @D------------------------------------@n
    elseif (%arg% == Orlen 1)
      wait 1
      %echoaround% %actor% @c%actor.name%@w asks what is %self.name% doing here.@n
      %send% %actor% @cYou@w ask %self.name% what he is doing here.@n
      wait 2 s
      Say I have defected from the World Trade and decided to help the Rebels of Namek. What else would you like to talk about?
    elseif (%arg% == Orlen 2)
      wait 1 s
      %echoaround% %actor% @c%actor.name%@w asks %self.name% why he decided to defect from the World Trade.@n
      %send% %actor% @cYou@w ask %self.name% why he decided to defect from the World Trade.@n
      wait 2 s
      say I was assigned to invade this planet alongside Kiwi. I don't really know, why I did what I did...
      wait 2 s
      emote shrugs. &1I guess maybe because the World Trade invaded my home planet and killed most us, so I saw myself in these Namekians&2
      wait 2 s
      say Or maybe I think that the World Trade is a declining Super Power within the Galaxy. I mean...
      wait 3 s
      say If the Monkeys can become independent then why couldn't other races?
      wait 2 s
      say And I heard that rumours that King Cold is contemplating invading Earth and that could spark a Galactic War because Earth is...
      wait 1 s 
      say ... Is its own rising Super Power in the Galaxy that won't tolerate slavery.
      wait 3 s
      emote takes a deep breathe, &1But I've spoken too much now. I don't have anything else to say&2   
    elseif (%arg% == Orlen 3)
      wait 1 s
      %echoaround% %actor% @c%actor.name%@w asks %self.name% if there is anything else.@n
      %send% %actor% @cYou@w ask %self.name% if there is anything else.@n
      wait 2 s
      say No, not from me but speak with Tsuburi. He may have something for you to do.
    end
  end
end
~
#64702
Talk: [64706] A Namekian Rebel Warrior~
0 c 100
Talk~
if (%cmd% == Talk)
  if %arg% == Warrior
    %send% %actor% @D------------------------------------@n
    %send% %actor% @yOption @G1@c) @WWhat is this place?@n
    %send% %actor% 
    %send% %actor% @RSyntax@w: @cTalk Warrior @G<option number>@n
    %send% %actor% @D------------------------------------@n
  elseif %arg% == Warrior 1
    %echoaround% %actor% @c%actor.name%@w asks %self.name%@w what this place is.@n
    %send% %actor% @w'@CI don't have time to talk you,@w' %self.name% @wwalks away as it struggles with the boxes it is carrying, '@CCan't you see that there's work to be done here?@w'@n
    wait 1 s
    leave
  end
end
~
#64703
Skill Check: Telepathy~
0 g 100
~
if (%actor.varexists(Bazzoon_Healer_Request)%) @R
  * @YBelow is if the PC can heal but has not done so yet.@n
  wait 1 s
  %send% %actor% %self.name% looks at you desperately and begs, '@CPlease! You look like you know how to heal somebody! Please try to @RHeal @Cthe @RDying@C Namekian!@w'@n
  %echoaround% %actor% %self.name% looks at %actor.name% and begs %actor.himher% to heal the dying Namekian.@n
elseif (%actor.varexists(Bazzoon_Healer_Completed)%) @R
  * @YBelow is if the PC has healed the Namekian already.@n
  wait 1 s
  %send% %actor% %self.name% smiles warmly at you and says, '@CThanks, I'm not as skilled at healing as you but I can handle doing any further healing now@w'@n
  %echoaround% %actor% %self.name% smiles warmly at %actor.name% and says, '@CThanks, I'm not as skilled at healing as you but I can handle doing any further healing now@w'@n
elseif (%actor.varexists(Bazzoon_Healer_Rejected)%) @R
  * @YBelow is if the PC can not heal.@n
  emote sobs silently as it tries to heal every Namekian it can.
elseif (%actor.skill(Telepathy)% > 30) @R
  * @YThe following fires if the character has 31 or higher @CTelepathy@Y skill.@n
  if (%actor.skill(Heal)% > 5) @R
    * @YThe following fires if the character has 6 or higher @CHeal@Y skill.@n
    %send% %actor% %self.name%'s loud thoughts force themselves into your mind and you hear them telepathically!
    wait 1 s
    %send% %actor% @c%self.name%@w thinks, @w'@CThere's too many. I can't save them! I'm a failure of a Healer if I can't save my patients...@w'@n
    wait 1 s
    %echoaround% %actor% %self.name% looks at %actor.name% and then says, '@CHey you look like you something about healing, could you please help me and try to heal the dying Namekian@w'@n
    %send% %actor% %self.name% looks at you and then says, '@CHey you look like you something about healing, could you please help me and try to heal the dying Namekian@w'@n
    if (%actor.room.vnum% = %self.room.vnum%)
      set Bazzoon_Healer_Request
      remote Bazzoon_Healer_Request %actor.id%
    else
      %send% %actor% @RIf you see this message report it to an IMM.
      * @YThe above should not be visible, but just in case. Cause really WTF is a PC doing with Heal skill below 5.@n 
    end
  elseif (%actor.skill(Heal)% > 5) @R
    * @YThe following fires if the character has 6 or higher @CHeal@Y skill.@n
    %echoaround% %actor% %self.name% looks at %actor.name% and then says, '@CHey you look like you something about healing, could you please help me and try to heal the dying Namekian@w'@n
    %send% %actor% %self.name% looks at you and then says, '@CHey you look like you something about healing, could you please help me and try to heal the dying Namekian@w'@n
    if (%actor.room.vnum% = %self.room.vnum%)
      set Bazzoon_Healer_Request
      remote Bazzoon_Healer_Request %actor.id%
    end
  else @R
    * @YThe below fires for anybody who doesnt have heal skill@n
    wait 1 s
    emote sobs softly, &1@CIf you're not here to help then get out!@w&2 @n
    if (%actor.room.vnum% = %self.room.vnum%)
      set Bazzoon_Healer_Rejected
      remote Bazzoon_Healer_Rejected %actor.id%
    end
  end
end
@R
* @YNote to Future Me@R
* @GBazzoon_Healer_Rejected @Yis for when a player does not have Heal Skill in their skill slot.@R
* @GBazzoon_Healer_Request @Yis for when the player hasn't used the Heal command on the NPC@R
* @GBazzoon_Healer_Complete @Yis for when the player has healed the NPC@n
~
#64704
Talk: [64707] Bazzoon~
0 c 100
Talk~
if %actor.varexists(Bazzoon_Healer_Completed)%) @R
  * @YThe above checks if the PC has healed a Namekian before.@n
  if (%cmd% == Talk)
    if (%arg% == Bazzoon)
      %send% %actor% @D------------------------------------@n
      %send% %actor% @yOption @G1@c) @WHow is the patient feeling?
      %send% %actor% @yOption @G2@c) @WDo I need to heal anymore people?
      %send% %actor% @yOption @G3@c) @WAnything I can help you with anything else?
      %send% %actor% 
      %send% %actor% @RSyntax@w: @cTalk Bazzoon @G<option number>@n
      %send% %actor% @D------------------------------------@n
    elseif (%arg% == Bazzoon 1)
      wait 1 s
      %echoaround% %actor% %actor.name% asks %self.name% how the patient is doing.
      say It is doing better now that you've healed it. Thank you again for that.
    elseif (%arg% == Bazzoon 2)
      %echoaround% %actor% %actor.name% asks %self.name% if there are anymore people that need healing.
      emote shakes its head, 'No thanks. I can handle it from here.'
    elseif (%arg% == Bazzoon 3)
      wait 1 s
      %echoaround% %actor% %actor.name% asks %self.name% if there is anything else %actor.heshe% can help with.
      say Like I said earlier, I can handle it from here but you may want to talk to other Namekians as they may have something that you can help them with
    end
  end
elseif %actor.varexists(Bazzoon_Healer_Request)%) @R
  * @YThe below is for if the PC has been asked to heal but hasn't done so yet.@n
  if (%cmd% == Talk)
    if (%arg% == Bazzoon)
      %send% %actor% @D------------------------------------@n
      %send% %actor% @yOption @G1@c) @WWhy are you sobbing?@n
      %send% %actor% 
      %send% %actor% @RSyntax@w: @cTalk Bazzoon @G<option number>@n
      %send% %actor% @D------------------------------------@n
    elseif (%arg% == Bazzoon 1)
      wait 1 s
      %echoaround% %actor% %actor.name% asks %self.name% why it is sobbing.
      say I don't have time for questions, get out or heal the Dying Namekian!
    end
  end
else @R
  * @YThe below should only be on PCs who CAN NOT HEAL@n
  if (%cmd% == Talk)
    if (%arg% == Bazzoon)
      %send% %actor% @D------------------------------------@n
      %send% %actor% @yOption @G1@c) @WWhy are you sobbing?@n
      %send% %actor%
      %send% %actor% @RSyntax@w: @cTalk Bazzoon @G<option number>@n
      %send% %actor% @D------------------------------------@n
    elseif (%arg% == Bazzoon 1)
      wait 1 s
      %echoaround% %actor% %actor.name% asks %self.name% why it is sobbing.
      say Get out of here instead of asking me stupid questions!
      wait 1 s
      %echoaround% %actor% %self.name% chases %actor.name% away!
      %send% %actor% %self.name% chases you away!
      %force% %actor% leave
    end
  end
end
~
#64705
Heal Dying Namekian: Quest Completion & Reward~
0 c 100
Heal~
if (%actor.skill(Heal)% > 59)
  if (%cmd% == Heal)
    if (%arg% == Dying)
      if (%actor.varexists(Bazzoon_Healer_Completed)%)
        wait 1 s
        %echoaround% %actor% %self.name% stops %actor.name% from doing any more healing and says, '@CI can take it from here. Thank you@w'@n
        %send% %actor% %self.name% shakes its head at you, '@CI can take it from here thank you@w'@n
      elseif (%actor.varexists(Bazzoon_Healer_Request)%)
        wait 1 s
        %echoaround% %actor% @C%actor.name%@W places their hands above A dying @GN@ga@Gm@gek@Gi@ga@Gn@W and starts to heal them...@n 
        %send% %actor% @CYou @Wplace your hands above A dying @GN@ga@Gm@gek@Gi@ga@Gn@W and start to heal them...@n
        wait 2 s
        %echo% @wA dying @GN@ga@Gm@gek@Gi@ga@Gn@w appears to have been brought back from the brink of death.@n
        wait 2 s
        emote claps excitedly, &1Well done! Now let me give you a reward!&2
        wait 1 s
        emote rummages through hands you a pair of gloves. '@COnly certain Namekians get these but since you've been a big help, you can use the ones I was given@w'@n
        if (%actor.room.vnum% = %self.room.vnum%)
          if (%actor.class% == Nail)
            rdelete Bazzoon_Healer_Request %actor.id%
            set Bazzoon_Healer_Completed
            remote Bazzoon_Healer_Completed %actor.id%
            %load% obj 64701
            * @YThe gloves above are wearable only by Nail students!@n
            give glove %actor.name%
          elseif (%actor.class% == Kibito)
            rdelete Bazzoon_Healer_Request %actor.id%
            set Bazzoon_Healer_Completed
            remote Bazzoon_Healer_Completed %actor.id%
            %load% obj 64702
            * @YThe gloves above are wearable only by Kibito students!@n
            give glove %actor.name%
          else
            %send% %actor% @YThis should not be possible. Please speak to an IMM@n
          end
        end
      elseif (%actor.varexists(Bazzoon_Healer_Rejected)%)\
        wait 1 s
        %echo% %actor.name% tries to heal A dying @GN@ga@Gm@gek@Gi@ga@Gn@w but is stopped by %self.name%
      end
    end
  end
else
  wait 1 s
  %send% %actor% @WYou feel that if you tried to Heal A dying @GN@ga@Gm@gek@Gi@ga@Gn@W at your current skill level that you would only make things worst. Come back when you know more about Healing.@n
end
~
#64706
Talk: [64704] Igor~
0 c 100
Talk~
if (%actor.race% == Truffle)
  if (%cmd% == Talk)
    if (%arg% == Igor) @R
      * @YThe below only fires for @RTruffles@Y.@n
      %send% %actor% @D------------------------------------@n
      %send% %actor% 
      %send% %actor% @yOption @G1@c) @WHello@n
      %send% %actor% @yOption @G2@c) @WWhat is a Truffle doing here?@n
      %send% %actor% @yOption @G3@c) @WThe Hyper Gates?@n
      %send% %actor% @yOption @G4@c) @WWhat are you building?@n
      %send% %actor% @yOption @G5@c) @WIs there anything else you want to say?@n
      %send% %actor% 
      %send% %actor% @RSyntax@w: @cTalk Igor @G<option number>@n
      %send% %actor% @D------------------------------------@n
    elseif (%arg% == Igor 1)
      wait 1 s
      %send% %actor% %self.name% takes his goggles and gives you a bright big smile. '@CPrivyet! Come comrade! I was not expecting one of my own!@w'@n
      %echoaround% %actor% %self.name% takes his goggles and gives %actor.name% a bright big smile. '@CPrivyet! Come comrade! I was not expecting one of my own!@w'@n
      wait 2 s
      say What shall we discuss first, comrade?
    elseif (%arg% == Igor 2)
      wait 1 s
      emote takes a sip of his Black Label Vodka.@n
      wait 1 s
      %echoaround% %actor% @W%actor.name% asks %self.name% what he is doing here.@n
      emote laughs loudly, &1Us Tuffles... we never change, eh?&2 Takes another sip, &1Always straight to the point&2
      wait 2 s
      say But if you must know, I am here to ensure the Hyper-Gates do not fall into the hands of the World Trade.
      wait 1 s
      emote puts his glass of vodka down and gets back to building. '@CBack to work now@w'@n
    elseif (%arg% == Igor 3)
      wait 1 s
      %echoaround% %actor% %actor.name% asks %self.name% what the Hyper Gates have to do with the World Trade.@n
      %send% %actor.name% You ask what the Hyper Gates have to do with the World Trade.@n
      wait 2 s
      emote puts more Black Label Vodka into a glass and then begins to speak, &1Comrade, many people think that it was the Truffles who built the Hyper Gates...&2
      wait 2 s
      emote shakes his glass of vodka a little, &1But that is not the truth. We are not the makers of the Hyper Gates&2 He winks, &1But this is a secret only high ranking Truffles know...&2
      wait 2 s
      say So because the World Trade enslaves many, it is possible their slaves will be able to learn from The Gates just as we did and if they do then we Truffles may not be able to use them
      wait 2 s
      emote shrugs, &1So the higher-ups back at the Neo Nirvana decided that it is best for Tuffles if we help the Namekians in this planetary dispute&2
      wait 1 s
      emote rubs his throat, &1Gah! See I speak so much that now my voice is tired. I go back to work now!&2 
      wait 2 s
      emote gets back to building.
    elseif (%arg% == Igor 4)
      wait 1 s
      %echoaround% %actor% %actor.name% asks %self.name% what he is building.@n
      say I am building equipment for the Namekian Resistance Fighters to use against their oppressors. I have an agreement with one of them.
      wait 2 s
      say The other Namekians think that he is building it when I secretly. Many in the universe think us dead, Comrade, so it is best for us Tuffles to work behind the scenes, ya
      wait 1 s
      emote puts his goggles back on and gets to building.
    elseif (%arg% == Igor 5)
      wait 1 s
      %echoaround% %actor% %actor.name% asks %self.name% if there is anything else he would like to say.@n
      emote looks around carefully, &1This is a secret between us...&2 The Truffle says, &1Though I am alone on this planet, I have not forgotten the true oppressors of this universe&2
      wait 1 s
      emote slaps a fist on his chest and says, &1I have placed anti-Blutz Wave emittors within my creations.&2 He gives off a hearty laugh, &1No Saiyajin will ever be able to potentially use what I've made against Tuffles&2
    end
  end
else
  if (%cmd% == Talk)
    if (%arg% == Igor) @R
      * @YThe below only fires for @Rnon-Truffles@Y.@n
      %send% %actor% @D------------------------------------@n
      %send% %actor% 
      %send% %actor% @yOption @G1@c) @WHello@n
      %send% %actor% @yOption @G2@c) @WWhat are you?@n
      %send% %actor% @yOption @G3@c) @WWhat are you building?@n
      %send% %actor% @yOption @G4@c) @WIs there anything else you want to say?@n
      %send% %actor% 
      %send% %actor% @RSyntax@w: @cTalk Igor @G<option number>@n
      %send% %actor% @D------------------------------------@n
    elseif (%arg% == Igor 1)
      wait 1 s
      emote stops building and coldly says, &1Hello&2 and then gets back to building.@n
    elseif (%arg% == Igor 2)
      wait 1 s
      %echoaround% %actor% %actor.name% asks %self.name% what he is.@n
      emote removes his goggles, &1I am what many in the universe would a "Mutant" and I am the last of my kind. What else is there to know?&2
    elseif (%arg% == Igor 3)
      wait 1 s
      %echoaround% %actor% %actor.name% asks %self.name% what he is building.@n
      emote takes off his goggles and frowns, &1As the last of my kind, I sympathise with the Namekians.&2
      wait 3 s
      emote looks down, &1My homeworld was also taken over by violent and barbaric outsiders and since I am the last of my people I do not want the Namekians to share my fate&2 
      wait 3 s
      say So I am secretly building equipment for the resistance, please do not tell anyone. I fear if others found out then the WTO would enslave me
      wait 3 s
      emote puts his goggles back on and gets back to building.
    elseif (%arg% == Igor 4)
      wait 1 s
      %echoaround% %actor% %actor.name% asks %self.name% if there is anything else he would like to say.@n
      emote shakes his head, &1No&2 and then gets back to building
    end
  end
end
~
#64707
@RCHAT@c: [64701] Tsuburi@n~
0 c 100
Chat~
if (%actor.class% == Frieza || %actor.class% == Ginyu)
  if (%actor.align% <= 50) @R
    * @YAbove should only fire on neutral or evil characters.@n
    if (%cmd% == Chat)
      if (%arg% == Tsuburi)
        %send% %actor% @D------------------------------------@n  
        %send% %actor%  @yOption @G1@c) @WHello@n
        %send% %actor%  @yOption @G2@c) @WI don't want to leave@n
        %send% %actor%   
        %send% %actor%  @RSyntax@w: @cChat Tsuburi @G<option number>@n
        %send% %actor% @D------------------------------------@n
      elseif (Tsuburi 1)
        wait 1 s
        %echoaround% %actor% %actor.name% greets %self.name%.
        %send% %actor% You greet %self.name%.
        wait 1 s
        %echoaround% %actor% %self.name% looks at %self.name% intently and says, '@CI sense a foul aura around you. I am giving you this warning to leave now or I will kill you@w'@n
        %send% %actor% %%self.name% looks intently at you and says, '@CI sense a foul aura around you. I am giving you this warning to leave now or I will kill you@w'@n
      elseif (Tsuburi 2)
        wait 1 s
        %echoaround% %actor% %actor.name% tells %self.name% that %actor.heshe% does not want to leave.
        %send% %actor% You tell %self.name% that you do not want to leave.
        if (%actor.room.vnum% == %self.room.vnum%)
          wait 1 s
          say fine, if you won't leave then I will make you!
          scatter %actor.name%
        end
      end
    end
  else
    if (%cmd% == Chat)
      if (%arg% == Tsuburi)
        %send% %actor% @D------------------------------------@n 
        %send% %actor%  @yOption @G1@c) @WHello@n
        %send% %actor%   
        %send% %actor%  @RSyntax@w: @cChat Tsuburi @G<option number>@n
        %send% %actor% @D------------------------------------@n
      elseif (Tsuburi 1)
        wait 1 s
        %echoaround% %actor% %actor.name% greets %self.name%.
        %send% %actor% You greet %self.name%.
        wait 2 s
        %echoaround% %actor% %self.name% looks at %self.name% intently and says, '@CI sense a good aura within you, however due to circumstances regarding whom you you were taught by...@w'@n
        %echoaround% %actor% '@CI can not Chat you any further, I am sorry, I am certain that you would have been of great help to the Resistance but I simply can not take that risk@w'@n
        %send% %actor% %%self.name% looks intently at you and says, '@CI sense a good aura within you, however due to circumstances regarding whom you you were taught by...@w'@n
        %send% %actor% '@CI can not Chat you any further, I am sorry, I am certain that you would have been of great help to the Resistance but I simply can not take that risk@w'@n
      end
    end
  end
else
  if (%actor.varexists(wto_kill <= 99)%)
    if (%cmd% == Chat)
      if (%arg% == Tsuburi)
        %send% %actor% @D------------------------------------@n 
        %send% %actor%  @yOption @G1@c) @WWhere can I find WTO soldiers to kill?@n
        %send% %actor%  
        %send% %actor%  @RSyntax@w: @cChat Tsuburi @G<option number>@n
        %send% %actor% @D------------------------------------@n
      elseif (Tsuburi 1)
        wait 1 s 
        %echoaround% %actor% %self.name% asks %self.name% where %actor.heshe% can find find WTO soldiers to kill.@n
        %send% %actor% You ask %self.name% where you can find WTO soldiers to kill.@n
        wait 2 s
        say Outside of Senzu Village, go east until you see an island with Namekian called Oboe. Speak with him if you haven't
        wait 2 s
        say From there, go south to the Frieza Encampment and kill them. Also...
        wait 2 s
        say You can go inside Senzu Village and kill the WTO soldiers that are in the battle dome.
        wait 2 s
        emote nods, &1Now go and help The Resistance!&2
      end
    end
  elseif (%actor.varexists(wto_kill_done)%) @R
    if (%cmd% == Chat)
      if (%arg% == Tsuburi)
        %send% %actor% @D------------------------------------@n 
        %send% %actor%  @yOption @G1@c) @WHello@n
        %send% %actor%  @yOption @G2@c) @WDo you need anything else?@n
        %send% %actor%  @yOption @G3@c) @WSo about that reward...@n
        %send% %actor%  
        %send% %actor%  @RSyntax@w: @cChat Tsuburi @G<option number>@n
        %send% %actor% @D------------------------------------@
      elseif (%arg% == Tsuburi 1)
        wait 1 s
        %echoaround% %actor% %actor.name% greets %self.name%.
        %send% %actor% You greet %self.name%.
        wait 2 s
        say Greetings! The Rebel Warriors have really appreciated your help with the WTO. What do you want to talk about?
      elseif (%arg% == Tsuburi 2)
        wait 1 s
        %echoaround% %actor% %actor.name% ask if %self.name% needs any more help.
        %send% %actor% You ask if %self.name% needs any more help.
        wait 2 s
        say For the time being, The Resistance does not need your help.
      elseif (%arg% == Tsuburi 3)
        wait 1 s
        %echoaround% %actor% %actor.name% asks %self.name% about the reward.
        %send% %actor% You ask %self.name% about the reward.
        wait 2 s
        say Oh yes! My apologies, you can go to Zaxo in the Supplies Room. Zaxo will supply you with the scouters they built
      end
    end
  else
    if (%cmd% == Chat)
      if (%arg% == Tsuburi)
        %send% %actor% @D------------------------------------@n 
        %send% %actor%  @yOption @G1@c) @WHello@n
        %send% %actor%  @yOption @G2@c) @WWhat is this place?@n
        %send% %actor%  @yOption @G3@c) @WIs there a reward for helping?@n
        %send% %actor%  @yOption @G4@c) @WI would like to help The Resistance@n
        %send% %actor%  
        %send% %actor%  @RSyntax@w: @cChat Tsuburi @G<option number>@n
        %send% %actor% @D------------------------------------@n
      elseif (%arg% == Tsuburi 1)
        wait 1 s
        %echoaround% %actor% %actor.name% greets %self.name%.
        %send% %actor% You greet %self.name%.
        wait 2 s
        say Greetings stranger, what brings you to the deepest parts of Planet Namek?
      elseif (%arg% == Tsuburi 2)
        wait 1 s
        %echoaround% %actor% %actor.name% asks what this place is.
        %send% %actor% You ask what this place is.
        wait 2 s
        say This is place is home to Namekian Underground Resistance, when the the World Trade invaded...
        wait 2 s 
        say We, Namekians, went to to the deepest parts of the planet.
        wait 2 s
        emote looks around the cavern. &1It's crazy to think that this place once protected our ancestors when the planet had a catastrophe...&2
        wait 2 s
        emote shakes his head and returns from his thoughts, &1But enough about that. We need fighters.&2
      elseif (%arg% == Tsuburi 3)
        wait 2 s
        %echoaround% %actor% %actor.name% asks if there is a reward for helping The Resistance.
        %send% %actor% You ask if there is a reward for helping The Resistance.
        wait 2 s
        say Yes, if you agree to help us then I will inform Zaxo to sell you the scouters it has modified into having nightvision.
      elseif (%arg% == Tsuburi 4)
        wait 1 s
        %echoaround% %actor% %actor.name% says that %actor.heshe% would like to help The Resistance.
        %send% %actor% You say that you would like to help The Resistance.
        wait 2 s
        say Excellent! The biggest issue The Resistance is currently facing is that the World Trade has a large numerical advantage...
        wait 2 s
        emote looks down to the planet of Namek. &1The World Trade can replace their soldiers and we can not replace our rebels.&2 
        wait 3 s
        emote sighs as a defeated expression appears on its face, &1If things continue like this then we may lose the planet&2
        wait 3 s
        emote forces a smile, &1That's why I need you to go kill 100 WTO soldiers...&2
        wait 2 s
        if (%actor.room.vnum% == %self.room.vnum%)
          set wto_kill
          eval wto_kill 1
          remote wto_kill %actor.id%
          %send% %actor% @D[@BOOC@D] @YQuest Added!@n
          say Now go to the surface of the planet and kill anybody you find that is aligned to the World Trade.
          wait 2 s
          say I would suggest first looking east of Senzu Village.
        end 
      end
    end
  end
end
~
#64708
Block Hyper Majin~
1 q 100
~
if %actor.varexists(candygate)%
  %send% %actor% @MYou can't do that right now!@n
  return 0
end
~
#64709
Majin Portal Block~
2 q 100
~
if %actor.varexists(candygate)%
  %send% %actor% @MYou can't do that right now!@n
  return 0
end
~
#64710
Talk: [64715] Angry Namekian~
0 c 100
Talk~
if (%cmd% == Talk)
  if (%arg% == Angry)
    %send% %actor% @D------------------------------------@n
    %send% %actor% @yOption @G1@c) @WWhat are you doing here?@n
    %send% %actor% @yOption @G2@c) @WWhy are you angry?@n
    %send% %actor% 
    %send% %actor% @RSyntax@w: @cTalk Angry @G<option number>@n
    %send% %actor% @D------------------------------------@n
  elseif (%arg% == Angry 1)
    wait 1 s
    %echoaround% %actor% @c%actor.name%@w asks what is %self.name% doing here.@n
    %send% %actor% @cYou@w ask %self.name% what it is doing here.@n
    wait 2 s
    %echo% %self.name% sulks as it fishes, '@CI am fishing. What does it look like?@w'@n
  elseif (%arg% == Angry 2)
    wait 1 s
    %echoaround% %actor% @c%actor.name%@w asks why %self.name% is angry.@n
    %send% %actor% @cYou@w ask %self.name% why it is angry.@n
    wait 2 s
    emote stops fishing and looks at the pool of water. &1Even though Namekians have no need to eat, I got stuck with the grunt job of fishing&2
    wait 2 s
    emote taps the ground, &1This place is also good for gardening so I have that job too. Feeding a bunch of off-worlders when I could be kicking some WTO butts&2
    wait 1 s
    say What a waste of my talents
    wait 2 s
    emote looks very angry. &1I get that we need help but shouldn't any non-Namekian Rebels bring their own food instead of having me fish for them?!&2
    wait 3 s
    emote becomes quiet and goes back to fishing.
  end
end
~
#64711
Igor's anti-Saiyan Scouter~
1 j 100
~
if (%actor.race% == Saiyan || %actor.race% == Halfbreed)
  wait 1 s
  %send% %actor% You hear a loud ringing in your ear and become dizzy, you quickly take off the scouter.@n
  %force% %actor% remove scouter
end
~
#64797
@REXPERIMENT. @WKill Counter on death~
0 f 100
~
if (%actor.varexists(wto_kills)%)
  eval wto_kills %actor.wto_kills%
  if (%wto_kills% == 0)
    eval wto_kills %wto_kills% + 1
    remote wto_kills %actor.id% 
    %send% %actor% You have killed numerous wtos...
  end
end
~
#64798
@REXPERIMENT. DO NOT USE.@W WTO Kill Count@n~
2 c 100
Start~
if (%cmd% == Start)
  set kill_wto
  eval kill_wto 0
  remote kill_wto %actor.id%
  %send% %actor% You now have the trigger to kill 'em.
end
~
#64799
@RIMM ONLY@W - Delete/Set Quest Triggers@n~
2 c 100
Gone~
if (%cmd% == Gone)
  if (%arg% == Request)
    rdelete Bazzoon_Healer_Request %actor.id%
  elseif (%arg% == Completed)
    rdelete Bazzoon_Healer_Completed %actor.id%
  elseif (%arg% == Rejected)
    rdelete Bazzoon_Healer_Rejected %actor.id%
  elseif (%arg%  == Place)
    set Bazzoon_Healer_Request
    remote Bazzoon_Healer_Request %actor.id%
    %echo% Request Variable is now placed onto you!
  elseif (%arg% == PCompleted)
    set Bazzoon_Healer_Completed
    remote Bazzoon_Healer_Completed %actor.id%
    %echo% Complete Variable is now placed onto you!
  elseif (%arg% == PRejected)
    set Bazzoon_Healer_Rejected
    remote Bazzoon_Healer_Rejected %actor.id%
    %echo% Rejected Variable is now placed onto you!
  end
end
~
$~
