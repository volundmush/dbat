#11400
No S or Se~
2 q 100
~
if %direction% == south || %direction% == southeast
 return 0
%send% %actor% You don't think they'll let you do that. Try going through the gate in the north.
end
~
#11401
No S, SE, E~
2 q 100
~
if %direction% == east || %direction% == south || %direction% == southeast
 return 0
%send% %actor% You don't think they'll let you do that. Try going through the gate in the north.
end
~
#11402
No E~
2 q 100
~
if %direction% == east
 return 0
%send% %actor% You don't think they'll let you do that. Try going through the gate in the north.
end
~
#11403
No N, NE, E~
2 q 100
~
if %direction% == north || %direction% == northeast || %direction% == east
 return 0
%send% %actor% You don't think they'll let you do that. Try going through the gate in the north.
end
~
#11404
No N, NE~
2 q 100
~
if %direction% == north || %direction% == northeast
 return 0
%send% %actor% You don't think they'll let you do that. Try going through the gate in the north.
end
~
#11405
No Nw, N, Ne~
2 q 100
~
if %direction% == northwest || %direction% == north || %direction% == northeast
  return 0
  %send% %actor% You don't think they'll let you do that. Try going through the gate in the north.
else
  return 1
end
~
#11406
No NW, N~
2 q 100
~
if %direction% == north || %direction% == northwest
 return 0
%send% %actor% You don't think they'll let you do that. Try going through the gate in the north.
end
~
#11407
No W, NW, N~
2 q 100
~
if %direction% == west || %direction% == northwest || %direction% == north
 return 0
%send% %actor% You don't think they'll let you do that. Try going through the gate in the north.
end
~
#11408
No W~
2 q 100
~
if %direction% == west
  return 0
  %send% %actor% You don't think they'll let you do that. Try going through the gate in the north.
end
~
#11409
No W, SW, S~
2 q 100
~
if %direction% == west || %direction% == southwest || %direction% == south
 return 0
%send% %actor% You don't think they'll let you do that. Try going through the gate in the north.
end
~
#11410
No S, SW~
2 q 100
~
if %direction% == south || %directino% == southwest
  return 0
  %send% %actor% You don't think they'll let you do that. Try going through the gate in the north.
end
~
$~
