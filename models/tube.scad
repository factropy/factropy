use <lib.scad>

hd = 36;
ld = 16;
vld = 8;

module cowling() {
	difference() {
		hull() {
			cyl(0.5,0.5,0.5);
			translate([0,0,0.5]) torus(0.1,0.25);
			translate([0,0,-0.5]) torus(0.1,0.25);
		}
		cyl(0.3,0.3,2.0);
	}
}

module cowlingHD() {
	cowling($fn=hd);
}

module cowlingLD() {
	cowling($fn=ld);
}

module fan(degrees) {
	translate([0,0,0.45]) union() {
		for (i = [0:degrees:360]) {
			rotate([0,0,i]) translate([0,0.15,0]) rotate([0,20,0]) box([0.01,0.3,0.1]);
		}
		hull() {
			translate([0,0,0.05]) sphere(0.1);
			translate([0,0,-0.7]) cyl(0.1,0.1,0.1);
		}
	}
}

module fanHD() {
	fan(10, $fn=hd/2);
}

module fanLD() {
	fan(30, $fn=ld/2);
}

module frame(h) {
	difference() {
		translate([0,0,(h-0.5)/2]) box([0.99, 0.99, h-0.5]);
		translate([0,0,1.1]) box([2, 0.85, 1.8]);
		translate([0,0,1.1]) box([0.85, 2, 1.8]);

		translate([0,0,(h-2.9)/2+2.2]) box([2, 0.85, h-2.9]);
		translate([0,0,(h-2.9)/2+2.2]) box([0.85, 2, h-2.9]);
		translate([0,0,(h-2)/2+1.8]) cyl(0.4,0.4,h-2,$fn=32);
	}
}

module frameHD(h) {
	minkowskiOutsideRound(0.01,$fn=8)
	frame(h, $fn=hd);
}

module frameLD() {
	frame(h, $fn=ld);
}

module frameVLD(h) {
	translate([0,0,(h+0.5)/2]) box([0.99,0.99,h+0.5]);
}

module frameAlt(h) {
	difference() {
		translate([0,0,(h-0.5)/2]) box([0.99, 0.99, h-0.5]);
	}
}

module frameAltHD(h) {
	minkowskiOutsideRound(0.01,$fn=8)
	frameAlt(h, $fn=hd);
}

module frameAltLD(h) {
	frameAlt(h, $fn=ld);
}

module frameAltVLD(h) {
	frameAlt(h, $fn=vld);
}

h=3;
//translate([0,0,h]) cowlingHD();
//translate([0,0,h]) fanHD();
//frameHD(h);
//frameAltHD(7);

//cowlingHD();
//fanHD();
//frameHD(h);
//frameAltLD(7);

//cowlingLD();
//fanLD();
//frameLD();

//frameVLD(7);
frameAltVLD(7);

module conduitGlassHD() {
	rotate([90,90,0]) difference() {
		cyl(0.4,0.4,1.0,$fn=hd);
		cyl(0.35,0.35,1.1,$fn=hd);
	}
}

module conduitGlassLD() {
	rotate([90,90,0]) difference() {
		cyl(0.4,0.4,1.0,$fn=ld);
		cyl(0.35,0.35,1.1,$fn=ld);
	}
}

//conduitGlassLD();

module conduitRingHD() {
	rotate([90,90,0]) difference() {
		cyl(0.425,0.425,0.1,$fn=16);
		cyl(0.4,0.4,1.1,$fn=16);
	}
}

module conduitRingLD() {
	rotate([90,90,0]) difference() {
		cyl(0.425,0.425,0.1,$fn=8);
		cyl(0.4,0.4,1.1,$fn=8);
	}
}

//conduitRingLD();
