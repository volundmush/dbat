#2100
Karaoke - Commands~
1 c 100
karaoke~
wait 1s
if %self.weight% == 1
  if %arg%
    nop %self.weight(5)%
    %send% %actor% @RSearching for song:  @Y%arg%@n
    %echoaround% %actor% @R%self.name% blips as something is entered.@n
    wait 2s
    %send% %actor% @RSong Found!  Playing in @Y5@R seconds.  Better warm up your voice!@n
    %echoaround% %actor% %self.name% @R spins to life as it prepares to play a song.@n 
    wait 1s
    %send% %actor% @RSong playing in @Y4@R seconds.@n
    wait 1s
    %send% %actor% @RSong playing in @Y3@R seconds.@n
    wait 1s
    %send% %actor% @RSong playing in @Y2@R seconds.@n
    wait 1s
    %send% %actor% @RSong playing in @Y1@R seconds.@n
    wait 1s
    %echo% %self.name% @Rbegins playing the instrumentals and backup vocals to the song:  @Y%arg%@R.@n
    wait 120s
    nop %self.weight(-5)%
  else
    %send% %actor% @RYou need to provide a song name as well.  @YKaraoke <song name>@R.@n
  end
else
  %send% %actor% @RPlease wait until the current song is finished.@n
end
~
$~
