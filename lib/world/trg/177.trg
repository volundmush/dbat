#17700
Instructor Quest Drop~
0 f 100
~
if (%actor.varexists(onSenQ3)%)
    %load% obj 17700
end
~
#17798
iMuffin Script~
1 c 3
iMuffin~
if (%arg% /= Earth)
if (%arg% /= 1)
%send% %actor% @cEarth - Popular Food and Beverage services@n
%send% %actor% @R1) @WMcStuff Restaurant @C- @WEvergreen Street @C- @WNexus City@n
%send% %actor% @R2) @WTaco Hell @C- @WHercule Street @C- @WSatan City@n
elseif (%arg% /= 2)
%send% %actor% @cEarth - Health Services@n
%send% %actor% @R1) @WSatan City Hospital @C- @WHospital Street @C- @WSatan City@n
%send% %actor% @R2) @WAhh Spa @C- @WAkibura District @C- @WNexus City@n
elseif (%arg% /= 3)
%send% %actor% @cEarth - Tourist Hotspots@n
%send% %actor% @R1) @WNexus Starport @C- @WShale Boulevard @C- @WNexus City@n
%send% %actor% @R2) @WNexus City Theater @C- @WShale Boulevard @C- @WNexus City@n
%send% %actor% @R3) @WWorld Martial Arts Tournament @C- @WCurrently being remodeled@n
%send% %actor% @R4) @WSatan City Zoo @C- @WFauna Street @C- @WSatan City@n
elseif (%arg% /= 4)
%send% %actor% @cEarth - Danger Zones@n
%send% %actor% @R1) @WCherry Blossom Mountain. @ROOC - Levels 10-20@n
%send% %actor% @R2) @WNorthern Plains. @ROOC - Level 20+@n
%send% %actor% @R3) @WSilver Mines, Lower Levels. @ROOC - Level 30+@n
elseif (%arg% /= 5)
%send% %actor% @cEarth - Notable Martial Artists - Rough Locations@n
%send% %actor% @R1) @WMaster Roshi @C- @WSouth Sea@n
%send% %actor% @R2) @WAndroid Sixteen @C- @WNexus Field@n
%send% %actor% @R3) @WMaster Krane @C- @WSatan City@n
%send% %actor% @R4) @WPiccolo @C- @WShadow Forest@n
else
%send% %actor% @CPlease select an option:@n
%send% %actor% @ROption @Y1) @WPopular Food and Beverages@n
%send% %actor% @ROption @Y2) @WHealth Services@n
%send% %actor% @ROption @Y3) @WTourist Hotspots@n
%send% %actor% @ROption @Y4) @WDanger Zones@n
%send% %actor% @ROption @Y5) @WNotable Martial Artists@n
%send% %actor% @D @n
%send% %actor% @RSyntax@n: @YiMuffin <planet name> <option number>@n
end
elseif (%arg% /= Vegeta)
if (%arg% /= 1)
%send% %actor% @cVegeta - Popular Food and Beverage services@n
%send% %actor% @R1) @WSweet Meats @C- @WVictory Street @C- @WVegetos City@n
%send% %actor% @R2) @WBusted Skull Pub @C- @WCrimson Street @C- @WVegetos City@n
elseif (%arg% /= 2)
%send% %actor% @cVegeta - Health Services@n
%send% %actor% @R1) @WSaiyan Nursery @C- @WKing's Way @C- @WVegetos City@n
elseif (%arg% /= 3)
%send% %actor% @cVegeta - Tourist Hotspots@n
%send% %actor% @R1) @WVegetos Starport @C- @WPalace Street @C- @WVegetos City@n
%send% %actor% @R2) @WVegetos Coliseum @C- @WOath Street @C- @WVegetos City@n
%send% %actor% @R3) @WVegetos Palace @C- @WPalace Street @C- @WVegetos City@n
elseif (%arg% /= 4)
%send% %actor% @cVegeta - Danger Zones@n
%send% %actor% @R1) @WRuby Cave. @ROOC - Level 30+@n
%send% %actor% @R2) @WBlood Dunes. @ROOC - Levels 15-25@n
%send% %actor% @R3) @WDestopa Swamp. @ROOC - Level 35+@n
elseif (%arg% /= 5)
%send% %actor% @cVegeta - Notable Martial Artists - Rough Locations@n
%send% %actor% @R1) @WBardock @C- @WVegetos City@n
else
%send% %actor% @CPlease select an option:@n
%send% %actor% @ROption @Y1) @WPopular Food and Beverages@n
%send% %actor% @ROption @Y2) @WHealth Services@n
%send% %actor% @ROption @Y3) @WTourist Hotspots@n
%send% %actor% @ROption @Y4) @WDanger Zones@n
%send% %actor% @ROption @Y5) @WNotable Martial Artists@n
%send% %actor% @D @n
%send% %actor% @RSyntax@n: @YiMuffin <planet name> <option number>@n
end
elseif (%arg% /= Namek)
if (%arg% /= 1)
%send% %actor% @cNamek - Popular Food and Beverage services@n
%send% %actor% @R1) @WMcStuff Restaurant @C- @WSenzu Village@n
elseif (%arg% /= 2)
%send% %actor% @cNamek - Health Services@n
%send% %actor% @R1) @WHospital @C- @WGrassy Street @C- @WElder Village@n
elseif (%arg% /= 3)
%send% %actor% @cNamek - Tourist Hotspots@n
%send% %actor% @R1) @WNamek Starport @C- @WSenzu Village@n
%send% %actor% @R2) @WVast untouched wilderness@n
elseif (%arg% /= 4)
%send% %actor% @cNamek - Danger Zones@n
%send% %actor% @R1) @WFrieza's Ship. @ROOC - Level 10+@n
%send% %actor% @R2) @WWhirlpool. @ROOC - Level 15+@n
%send% %actor% @R3) @WCrystalline Cave. @ROOC - Levels 20-35@n
elseif (%arg% /= 5)
%send% %actor% @cNamek - Notable Martial Artists - Rough Locations@n
%send% %actor% @R1) @WNail @C- @WGuru's House@n
else
%send% %actor% @CPlease select an option:@n
%send% %actor% @ROption @Y1) @WPopular Food and Beverages@n
%send% %actor% @ROption @Y2) @WHealth Services@n
%send% %actor% @ROption @Y3) @WTourist Hotspots@n
%send% %actor% @ROption @Y4) @WDanger Zones@n
%send% %actor% @ROption @Y5) @WNotable Martial Artists@n
%send% %actor% @D @n
%send% %actor% @RSyntax@n: @YiMuffin <planet name> <option number>@n
end
elseif (%arg% /= Aether)
if (%arg% /= 1)
%send% %actor% @cAether - Popular Food and Beverage services@n
%send% %actor% @R1) @WHealthy Choices @C- @WTranquility Avenue @C- @WHaven City@n
elseif (%arg% /= 2)
%send% %actor% @cAether - Health Services@n
%send% %actor% @R1) @WHealing techniques come natural to the Kaio, as such they have no established health services.@n
elseif (%arg% /= 3)
%send% %actor% @cAether - Tourist Hotspots@n
%send% %actor% @R1) @WHaven Spaceport @C- @WOpulence Street @C- @WHaven City@n
%send% %actor% @R2) @WSerenity Lake @C- @WEast of Haven City@n
%send% %actor% @R3) @WHarmony Park @C - @WHarmony Street @C - @WHaven City@n
elseif (%arg% /= 4)
%send% %actor% @cAether - Danger Zones@n
%send% %actor% @R1) @WKaiju Forest. @ROOC - Level 15+@n
%send% %actor% @R2) @WOrtusian Temple. @ROOC - Level 20+@n
%send% %actor% @R3) @WShallow Cave. @ROOC - Level 30+@n
elseif (%arg% /= 5)
%send% %actor% @cAether - Notable Martial Artists - Rough Locations@n
%send% %actor% @R1) @WKibito @C- @WHarmony Park@n
else
%send% %actor% @CPlease select an option:@n
%send% %actor% @ROption @Y1) @WPopular Food and Beverages@n
%send% %actor% @ROption @Y2) @WHealth Services@n
%send% %actor% @ROption @Y3) @WTourist Hotspots@n
%send% %actor% @ROption @Y4) @WDanger Zones@n
%send% %actor% @ROption @Y5) @WNotable Martial Artists@n
%send% %actor% @D @n
%send% %actor% @RSyntax@n: @YiMuffin <planet name> <option number>@n
end
elseif (%arg% /= Konack)
if (%arg% /= 1)
%send% %actor% @cKonack - Popular Food and Beverage services@n
%send% %actor% @R1) @WGladroon's General Store @C- @WGolden Circlet Road @C- @WTiranoc City@n
%send% %actor% @R2) @WBorwen's Teahouse @C- @WGolden Circlet Road @C- @WTiranoc City@n
elseif (%arg% /= 2)
%send% %actor% @cKonack - Health Services@n
%send% %actor% @R1) @WGreat Oroist Temple @C- @WTaloc Street @C- @WTiranoc City@n
elseif (%arg% /= 3)
%send% %actor% @cKonack - Tourist Hotspots@n
%send% %actor% @R1) @WKonack Starport @C- @WWest of Pontius Gate @C- @WTiranoc Outskirts@n
%send% %actor% @R2) @WMeridius Memorial @C- @WNorth of Meridius Gate@n @C - @WTiranoc Outskirts@n
%send% %actor% @R3) @WTurlon Fair @C - @WTaesal district @C - @WSE of Tiranoc@n
elseif (%arg% /= 4)
%send% %actor% @cKonack - Danger Zones@n
%send% %actor% @R1) @WElzthuan Forest. @ROOC - Level 20+@n
%send% %actor% @R2) @WSt Alucia. @ROOC - Level 30+@n
%send% %actor% @R3) @WOld Kerberos. @ROOC - Level 40+@n
elseif (%arg% /= 5)
%send% %actor% @cKonack - Notable Martial Artists - Rough Locations@n
%send% %actor% @R1) @WTapion @C- @WTiranoc City@n
else
%send% %actor% @CPlease select an option:@n
%send% %actor% @ROption @Y1) @WPopular Food and Beverages@n
%send% %actor% @ROption @Y2) @WHealth Services@n
%send% %actor% @ROption @Y3) @WTourist Hotspots@n
%send% %actor% @ROption @Y4) @WDanger Zones@n
%send% %actor% @ROption @Y5) @WNotable Martial Artists@n
%send% %actor% @D @n
%send% %actor% @RSyntax@n: @YiMuffin <planet name> <option number>@n
end
elseif (%arg% /= Frigid)
if (%arg% /= 1)
%send% %actor% @cFrigid- Popular Food and Beverage services@n
%send% %actor% @R1) @WFrigid Fried Rat @C- @WMiddle Level @C- @WIce Crown City@n
%send% %actor% @R2) @WCoffee Hut @C- @WMiddle Level @C- @WIce Crown City@n
%send% %actor% @R3) @WWarlord and Wenches Pub @C - @WLower Level @C- @WIce Crown City@n
elseif (%arg% /= 2)
%send% %actor% @cFrigid - Health Services@n
%send% %actor% @R1) @WCicle Hotel @C- @WMiddle Level @C- @WIce Crown City@n
%send% %actor% @R2) @WJerodel's Surgery @C- @WLower Level @C- @WIce Crown City@n
elseif (%arg% /= 3)
%send% %actor% @cFrigid - Tourist Hotspots@n
%send% %actor% @R1) @WIce Crown Starport @C- @WTop Level @C- @WIce Crown City@n
%send% %actor% @R2) @WRoyal Palace @C- @WSurface Side@n @C - @WIce Crown City@n
%send% %actor% @R3) @WTravel Advisory Warning @C - @WFrigid is not a suitable tourist destination!@n
elseif (%arg% /= 4)
%send% %actor% @cFrigid - Danger Zones@n
%send% %actor% @R1) @WAbandoned Level of Ice Crown City @ROOC - Level 20+@n
%send% %actor% @R2) @WGlug's Volcano. @ROOC - Level 35+@n
%send% %actor% @R3) @WWrym Spine Mountain. @ROOC - Level 50+@n
elseif (%arg% /= 5)
%send% %actor% @cFrigid - Notable Martial Artists - Rough Locations@n
%send% %actor% @R1) @WFrieza @C- @WIce Crown City@n
%send% %actor% @R2) @WCaptain Ginyu @C- @WIce Crown City@n
else
%send% %actor% @CPlease select an option:@n
%send% %actor% @ROption @Y1) @WPopular Food and Beverages@n
%send% %actor% @ROption @Y2) @WHealth Services@n
%send% %actor% @ROption @Y3) @WTourist Hotspots@n
%send% %actor% @ROption @Y4) @WDanger Zones@n
%send% %actor% @ROption @Y5) @WNotable Martial Artists@n
%send% %actor% @D @n
%send% %actor% @RSyntax@n: @YiMuffin <planet name> <option number>@n
end
elseif (%arg% /= Arlia)
if (%actor.race% != Arlian)
%send% %actor% @WThe Planet Arlia is a desolate, barren backwater home to the bug species known as Arlians. There is little of noteworthy mention here.@n
else
if (%arg% /= 1)
%send% %actor% @cArlia - Popular Food and Beverage services@n
%send% %actor% @R1) @WEndoskelefoods @C- @WCatachan Road @C- @WJanacre@n
elseif (%arg% /= 2)
%send% %actor% @cArlia - Health Services@n
%send% %actor% @R1) @WNone@n
elseif (%arg% /= 3)
%send% %actor% @cArlia - Tourist Hotspots@n
%send% %actor% @R1) @WJanacre Starport @C- @WStarport Drive @C- @WJanacre@n
%send% %actor% @R2) @WStarport is freedomzzzzz.@n
elseif (%arg% /= 4)
%send% %actor% @cArlia - Danger Zones@n
%send% %actor% @R1) @WYatamari Wasteland @ROOC - Level 10@n
%send% %actor% @R2) @WArlia Mine. @ROOC - Level 20+@n
elseif (%arg% /= 5)
%send% %actor% @cArlia - Notable Martial Artists - Rough Locations@n
%send% %actor% @R1) @WKurzak @C- @WJanacre@n
else
%send% %actor% @CPlease select an option:@n
%send% %actor% @ROption @Y1) @WPopular Food and Beverages@n
%send% %actor% @ROption @Y2) @WHealth Services@n
%send% %actor% @ROption @Y3) @WTourist Hotspots@n
%send% %actor% @ROption @Y4) @WDanger Zones@n
%send% %actor% @ROption @Y5) @WNotable Martial Artists@n
%send% %actor% @D @n
%send% %actor% @RSyntax@n: @YiMuffin <planet name> <option number>@n
end
end
elseif (%arg% /= Zenith)
if (%actor.race% != Hoshijin)
%send% %actor% @WThis lush, forest moon is located near the planet Arlia, but otherwise is lacking in noteworthiness.@n
else
if (%arg% /= 1)
%send% %actor% @cZenith - Popular Food and Beverage services@n
%send% %actor% @R1) @WBizarre Bazaar @C- @WMayapo Complex @C- @WUtatlan@n
elseif (%arg% /= 2)
%send% %actor% @cZenith - Health Services@n
%send% %actor% @R1) @WNone@n
elseif (%arg% /= 3)
%send% %actor% @cZenith - Tourist Hotspots@n
%send% %actor% @R1) @WKukul Library @C- @WUtatlan@n
%send% %actor% @R2) @WJaguar Dojo @C- @WUtatlan@n
elseif (%arg% /= 4)
%send% %actor% @cZenith - Danger Zones@n
%send% %actor% @R1) @WZenith Jungle @ROOC - Levels 8-15@n
elseif (%arg% /= 5)
%send% %actor% @cZenith - Notable Martial Artists - Rough Locations@n
%send% %actor% @R1) @WJinto @C- @WUtatlan@n
else
%send% %actor% @CPlease select an option:@n
%send% %actor% @ROption @Y1) @WPopular Food and Beverages@n
%send% %actor% @ROption @Y2) @WHealth Services@n
%send% %actor% @ROption @Y3) @WTourist Hotspots@n
%send% %actor% @ROption @Y4) @WDanger Zones@n
%send% %actor% @ROption @Y5) @WNotable Martial Artists@n
%send% %actor% @D @n
%send% %actor% @RSyntax@n: @YiMuffin <planet name> <option number>@n
end
end
elseif (%arg% /= Kanassa)
if (%actor.race% != Kanassan)
%send% %actor% @WKanassa was once the homeworld of the Kanassans, but the World Trade Organization rendered the planet inhospitable.@n
else
if (%arg% /= 1)
%send% %actor% @cKanassa - Popular Food and Beverage services@n
%send% %actor% @R1) @WOcean's Bounty @C- @WUndine Street @C- @WAquis City@n
%send% %actor% @R2) @WWhale Tears Tavern @C- @WHydro Way @C- @WAquis City@n
elseif (%arg% /= 2)
%send% %actor% @cKanassa - Health Services@n
%send% %actor% @R1) @WNone@n
elseif (%arg% /= 3)
%send% %actor% @cKanassa - Tourist Hotspots@n
%send% %actor% @R1) @WLanding Pad @C- @WCentral Square @C- @WAquis City@n
%send% %actor% @R2) @WAquis Temple @C- @WSylph Street @C- @WAquis City@n
elseif (%arg% /= 4)
%send% %actor% @cKanassa - Danger Zones@n
%send% %actor% @R1) @WTambus Ocean @ROOC - Levels 8-30. Beware the Griknaug@n
%send% %actor% @R2) @WYunkai Pirate Base @ROOC - Avoid!@n
elseif (%arg% /= 5)
%send% %actor% @cKanassa - Notable Martial Artists - Rough Locations@n
%send% %actor% @R1) @WTsuna @C- @WAquis City@n
else
%send% %actor% @CPlease select an option:@n
%send% %actor% @ROption @Y1) @WPopular Food and Beverages@n
%send% %actor% @ROption @Y2) @WHealth Services@n
%send% %actor% @ROption @Y3) @WTourist Hotspots@n
%send% %actor% @ROption @Y4) @WDanger Zones@n
%send% %actor% @ROption @Y5) @WNotable Martial Artists@n
%send% %actor% @D @n
%send% %actor% @RSyntax@n: @YiMuffin <planet name> <option number>@n
end
end
else
%echoaround% %actor% %actor.name% taps their iMuffin screen a few times.
%send% %actor% @CPlease select an option:@n
%send% %actor% @ROption @Y1) @WEarth@n
%send% %actor% @ROption @Y2) @WVegeta@n
%send% %actor% @ROption @Y3) @WNamek@n
%send% %actor% @ROption @Y4) @WAether@n
%send% %actor% @ROption @Y5) @WKonack@n
%send% %actor% @ROption @Y6) @WFrigid@n
%send% %actor% @ROption @Y7) @WArlia@n
%send% %actor% @ROption @Y8) @WZenith@n
%send% %actor% @ROption @Y9) @WKanassa@n
%send% %actor% @D @n
%send% %actor% @RSyntax@n: @YiMuffin <planet name>@n
end
~
$~
