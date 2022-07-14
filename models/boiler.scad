use <lib.scad>

module rivetHD($fn=16) {
	union() {
		translate([0,0,0.005]) sphere(0.019);
		translate([0,0,-0.015]) cyl(0.025, 0.025, 0.05);
	}
}

module flangeHD() {
	rotate([0,90,0]) difference() {
		cyl(0.85,0.85,0.1);
		difference() {
			cyl(0.9,0.9,0.01);
			cyl(0.82,0.82,0.05);
		}
	}
}

module flangeLD() {
	rotate([0,90,0]) difference() {
		cyl(0.85,0.85,0.1);
	}
}

module fireboxHD($fn=64) {
	difference() {
		union() {
			hull() {
				translate([1.1,0,0]) rotate([0,90,0]) rotate_extrude(convexity=10) translate([0.6,0,0]) circle(r=0.2);
				translate([-1.1,0,0]) rotate([0,90,0]) rotate_extrude(convexity=10) translate([0.6,0,0]) circle(r=0.2);
			}
			translate([1.1,0,0]) flangeHD();
			translate([-1.1,0,0]) flangeHD();
		}
		translate([0,0,-0.5]) box([2.9,1.9,1.0]);
	}
}

module fireboxLD($fn=32) {
	difference() {
		union() {
			hull() {
				translate([1.1,0,0]) rotate([0,90,0]) rotate_extrude(convexity=10) translate([0.6,0,0]) circle(r=0.2);
				translate([-1.1,0,0]) rotate([0,90,0]) rotate_extrude(convexity=10) translate([0.6,0,0]) circle(r=0.2);
			}
			translate([1.1,0,0]) flangeLD();
			translate([-1.1,0,0]) flangeLD();
		}
		translate([0,0,-0.5]) box([2.9,1.9,1.0]);
	}
}

module fireboxVLD($fn=32) {
	difference() {
		rotate([0,90,0]) cyl(0.85, 0.85, 2.6, $fn=8);
		translate([0,0,-0.5]) box([2.9,1.9,1.0]);
	}
}

module stackHD($fn=32) {
	translate([0.6,0,0.5]) intersection() {
		translate([0,0,-0.5]) union() {
			difference() {
				cyl(0.3,0.3,2.0);
				cyl(0.25,0.25,2.1);
			}
			translate([0,0,0.1]) intersection() {
				rotate([0,90,0]) cyl(0.75,0.75,1.0);
				cyl(0.44,0.44,10);
			}
		}
		box([1,1,1]);
	}
}

module stackLD($fn=16) {
	translate([0.6,0,0.5]) intersection() {
		translate([0,0,-0.5]) union() {
			difference() {
				cyl(0.3,0.3,2.0);
				cyl(0.25,0.25,2.1);
			}
			translate([0,0,0.1]) intersection() {
				rotate([0,90,0]) cyl(0.75,0.75,1.0);
				cyl(0.44,0.44,10);
			}
		}
		box([1,1,1]);
	}
}

module coverHD($fn=32) {
	translate([0.6,0,0.81]) cyl(0.25,0.25,0.1);
}

module coverLD($fn=16) {
	translate([0.6,0,0.81]) cyl(0.25,0.25,0.1);
}

module chassisHD() {
	translate([0,0,-0.5]) rbox([2.9,1.9,1.0], 0.025, $fn=8);
}

module chassisLD() {
	translate([0,0,-0.5]) box([2.9,1.9,1.0]);
}

module chassisVLD() {
	translate([0,0,-0.5]) box([2.9,1.9,1.0]);
}

//chassisHD();
//chassisLD();
//chassisVLD();
//fireboxHD();
//fireboxLD();
//fireboxVLD();
//stackHD();
//stackLD();
//coverHD();
//coverLD();
rivetHD();

//for (i=[0:1:11]) translate([1.3,0,0]) rotate([i*30,0,0]) translate([0,0,0.55]) rotate([0,90,0]) rivet();
//for (i=[0:1:11]) translate([-1.3,0,0]) rotate([i*30,0,0]) translate([0,0,0.55]) rotate([0,-90,0]) rivet();
//for (i=[0:1:17]) translate([0.9,0,0]) rotate([i*20,0,0]) translate([0,0,0.8]) rivet();
//for (i=[0:1:17]) translate([-0.9,0,0]) rotate([i*20,0,0]) translate([0,0,0.8]) rivet();
//for (i=[0:1:17]) translate([0.3,0,0]) rotate([i*20,0,0]) translate([0,0,0.8]) rivet();
//for (i=[0:1:17]) translate([-0.3,0,0]) rotate([i*20,0,0]) translate([0,0,0.8]) rivet();
