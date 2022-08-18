use <lib.scad>

bounds = [1,1,1];
body = [0.9, 0.9, 0.9];

module blade() {
	translate([0.1,0,0.03]) rotate([30,0,0]) scale([3,1,1]) cyl(0.03, 0.03, 0.01);
}

module rotorHD($fn=8) {
	translate([0,0,0.01]) union() {
		cyl(0.03, 0.03, 0.1);
		blade();
		rotate([0,0,180]) blade();
	}
}

module rotorLD($fn=4) {
	translate([0,0,0.01]) union() {
		cyl(0.03, 0.03, 0.1);
		blade();
		rotate([0,0,180]) blade();
	}
}

module spar() {
	hull() {
		translate([0,0,0]) box([0.2, 0.2, 0.02]);
		translate([0,0.5,0]) box([0.02, 0.02, 0.02]);
	}
}

module spars() {
	for (i = [0:90:360]) rotate([0,0,i+45]) spar();
}

module chassisHD() {
	union() {
		difference() {
			sphere(r=0.2, $fn=24);
			translate([0,0,-0.35]) box([0.45,0.45,0.45]);
			translate([0,0,0.35]) box([0.45,0.45,0.45]);
		}
		rotate([90,0,0]) translate([0,0,0.05]) cyl(0.075, 0.075, 0.4,$fn=16);
	}
}

module chassisLD() {
	union() {
		difference() {
			sphere(r=0.2, $fn=12);
			translate([0,0,-0.35]) box([0.45,0.45,0.45]);
			translate([0,0,0.35]) box([0.45,0.45,0.45]);
		}
		rotate([90,0,0]) translate([0,0,0.05]) cyl(0.075, 0.075, 0.4,$fn=8);
	}
}

d = 4;

//chassisHD();
//chassisLD();
spars();
//rotorHD();
//rotorLD();
