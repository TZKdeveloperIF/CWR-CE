// test for "LOAD MAGAZINE" action

triSimUntil { time >= 1 }

// move unit into tank. "LOAD MAGAZINE" action requires a gunner to be inside the vehicle
s1 moveInGunner m1;
triSimFrames 40

// initlal "sabot" magazine verification
if ("heat120" != (m1 ammoArray "gun120") select 0) exitWith { "FAIL:abnormal initial magazine" };

s1 action ["LOAD MAGAZINE", m1, 0, 0, "gun120|shell120"]

// verify whether "heat" magazine is reloaded
triSimFrames 40
if ("shell120" != (m1 ammoArray "gun120") select 0) exitWith { "FAIL:'LOAD MAGAZINE' action failed to work" };
triEndTest
