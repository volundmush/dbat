#12200
Yakavita Yacht block~
0 q 100
~
if (%direction% == north && %actor.varexists(yakavitafavor)%)
 say Welcome aboard. Mister Yakavita is in his cabin.
 return 1
elseif (%direction% == north && !%actor.varexists(yakavitafavor)%)
 return 0
 say Looking to offload some goods eh?
 wait 2 s
 say Well no one gets to see Mister Yakavita without doing us a favor first.
 wait 2 s
 say Bring me an Ortusian pearl that the crabs near here often hoard.
end
~
#12201
Yakavita Guard receive~
0 j 100
~
if (%object.vnum% == 12100 && !%actor.varexists(yakavitafavor)%)
set yakavitafavor
remote yakavitafavor %actor.id%
%purge% %object%
wait 2 s
say Good. You can see Mister Yakavita now. He'll buy anything you've got to sell.
else
return 0
say I don't want this. You've already gotten in our good books.
end
~
$~
