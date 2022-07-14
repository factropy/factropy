use <lib.scad>

hd=72;
ld=8;

module pipe() {
	translate([0,0,-2.5]) cyl(0.5, 0.5, 5);
}

module wheelHD($fn=hd) {
	rotate([0,90,0]) difference() {
		union() {
			difference() {
				cyl(0.8,0.8,0.5);
				cyl(0.7,0.7,0.6);
			}
			rotate([0,0,0]) translate([0.4,0,0]) box([0.75,0.1,0.2]);
			rotate([0,0,120]) translate([0.4,0,0]) box([0.75,0.1,0.2]);
			rotate([0,0,240]) translate([0.4,0,0]) box([0.75,0.1,0.2]);
			cyl(0.1,0.1,0.5,$fn=$fn/4);
		}
		difference() {
			cyl(0.9,0.9,0.4);
			cyl(0.775,0.775,0.5);
		}
	}
}

module wheelLD($fn=ld) {
	rotate([0,90,0]) difference() {
		union() {
			difference() {
				cyl(0.8,0.8,0.5);
				cyl(0.7,0.7,0.6);
			}
			rotate([0,0,0]) translate([0.4,0,0]) box([0.75,0.1,0.2]);
			rotate([0,0,120]) translate([0.4,0,0]) box([0.75,0.1,0.2]);
			rotate([0,0,240]) translate([0.4,0,0]) box([0.75,0.1,0.2]);
			cyl(0.1,0.1,0.5);
		}
		difference() {
			cyl(0.9,0.9,0.4);
			cyl(0.775,0.775,0.5);
		}
	}
}

module thingyHD($fn=hd) {
	union() {
		hull() {
			translate([0,0,0]) cyl(0.35,0.35,1);
			translate([0,0,-0.5]) sphere(0.35);
			translate([0,0,0.5]) sphere(0.35);
		}
		translate([0,0,0.5]) torus(0.025, 0.32);
		translate([0,0,-0.5]) torus(0.025, 0.32);
		translate([0,0,0.65]) cyl(0.1,0.1,0.5);
		translate([0,0,0.7]) cyl(0.08,0.08,0.5,$fn=6);
	}
}

module thingyLD($fn=ld) {
	union() {
		hull() {
			translate([0,0,0]) cyl(0.35,0.35,1);
			translate([0,0,-0.5]) sphere(0.35);
			translate([0,0,0.5]) sphere(0.35);
		}
		translate([0,0,0.65]) cyl(0.1,0.1,0.5);
	}
}

module motorHD($fn=hd) {
	union() {
		hull() {
			rotate([0,90,0]) cyl(0.3,0.3,0.6);
			translate([0.4,0,0]) rotate([0,90,0]) torus(0.01, 0.2);
		}
		translate([-0.3,0,0]) rotate([0,90,0]) hull() {
			torus(0.1,0.125);
			translate([0,0,0.1]) cyl(0.325,0.325,0.1);
		}
		for (i = [0:10:360])
			rotate([i,0,0]) box([0.64,0.01,0.64]);
	}
}

module motorLD($fn=ld) {
	union() {
		hull() {
			rotate([0,90,0]) cyl(0.325,0.326,0.6);
			translate([0.4,0,0]) rotate([0,90,0]) cyl(0.2, 0.2, 0.05);
			translate([-0.4,0,0]) rotate([0,90,0]) cyl(0.2, 0.2, 0.05);
		}
	}
}

module motorShaftHD($fn=hd/2) {
	union() {
		translate([0.25,0,0]) rotate([0,90,0]) cyl(0.05,0.05,1.3);
		translate([0.7,0,0]) rotate([0,90,0]) difference() {
			cyl(0.1,0.1,0.45);
			difference() {
				cyl(0.15,0.15,0.4);
				cyl(0.075,0.075,0.5);
			}
			translate([0,0,0.225]) union() {
				box([0.1,0.025,0.075]);
				rotate([0,0,90]) box([0.1,0.025,0.075]);
			}
		}
	}
}

module motorShaftLD($fn=ld) {
	union() {
		translate([0.25,0,0]) rotate([0,90,0]) cyl(0.05,0.05,1.3);
		translate([0.7,0,0]) rotate([0,90,0]) difference() {
			cyl(0.1,0.1,0.45);
			difference() {
				cyl(0.15,0.15,0.4);
				cyl(0.075,0.075,0.5);
			}
		}
	}
}

module beltHD($fn=hd) {
	difference() {
		hull() {
			translate([0,0.75,0]) rotate([0,90,0]) difference() {
				cyl(0.1,0.1,0.38);
				cyl(0.08,0.08,0.5);
			}
			translate([0,-0.5,0.5]) rotate([0,90,0]) difference() {
				cyl(0.8,0.8,0.38);
				cyl(0.08,0.08,0.5);
			}
		}
		scale([1.1,1,1]) hull() {
			translate([0,0.75,0]) rotate([0,90,0]) cyl(0.08,0.08,0.4);
			translate([0,-0.5,0.5]) rotate([0,90,0]) cyl(0.78,0.78,0.4);
		}
	}
}

module beltLD($fn=ld) {
	difference() {
		hull() {
			translate([0,0.75,0]) rotate([0,90,0]) difference() {
				cyl(0.1,0.1,0.38);
				cyl(0.08,0.08,0.5);
			}
			translate([0,-0.5,0.5]) rotate([0,90,0]) difference() {
				cyl(0.8,0.8,0.38);
				cyl(0.08,0.08,0.5);
			}
		}
		scale([1.1,1,1]) hull() {
			translate([0,0.75,0]) rotate([0,90,0]) cyl(0.08,0.08,0.4);
			translate([0,-0.5,0.5]) rotate([0,90,0]) cyl(0.78,0.78,0.4);
		}
	}
}

module pump() {
//	translate([0,0,1.5]) %box([3,3,3]);
//	translate([0,0,0.5]) rbox([3,3,1],0.01,$fn=8);
//	translate([0,0,0.5]) box([3,3,1]);
//	translate([-1,0.5,2]) thingy();
//	translate([-1,-0.5,2]) thingy();
//	translate([1,-0.5,2]) wheel();
//	translate([0.3,0.75,1.5]) motor();
//	translate([0.3,0.75,1.5]) motorShaft();
//	translate([1,0,1.5]) belt();
}


//pump($fn=hd);
beltLD();
