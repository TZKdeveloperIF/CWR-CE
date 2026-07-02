// test enableAi

triSimUntil { time >= 1 }

// record s1 current position
_pos0 = getPosASL s1;
_obj0 = "EmptyDetector" camCreate _pos0;
_dest = +_pos0;
_dest set [0, (_dest select 0) + 10]; // move 10

// disableAI "MOVE"
s1 disableAI "MOVE";
s1 doMove _dest;
triSimFrames 40

// verify s1 has not moved
_pos1 = getPosASL s1;
_obj1 = "EmptyDetector" camCreate _pos1;
if (_obj1 distance _obj0 > 0.1) exitWith { "FAIL:'disableAI MOVE' failed to prevent movement" };

// enableAI "MOVE"
s1 enableAI "MOVE";
s1 doMove _dest;
triSimFrames 40

// verify s1 has moved
_pos2 = getPosASL s1;
_obj2 = "EmptyDetector" camCreate _pos2;
if (_obj2 distance _obj0 <= 0.1) exitWith { "FAIL:'enableAI MOVE' failed to enable movement" };

triEndTest
