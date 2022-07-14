use <lib.scad>

module baseLD() {
	hull() {
		translate([0,0,0.75]) intersection() {
			box([10,10,1.5]);
			rotate([0,0,45]) box([11.5,11.5,2]);
		}
		translate([0,0,2.75]) box([7,7,0.1]);
	}
}

module baseHD() {
	minkowskiOutsideRound(0.01,$fn=8)
	baseLD();
}

module padLD() {
	rotate([0,0,22.5]) translate([0,0,2.75]) cyl(4,4,0.5,$fn=8);
}

module padHD() {
	minkowskiOutsideRound(0.01,$fn=8) padLD();
}

//baseHD();
//baseLD();
padHD();
//padLD();

