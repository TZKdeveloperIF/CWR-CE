// test enableAi and checkAIFeature

triSimUntil { time >= 1 }

// create exist marker will fail
_existedMarker = "conflictName";
createMarker [_existedMarker, [0, 0, 0]]; // checked in the end

// verify first marker
if (getMarkerColor _existedMarker != "ColorRed") exitWith { "FAIL: first marker is red" };

// verify second marker
deleteMarker _existedMarker;
if (getMarkerColor _existedMarker != "ColorBlue") exitWith { "FAIL: second marker is blue" };

// verify third marker
deleteMarker _existedMarker;
if (getMarkerColor _existedMarker != "ColorGreen") exitWith { "FAIL: third marker is green" };

// check no marker left
if (getMarkerColor _existedMarker != "") exitWith { "FAIL: expected no more marker left after having remoted existed 3 markers defined by mission.sqm" };

triEndTest
