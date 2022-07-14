use <lib.scad>

hd = 36;
ld = 8;

module towerHD($fn=hd) {
	translate([0,0,2.5]) union() {
		cyl(0.1,0.1,5);
		translate([0,0,-2.4]) cyl(0.3,0.3,0.2);
	}
}

module towerLD($fn=ld) {
	translate([0,0,2.5]) union() {
		cyl(0.1,0.1,5);
		translate([0,0,-2.4]) cyl(0.3,0.3,0.2);
	}
}

module plateHD($fn=hd) {
	translate([0.15,0,4.25]) rotate([0,90,0]) hull() {
		translate([0.5,0,0]) cyl(0.25,0.25,0.1);
		translate([-0.5,0,0]) cyl(0.25,0.25,0.1);
	}
}

module plateLD($fn=ld) {
	translate([0.15,0,4.25]) rotate([0,90,0]) hull() {
		translate([0.5,0,0]) cyl(0.25,0.25,0.1);
		translate([-0.5,0,0]) cyl(0.25,0.25,0.1);
	}
}

module lightHD($fn=hd) {
	translate([0.25,0,4.25]) rotate([0,90,0]) cyl(0.2,0.2,0.1);
}

module lightLD($fn=ld) {
	translate([0.25,0,4.25]) rotate([0,90,0]) cyl(0.2,0.2,0.1);
}

//towerHD();
//towerLD();
//plateHD();
//plateLD();
//lightHD();
lightLD();
