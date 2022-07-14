use <lib.scad>

module frame() {
	difference() {
		box([0.95,0.95,2.0]);
		translate([0,0,-0.49]) box([0.85,1,0.925]);
		translate([0,0,-0.49]) box([1,0.85,0.925]);
		translate([0,0, 0.49]) box([0.85,1,0.925]);
		translate([0,0, 0.49]) box([1,0.85,0.925]);
	}
}

module gaps() {
	for (i = [-2:1:2])
		for (j = [-2:1:2])
			translate([i*0.175,j*0.175,0.4]) box([0.15,0.15,1.5]);
}

module frameLD() {
	difference() {
		frame();
		gaps();
	}
}

module frameHD() {
	difference() {
		minkowskiOutsideRound(0.01,$fn=8) frame();
		gaps();
	}
}

frameLD();
