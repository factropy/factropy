use <lib.scad>

hd=72;
ld=12;

module baseHD($fn=hd) {
	translate([0,0,1]) rbox([11.95,11.95,2],0.01,$fn=8);
}

module baseLD($fn=ld) {
	translate([0,0,1]) box([11.95,11.95,2]);
}

module vesselHD($fn=hd) {
	translate([0,0,8]) difference() {
		union() {
			hull() {
				translate([0,0,0]) cyl(1.5,1.5,1);
				translate([0,0,5]) torus(0.5,0.5);
			}
			translate([0,0,-2.5]) sphere(3);
		}
	}
}

module vesselLD($fn=ld) {
	translate([0,0,8]) difference() {
		union() {
			hull() {
				translate([0,0,0]) cyl(1.5,1.5,1);
				translate([0,0,5]) torus(0.5,0.5);
			}
			translate([0,0,-2.5]) sphere(3);
		}
	}
}

module poolHD($fn=hd) {
	translate([0,0,3]) torus(1,4);
}

module poolLD($fn=hd/2) {
	translate([0,0,3]) torus(1,4);
}

module poolPipeHD($fn=hd) {
	translate([0,3,4]) rotate([60,0,0]) cyl(0.5,0.5,3);
}

module poolPipeLD($fn=ld) {
	translate([0,3,4]) rotate([60,0,0]) cyl(0.5,0.5,3);
}

module portalHD($fn=hd) {
	translate([0,3,6.5]) rotate([-70,0,0]) difference() {
		union() {
			cyl(0.5,0.5,0.5);
			cyl(0.6,0.6,0.1);
		}
		cyl(0.45,0.45,0.6);
	}
}

module portalLD($fn=ld) {
	translate([0,3,6.5]) rotate([-70,0,0]) difference() {
		union() {
			cyl(0.5,0.5,0.5);
			cyl(0.6,0.6,0.1);
		}
		cyl(0.45,0.45,0.6);
	}
}

module portalScreenHD($fn=hd) {
	translate([0,3,6.5]) rotate([-70,0,0]) {
		cyl(0.5,0.5,0.1);
	}
}

module portalScreenLD($fn=ld) {
	translate([0,3,6.5]) rotate([-70,0,0]) {
		cyl(0.5,0.5,0.1);
	}
}

module flowPipeHD($fn=hd/2) {
	points = [
		[0,0,4],
		[3,0,4],
		[4,0,3],
		[4,0,0],
	];
	translate([3,0,1.5]) pipe(points, 0.5);
}

module flowPipeLD($fn=ld) {
	points = [
		[0,0,4],
		[3,0,4],
		[4,0,3],
		[4,0,0],
	];
	translate([3,0,1.5]) pipe(points, 0.5);
}

module reactor() {
	baseHD();
	vesselHD();
	poolHD();
	for (i = [0:45:360]) rotate([0,0,i]) poolPipeHD();
	for (i = [0:90:360]) rotate([0,0,i+45]) flowPipeHD();
	for (i = [0:90:360]) rotate([0,0,i]) portalHD();
	for (i = [0:90:360]) rotate([0,0,i]) portalScreenHD();
}

reactor();

//baseHD();
//baseLD();
//vesselHD();
//vesselLD();
//poolHD();
//poolLD();
//poolPipeHD();
//poolPipeLD();
//flowPipeHD();
//flowPipeLD();
//portalHD();
//portalLD();
//portalScreenHD();
//portalScreenLD();