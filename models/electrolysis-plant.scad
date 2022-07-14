use <lib.scad>

hd = 64;
ld = 16;


module flangeHD() {
	cyl(0.25,0.25,0.1);
}

module epipe() {
	points = [
		[2.6,0,-1],
		[2.6,0,1.3],
		[2.53,0,1.43],
		[2.4,0,1.5],
		[-2.4,0,1.5],
		[-2.53,0,1.43],
		[-2.6,0,1.3],
		[-2.6,0,-1],
	];
	rotate([0,0,90]) difference() {
		union() {
			pipe(points, 0.2);
			translate([-2.6,0,1.2]) flangeHD();
			translate([-2.3,0,1.5]) rotate([0,90,0]) flangeHD();
			translate([2.6,0,1.2]) flangeHD();
			translate([2.3,0,1.5]) rotate([0,90,0]) flangeHD();
		}
		translate([0,0,-0.8]) box([6,6,1]);
	}
}

module pipeHD($fn=32) {
	epipe();
}

module pipeLD($fn=12) {
	epipe();
}

module glassHD() {
	translate([0,0,0.4]) box([4.3,0.3,1.4]);
}

module frameLD() {
	translate([0,0,0.4]) difference() {
		box([4.5,0.25,1.6]);
		box([4.3,0.3,1.4]);
	}
}

module frameHD() {
	minkowskiOutsideRound(0.01,$fn=8)
	frameLD();
}

module baseLD($fn=ld) {
	 difference() {
		hull() {
			translate([0,0,-1]) box([5.95, 5.95, 1]);
			translate([0,0,0]) box([5, 5, 0.1]);
		}
		translate([0,0,0.1]) box([4.75, 4.75, 1]);
	}
}

module baseHD($fn=hd) {
	minkowskiOutsideRound(0.01,$fn=8)
	baseLD();
}

module baseVLD($fn=ld) {
	union() {
		hull() baseLD();
		hull() for (i = [-2:1:2])
			translate([0,-i,0]) frameHD();
	}
}

module entity() {
	baseHD();
	for (i = [-2:1:2])
		translate([0,-i,0]) frameHD();
	for (i = [-2:1:2])
		translate([0,-i,0]) glassHD();
	translate([-1,0,0]) pipeHD();
	translate([1,0,0]) pipeHD();
}

//entity();

//baseHD();
//baseLD();
//baseVLD();

//frameHD();
//frameLD();

//pipeHD();
pipeLD();

//glassHD();

//%box([5.95,5.95,3]);
