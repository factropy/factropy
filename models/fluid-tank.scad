use <lib.scad>

hd=64;
ld=32;
vld=8;

module pipe() {
	rotate([180/$fn,0,0]) rotate([0,90,0]) cyl(0.4, 0.4, 1.0);
}

module meterHD() {
	translate([0,0,1.25]) difference() {
		rbox([0.2,0.5,2],0.01, $fn=6);
		translate([0.1,0,0]) box([0.2,0.4,1.9]);
	}
}

module meterLD() {
	translate([0,0,1.25]) difference() {
		box([0.2,0.5,2]);
		translate([0.1,0,0]) box([0.2,0.4,1.9]);
	}
}

module tankHD($fn=hd) {
	union() {
		difference() {
			translate([0,0,-2]) intersection() {
				translate([0,0,0.25]) sphere(r=5);
				cyl(2.45,2.45,10);
				translate([0,0,3.5]) box([5,5,3]);
			}
			translate([0,0,-1.8]) scale([0.95,0.95,0.95]) intersection() {
				translate([0,0,0.25]) sphere(r=5);
				cyl(2.45,2.45,10);
				translate([0,0,3.5]) box([5,5,3]);
			}
		}
//		translate([2,0,0.5]) rotate([0,0,0]) pipe();
//		translate([0,2,0.5]) rotate([0,0,90]) pipe();
//		translate([-2,0,0.5]) rotate([0,0,0]) pipe();
//		translate([0,-2,0.5]) rotate([0,0,90]) pipe();
	}
}

module tankLD($fn=ld) {
	union() {
		difference() {
			translate([0,0,-2]) intersection() {
				translate([0,0,0.25]) sphere(r=5);
				cyl(2.45,2.45,10);
				translate([0,0,3.5]) box([5,5,3]);
			}
			translate([0,0,-1.8]) scale([0.95,0.95,0.95]) intersection() {
				translate([0,0,0.25]) sphere(r=5);
				cyl(2.45,2.45,10);
				translate([0,0,3.5]) box([5,5,3]);
			}
		}
	}
}

module tankVLD($fn=vld) {
	difference() {
		translate([0,0,-2]) intersection() {
			translate([0,0,0.25]) sphere(r=5);
			cyl(2.5,2.5,10);
			translate([0,0,3.5]) box([5,5,3]);
		}
		translate([0,0,-1.975]) scale([0.99,0.99,0.99]) intersection() {
			translate([0,0,0.25]) sphere(r=5);
			cyl(2.5,2.5,10);
			translate([0,0,3.5]) box([5,5,3]);
		}
	}
}

module tankFluid($fn=ld) {
	#translate([0,0,1.27]) cyl(2.45,2.45,2.5);
}

module tank2() {
	translate([0,0,-2]) union() {
		intersection() {
			translate([0,0,-7]) sphere(r=12);
			cyl(4,4,10);
			translate([0,0,3.5]) box([8,8,3]);
		}
		if ($fn >= ld) for (i = [0:1:2])
			translate([0,0,3.95-(i*0.8)]) difference() {
				cyl(4.03,4.03,0.1);
				cyl(3.99,3.99,0.2);
			}
	}
}


//tank2($fn=12);
//tankHD();
//meterHD();
//tankLD();
meterLD();
//tankFluid();
