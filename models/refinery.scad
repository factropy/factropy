use <lib.scad>

hd = 72;
ld = 8;

module baseHD() {
	translate([0,0,0.5], $fn=hd) union() {
		rbox([5.8,5.8,1], 0.05, $fn=8);
		translate([0,-1.55,1.4]) difference() {
			rbox([5.2,2,2], 0.025, $fn=8);
			translate([-2.6,0,0]) rotate([0,90,0]) cyl(0.75,0.75,1);
			translate([2.6,0,0]) rotate([0,90,0]) cyl(0.75,0.75,1);
		}
	}
}

module baseLD() {
	translate([0,0,0.5], $fn=ld) union() {
		box([5.8,5.8,1]);
		translate([0,-1.55,1.4]) difference() {
			box([5.2,2,2]);
			translate([-2.6,0,0]) rotate([0,90,0]) cyl(0.75,0.75,1);
			translate([2.6,0,0]) rotate([0,90,0]) cyl(0.75,0.75,1);
		}
	}
}

module baseVLD() {
	translate([0,0,0.5], $fn=ld) union() {
		box([5.8,5.8,1]);
		translate([0,-1.55,1.4]) difference() {
			box([5.2,2,2]);
		}
	}
}

module stackHD() {
	translate([1.6,1.6,4], $fn=hd) union() {
		difference() {
			cyl(0.5, 0.5, 8.0);
			cyl(0.3, 0.3, 8.1);
		}
		translate([0,0,-2.5]) hull() {
			cyl(1.0, 1.0, 3.0);
			translate([0,0,3]) cyl(0.5, 0.5, 0.8);
		}
		for (i = [1:1:4]) translate([0,0,i-0.1]) difference() {
			cyl(0.6, 0.6, 0.1);
			cyl(0.5, 0.5, 0.15);
		}
	}
}

module stackLD() {
	translate([1.6,1.6,4], $fn=ld) union() {
		difference() {
			cyl(0.5, 0.5, 8.0);
			cyl(0.3, 0.3, 8.1);
		}
		translate([0,0,-2.5]) hull() {
			cyl(1.0, 1.0, 3.0);
			translate([0,0,3]) cyl(0.5, 0.5, 0.8);
		}
	}
}

module stackVLD() {
	hull() stackLD();
}

module tankHD() {
	translate([-1.1,1.1,3], $fn=hd) {
		sphere(1.5);
		for (i = [0:45:360]) rotate([0,0,i]) translate([1.4,0,-1]) cyl(0.1,0.1,2);
		for (i = [-1:-0.5:-1.5]) translate([0,0,i]) difference() {
			cyl(1.5,1.5,0.1);
			cyl(1.3,1.3,0.2);
		}
	}
}

module tankLD() {
	translate([-1.1,1.1,3], $fn=ld) {
		sphere(1.5);
		for (i = [0:45:360]) rotate([0,0,i]) translate([1.4,0,-1]) cyl(0.1,0.1,2);
		for (i = [-1:-0.5:-1.5]) translate([0,0,i]) difference() {
			cyl(1.5,1.5,0.1);
			cyl(1.3,1.3,0.2);
		}
	}
}

module tankVLD() {
	translate([-1.1,1.1,3], $fn=ld) {
		sphere(1.5);
	}
}

//baseVLD();
//stackVLD();
tankVLD();