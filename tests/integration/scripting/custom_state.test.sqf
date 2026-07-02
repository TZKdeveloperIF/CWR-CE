// test enableAi

triSimUntil { time >= 1 }

// verify initial state
_state0 = player getCustomState 1;
if (_state0) exitWith { "FAIL: initial state of custom state 1 should be false" };

// set state to true
player setCustomState [1, true];

// verify state is true
_state1 = player getCustomState 1;
if (!_state1) exitWith { "FAIL: state of custom state 1 should be true" };

// verify another state bit is still false
_state2 = player getCustomState 2;
if (_state2) exitWith { "FAIL: state of custom state 2 should be false" };

// set state to false
player setCustomState [1, false];

// verify state is false
_state1 = player getCustomState 1;
if (!_state1) exitWith { "FAIL: state of custom state 1 should be false" };

// check max bit limit (31)
player setCustomState [31, true];
_state1 = player getCustomState 31;
if (!_state1) exitWith { "FAIL: state of custom state 31 should be true" };

// check invalid index (32 and -1)
player setCustomState [32, true];
_state1 = player getCustomState 32;
if (_state1) exitWith { "FAIL: state of custom state 32 should be false" };

player setCustomState [-1, true];
_state1 = player getCustomState -1;
if (_state1) exitWith { "FAIL: state of custom state -1 should be false" };


triEndTest
